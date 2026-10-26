#include "gst-dxvnpudec.hpp"
#include <gst/video/video.h>
#include <array>
#include <cstring>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpudec_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpudec_debug_category

enum class DecPropertyID {
    PROP_0, PROP_DEVICE_ID, N_PROPERTIES
};

#define DXVNPUDEC_SINK_CAPS \
    "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; " \
    "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"

#define DXVNPUDEC_SRC_CAPS "video/x-raw, format=(string)NV12"

constexpr int kSdkPollTimeoutMs = 50;

static void gst_dxvnpudec_set_property(GObject* object, guint property_id,
                                       const GValue* value, GParamSpec* pspec);
static void gst_dxvnpudec_get_property(GObject* object, guint property_id,
                                       GValue* value, GParamSpec* pspec);
static void gst_dxvnpudec_finalize(GObject* object);

static gboolean gst_dxvnpudec_start(GstVideoDecoder* decoder);
static gboolean gst_dxvnpudec_stop(GstVideoDecoder* decoder);
static gboolean gst_dxvnpudec_set_format(GstVideoDecoder* decoder,
                                         GstVideoCodecState* state);
static GstFlowReturn gst_dxvnpudec_handle_frame(GstVideoDecoder* decoder,
                                                GstVideoCodecFrame* frame);
static GstFlowReturn gst_dxvnpudec_finish(GstVideoDecoder* decoder);
static gboolean gst_dxvnpudec_flush(GstVideoDecoder* decoder);
static void gst_dxvnpudec_output_task(void* data);
static gboolean start_output_task(GstDxVnpuDec* self);
static GstStateChangeReturn gst_dxvnpudec_change_state(GstElement* element,
                                                        GstStateChange transition);

static GstFlowReturn sdk_flow_error(GstDxVnpuDec* self, const gchar* operation,
                                    dxvnpu_status_t status) {
    GST_ELEMENT_ERROR(self, CORE, FAILED,
                      ("%s failed: %s", operation, dxvnpu_status_string(status)),
                      (nullptr));
    return GST_FLOW_ERROR;
}

static GstFlowReturn release_buffer(GstDxVnpuDec* self, dxvnpu_buffer_t* buffer) {
    if (!*buffer)
        return GST_FLOW_OK;

    dxvnpu_status_t status = DXVNPU_OK;
    g_mutex_lock(&self->sdk_io_lock);
    for (guint attempt = 0; *buffer && attempt < 3; ++attempt) {
        status = dxvnpu_buffer_release(buffer);
        if (status == DXVNPU_OK)
            break;
    }
    g_mutex_unlock(&self->sdk_io_lock);
    if (status == DXVNPU_OK)
        return GST_FLOW_OK;

    return sdk_flow_error(self, "dxvnpu_buffer_release", status);
}

static gboolean begin_pipeline_call(GstDxVnpuDec* self,
                                    dxvnpu_pipeline_t* pipeline) {
    g_mutex_lock(&self->sdk_lock);
    if (g_atomic_int_get(&self->flushing) || !self->pipeline) {
        g_mutex_unlock(&self->sdk_lock);
        return FALSE;
    }
    ++self->sdk_calls;
    *pipeline = self->pipeline;
    g_mutex_unlock(&self->sdk_lock);
    return TRUE;
}

static void end_pipeline_call(GstDxVnpuDec* self) {
    g_mutex_lock(&self->sdk_lock);
    if (--self->sdk_calls == 0)
        g_cond_broadcast(&self->sdk_idle);
    g_mutex_unlock(&self->sdk_lock);
}

static void reset_output_result(GstDxVnpuDec* self) {
    g_mutex_lock(&self->output_lock);
    self->output_flow = GST_FLOW_OK;
    self->output_eos = FALSE;
    g_mutex_unlock(&self->output_lock);
}

static void complete_output_result(GstDxVnpuDec* self, GstFlowReturn flow,
                                   gboolean eos) {
    g_mutex_lock(&self->output_lock);
    if (self->output_flow == GST_FLOW_OK)
        self->output_flow = flow;
    self->output_eos = eos;
    g_cond_broadcast(&self->output_ready);
    g_mutex_unlock(&self->output_lock);
}

static GstFlowReturn output_result(GstDxVnpuDec* self) {
    g_mutex_lock(&self->output_lock);
    const GstFlowReturn result = self->output_flow;
    g_mutex_unlock(&self->output_lock);
    return result;
}

static void stop_output_task(GstDxVnpuDec* self) {
    gst_pad_stop_task(GST_VIDEO_DECODER_SRC_PAD(self));
    complete_output_result(self, GST_FLOW_FLUSHING, FALSE);
}

static gboolean create_pipeline(GstDxVnpuDec* self) {
    dxvnpu_config_t decoder_config = nullptr;
    dxvnpu_status_t status = dxvnpu_decoder_config_create(&decoder_config);
    const gchar* operation = "dxvnpu_decoder_config_create";
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_decoder_config_set_codec";
        status = dxvnpu_decoder_config_set_codec(decoder_config, self->codec);
    }
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_decoder_config_set_frame_mode";
        status = dxvnpu_decoder_config_set_frame_mode(decoder_config, TRUE);
    }
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_decoder_config_set_stream_size";
        status = dxvnpu_decoder_config_set_stream_size(decoder_config, self->coded_width,
                                                        self->coded_height);
    }
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_pipeline_create";
        g_mutex_lock(&self->sdk_lock);
        status = dxvnpu_pipeline_create(&self->pipeline, decoder_config,
                                        DXVNPU_PIPELINE_FLAG_DIRECT_OUTPUT,
                                        self->device_id);
        g_mutex_unlock(&self->sdk_lock);
    }
    dxvnpu_config_destroy(&decoder_config);

    if (status == DXVNPU_OK && self->pipeline)
        return TRUE;

    if (status != DXVNPU_OK && status != DXVNPU_TIMEOUT)
        sdk_flow_error(self, operation, status);
    else if (status == DXVNPU_TIMEOUT)
        GST_WARNING_OBJECT(self, "%s timed out", operation);
    else
        GST_ELEMENT_ERROR(self, CORE, FAILED,
                          ("dxvnpu_pipeline_create returned no pipeline"), (nullptr));
    return FALSE;
}

static gboolean abort_pipeline(GstDxVnpuDec* self, GstVideoDecoder* decoder) {
    g_atomic_int_set(&self->flushing, TRUE);
    GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
    stop_output_task(self);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->pipeline) {
        dxvnpu_pipeline_destroy(&self->pipeline);
    }
    g_mutex_unlock(&self->sdk_lock);
    GST_VIDEO_DECODER_STREAM_LOCK(decoder);

    self->output_configured = FALSE;
    self->eos_sent = FALSE;
    reset_output_result(self);
    return TRUE;
}

static gboolean flush_pipeline(GstDxVnpuDec* self, GstVideoDecoder* decoder) {
    g_atomic_int_set(&self->flushing, TRUE);
    GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
    stop_output_task(self);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->pipeline) {
        dxvnpu_pipeline_destroy(&self->pipeline);
        self->output_configured = FALSE;
    }
    g_mutex_unlock(&self->sdk_lock);
    GST_VIDEO_DECODER_STREAM_LOCK(decoder);

    g_atomic_int_set(&self->flushing, FALSE);
    self->eos_sent = FALSE;
    reset_output_result(self);
    if (!create_pipeline(self))
        return FALSE;
    self->output_configured = TRUE;
    return start_output_task(self);
}

static GstVideoCodecFrame* find_output_frame(GstDxVnpuDec* self,
                                             GstVideoDecoder* decoder,
                                             GstClockTime output_pts,
                                             GstFlowReturn* result) {
    GstVideoCodecFrame* match = nullptr;
    GList* frames = gst_video_decoder_get_frames(decoder);

    for (GList* item = frames; item; item = item->next) {
        auto* frame = static_cast<GstVideoCodecFrame*>(item->data);
        const GstClockTime frame_pts = GST_BUFFER_PTS(frame->input_buffer);
        const GstClockTime comparable_pts =
            frame_pts == GST_CLOCK_TIME_NONE ? 0 : frame_pts;
        if (comparable_pts < output_pts) {
            const GstFlowReturn ret = gst_video_decoder_drop_frame(
                decoder, gst_video_codec_frame_ref(frame));
            if (ret != GST_FLOW_OK) {
                *result = ret;
                g_list_free_full(frames, (GDestroyNotify)gst_video_codec_frame_unref);
                return nullptr;
            }
        }
    }

    for (GList* item = frames; item; item = item->next) {
        auto* frame = static_cast<GstVideoCodecFrame*>(item->data);
        const GstClockTime frame_pts = GST_BUFFER_PTS(frame->input_buffer);
        if ((frame_pts == GST_CLOCK_TIME_NONE ? 0 : frame_pts) == output_pts) {
            match = gst_video_codec_frame_ref(frame);
            break;
        }
    }

    g_list_free_full(frames, (GDestroyNotify)gst_video_codec_frame_unref);
    return match;
}

static GstFlowReturn process_hw_output(GstDxVnpuDec* self,
                                       GstVideoDecoder* decoder,
                                       dxvnpu_buffer_t* output) {
    dxvnpu_buffer_type_t type = DXVNPU_BUFFER_BITSTREAM;
    dxvnpu_status_t status = dxvnpu_buffer_get_type(*output, &type);
    if (status != DXVNPU_OK) {
        GstFlowReturn ret = status == DXVNPU_TIMEOUT
                                ? GST_FLOW_ERROR
                                : sdk_flow_error(self, "dxvnpu_buffer_get_type", status);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? ret : release_ret;
    }

    if (type != DXVNPU_BUFFER_FRAME) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                          ("Decoder returned a non-frame output buffer"), (nullptr));
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }

    dxvnpu_frame_info_t source_info = {};
    status = dxvnpu_buffer_get_frame_info(*output, &source_info);
    if (status != DXVNPU_OK) {
        GstFlowReturn ret = status == DXVNPU_TIMEOUT
                                ? GST_FLOW_ERROR
                                : sdk_flow_error(self, "dxvnpu_buffer_get_frame_info", status);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? ret : release_ret;
    }
    if (source_info.format != DXVNPU_COLOR_YUV420SP ||
        source_info.width != static_cast<guint>(self->coded_width) ||
        source_info.height != static_cast<guint>(self->coded_height)) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                          ("Decoder output must be NV12 at coded dimensions"), (nullptr));
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }

    uint64_t pts = 0;
    status = dxvnpu_buffer_get_pts(*output, &pts);
    if (status != DXVNPU_OK) {
        GstFlowReturn ret = status == DXVNPU_TIMEOUT
                                ? GST_FLOW_ERROR
                                : sdk_flow_error(self, "dxvnpu_buffer_get_pts", status);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? ret : release_ret;
    }
    const void* data = nullptr;
    size_t data_size = 0;
    status = dxvnpu_buffer_view(*output, &data, &data_size);
    if (status != DXVNPU_OK) {
        GstFlowReturn ret = status == DXVNPU_TIMEOUT
                                ? GST_FLOW_ERROR
                                : sdk_flow_error(self, "dxvnpu_buffer_view", status);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? ret : release_ret;
    }

    const size_t source_stride = source_info.stride;
    const size_t source_slice_height = source_info.slice_height;
    if (!data || source_info.format != DXVNPU_COLOR_YUV420SP ||
        source_info.width != static_cast<guint>(self->coded_width) ||
        source_info.height != static_cast<guint>(self->coded_height) ||
        source_stride < source_info.width || source_slice_height < source_info.height ||
        source_stride > G_MAXSIZE / source_slice_height ||
        source_stride * source_slice_height > G_MAXSIZE / 3 ||
        data_size < source_stride * source_slice_height * 3 / 2) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                          ("Invalid NV12 frame layout: %ux%u stride=%u slice-height=%u, "
                           "payload=%zu",
                           source_info.width, source_info.height, source_info.stride,
                           source_info.slice_height, data_size),
                          (nullptr));
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }

    GstVideoCodecState* output_state = gst_video_decoder_get_output_state(decoder);
    if (!output_state || GST_VIDEO_INFO_FORMAT(&output_state->info) != GST_VIDEO_FORMAT_NV12 ||
        GST_VIDEO_INFO_WIDTH(&output_state->info) != self->coded_width ||
        GST_VIDEO_INFO_HEIGHT(&output_state->info) != self->coded_height) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                          ("Decoder output must be NV12 at coded dimensions"), (nullptr));
        if (output_state)
            gst_video_codec_state_unref(output_state);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }

    GstFlowReturn frame_ret = GST_FLOW_OK;
    GstVideoCodecFrame* frame = find_output_frame(
        self, decoder, static_cast<GstClockTime>(pts), &frame_ret);
    if (frame_ret != GST_FLOW_OK) {
        gst_video_codec_state_unref(output_state);
        GstFlowReturn release_ret = release_buffer(self, output);
        return release_ret == GST_FLOW_OK ? frame_ret : release_ret;
    }

    if (!frame) {
        GST_WARNING_OBJECT(self, "No pending frame for decoder PTS=%" GST_TIME_FORMAT,
                           GST_TIME_ARGS(static_cast<GstClockTime>(pts)));
        gst_video_codec_state_unref(output_state);
        return release_buffer(self, output);
    }

    GstFlowReturn ret = gst_video_decoder_allocate_output_frame(decoder, frame);
    if (ret != GST_FLOW_OK) {
        gst_video_codec_state_unref(output_state);
        GstFlowReturn release_ret = release_buffer(self, output);
        gst_video_codec_frame_unref(frame);
        return release_ret == GST_FLOW_OK ? ret : release_ret;
    }

    GstVideoFrame destination;
    if (!gst_video_frame_map(&destination, &output_state->info, frame->output_buffer,
                             GST_MAP_WRITE)) {
        GST_ELEMENT_ERROR(self, RESOURCE, FAILED, ("Failed to map allocated NV12 frame"),
                          (nullptr));
        gst_video_codec_state_unref(output_state);
        GstFlowReturn release_ret = release_buffer(self, output);
        gst_video_decoder_drop_frame(decoder, frame);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }

    const auto* source = static_cast<const guint8*>(data);
    auto* destination_y = static_cast<guint8*>(GST_VIDEO_FRAME_PLANE_DATA(&destination, 0));
    const gint destination_y_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&destination, 0);
    auto* destination_uv = static_cast<guint8*>(GST_VIDEO_FRAME_PLANE_DATA(&destination, 1));
    const gint destination_uv_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&destination, 1);
    if (!destination_y || !destination_uv || destination_y_stride < self->coded_width ||
        destination_uv_stride < self->coded_width) {
        GST_ELEMENT_ERROR(self, STREAM, FORMAT, ("Invalid downstream NV12 frame layout"),
                          (nullptr));
        gst_video_frame_unmap(&destination);
        gst_video_codec_state_unref(output_state);
        GstFlowReturn release_ret = release_buffer(self, output);
        gst_video_decoder_drop_frame(decoder, frame);
        return release_ret == GST_FLOW_OK ? GST_FLOW_ERROR : release_ret;
    }
    for (gint row = 0; row < self->coded_height; ++row) {
        std::memcpy(destination_y + row * destination_y_stride,
                    source + static_cast<size_t>(row) * source_stride,
                    self->coded_width);
    }

    const auto* source_uv = source + source_stride * source_slice_height;
    for (gint row = 0; row < self->coded_height / 2; ++row) {
        std::memcpy(destination_uv + row * destination_uv_stride,
                    source_uv + static_cast<size_t>(row) * source_stride,
                    self->coded_width);
    }

    gst_video_frame_unmap(&destination);
    gst_video_codec_state_unref(output_state);

    ret = release_buffer(self, output);
    if (ret != GST_FLOW_OK) {
        gst_video_decoder_drop_frame(decoder, frame);
        return ret;
    }

    frame->pts = static_cast<GstClockTime>(pts);
    GST_BUFFER_PTS(frame->output_buffer) = frame->pts;
    GST_BUFFER_DTS(frame->output_buffer) = GST_CLOCK_TIME_NONE;
    ret = gst_video_decoder_finish_frame(decoder, frame);
    return ret;
}

static GstFlowReturn drop_pending_frames(GstDxVnpuDec* self,
                                         GstVideoDecoder* decoder) {
    GstFlowReturn result = GST_FLOW_OK;
    GList* frames = gst_video_decoder_get_frames(decoder);
    for (GList* item = frames; item; item = item->next) {
        const GstFlowReturn ret = gst_video_decoder_drop_frame(
            decoder, gst_video_codec_frame_ref(
                         static_cast<GstVideoCodecFrame*>(item->data)));
        if (result == GST_FLOW_OK && ret != GST_FLOW_OK)
            result = ret;
    }
    g_list_free_full(frames, (GDestroyNotify)gst_video_codec_frame_unref);
    return result;
}

static gboolean start_output_task(GstDxVnpuDec* self) {
    if (gst_pad_start_task(GST_VIDEO_DECODER_SRC_PAD(self),
                           gst_dxvnpudec_output_task, self, nullptr))
        return TRUE;

    GST_ELEMENT_ERROR(self, RESOURCE, FAILED, ("Failed to start decoder output task"), (nullptr));
    return FALSE;
}

static void gst_dxvnpudec_output_task(void* data) {
    auto* self = GST_DXVNPUDEC(data);
    auto* decoder = GST_VIDEO_DECODER(self);
    GstFlowReturn result = GST_FLOW_OK;
    gboolean eos = FALSE;

    while (!g_atomic_int_get(&self->flushing)) {
        dxvnpu_buffer_t output = nullptr;
        dxvnpu_pipeline_t pipeline = nullptr;
        if (!begin_pipeline_call(self, &pipeline)) {
            result = GST_FLOW_FLUSHING;
            break;
        }
        const dxvnpu_status_t status = dxvnpu_pipeline_get_buffer(
            pipeline, &output, kSdkPollTimeoutMs);

        if (g_atomic_int_get(&self->flushing)) {
            result = release_buffer(self, &output);
            end_pipeline_call(self);
            if (result == GST_FLOW_OK)
                result = GST_FLOW_FLUSHING;
            break;
        }

        if (status == DXVNPU_TIMEOUT) {
            result = release_buffer(self, &output);
            end_pipeline_call(self);
            if (result != GST_FLOW_OK)
                break;
            continue;
        }

        if (status == DXVNPU_END_OF_STREAM) {
            result = release_buffer(self, &output);
            if (result == GST_FLOW_OK) {
                GST_VIDEO_DECODER_STREAM_LOCK(decoder);
                if (!g_atomic_int_get(&self->flushing))
                    result = drop_pending_frames(self, decoder);
                GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
            }
            end_pipeline_call(self);
            eos = result == GST_FLOW_OK;
            break;
        }

        if (status != DXVNPU_OK) {
            result = release_buffer(self, &output);
            if (result == GST_FLOW_OK)
                result = sdk_flow_error(self, "dxvnpu_pipeline_get_buffer", status);
            end_pipeline_call(self);
            break;
        }

        if (!output) {
            GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                              ("Decoder returned a null output buffer"), (nullptr));
            end_pipeline_call(self);
            result = GST_FLOW_ERROR;
            break;
        }

        GST_VIDEO_DECODER_STREAM_LOCK(decoder);
        result = g_atomic_int_get(&self->flushing)
                     ? release_buffer(self, &output)
                     : process_hw_output(self, decoder, &output);
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        end_pipeline_call(self);
        if (result != GST_FLOW_OK)
            break;
    }

    if (g_atomic_int_get(&self->flushing) && result == GST_FLOW_OK)
        result = GST_FLOW_FLUSHING;
    complete_output_result(self, result, eos);
    gst_pad_pause_task(GST_VIDEO_DECODER_SRC_PAD(decoder));
}

G_DEFINE_TYPE_WITH_CODE(
    GstDxVnpuDec, gst_dxvnpudec, GST_TYPE_VIDEO_DECODER,
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpudec_debug_category, "dxvnpudec", 0,
                            "debug category for dxvnpudec element"))

static void gst_dxvnpudec_class_init(GstDxVnpuDecClass* klass) {
    auto* gobject_class = G_OBJECT_CLASS(klass);
    auto* videodec_class = GST_VIDEO_DECODER_CLASS(klass);
    auto* element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_dxvnpudec_set_property;
    gobject_class->get_property = gst_dxvnpudec_get_property;
    gobject_class->finalize = gst_dxvnpudec_finalize;
    element_class->change_state = GST_DEBUG_FUNCPTR(gst_dxvnpudec_change_state);

    static std::array<GParamSpec*,
                      static_cast<int>(DecPropertyID::N_PROPERTIES)> obj_properties = {nullptr};
    obj_properties[static_cast<guint>(DecPropertyID::PROP_DEVICE_ID)] =
        g_param_spec_int("device-id", "Device ID",
                         "DXVNPU device index (-1 selects automatically)",
                         -1, G_MAXINT8, -1,
                         static_cast<GParamFlags>(G_PARAM_READWRITE |
                                                  G_PARAM_STATIC_STRINGS |
                                                  GST_PARAM_MUTABLE_READY));
    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(DecPropertyID::N_PROPERTIES),
                                      obj_properties.data());

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXVNPUDEC_SINK_CAPS)));
    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXVNPUDEC_SRC_CAPS)));

    gst_element_class_set_static_metadata(
        element_class,
        "DX-VNPU Video Decoder",
        "Codec/Decoder/Video/Hardware",
        "Hardware-accelerated NV12 H.264/H.265 decoder using DEEPX VNPU",
        "Sangil Jo <sijo@deepx.ai>");

    videodec_class->start = GST_DEBUG_FUNCPTR(gst_dxvnpudec_start);
    videodec_class->stop = GST_DEBUG_FUNCPTR(gst_dxvnpudec_stop);
    videodec_class->set_format = GST_DEBUG_FUNCPTR(gst_dxvnpudec_set_format);
    videodec_class->handle_frame = GST_DEBUG_FUNCPTR(gst_dxvnpudec_handle_frame);
    videodec_class->finish = GST_DEBUG_FUNCPTR(gst_dxvnpudec_finish);
    videodec_class->flush = GST_DEBUG_FUNCPTR(gst_dxvnpudec_flush);
}

static GstStateChangeReturn gst_dxvnpudec_change_state(
    GstElement* element, GstStateChange transition) {
    if (transition == GST_STATE_CHANGE_PAUSED_TO_READY)
        g_atomic_int_set(&GST_DXVNPUDEC(element)->flushing, TRUE);
    return GST_ELEMENT_CLASS(gst_dxvnpudec_parent_class)->change_state(element, transition);
}

static void gst_dxvnpudec_init(GstDxVnpuDec* self) {
    self->pipeline = nullptr;
    self->codec = DXVNPU_CODEC_H264;
    self->device_id = -1;
    self->coded_width = 0;
    self->coded_height = 0;
    self->output_configured = FALSE;
    g_atomic_int_set(&self->flushing, FALSE);
    self->eos_sent = FALSE;
    g_mutex_init(&self->sdk_lock);
    g_mutex_init(&self->sdk_io_lock);
    g_cond_init(&self->sdk_idle);
    self->sdk_calls = 0;
    g_mutex_init(&self->output_lock);
    g_cond_init(&self->output_ready);
    self->output_flow = GST_FLOW_OK;
    self->output_eos = FALSE;
    gst_video_decoder_set_packetized(GST_VIDEO_DECODER(self), TRUE);
}

static void gst_dxvnpudec_finalize(GObject* object) {
    auto* self = GST_DXVNPUDEC(object);
    g_atomic_int_set(&self->flushing, TRUE);
    g_mutex_lock(&self->sdk_lock);
    while (self->sdk_calls)
        g_cond_wait(&self->sdk_idle, &self->sdk_lock);
    if (self->pipeline)
        dxvnpu_pipeline_destroy(&self->pipeline);
    g_mutex_unlock(&self->sdk_lock);
    g_cond_clear(&self->output_ready);
    g_mutex_clear(&self->output_lock);
    g_cond_clear(&self->sdk_idle);
    g_mutex_clear(&self->sdk_io_lock);
    g_mutex_clear(&self->sdk_lock);
    G_OBJECT_CLASS(gst_dxvnpudec_parent_class)->finalize(object);
}

static void gst_dxvnpudec_set_property(GObject* object, guint property_id,
                                       const GValue* value, GParamSpec* pspec) {
    auto* self = GST_DXVNPUDEC(object);
    switch (property_id) {
        case static_cast<guint>(DecPropertyID::PROP_DEVICE_ID):
            self->device_id = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpudec_get_property(GObject* object, guint property_id,
                                       GValue* value, GParamSpec* pspec) {
    auto* self = GST_DXVNPUDEC(object);
    switch (property_id) {
        case static_cast<guint>(DecPropertyID::PROP_DEVICE_ID):
            g_value_set_int(value, self->device_id);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static gboolean gst_dxvnpudec_start(GstVideoDecoder* decoder) {
    auto* self = GST_DXVNPUDEC(decoder);
    g_atomic_int_set(&self->flushing, FALSE);
    self->output_configured = FALSE;
    self->eos_sent = FALSE;
    reset_output_result(self);
    return TRUE;
}

static gboolean gst_dxvnpudec_stop(GstVideoDecoder* decoder) {
    auto* self = GST_DXVNPUDEC(decoder);
    const gboolean result = abort_pipeline(self, decoder);
    return result;
}

static gboolean gst_dxvnpudec_set_format(GstVideoDecoder* decoder,
                                         GstVideoCodecState* state) {
    auto* self = GST_DXVNPUDEC(decoder);

    if (!state || !state->caps || gst_caps_is_empty(state->caps)) {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION, ("Missing decoder input caps"), (nullptr));
        return FALSE;
    }

    const GstStructure* structure = gst_caps_get_structure(state->caps, 0);
    const gchar* name = gst_structure_get_name(structure);
    const gchar* stream_format = gst_structure_get_string(structure, "stream-format");
    const gchar* alignment = gst_structure_get_string(structure, "alignment");
    if (!g_strcmp0(name, "video/x-h264"))
        self->codec = DXVNPU_CODEC_H264;
    else if (!g_strcmp0(name, "video/x-h265"))
        self->codec = DXVNPU_CODEC_H265;
    else {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION, ("Unsupported codec: %s", name), (nullptr));
        return FALSE;
    }

    if (g_strcmp0(stream_format, "byte-stream") || g_strcmp0(alignment, "au")) {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION,
                          ("Decoder requires byte-stream access-unit input"), (nullptr));
        return FALSE;
    }

    gint width = 0;
    gint height = 0;
    if (!gst_structure_get_int(structure, "width", &width) ||
        !gst_structure_get_int(structure, "height", &height) ||
        width <= 0 || height <= 0) {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION,
                          ("Coded width and height must be positive"), (nullptr));
        return FALSE;
    }

    if (!abort_pipeline(self, decoder))
        return FALSE;

    GstVideoCodecState* output_state = gst_video_decoder_set_output_state(
        decoder, GST_VIDEO_FORMAT_NV12, width, height, state);
    gst_video_codec_state_unref(output_state);
    if (!gst_video_decoder_negotiate(decoder)) {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION, ("Downstream caps negotiation failed"),
                          (nullptr));
        return FALSE;
    }

    self->coded_width = width;
    self->coded_height = height;
    if (!create_pipeline(self)) {
        abort_pipeline(self, decoder);
        return FALSE;
    }

    g_atomic_int_set(&self->flushing, FALSE);
    self->eos_sent = FALSE;
    self->output_configured = TRUE;
    reset_output_result(self);
    if (start_output_task(self))
        return TRUE;

    abort_pipeline(self, decoder);
    return FALSE;
}

static GstFlowReturn gst_dxvnpudec_handle_frame(GstVideoDecoder* decoder,
                                                GstVideoCodecFrame* frame) {
    auto* self = GST_DXVNPUDEC(decoder);

    if (g_atomic_int_get(&self->flushing)) {
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_FLUSHING;
    }
    if (!self->pipeline || !self->output_configured) {
        GST_ELEMENT_ERROR(self, CORE, NEGOTIATION, ("Decoder pipeline is not configured"),
                          (nullptr));
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_NOT_NEGOTIATED;
    }
    GstBuffer* input_buffer = gst_buffer_ref(frame->input_buffer);
    const GstClockTime input_pts = GST_BUFFER_PTS(input_buffer);
    GstMapInfo input_map;
    if (!gst_buffer_map(input_buffer, &input_map, GST_MAP_READ)) {
        GST_ELEMENT_ERROR(self, RESOURCE, FAILED, ("Failed to map compressed input"), (nullptr));
        gst_buffer_unref(input_buffer);
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_ERROR;
    }

    dxvnpu_buffer_t input = nullptr;
    dxvnpu_pipeline_t pipeline = nullptr;
    dxvnpu_status_t status;
    const gchar* operation = "dxvnpu_pipeline_acquire_input_buffer";
    if (!begin_pipeline_call(self, &pipeline)) {
        gst_buffer_unmap(input_buffer, &input_map);
        gst_buffer_unref(input_buffer);
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_FLUSHING;
    }

    GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
    status = dxvnpu_pipeline_acquire_input_buffer(pipeline, &input, kSdkPollTimeoutMs);
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_buffer_set_data";
        status = dxvnpu_buffer_set_data(input, input_map.data, input_map.size);
    }
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_buffer_set_pts";
        status = dxvnpu_buffer_set_pts(
            input, input_pts == GST_CLOCK_TIME_NONE ? 0 : input_pts);
    }
    if (status == DXVNPU_OK) {
        operation = "dxvnpu_pipeline_put_buffer";
        do {
            g_mutex_lock(&self->sdk_io_lock);
            status = dxvnpu_pipeline_put_buffer(pipeline, input, kSdkPollTimeoutMs);
            g_mutex_unlock(&self->sdk_io_lock);
        } while (status == DXVNPU_TIMEOUT && !g_atomic_int_get(&self->flushing));
    }
    GST_VIDEO_DECODER_STREAM_LOCK(decoder);

    if (status != DXVNPU_OK && status != DXVNPU_TIMEOUT)
        sdk_flow_error(self, operation, status);
    const GstFlowReturn release_ret = release_buffer(self, &input);
    end_pipeline_call(self);
    gst_buffer_unmap(input_buffer, &input_map);
    gst_buffer_unref(input_buffer);

    if (release_ret != GST_FLOW_OK)
        return release_ret;
    if (g_atomic_int_get(&self->flushing))
        return GST_FLOW_FLUSHING;
    if (status == DXVNPU_TIMEOUT) {
        return GST_FLOW_FLUSHING;
    }
    if (status != DXVNPU_OK)
        return GST_FLOW_ERROR;

    return output_result(self);
}

static GstFlowReturn gst_dxvnpudec_finish(GstVideoDecoder* decoder) {
    auto* self = GST_DXVNPUDEC(decoder);

    if (g_atomic_int_get(&self->flushing) || !self->pipeline)
        return g_atomic_int_get(&self->flushing) ? GST_FLOW_FLUSHING : GST_FLOW_OK;

    if (!self->eos_sent) {
        dxvnpu_pipeline_t pipeline = nullptr;
        if (!begin_pipeline_call(self, &pipeline))
            return GST_FLOW_FLUSHING;
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        g_mutex_lock(&self->sdk_io_lock);
        const dxvnpu_status_t status = dxvnpu_pipeline_put_eos(pipeline);
        g_mutex_unlock(&self->sdk_io_lock);
        GST_VIDEO_DECODER_STREAM_LOCK(decoder);
        end_pipeline_call(self);
        if (status != DXVNPU_OK) {
            if (status != DXVNPU_TIMEOUT)
                return sdk_flow_error(self, "dxvnpu_pipeline_put_eos", status);
            GST_WARNING_OBJECT(self, "dxvnpu_pipeline_put_eos timed out");
            return GST_FLOW_ERROR;
        }
        self->eos_sent = TRUE;
    }

    g_mutex_lock(&self->output_lock);
    while (!self->output_eos && self->output_flow == GST_FLOW_OK &&
           !g_atomic_int_get(&self->flushing)) {
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        g_cond_wait(&self->output_ready, &self->output_lock);
        GST_VIDEO_DECODER_STREAM_LOCK(decoder);
    }
    const GstFlowReturn result = g_atomic_int_get(&self->flushing)
                                     ? GST_FLOW_FLUSHING
                                     : self->output_flow;
    g_mutex_unlock(&self->output_lock);
    return result;
}

static gboolean gst_dxvnpudec_flush(GstVideoDecoder* decoder) {
    auto* self = GST_DXVNPUDEC(decoder);
    return flush_pipeline(self, decoder);
}
