#include "gst-dxvnpuenc.hpp"
#include <gst/video/video.h>
#include <cstring>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpuenc_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpuenc_debug_category

// ---------------------------------------------------------------------------
// Codec enum type for GObject property
// ---------------------------------------------------------------------------
#define GST_TYPE_DXVNPUENC_CODEC (gst_dxvnpuenc_codec_get_type())

static GType gst_dxvnpuenc_codec_get_type(void) {
    static GType codec_type = 0;
    if (g_once_init_enter(&codec_type)) {
        static const GEnumValue codec_values[] = {
            {static_cast<int>(dxvnpu::CODEC_H264), "H.264/AVC", "h264"},
            {static_cast<int>(dxvnpu::CODEC_H265), "H.265/HEVC", "h265"},
            {0, nullptr, nullptr}
        };
        GType tmp = g_enum_register_static("GstDxVnpuEncCodec", codec_values);
        g_once_init_leave(&codec_type, tmp);
    }
    return codec_type;
}

// ---------------------------------------------------------------------------
// Property IDs
// ---------------------------------------------------------------------------
enum class EncPropertyID {
    PROP_0, PROP_CODEC, PROP_BITRATE, N_PROPERTIES
};

// ---------------------------------------------------------------------------
// Caps templates
// ---------------------------------------------------------------------------
#define DXVNPUENC_SINK_CAPS \
    "video/x-raw, format=(string)NV12"

#define DXVNPUENC_SRC_CAPS \
    "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; " \
    "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void gst_dxvnpuenc_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec);
static void gst_dxvnpuenc_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec);
static void gst_dxvnpuenc_finalize(GObject *object);

static gboolean gst_dxvnpuenc_start(GstVideoEncoder *encoder);
static gboolean gst_dxvnpuenc_stop(GstVideoEncoder *encoder);
static gboolean gst_dxvnpuenc_set_format(GstVideoEncoder *encoder,
                                           GstVideoCodecState *state);
static GstFlowReturn gst_dxvnpuenc_handle_frame(GstVideoEncoder *encoder,
                                                   GstVideoCodecFrame *frame);
static GstFlowReturn gst_dxvnpuenc_finish(GstVideoEncoder *encoder);
static gboolean gst_dxvnpuenc_flush(GstVideoEncoder *encoder);

// ---------------------------------------------------------------------------
// drain_ready_packets — pull all available encoded packets
//
// Returns GST_FLOW_OK on success. Drains until GetPacket returns nullptr.
// ---------------------------------------------------------------------------
static GstFlowReturn drain_ready_packets(GstDxVnpuEnc *self,
                                          GstVideoEncoder *encoder,
                                          int timeout_ms) {
    if (!self->encoder_module)
        return GST_FLOW_OK;

    while (true) {
        GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
        auto pkt = self->encoder_module->GetPacket(timeout_ms);
        GST_VIDEO_ENCODER_STREAM_LOCK(encoder);

        if (!pkt) break;

        if (pkt->data_size == 0) {
            GST_DEBUG_OBJECT(self, "Received EOS marker from HW");
            self->hw_pending.store(0, std::memory_order_release);
            break;
        }

        if (!pkt->isBitStream()) {
            GST_DEBUG_OBJECT(self, "Skipping non-bitstream packet (size=%zu)", pkt->data_size);
            continue;
        }

        self->hw_pending.fetch_sub(1, std::memory_order_release);

        size_t pkt_size = pkt->data_size;
        GstBuffer *outbuf = gst_buffer_new_allocate(nullptr, pkt_size, nullptr);
        if (!outbuf)
            return GST_FLOW_ERROR;

        GstMapInfo out_map;
        if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
            gst_buffer_unref(outbuf);
            return GST_FLOW_ERROR;
        }
        std::memcpy(out_map.data, pkt->getRawData(), pkt_size);
        gst_buffer_unmap(outbuf, &out_map);
        pkt.reset();

        GstVideoCodecFrame *frame = gst_video_encoder_get_oldest_frame(encoder);
        if (!frame) {
            GST_WARNING_OBJECT(self, "No pending frame to match encoded output");
            gst_buffer_unref(outbuf);
            self->hw_pending.fetch_add(1, std::memory_order_release);
            continue;
        }

        frame->output_buffer = outbuf;
        GstFlowReturn ret = gst_video_encoder_finish_frame(encoder, frame);
        if (ret != GST_FLOW_OK) {
            GST_DEBUG_OBJECT(self, "finish_frame returned %s",
                            gst_flow_get_name(ret));
            return ret;
        }
    }

    return GST_FLOW_OK;
}

// ---------------------------------------------------------------------------
// GObject boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxVnpuEnc, gst_dxvnpuenc, GST_TYPE_VIDEO_ENCODER,
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpuenc_debug_category, "dxvnpuenc", 0,
                            "debug category for dxvnpuenc element"))

// ---------------------------------------------------------------------------
// class_init
// ---------------------------------------------------------------------------
static void gst_dxvnpuenc_class_init(GstDxVnpuEncClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *videoenc_class = GST_VIDEO_ENCODER_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_dxvnpuenc_set_property;
    gobject_class->get_property = gst_dxvnpuenc_get_property;
    gobject_class->finalize = gst_dxvnpuenc_finalize;

    // Properties
    static std::array<GParamSpec*,
                      static_cast<int>(EncPropertyID::N_PROPERTIES)> obj_properties = {nullptr};

    obj_properties[static_cast<guint>(EncPropertyID::PROP_CODEC)] =
        g_param_spec_enum("codec", "Codec",
                          "Video codec to use for encoding",
                          GST_TYPE_DXVNPUENC_CODEC,
                          static_cast<int>(dxvnpu::CODEC_H264),
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(EncPropertyID::PROP_BITRATE)] =
        g_param_spec_uint("bitrate", "Bitrate",
                          "Target encoding bitrate in kbps",
                          1, 100000, 4096,
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(EncPropertyID::N_PROPERTIES),
                                      obj_properties.data());

    // Pad templates
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

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxvnpuenc_init(GstDxVnpuEnc *self) {
    new (&self->encoder_module) std::shared_ptr<dxvnpu::VideoEncoder>();
    new (&self->hw_pending) std::atomic<int>(0);

    self->codec = dxvnpu::CODEC_H264;
    self->bitrate = 4096;
    self->input_state = nullptr;
}

static void gst_dxvnpuenc_finalize(GObject *object) {
    auto *self = GST_DXVNPUENC(object);

    self->encoder_module.~shared_ptr();

    G_OBJECT_CLASS(gst_dxvnpuenc_parent_class)->finalize(object);
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
static void gst_dxvnpuenc_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUENC(object);
    switch (property_id) {
        case static_cast<guint>(EncPropertyID::PROP_CODEC):
            self->codec = static_cast<dxvnpu::VideoCodec>(g_value_get_enum(value));
            break;
        case static_cast<guint>(EncPropertyID::PROP_BITRATE):
            self->bitrate = g_value_get_uint(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpuenc_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUENC(object);
    switch (property_id) {
        case static_cast<guint>(EncPropertyID::PROP_CODEC):
            g_value_set_enum(value, static_cast<int>(self->codec));
            break;
        case static_cast<guint>(EncPropertyID::PROP_BITRATE):
            g_value_set_uint(value, self->bitrate);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

// ---------------------------------------------------------------------------
// start / stop
//
// Encoder lifecycle: set_format() creates, stop() resets.
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpuenc_start(GstVideoEncoder *encoder) {
    auto *self = GST_DXVNPUENC(encoder);
    self->hw_pending.store(0, std::memory_order_relaxed);

    GST_DEBUG_OBJECT(self, "Started");
    return TRUE;
}

static gboolean gst_dxvnpuenc_stop(GstVideoEncoder *encoder) {
    auto *self = GST_DXVNPUENC(encoder);

    self->encoder_module.reset();
    self->hw_pending.store(0, std::memory_order_relaxed);

    if (self->input_state) {
        gst_video_codec_state_unref(self->input_state);
        self->input_state = nullptr;
    }

    GST_DEBUG_OBJECT(self, "Stopped");
    return TRUE;
}

// ---------------------------------------------------------------------------
// set_format — create/replace HW encoder
//
// On caps renegotiation, base class drains before calling set_format().
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpuenc_set_format(GstVideoEncoder *encoder,
                                           GstVideoCodecState *state) {
    auto *self = GST_DXVNPUENC(encoder);

    gint width = GST_VIDEO_INFO_WIDTH(&state->info);
    gint height = GST_VIDEO_INFO_HEIGHT(&state->info);

    self->encoder_module.reset();

    try {
        dxvnpu::EncoderConfig enc_cfg;
        enc_cfg.codec = self->codec;
        enc_cfg.input_width = width;
        enc_cfg.input_height = height;
        enc_cfg.input_format = dxvnpu::COLOR_FORMAT_YUV420SP;
        enc_cfg.fps = -1;  // device unlimited
        enc_cfg.bitrate = self->bitrate;

        self->encoder_module = std::make_shared<dxvnpu::VideoEncoder>(enc_cfg);
    } catch (const std::exception &e) {
        GST_ERROR_OBJECT(self, "Failed to create VideoEncoder: %s", e.what());
        return FALSE;
    }

    if (!self->encoder_module->IsStarted()) {
        GST_ERROR_OBJECT(self, "VideoEncoder failed to start");
        self->encoder_module.reset();
        return FALSE;
    }

    const gchar *output_mime = (self->codec == dxvnpu::CODEC_H265)
                                   ? "video/x-h265" : "video/x-h264";
    GstCaps *output_caps = gst_caps_new_simple(output_mime,
        "stream-format", G_TYPE_STRING, "byte-stream",
        "alignment", G_TYPE_STRING, "au",
        nullptr);

    GstVideoCodecState *output_state = gst_video_encoder_set_output_state(
        encoder, output_caps, state);
    gst_video_codec_state_unref(output_state);

    if (self->input_state)
        gst_video_codec_state_unref(self->input_state);
    self->input_state = gst_video_codec_state_ref(state);

    self->hw_pending.store(0, std::memory_order_relaxed);

    GstClockTime latency = gst_util_uint64_scale(GST_SECOND, 1, 30);
    gst_video_encoder_set_latency(encoder, latency, latency);

    GST_INFO_OBJECT(self,
        "VNPU encoder created: %s %dx%d, %u kbps",
        (self->codec == dxvnpu::CODEC_H264) ? "H.264" : "H.265",
        width, height, self->bitrate);

    return TRUE;
}

// ---------------------------------------------------------------------------
// handle_frame — drain ready packets, then submit frame to HW encoder
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpuenc_handle_frame(GstVideoEncoder *encoder,
                                                   GstVideoCodecFrame *frame) {
    auto *self = GST_DXVNPUENC(encoder);

    GST_LOG_OBJECT(self, "handle_frame: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(frame->input_buffer)));

    if (!self->input_state) {
        GST_ERROR_OBJECT(self, "Input state not set");
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GstFlowReturn drain_ret = drain_ready_packets(self, encoder, 5);
    if (drain_ret != GST_FLOW_OK) {
        gst_video_codec_frame_unref(frame);
        return drain_ret;
    }

    GstVideoInfo *info = &self->input_state->info;
    gint width = GST_VIDEO_INFO_WIDTH(info);
    gint height = GST_VIDEO_INFO_HEIGHT(info);

    GstVideoFrame vframe;
    if (!gst_video_frame_map(&vframe, info, frame->input_buffer, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_ERROR;
    }

    size_t expected_size = static_cast<size_t>(width) * height * 3 / 2;
    auto dx_frame = std::make_shared<dxvnpu::DXFrame>(width, height, dxvnpu::COLOR_FORMAT_YUV420SP);
    dx_frame->allocateData(expected_size);
    uint8_t *dst = dx_frame->getRawData();

    const uint8_t *src_y = static_cast<const uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 0));
    int src_stride_y = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
    for (int row = 0; row < height; ++row) {
        std::memcpy(dst + row * width, src_y + row * src_stride_y, width);
    }

    const uint8_t *src_uv = static_cast<const uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 1));
    int src_stride_uv = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 1);
    int uv_height = height / 2;
    uint8_t *dst_uv = dst + static_cast<size_t>(width) * height;
    for (int row = 0; row < uv_height; ++row) {
        std::memcpy(dst_uv + row * width, src_uv + row * src_stride_uv, width);
    }

    dx_frame->timestamp = GST_BUFFER_PTS(frame->input_buffer);
    gst_video_frame_unmap(&vframe);

    if (!self->encoder_module) {
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    auto status = self->encoder_module->PutFrame(dx_frame);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);

    if (status != dxvnpu::Status::OK) {
        GST_ERROR_OBJECT(self, "PutFrame failed: %d", static_cast<int>(status));
        gst_video_codec_frame_unref(frame);
        return GST_FLOW_ERROR;
    }

    self->hw_pending.fetch_add(1, std::memory_order_release);
    gst_video_codec_frame_unref(frame);
    return GST_FLOW_OK;
}

// ---------------------------------------------------------------------------
// finish — EOS: send EOS to HW, drain all remaining packets inline
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpuenc_finish(GstVideoEncoder *encoder) {
    auto *self = GST_DXVNPUENC(encoder);
    GST_DEBUG_OBJECT(self, "Finishing: sending EOS, hw_pending=%d",
                     self->hw_pending.load(std::memory_order_relaxed));

    if (!self->encoder_module)
        return GST_FLOW_OK;

    auto eos_frame = std::make_shared<dxvnpu::DXFrame>();
    eos_frame->isEOS = true;
    eos_frame->data = nullptr;
    eos_frame->data_size = 0;

    GST_VIDEO_ENCODER_STREAM_UNLOCK(encoder);
    auto eos_status = self->encoder_module->PutFrame(eos_frame);
    GST_VIDEO_ENCODER_STREAM_LOCK(encoder);

    if (eos_status != dxvnpu::Status::OK) {
        GST_WARNING_OBJECT(self, "Failed to send EOS to encoder (status=%d)",
                           static_cast<int>(eos_status));
    }

    constexpr int kFinishTimeoutMs = 5000;
    constexpr int kDrainIntervalMs = 50;
    int elapsed = 0;

    while (self->hw_pending.load(std::memory_order_acquire) > 0 &&
           elapsed < kFinishTimeoutMs) {
        GstFlowReturn ret = drain_ready_packets(self, encoder, kDrainIntervalMs);
        if (ret != GST_FLOW_OK) {
            GST_WARNING_OBJECT(self, "Drain error during finish: %s",
                               gst_flow_get_name(ret));
            break;
        }
        elapsed += kDrainIntervalMs;
    }

    if (self->hw_pending.load(std::memory_order_relaxed) > 0) {
        GST_WARNING_OBJECT(self, "Finish drain timed out, hw_pending=%d",
                           self->hw_pending.load(std::memory_order_relaxed));
    }

    GST_DEBUG_OBJECT(self, "Finish complete");
    return GST_FLOW_OK;
}

// ---------------------------------------------------------------------------
// flush — reset state after seek (FLUSH_STOP)
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpuenc_flush(GstVideoEncoder *encoder) {
    auto *self = GST_DXVNPUENC(encoder);

    GST_DEBUG_OBJECT(self, "Flushing: resetting state");

    self->hw_pending.store(0, std::memory_order_relaxed);

    return TRUE;
}
