#include "gst-dxvnpudec.hpp"
#include <gst/video/video.h>
#include <cstring>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpudec_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpudec_debug_category

// ---------------------------------------------------------------------------
// Output format enum type for GObject property
// ---------------------------------------------------------------------------
#define GST_TYPE_DXVNPUDEC_FORMAT (gst_dxvnpudec_format_get_type())

static GType gst_dxvnpudec_format_get_type(void) {
    static GType fmt_type = 0;
    if (g_once_init_enter(&fmt_type)) {
        static const GEnumValue fmt_values[] = {
            {GST_VIDEO_FORMAT_NV12, "NV12 (YUV 4:2:0 semi-planar)", "NV12"},
            {GST_VIDEO_FORMAT_RGB,  "RGB (24-bit)", "RGB"},
            {GST_VIDEO_FORMAT_BGR,  "BGR (24-bit)", "BGR"},
            {0, nullptr, nullptr}
        };
        GType tmp = g_enum_register_static("GstDxVnpuDecFormat", fmt_values);
        g_once_init_leave(&fmt_type, tmp);
    }
    return fmt_type;
}

// ---------------------------------------------------------------------------
// Property IDs
// ---------------------------------------------------------------------------
enum class DecPropertyID {
    PROP_0, PROP_OUTPUT_FORMAT, PROP_OUTPUT_WIDTH, PROP_OUTPUT_HEIGHT, N_PROPERTIES
};

// ---------------------------------------------------------------------------
// Caps templates
// ---------------------------------------------------------------------------
#define DXVNPUDEC_SINK_CAPS \
    "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; " \
    "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"

#define DXVNPUDEC_SRC_CAPS \
    "video/x-raw, format=(string){ NV12, RGB, BGR }"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void gst_dxvnpudec_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec);
static void gst_dxvnpudec_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec);
static void gst_dxvnpudec_finalize(GObject *object);

static gboolean gst_dxvnpudec_start(GstVideoDecoder *decoder);
static gboolean gst_dxvnpudec_stop(GstVideoDecoder *decoder);
static gboolean gst_dxvnpudec_set_format(GstVideoDecoder *decoder,
                                           GstVideoCodecState *state);
static GstFlowReturn gst_dxvnpudec_handle_frame(GstVideoDecoder *decoder,
                                                   GstVideoCodecFrame *frame);
static GstFlowReturn gst_dxvnpudec_finish(GstVideoDecoder *decoder);
static GstFlowReturn gst_dxvnpudec_drain(GstVideoDecoder *decoder);
static gboolean gst_dxvnpudec_flush(GstVideoDecoder *decoder);

// ---------------------------------------------------------------------------
// GObject boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxVnpuDec, gst_dxvnpudec, GST_TYPE_VIDEO_DECODER,
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpudec_debug_category, "dxvnpudec", 0,
                            "debug category for dxvnpudec element"))

// ---------------------------------------------------------------------------
// Helper: GstVideoFormat -> dxvnpu::ColorFormat
// ---------------------------------------------------------------------------
static dxvnpu::ColorFormat gst_to_dxvnpu_color(GstVideoFormat fmt) {
    switch (fmt) {
        case GST_VIDEO_FORMAT_NV12: return dxvnpu::COLOR_FORMAT_YUV420SP;
        case GST_VIDEO_FORMAT_RGB:  return dxvnpu::COLOR_FORMAT_RGB888;
        case GST_VIDEO_FORMAT_BGR:  return dxvnpu::COLOR_FORMAT_BGR888;
        default: return dxvnpu::COLOR_FORMAT_YUV420SP;
    }
}

// ---------------------------------------------------------------------------
// class_init
// ---------------------------------------------------------------------------
static void gst_dxvnpudec_class_init(GstDxVnpuDecClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *videodec_class = GST_VIDEO_DECODER_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_dxvnpudec_set_property;
    gobject_class->get_property = gst_dxvnpudec_get_property;
    gobject_class->finalize = gst_dxvnpudec_finalize;

    // Properties
    static std::array<GParamSpec*,
                      static_cast<int>(DecPropertyID::N_PROPERTIES)> obj_properties = {nullptr};

    obj_properties[static_cast<guint>(DecPropertyID::PROP_OUTPUT_FORMAT)] =
        g_param_spec_enum("output-format", "Output Format",
                          "Output pixel format from HW Processor",
                          GST_TYPE_DXVNPUDEC_FORMAT,
                          GST_VIDEO_FORMAT_NV12,
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(DecPropertyID::PROP_OUTPUT_WIDTH)] =
        g_param_spec_int("output-width", "Output Width",
                         "Output width (0 = same as input)",
                         0, 8192, 0,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(DecPropertyID::PROP_OUTPUT_HEIGHT)] =
        g_param_spec_int("output-height", "Output Height",
                         "Output height (0 = same as input)",
                         0, 8192, 0,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(DecPropertyID::N_PROPERTIES),
                                      obj_properties.data());

    // Pad templates
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
        "Hardware-accelerated video decoder using DEEPX VNPU (H.264/H.265)",
        "Sangil Jo <sijo@deepx.ai>");

    videodec_class->start = GST_DEBUG_FUNCPTR(gst_dxvnpudec_start);
    videodec_class->stop = GST_DEBUG_FUNCPTR(gst_dxvnpudec_stop);
    videodec_class->set_format = GST_DEBUG_FUNCPTR(gst_dxvnpudec_set_format);
    videodec_class->handle_frame = GST_DEBUG_FUNCPTR(gst_dxvnpudec_handle_frame);
    videodec_class->finish = GST_DEBUG_FUNCPTR(gst_dxvnpudec_finish);
    videodec_class->drain = GST_DEBUG_FUNCPTR(gst_dxvnpudec_drain);
    videodec_class->flush = GST_DEBUG_FUNCPTR(gst_dxvnpudec_flush);
}

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxvnpudec_init(GstDxVnpuDec *self) {
    new (&self->pipeline) std::shared_ptr<dxvnpu::MediaPipeline>();

    self->codec = dxvnpu::CODEC_H264;
    self->input_state = nullptr;
    self->output_configured = FALSE;

    self->output_format = GST_VIDEO_FORMAT_NV12;
    self->output_width = 0;
    self->output_height = 0;

    self->hw_pending = 0;
    self->last_flow = GST_FLOW_OK;

    gst_video_decoder_set_packetized(GST_VIDEO_DECODER(self), TRUE);
}

static void gst_dxvnpudec_finalize(GObject *object) {
    auto *self = GST_DXVNPUDEC(object);
    self->pipeline.~shared_ptr();
    G_OBJECT_CLASS(gst_dxvnpudec_parent_class)->finalize(object);
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
static void gst_dxvnpudec_set_property(GObject *object, guint property_id,
                                        const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUDEC(object);
    switch (property_id) {
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_FORMAT):
            self->output_format = static_cast<GstVideoFormat>(g_value_get_enum(value));
            break;
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_WIDTH):
            self->output_width = g_value_get_int(value);
            break;
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_HEIGHT):
            self->output_height = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpudec_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUDEC(object);
    switch (property_id) {
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_FORMAT):
            g_value_set_enum(value, self->output_format);
            break;
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_WIDTH):
            g_value_set_int(value, self->output_width);
            break;
        case static_cast<guint>(DecPropertyID::PROP_OUTPUT_HEIGHT):
            g_value_set_int(value, self->output_height);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

// ---------------------------------------------------------------------------
// process_hw_output — match HW output to correct GstVideoCodecFrame by PTS
//
// HW outputs frames in display order (monotonically increasing PTS).
// Uses non-destructive search: get_frames() to find PTS match.
// Drops frames with PTS < HW_PTS (will never be output by HW).
// Caller MUST hold stream lock.
// ---------------------------------------------------------------------------
static GstFlowReturn process_hw_output(GstDxVnpuDec *self,
                                        std::shared_ptr<dxvnpu::BaseData> out) {
    GstVideoDecoder *decoder = GST_VIDEO_DECODER(self);

    if (out->isEOS) {
        GST_DEBUG_OBJECT(self, "Received EOS from HW");
        self->hw_pending = 0;
        return GST_FLOW_OK;
    }

    if (!out->isFrame()) return GST_FLOW_OK;

    auto dx_frame = std::dynamic_pointer_cast<dxvnpu::DXFrame>(out);
    if (!dx_frame) {
        GST_WARNING_OBJECT(self, "isFrame() true but cast failed, skipping");
        return GST_FLOW_OK;
    }

    GstClockTime hw_pts = (GstClockTime)dx_frame->timestamp;

    GST_LOG_OBJECT(self, "GetData OK, hw_pending=%d, HW_PTS=%" GST_TIME_FORMAT,
                     self->hw_pending, GST_TIME_ARGS(hw_pts));

    // Non-destructive search: get all pending frames
    GList *frames = gst_video_decoder_get_frames(decoder);

    // Pass 1: drop frames with PTS < HW_PTS (HW will never output these)
    for (GList *l = frames; l != nullptr; l = l->next) {
        auto *f = static_cast<GstVideoCodecFrame *>(l->data);
        GstClockTime f_pts = GST_BUFFER_PTS(f->input_buffer);
        if (f_pts < hw_pts) {
            GST_DEBUG_OBJECT(self, "Dropping HW-skipped frame PTS=%" GST_TIME_FORMAT,
                             GST_TIME_ARGS(f_pts));
            gst_video_codec_frame_ref(f);
            gst_video_decoder_drop_frame(decoder, f);
            if (self->hw_pending > 0)
                self->hw_pending--;
        }
    }

    // Pass 2: find exact PTS match
    GstVideoCodecFrame *match = nullptr;
    for (GList *l = frames; l != nullptr; l = l->next) {
        auto *f = static_cast<GstVideoCodecFrame *>(l->data);
        GstClockTime f_pts = GST_BUFFER_PTS(f->input_buffer);
        if (f_pts == hw_pts) {
            match = gst_video_codec_frame_ref(f);
            break;
        }
    }

    g_list_free_full(frames, (GDestroyNotify)gst_video_codec_frame_unref);

    if (!match) {
        GST_WARNING_OBJECT(self, "No pending frame matches HW_PTS=%" GST_TIME_FORMAT,
                           GST_TIME_ARGS(hw_pts));
        if (self->hw_pending > 0)
            self->hw_pending--;
        return GST_FLOW_OK;
    }

    if (self->hw_pending > 0)
        self->hw_pending--;

    GstFlowReturn ret = gst_video_decoder_allocate_output_frame(decoder, match);
    if (ret != GST_FLOW_OK) {
        GST_WARNING_OBJECT(self, "allocate_output_frame failed: %s",
                           gst_flow_get_name(ret));
        gst_video_codec_frame_unref(match);
        return ret;
    }

    GstMapInfo out_map;
    if (!gst_buffer_map(match->output_buffer, &out_map, GST_MAP_WRITE)) {
        GST_ERROR_OBJECT(self, "Failed to map output buffer");
        gst_video_codec_frame_unref(match);
        return GST_FLOW_ERROR;
    }

    if (out_map.size != dx_frame->data_size) {
        GST_WARNING_OBJECT(self,
            "Output buffer size mismatch: GstBuffer=%zu, DXFrame=%zu",
            out_map.size, dx_frame->data_size);
    }
    std::memcpy(out_map.data, dx_frame->getRawData(),
                std::min(out_map.size, dx_frame->data_size));
    gst_buffer_unmap(match->output_buffer, &out_map);

    // Use HW_PTS as output PTS (display order), DTS is meaningless post-decode
    match->pts = hw_pts;
    GST_BUFFER_PTS(match->output_buffer) = hw_pts;
    GST_BUFFER_DTS(match->output_buffer) = GST_CLOCK_TIME_NONE;

    ret = gst_video_decoder_finish_frame(decoder, match);
    GST_LOG_OBJECT(self, "finish_frame returned %s", gst_flow_get_name(ret));
    self->last_flow = ret;
    return ret;
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpudec_start(GstVideoDecoder *decoder) {
    auto *self = GST_DXVNPUDEC(decoder);
    self->output_configured = FALSE;
    self->last_flow = GST_FLOW_OK;
    self->hw_pending = 0;
    GST_DEBUG_OBJECT(self, "Started");
    return TRUE;
}

static gboolean gst_dxvnpudec_stop(GstVideoDecoder *decoder) {
    auto *self = GST_DXVNPUDEC(decoder);
    self->output_configured = FALSE;
    self->hw_pending = 0;
    self->pipeline.reset();
    if (self->input_state) {
        gst_video_codec_state_unref(self->input_state);
        self->input_state = nullptr;
    }
    GST_DEBUG_OBJECT(self, "Stopped");
    return TRUE;
}

// ---------------------------------------------------------------------------
// set_format — create/replace HW pipeline using property values
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpudec_set_format(GstVideoDecoder *decoder,
                                           GstVideoCodecState *state) {
    auto *self = GST_DXVNPUDEC(decoder);

    // Determine codec from caps
    GstStructure *s = gst_caps_get_structure(state->caps, 0);
    const gchar *name = gst_structure_get_name(s);
    if (g_str_equal(name, "video/x-h264"))
        self->codec = dxvnpu::CODEC_H264;
    else if (g_str_equal(name, "video/x-h265"))
        self->codec = dxvnpu::CODEC_H265;
    else {
        GST_ERROR_OBJECT(self, "Unsupported codec: %s", name);
        return FALSE;
    }

    gint width = 0, height = 0;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;

    // Store input state
    if (self->input_state)
        gst_video_codec_state_unref(self->input_state);
    self->input_state = gst_video_codec_state_ref(state);

    // Resolve output dimensions from properties (0 = same as input)
    GstVideoFormat out_fmt = self->output_format;
    gint out_width = (self->output_width > 0) ? self->output_width : width;
    gint out_height = (self->output_height > 0) ? self->output_height : height;
    dxvnpu::ColorFormat dxvnpu_out_fmt = gst_to_dxvnpu_color(out_fmt);

    // Set output state and negotiate
    GstVideoCodecState *output_state = gst_video_decoder_set_output_state(
        decoder, out_fmt, out_width, out_height, state);
    gst_video_codec_state_unref(output_state);
    if (!gst_video_decoder_negotiate(decoder)) {
        GST_ERROR_OBJECT(self, "Downstream caps negotiation failed");
        return FALSE;
    }

    // Build HW pipeline
    self->pipeline.reset();

    try {
        dxvnpu::DecoderConfig dec_cfg;
        dec_cfg.codec = self->codec;
        dec_cfg.stream_width = width;
        dec_cfg.stream_height = height;

        dxvnpu::ProcessorConfig proc_cfg;
        proc_cfg.input_width = width;
        proc_cfg.input_height = height;
        proc_cfg.input_format = dxvnpu::COLOR_FORMAT_YUV420SP;
        proc_cfg.input_fps = -1;
        proc_cfg.output_width = out_width;
        proc_cfg.output_height = out_height;
        proc_cfg.output_format = dxvnpu_out_fmt;
        proc_cfg.output_fps = -1;

        self->pipeline = dxvnpu::MediaPipelineBuilder()
            .AddDecoder("dec", dec_cfg)
            .AddProcessor("proc", proc_cfg)
            .Connect("dec", "proc")
            .Build();
    } catch (const std::exception &e) {
        GST_ERROR_OBJECT(self, "Failed to create VNPU pipeline: %s", e.what());
        return FALSE;
    }

    if (!self->pipeline) {
        GST_ERROR_OBJECT(self, "Failed to create VNPU pipeline: Build() returned null");
        return FALSE;
    }

    self->hw_pending = 0;
    self->last_flow = GST_FLOW_OK;
    self->output_configured = TRUE;

    // Report HW decoder latency to downstream elements
    gst_video_decoder_set_latency(decoder, 100 * GST_MSECOND, GST_CLOCK_TIME_NONE);

    GST_INFO_OBJECT(self, "VNPU decoder pipeline created: %s %dx%d -> %s %dx%d",
                    (self->codec == dxvnpu::CODEC_H264) ? "H.264" : "H.265",
                    width, height,
                    gst_video_format_to_string(out_fmt),
                    out_width, out_height);

    return TRUE;
}

// ---------------------------------------------------------------------------
// handle_frame — submit input to HW, collect ready outputs
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpudec_handle_frame(GstVideoDecoder *decoder,
                                                   GstVideoCodecFrame *frame) {
    auto *self = GST_DXVNPUDEC(decoder);
    auto pl = self->pipeline;

    GST_LOG_OBJECT(self, "handle_frame: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(frame->input_buffer)));

    if (!pl) {
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    if (GST_BUFFER_PTS(frame->input_buffer) == GST_CLOCK_TIME_NONE) {
        GST_DEBUG_OBJECT(self, "Dropping frame with PTS NONE");
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_OK;
    }

    // Step 1: Drain all ready HW outputs (non-blocking)
    while (true) {
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        auto out = pl->GetData("proc", 0);
        GST_VIDEO_DECODER_STREAM_LOCK(decoder);

        if (!out) break;

        GstFlowReturn ret = process_hw_output(self, std::move(out));
        if (ret != GST_FLOW_OK) {
            gst_video_decoder_drop_frame(decoder, frame);
            return ret;
        }
    }

    // Step 2: Map input buffer and create bitstream packet
    GstMapInfo map;
    if (!gst_buffer_map(frame->input_buffer, &map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        gst_video_decoder_drop_frame(decoder, frame);
        return GST_FLOW_ERROR;
    }

    auto pkt = std::make_shared<dxvnpu::DXBitStream>();
    pkt->codec = self->codec;
    pkt->timestamp = GST_BUFFER_PTS(frame->input_buffer);
    pkt->allocateData(map.size);
    std::memcpy(pkt->getRawData(), map.data, map.size);

    gst_buffer_unmap(frame->input_buffer, &map);

    // Step 3: PutData — single attempt, drop on failure
    GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
    dxvnpu::Status status = pl->PutData("dec", pkt, 100);
    GST_VIDEO_DECODER_STREAM_LOCK(decoder);

    if (status == dxvnpu::Status::OK) {
        self->hw_pending++;
        GST_LOG_OBJECT(self, "PutData OK, hw_pending=%d, PTS=%" GST_TIME_FORMAT,
                         self->hw_pending, GST_TIME_ARGS(pkt->timestamp));
        gst_video_codec_frame_unref(frame);
        return self->last_flow;
    }

    GST_WARNING_OBJECT(self, "PutData FAILED status=%d, hw_pending=%d",
                       (int)status, self->hw_pending);
    gst_video_decoder_drop_frame(decoder, frame);
    return (status == dxvnpu::Status::ERROR_TIMEOUT)
               ? self->last_flow : GST_FLOW_ERROR;
}

// ---------------------------------------------------------------------------
// finish — EOS: send EOS to HW, drain all remaining frames (blocking)
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpudec_finish(GstVideoDecoder *decoder) {
    auto *self = GST_DXVNPUDEC(decoder);
    auto pl = self->pipeline;

    if (!pl)
        return GST_FLOW_OK;

    GST_DEBUG_OBJECT(self, "Finishing: sending EOS to HW, hw_pending=%d",
                     self->hw_pending);

    // Send EOS to HW
    auto eos_pkt = std::make_shared<dxvnpu::DXBitStream>();
    eos_pkt->isEOS = true;

    GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
    pl->PutData("dec", eos_pkt, 1000);
    GST_VIDEO_DECODER_STREAM_LOCK(decoder);

    // Drain all remaining frames with blocking GetData
    constexpr int kFinishTimeoutMs = 5000;
    constexpr int kPollMs = 100;

    for (int elapsed = 0;
         elapsed < kFinishTimeoutMs && self->hw_pending > 0;
         elapsed += kPollMs) {
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        auto out = pl->GetData("proc", kPollMs);
        GST_VIDEO_DECODER_STREAM_LOCK(decoder);

        if (out) {
            process_hw_output(self, std::move(out));
            elapsed = 0;  // reset timeout on progress
        }
    }

    if (self->hw_pending > 0) {
        GST_WARNING_OBJECT(self, "Finish drain timed out, hw_pending=%d",
                           self->hw_pending);
    }

    GST_DEBUG_OBJECT(self, "Finish complete");
    return GST_FLOW_OK;
}

// ---------------------------------------------------------------------------
// drain — flush buffered frames without ending the stream
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpudec_drain(GstVideoDecoder *decoder) {
    auto *self = GST_DXVNPUDEC(decoder);
    auto pl = self->pipeline;

    if (!pl || self->hw_pending <= 0)
        return GST_FLOW_OK;

    GST_DEBUG_OBJECT(self, "Draining: hw_pending=%d", self->hw_pending);

    constexpr int kDrainTimeoutMs = 3000;
    constexpr int kPollMs = 100;

    for (int elapsed = 0;
         elapsed < kDrainTimeoutMs && self->hw_pending > 0;
         elapsed += kPollMs) {
        GST_VIDEO_DECODER_STREAM_UNLOCK(decoder);
        auto out = pl->GetData("proc", kPollMs);
        GST_VIDEO_DECODER_STREAM_LOCK(decoder);

        if (out) {
            process_hw_output(self, std::move(out));
            elapsed = 0;
        }
    }

    if (self->hw_pending > 0) {
        GST_WARNING_OBJECT(self, "Drain timed out, hw_pending=%d",
                           self->hw_pending);
    }

    return GST_FLOW_OK;
}

// ---------------------------------------------------------------------------
// flush — reset state after seek (FLUSH_STOP)
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpudec_flush(GstVideoDecoder *decoder) {
    auto *self = GST_DXVNPUDEC(decoder);

    GST_DEBUG_OBJECT(self, "Flushing: resetting state");

    self->hw_pending = 0;
    self->last_flow = GST_FLOW_OK;

    return TRUE;
}
