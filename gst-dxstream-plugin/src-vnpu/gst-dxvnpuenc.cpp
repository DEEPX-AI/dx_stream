#include "gst-dxvnpuenc.hpp"
#include <gst/video/video.h>
#include <array>
#include <cstring>
#include <vector>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpuenc_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpuenc_debug_category

#define GST_TYPE_DXVNPUENC_CODEC (gst_dxvnpuenc_codec_get_type())

static GType gst_dxvnpuenc_codec_get_type(void) {
    static GType codec_type = 0;
    if (g_once_init_enter(&codec_type)) {
        static const GEnumValue codec_values[] = {
            {static_cast<int>(DXVNPU_CODEC_H264), "H.264/AVC", "h264"},
            {static_cast<int>(DXVNPU_CODEC_H265), "H.265/HEVC", "h265"},
            {0, nullptr, nullptr}
        };
        GType tmp = g_enum_register_static("GstDxVnpuEncCodec", codec_values);
        g_once_init_leave(&codec_type, tmp);
    }
    return codec_type;
}

enum class EncPropertyID {
    PROP_0, PROP_CODEC, PROP_BITRATE, PROP_DEVICE_ID, N_PROPERTIES
};

#define DXVNPUENC_SINK_CAPS \
    "video/x-raw, format=(string)NV12"

#define DXVNPUENC_SRC_CAPS \
    "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; " \
    "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"

constexpr int kInputTimeoutMs = 100;

static void gst_dxvnpuenc_set_property(GObject* object, guint property_id,
                                       const GValue* value, GParamSpec* pspec);
static void gst_dxvnpuenc_get_property(GObject* object, guint property_id,
                                       GValue* value, GParamSpec* pspec);
static void gst_dxvnpuenc_finalize(GObject* object);

static gboolean gst_dxvnpuenc_start(GstVideoEncoder* encoder);
static gboolean gst_dxvnpuenc_stop(GstVideoEncoder* encoder);
static gboolean gst_dxvnpuenc_set_format(GstVideoEncoder* encoder,
                                         GstVideoCodecState* state);
static GstFlowReturn gst_dxvnpuenc_handle_frame(GstVideoEncoder* encoder,
                                                GstVideoCodecFrame* frame);
static GstFlowReturn gst_dxvnpuenc_finish(GstVideoEncoder* encoder);
static gboolean gst_dxvnpuenc_flush(GstVideoEncoder* encoder);
static void gst_dxvnpuenc_output_task(void* data);

static gboolean begin_pipeline_call(GstDxVnpuEnc* self, dxvnpu_pipeline_t* pipeline) {
    g_mutex_lock(&self->sdk_lock);
    if (g_atomic_int_get(&self->flushing) || !self->encoder_pipeline) {
        g_mutex_unlock(&self->sdk_lock);
        return FALSE;
    }
    ++self->sdk_calls;
    *pipeline = self->encoder_pipeline;
    g_mutex_unlock(&self->sdk_lock);
    return TRUE;
}

static void end_pipeline_call(GstDxVnpuEnc* self) {
    g_mutex_lock(&self->sdk_lock);
    if (--self->sdk_calls == 0)
        g_cond_broadcast(&self->sdk_idle);
    g_mutex_unlock(&self->sdk_lock);
}

static void complete_output(GstDxVnpuEnc* self, GstFlowReturn flow, gboolean eos) {
    g_mutex_lock(&self->output_lock);
    if (self->output_flow == GST_FLOW_OK)
        self->output_flow = flow;
    self->output_eos = eos;
    g_cond_broadcast(&self->output_ready);
    g_mutex_unlock(&self->output_lock);
}

static void release_packet(GstDxVnpuEnc* self, dxvnpu_buffer_t* packet) {
    g_mutex_lock(&self->sdk_io_lock);
    dxvnpu_buffer_release(packet);
    g_mutex_unlock(&self->sdk_io_lock);
}

static GstFlowReturn drain_ready_packets(GstDxVnpuEnc* self,
                                         GstVideoEncoder* encoder,
                                         int timeout_ms) {
    while (true) {
        dxvnpu_buffer_t pkt = nullptr;
        dxvnpu_pipeline_t pipeline = nullptr;
        if (!begin_pipeline_call(self, &pipeline))
            return GST_FLOW_FLUSHING;
        dxvnpu_status_t status = dxvnpu_pipeline_get_buffer(
            pipeline, &pkt, timeout_ms);
        end_pipeline_call(self);

        if (status == DXVNPU_TIMEOUT)
            break;

        if (status == DXVNPU_END_OF_STREAM) {
            GST_DEBUG_OBJECT(self, "Received EOS marker from HW");
            complete_output(self, GST_FLOW_OK, TRUE);
            break;
        }

        if (status != DXVNPU_OK) {
            GST_ERROR_OBJECT(self, "dxvnpu_pipeline_get_buffer failed: %s",
                             dxvnpu_status_string(status));
            return GST_FLOW_ERROR;
        }

        dxvnpu_buffer_type_t type = DXVNPU_BUFFER_FRAME;
        const void* data = nullptr;
        size_t data_size = 0;
        if (dxvnpu_buffer_get_type(pkt, &type) != DXVNPU_OK ||
            dxvnpu_buffer_view(pkt, &data, &data_size) != DXVNPU_OK) {
            GST_ERROR_OBJECT(self, "Failed to read encoder output buffer");
            release_packet(self, &pkt);
            return GST_FLOW_ERROR;
        }

        if (type != DXVNPU_BUFFER_BITSTREAM || data_size == 0) {
            release_packet(self, &pkt);
            continue;
        }

        GstBuffer* outbuf = gst_buffer_new_allocate(nullptr, data_size, nullptr);
        if (!outbuf) {
            release_packet(self, &pkt);
            return GST_FLOW_ERROR;
        }

        GstMapInfo out_map;
        if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
            gst_buffer_unref(outbuf);
            release_packet(self, &pkt);
            return GST_FLOW_ERROR;
        }
        std::memcpy(out_map.data, data, data_size);
        gst_buffer_unmap(outbuf, &out_map);
        release_packet(self, &pkt);

        GST_VIDEO_ENCODER_STREAM_LOCK(encoder);
        --self->hw_pending;
        GstVideoCodecFrame* frame = gst_video_encoder_get_oldest_frame(encoder);
        if (!frame) {
            GST_WARNING_OBJECT(self, "No pending frame to match encoded output");
            gst_buffer_unref(outbuf);
            self->hw_pending++;
            GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
            continue;
        }

        frame->output_buffer = outbuf;
        GstFlowReturn ret = gst_video_encoder_finish_frame(encoder, frame);
        GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
        if (ret != GST_FLOW_OK) {
            GST_DEBUG_OBJECT(self, "finish_frame returned %s",
                             gst_flow_get_name(ret));
            return ret;
        }
    }

    return GST_FLOW_OK;
}

static void gst_dxvnpuenc_output_task(void* data) {
    auto* self = GST_DXVNPUENC(data);
    auto* encoder = GST_VIDEO_ENCODER(self);
    GstFlowReturn result = GST_FLOW_OK;

    while (!g_atomic_int_get(&self->flushing)) {
        result = drain_ready_packets(self, encoder, 50);
        if (result != GST_FLOW_OK || self->output_eos)
            break;
    }
    if (g_atomic_int_get(&self->flushing) && result == GST_FLOW_OK)
        result = GST_FLOW_FLUSHING;
    complete_output(self, result, self->output_eos);
    gst_pad_pause_task(GST_VIDEO_ENCODER_SRC_PAD(encoder));
}

static gboolean start_output_task(GstDxVnpuEnc* self) {
    if (gst_pad_start_task(GST_VIDEO_ENCODER_SRC_PAD(self),
                           gst_dxvnpuenc_output_task, self, nullptr))
        return TRUE;
    GST_ELEMENT_ERROR(self, RESOURCE, FAILED, ("Failed to start encoder output task"), (nullptr));
    return FALSE;
}

static void stop_output_task(GstDxVnpuEnc* self) {
    gst_pad_stop_task(GST_VIDEO_ENCODER_SRC_PAD(self));
}

G_DEFINE_TYPE_WITH_CODE(
    GstDxVnpuEnc, gst_dxvnpuenc, GST_TYPE_VIDEO_ENCODER,
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpuenc_debug_category, "dxvnpuenc", 0,
                            "debug category for dxvnpuenc element"))

static void gst_dxvnpuenc_class_init(GstDxVnpuEncClass* klass) {
    auto* gobject_class = G_OBJECT_CLASS(klass);
    auto* videoenc_class = GST_VIDEO_ENCODER_CLASS(klass);
    auto* element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_dxvnpuenc_set_property;
    gobject_class->get_property = gst_dxvnpuenc_get_property;
    gobject_class->finalize = gst_dxvnpuenc_finalize;

    static std::array<GParamSpec*,
                      static_cast<int>(EncPropertyID::N_PROPERTIES)> obj_properties = {nullptr};

    obj_properties[static_cast<guint>(EncPropertyID::PROP_CODEC)] =
        g_param_spec_enum("codec", "Codec",
                          "Video codec to use for encoding",
                          GST_TYPE_DXVNPUENC_CODEC,
                          static_cast<int>(DXVNPU_CODEC_H264),
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(EncPropertyID::PROP_BITRATE)] =
        g_param_spec_uint("bitrate", "Bitrate",
                          "Target encoding bitrate in kbps",
                          1, 100000, 4096,
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    obj_properties[static_cast<guint>(EncPropertyID::PROP_DEVICE_ID)] =
        g_param_spec_int("device-id", "Device ID",
                         "DXVNPU device index (-1 selects automatically)",
                         -1, G_MAXINT8, -1,
                         static_cast<GParamFlags>(G_PARAM_READWRITE |
                                                  G_PARAM_STATIC_STRINGS |
                                                  GST_PARAM_MUTABLE_READY));

    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(EncPropertyID::N_PROPERTIES),
                                      obj_properties.data());

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXVNPUENC_SINK_CAPS)));

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXVNPUENC_SRC_CAPS)));

    gst_element_class_set_static_metadata(
        element_class,
        "DX-VNPU Video Encoder",
        "Codec/Encoder/Video/Hardware",
        "Hardware-accelerated video encoder using DEEPX VNPU (H.264/H.265)",
        "Sangil Jo <sijo@deepx.ai>");

    videoenc_class->start = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_start);
    videoenc_class->stop = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_stop);
    videoenc_class->set_format = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_set_format);
    videoenc_class->handle_frame = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_handle_frame);
    videoenc_class->finish = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_finish);
    videoenc_class->flush = GST_DEBUG_FUNCPTR(gst_dxvnpuenc_flush);
}

static void gst_dxvnpuenc_init(GstDxVnpuEnc* self) {
    self->encoder_pipeline = nullptr;
    self->codec = DXVNPU_CODEC_H264;
    self->bitrate = 4096;
    self->device_id = -1;
    self->input_state = nullptr;
    self->hw_pending = 0;
    g_atomic_int_set(&self->flushing, FALSE);
    g_mutex_init(&self->sdk_lock);
    g_mutex_init(&self->sdk_io_lock);
    g_cond_init(&self->sdk_idle);
    self->sdk_calls = 0;
    g_mutex_init(&self->output_lock);
    g_cond_init(&self->output_ready);
    self->output_flow = GST_FLOW_OK;
    self->output_eos = FALSE;
}

static void gst_dxvnpuenc_finalize(GObject* object) {
    auto* self = GST_DXVNPUENC(object);
    g_atomic_int_set(&self->flushing, TRUE);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->encoder_pipeline) {
        dxvnpu_pipeline_destroy(&self->encoder_pipeline);
    }
    g_mutex_unlock(&self->sdk_lock);
    g_cond_clear(&self->output_ready);
    g_mutex_clear(&self->output_lock);
    g_cond_clear(&self->sdk_idle);
    g_mutex_clear(&self->sdk_io_lock);
    g_mutex_clear(&self->sdk_lock);
    G_OBJECT_CLASS(gst_dxvnpuenc_parent_class)->finalize(object);
}

static void gst_dxvnpuenc_set_property(GObject* object, guint property_id,
                                       const GValue* value, GParamSpec* pspec) {
    auto* self = GST_DXVNPUENC(object);
    switch (property_id) {
        case static_cast<guint>(EncPropertyID::PROP_CODEC):
            self->codec = static_cast<dxvnpu_codec_t>(g_value_get_enum(value));
            break;
        case static_cast<guint>(EncPropertyID::PROP_BITRATE):
            self->bitrate = g_value_get_uint(value);
            break;
        case static_cast<guint>(EncPropertyID::PROP_DEVICE_ID):
            self->device_id = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpuenc_get_property(GObject* object, guint property_id,
                                       GValue* value, GParamSpec* pspec) {
    auto* self = GST_DXVNPUENC(object);
    switch (property_id) {
        case static_cast<guint>(EncPropertyID::PROP_CODEC):
            g_value_set_enum(value, static_cast<int>(self->codec));
            break;
        case static_cast<guint>(EncPropertyID::PROP_BITRATE):
            g_value_set_uint(value, self->bitrate);
            break;
        case static_cast<guint>(EncPropertyID::PROP_DEVICE_ID):
            g_value_set_int(value, self->device_id);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static gboolean gst_dxvnpuenc_start(GstVideoEncoder* encoder) {
    auto* self = GST_DXVNPUENC(encoder);
    self->hw_pending = 0;
    g_atomic_int_set(&self->flushing, FALSE);
    GST_DEBUG_OBJECT(self, "Started");
    return TRUE;
}

static gboolean gst_dxvnpuenc_stop(GstVideoEncoder* encoder) {
    auto* self = GST_DXVNPUENC(encoder);

    g_atomic_int_set(&self->flushing, TRUE);
    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    stop_output_task(self);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->encoder_pipeline) {
        dxvnpu_pipeline_destroy(&self->encoder_pipeline);
    }
    g_mutex_unlock(&self->sdk_lock);
    self->hw_pending = 0;

    if (self->input_state) {
        gst_video_codec_state_unref(self->input_state);
        self->input_state = nullptr;
    }

    GST_DEBUG_OBJECT(self, "Stopped");
    return TRUE;
}

static gboolean gst_dxvnpuenc_set_format(GstVideoEncoder* encoder,
                                         GstVideoCodecState* state) {
    auto* self = GST_DXVNPUENC(encoder);

    gint width = GST_VIDEO_INFO_WIDTH(&state->info);
    gint height = GST_VIDEO_INFO_HEIGHT(&state->info);
    if (self->encoder_pipeline) {
        g_atomic_int_set(&self->flushing, TRUE);
        GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
        stop_output_task(self);
        GST_VIDEO_ENCODER_STREAM_LOCK(encoder);
        g_mutex_lock(&self->sdk_lock);
        while (self->sdk_calls)
            g_cond_wait(&self->sdk_idle, &self->sdk_lock);
        dxvnpu_pipeline_destroy(&self->encoder_pipeline);
        g_mutex_unlock(&self->sdk_lock);
    }

    guint fps = GST_VIDEO_INFO_FPS_N(&state->info) > 0 && GST_VIDEO_INFO_FPS_D(&state->info) > 0
                    ? GST_VIDEO_INFO_FPS_N(&state->info) / GST_VIDEO_INFO_FPS_D(&state->info)
                    : 30;
    if (fps == 0) {
        fps = 30;
    }

    dxvnpu_config_t enc_cfg = nullptr;
    dxvnpu_status_t status = dxvnpu_encoder_config_create(&enc_cfg);
    if (status == DXVNPU_OK) {
        status = dxvnpu_encoder_config_set_codec(enc_cfg, self->codec);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_encoder_config_set_input_format(
            enc_cfg, width, height, DXVNPU_COLOR_YUV420SP);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_encoder_config_set_framerate(enc_cfg, fps);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_encoder_config_set_bitrate_kbps(enc_cfg, self->bitrate);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_pipeline_create(&self->encoder_pipeline, enc_cfg,
                                        DXVNPU_PIPELINE_FLAG_NONE, self->device_id);
    }
    if (enc_cfg) {
        dxvnpu_config_destroy(&enc_cfg);
    }
    if (status != DXVNPU_OK) {
        GST_ERROR_OBJECT(self, "Failed to create encoder pipeline: %s",
                         dxvnpu_status_string(status));
        return FALSE;
    }

    const gchar* output_mime = (self->codec == DXVNPU_CODEC_H265)
                                   ? "video/x-h265" : "video/x-h264";
    GstCaps* output_caps = gst_caps_new_simple(output_mime,
        "stream-format", G_TYPE_STRING, "byte-stream",
        "alignment", G_TYPE_STRING, "au",
        nullptr);

    GstVideoCodecState* output_state = gst_video_encoder_set_output_state(
        encoder, output_caps, state);
    gst_video_codec_state_unref(output_state);

    if (self->input_state)
        gst_video_codec_state_unref(self->input_state);
    self->input_state = gst_video_codec_state_ref(state);

    self->hw_pending = 0;
    g_atomic_int_set(&self->flushing, FALSE);
    g_mutex_lock(&self->output_lock);
    self->output_flow = GST_FLOW_OK;
    self->output_eos = FALSE;
    g_mutex_unlock(&self->output_lock);

    GstClockTime latency = gst_util_uint64_scale(GST_SECOND, 1, fps);
    gst_video_encoder_set_latency(encoder, latency, latency);

    GST_INFO_OBJECT(self,
        "VNPU encoder created: %s %dx%d, %u kbps",
        (self->codec == DXVNPU_CODEC_H264) ? "H.264" : "H.265",
        width, height, self->bitrate);

    return start_output_task(self);
}

static GstFlowReturn gst_dxvnpuenc_handle_frame(GstVideoEncoder* encoder,
                                                GstVideoCodecFrame* frame) {
    auto* self = GST_DXVNPUENC(encoder);

    GST_LOG_OBJECT(self, "handle_frame: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(frame->input_buffer)));

    if (g_atomic_int_get(&self->flushing))
        return GST_FLOW_FLUSHING;
    if (!self->input_state || !self->encoder_pipeline) {
        GST_ERROR_OBJECT(self, "Encoder pipeline not initialized");
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GstVideoInfo* info = &self->input_state->info;
    gint width = GST_VIDEO_INFO_WIDTH(info);
    gint height = GST_VIDEO_INFO_HEIGHT(info);

    GstVideoFrame vframe;
    if (!gst_video_frame_map(&vframe, info, frame->input_buffer, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_ERROR;
    }

    size_t expected_size = static_cast<size_t>(width) * height * 3 / 2;
    std::vector<uint8_t> packed(expected_size);
    uint8_t* dst = packed.data();

    const uint8_t* src_y = static_cast<const uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 0));
    int src_stride_y = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
    for (int row = 0; row < height; ++row) {
        std::memcpy(dst + row * width, src_y + row * src_stride_y, width);
    }

    const uint8_t* src_uv = static_cast<const uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 1));
    int src_stride_uv = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 1);
    int uv_height = height / 2;
    uint8_t* dst_uv = dst + static_cast<size_t>(width) * height;
    for (int row = 0; row < uv_height; ++row) {
        std::memcpy(dst_uv + row * width, src_uv + row * src_stride_uv, width);
    }

    gst_video_frame_unmap(&vframe);

    dxvnpu_frame_info_t frame_info = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        DXVNPU_COLOR_YUV420SP
    };
    dxvnpu_buffer_t input = nullptr;
    dxvnpu_pipeline_t pipeline = nullptr;
    if (!begin_pipeline_call(self, &pipeline)) {
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_FLUSHING;
    }
    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    g_mutex_lock(&self->sdk_io_lock);
    dxvnpu_status_t status = dxvnpu_pipeline_acquire_input_buffer(
        pipeline, &input, kInputTimeoutMs);
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_data(input, packed.data(), packed.size());
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_pts(input, GST_BUFFER_PTS(frame->input_buffer));
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_frame_info(input, &frame_info);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_pipeline_put_buffer(pipeline, input, kInputTimeoutMs);
    }
    if (input) {
        dxvnpu_buffer_release(&input);
    }
    g_mutex_unlock(&self->sdk_io_lock);
    end_pipeline_call(self);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);

    if (status != DXVNPU_OK) {
        GST_ERROR_OBJECT(self, "dxvnpu_pipeline_put_buffer failed: %s",
                         dxvnpu_status_string(status));
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_ERROR;
    }

    self->hw_pending++;
    gst_video_codec_frame_unref(frame);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_dxvnpuenc_finish(GstVideoEncoder* encoder) {
    auto* self = GST_DXVNPUENC(encoder);
    GST_DEBUG_OBJECT(self, "Finishing: sending EOS, hw_pending=%d", self->hw_pending);

    if (!self->encoder_pipeline)
        return GST_FLOW_OK;

    dxvnpu_pipeline_t pipeline = nullptr;
    if (!begin_pipeline_call(self, &pipeline))
        return GST_FLOW_FLUSHING;
    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    g_mutex_lock(&self->sdk_io_lock);
    dxvnpu_status_t eos_status = dxvnpu_pipeline_put_eos(pipeline);
    g_mutex_unlock(&self->sdk_io_lock);
    end_pipeline_call(self);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);

    if (eos_status != DXVNPU_OK)
        return GST_FLOW_ERROR;

    g_mutex_lock(&self->output_lock);
    while (!self->output_eos && self->output_flow == GST_FLOW_OK &&
           !g_atomic_int_get(&self->flushing)) {
        GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
        g_cond_wait(&self->output_ready, &self->output_lock);
        GST_VIDEO_ENCODER_STREAM_LOCK(encoder);
    }
    GstFlowReturn result = g_atomic_int_get(&self->flushing)
                               ? GST_FLOW_FLUSHING : self->output_flow;
    g_mutex_unlock(&self->output_lock);
    return result;
}

static gboolean gst_dxvnpuenc_flush(GstVideoEncoder* encoder) {
    auto* self = GST_DXVNPUENC(encoder);
    GST_DEBUG_OBJECT(self, "Flushing: resetting state");
    g_atomic_int_set(&self->flushing, TRUE);
    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    stop_output_task(self);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->encoder_pipeline)
        dxvnpu_pipeline_destroy(&self->encoder_pipeline);
    g_mutex_unlock(&self->sdk_lock);
    if (!self->input_state ||
        !gst_dxvnpuenc_set_format(encoder, self->input_state))
        return FALSE;
    self->hw_pending = 0;
    return TRUE;
}
