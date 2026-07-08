#include "gst-dxvnpupipeline.hpp"
#include "utils.hpp"
#include <cstring>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpupipeline_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpupipeline_debug_category

// ---------------------------------------------------------------------------
// Property IDs
// ---------------------------------------------------------------------------
enum class PipelinePropertyID {
    PROP_0, PROP_MODEL_PATH, PROP_INFERENCE_ID,
    PROP_KEEP_RATIO, PROP_USE_ORT, PROP_DEVICE_ID, PROP_USE_VNPU_HDMI, PROP_MAX_HDMI_CHANNELS,
    N_PROPERTIES
};

// ---------------------------------------------------------------------------
// Caps templates — bitstream in, tensor out
// ---------------------------------------------------------------------------
#define DXVNPUPIPELINE_SINK_CAPS \
    "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; " \
    "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"

#define DXVNPUPIPELINE_SRC_CAPS "application/x-dxtensor"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_set_property(GObject *object, guint property_id,
                                              const GValue *value, GParamSpec *pspec);
static void gst_dxvnpupipeline_get_property(GObject *object, guint property_id,
                                              GValue *value, GParamSpec *pspec);
static void gst_dxvnpupipeline_dispose(GObject *object);
static void gst_dxvnpupipeline_finalize(GObject *object);

static GstPad *gst_dxvnpupipeline_request_new_pad(GstElement *element,
                                                    GstPadTemplate *templ,
                                                    const gchar *name,
                                                    const GstCaps *caps);
static void gst_dxvnpupipeline_release_pad(GstElement *element, GstPad *pad);
static GstStateChangeReturn gst_dxvnpupipeline_change_state(GstElement *element,
                                                              GstStateChange transition);

static GstFlowReturn gst_dxvnpupipeline_chain(GstPad *pad, GstObject *parent,
                                                GstBuffer *buf);
static gboolean gst_dxvnpupipeline_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event);

static void stop_thread(DxVnpuChannelCtx *ctx);
static void stop_channel(GstDxVnpuPipeline *self, DxVnpuChannelCtx *ctx);
static gboolean setup_channel_pipeline(GstDxVnpuPipeline *self,
                                        std::shared_ptr<DxVnpuChannelCtx> ctx,
                                        GstCaps *caps);
static void channel_output_loop(GstDxVnpuPipeline *self, std::shared_ptr<DxVnpuChannelCtx> ctx);
static gboolean gst_dxvnpupipeline_src_event(GstPad *pad, GstObject *parent,
                                              GstEvent *event);
static gboolean gst_dxvnpupipeline_src_query(GstPad *pad, GstObject *parent,
                                              GstQuery *query);
static gboolean gst_dxvnpupipeline_sink_query(GstPad *pad, GstObject *parent,
                                               GstQuery *query);

// ---------------------------------------------------------------------------
// Helper: find channel context from sink pad
// ---------------------------------------------------------------------------
static std::shared_ptr<DxVnpuChannelCtx> get_channel_from_sink(GstDxVnpuPipeline *self, GstPad *pad) {
    std::lock_guard<std::mutex> lock(self->channels_lock);
    gint idx = get_sink_pad_index(pad);
    auto it = self->channels.find(idx);
    if (it != self->channels.end())
        return it->second;
    return nullptr;
}

static std::shared_ptr<DxVnpuChannelCtx> get_channel_from_src(GstDxVnpuPipeline *self, GstPad *pad) {
    std::lock_guard<std::mutex> lock(self->channels_lock);
    gint idx = get_src_pad_index(pad);
    auto it = self->channels.find(idx);
    if (it != self->channels.end())
        return it->second;
    return nullptr;
}

// ---------------------------------------------------------------------------
// GObject boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE(GstDxVnpuPipeline, gst_dxvnpupipeline, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;

// ---------------------------------------------------------------------------
// class_init
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_class_init(GstDxVnpuPipelineClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpupipeline_debug_category,
                            "dxvnpupipeline", 0, "DXVnpuPipeline plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = gst_dxvnpupipeline_set_property;
    gobject_class->get_property = gst_dxvnpupipeline_get_property;
    gobject_class->dispose = gst_dxvnpupipeline_dispose;
    gobject_class->finalize = gst_dxvnpupipeline_finalize;

    static std::array<GParamSpec*,
                      static_cast<int>(PipelinePropertyID::N_PROPERTIES)> obj_properties = {nullptr};

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_MODEL_PATH)] =
        g_param_spec_string("model-path", "Model Path",
                            "Path to .dxnn model file",
                            nullptr,
                            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_INFERENCE_ID)] =
        g_param_spec_uint("inference-id", "Inference ID",
                          "Key for _output_tensors map in DXFrameMeta",
                          0, G_MAXUINT, 0,
                          static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_KEEP_RATIO)] =
        g_param_spec_boolean("keep-ratio", "Keep Ratio",
                             "Maintain aspect ratio during processor resize",
                             TRUE,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_USE_ORT)] =
        g_param_spec_boolean("use-ort", "Use ORT",
                             "Use ORT runtime for inference",
                             TRUE,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_DEVICE_ID)] =
        g_param_spec_int("device-id", "Device ID",
                         "VNPU device ID (-1 for auto round-robin)",
                         -1, G_MAXINT, -1,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_USE_VNPU_HDMI)] =
        g_param_spec_boolean("use-vnpu-hdmi", "Use VNPU HDMI",
                             "Enable VNPU device HDMI video output",
                             FALSE,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    obj_properties[static_cast<guint>(PipelinePropertyID::PROP_MAX_HDMI_CHANNELS)] =
        g_param_spec_int("max-hdmi-channels", "Max HDMI Channels",
                         "Maximum number of channels to output to HDMI",
                         1, G_MAXINT, 32,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(PipelinePropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *element_class = GST_ELEMENT_CLASS(klass);

    static GstStaticPadTemplate sink_template =
        GST_STATIC_PAD_TEMPLATE("sink_%u", GST_PAD_SINK, GST_PAD_REQUEST,
                                GST_STATIC_CAPS(DXVNPUPIPELINE_SINK_CAPS));

    static GstStaticPadTemplate src_template =
        GST_STATIC_PAD_TEMPLATE("src_%u", GST_PAD_SRC, GST_PAD_SOMETIMES,
                                GST_STATIC_CAPS(DXVNPUPIPELINE_SRC_CAPS));

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    gst_element_class_set_static_metadata(
        element_class,
        "DX-VNPU Pipeline (VO mode)",
        "Generic",
        "Multi-channel bitstream decode/infer pipeline with HDMI VO output using DEEPX VNPU",
        "Sangil Jo <sijo@deepx.ai>");

    element_class->request_new_pad = GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_request_new_pad);
    element_class->release_pad = GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_release_pad);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = gst_dxvnpupipeline_change_state;
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_init(GstDxVnpuPipeline *self) {
    new (&self->channels) std::map<int, std::shared_ptr<DxVnpuChannelCtx>>();
    new (&self->channels_lock) std::mutex();

    self->model_path = nullptr;
    self->inference_id = 0;
    self->keep_ratio = TRUE;
    self->use_ort = TRUE;
    self->device_id = -1;
    self->use_vnpu_hdmi = FALSE;
    self->max_hdmi_channels = 32;
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_set_property(GObject *object, guint property_id,
                                              const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUPIPELINE(object);
    switch (property_id) {
        case static_cast<guint>(PipelinePropertyID::PROP_MODEL_PATH):
            g_free(self->model_path);
            self->model_path = g_value_dup_string(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_INFERENCE_ID):
            self->inference_id = g_value_get_uint(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_KEEP_RATIO):
            self->keep_ratio = g_value_get_boolean(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_USE_ORT):
            self->use_ort = g_value_get_boolean(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_DEVICE_ID):
            self->device_id = g_value_get_int(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_USE_VNPU_HDMI):
            self->use_vnpu_hdmi = g_value_get_boolean(value);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_MAX_HDMI_CHANNELS):
            self->max_hdmi_channels = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpupipeline_get_property(GObject *object, guint property_id,
                                              GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUPIPELINE(object);
    switch (property_id) {
        case static_cast<guint>(PipelinePropertyID::PROP_MODEL_PATH):
            g_value_set_string(value, self->model_path);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_INFERENCE_ID):
            g_value_set_uint(value, self->inference_id);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_KEEP_RATIO):
            g_value_set_boolean(value, self->keep_ratio);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_USE_ORT):
            g_value_set_boolean(value, self->use_ort);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_DEVICE_ID):
            g_value_set_int(value, self->device_id);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_USE_VNPU_HDMI):
            g_value_set_boolean(value, self->use_vnpu_hdmi);
            break;
        case static_cast<guint>(PipelinePropertyID::PROP_MAX_HDMI_CHANNELS):
            g_value_set_int(value, self->max_hdmi_channels);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

// ---------------------------------------------------------------------------
// dispose / finalize
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_dispose(GObject *object) {
    auto *self = GST_DXVNPUPIPELINE(object);
    std::map<int, std::shared_ptr<DxVnpuChannelCtx>> local_channels;
    {
        std::lock_guard<std::mutex> lock(self->channels_lock);
        local_channels = std::move(self->channels);
    }
    for (auto &pair : local_channels) {
        stop_channel(self, pair.second.get());
        gst_element_remove_pad(GST_ELEMENT(self), pair.second->sinkpad);
        gst_element_remove_pad(GST_ELEMENT(self), pair.second->srcpad);
    }
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxvnpupipeline_finalize(GObject *object) {
    auto *self = GST_DXVNPUPIPELINE(object);

    // channels already cleaned in dispose()

    g_free(self->model_path);
    self->model_path = nullptr;

    self->channels.~map();
    self->channels_lock.~mutex();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

// ---------------------------------------------------------------------------
// stop_thread / stop_channel
// ---------------------------------------------------------------------------
static void stop_thread(DxVnpuChannelCtx *ctx) {
    ctx->output_running.store(false, std::memory_order_release);
    if (ctx->output_thread && ctx->output_thread->joinable())
        ctx->output_thread->join();
    ctx->output_thread.reset();

    ctx->put_count.store(0, std::memory_order_relaxed);
    ctx->get_count.store(0, std::memory_order_relaxed);
}

static void stop_channel(GstDxVnpuPipeline *self, DxVnpuChannelCtx *ctx) {
    std::ignore = self;
    ctx->flushing.store(true, std::memory_order_release);
    stop_thread(ctx);
    ctx->pipeline.reset();
}

// ---------------------------------------------------------------------------
// request_new_pad — create paired sink_%u + src_%u
// ---------------------------------------------------------------------------
static GstPad *gst_dxvnpupipeline_request_new_pad(GstElement *element,
                                                    GstPadTemplate *templ,
                                                    const gchar *name,
                                                    const GstCaps *caps) {
    std::ignore = caps;
    auto *self = GST_DXVNPUPIPELINE(element);

    std::lock_guard<std::mutex> lock(self->channels_lock);

    if (templ->direction != GST_PAD_SINK) {
        GST_ERROR_OBJECT(self, "Only sink pad requests are supported");
        return nullptr;
    }

    gchar *sink_name = name
        ? g_strdup(name)
        : g_strdup_printf("sink_%u", static_cast<guint>(self->channels.size()));

    gint channel_id = atoi(sink_name + 5);

    auto existing = self->channels.find(channel_id);
    if (existing != self->channels.end()) {
        g_free(sink_name);
        return existing->second->sinkpad;
    }

    gchar *src_name = g_strdup_printf("src_%u", channel_id);

    auto ctx = std::make_shared<DxVnpuChannelCtx>();
    ctx->channel_id = channel_id;

    GstPadTemplate *sink_templ = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(element), "sink_%u");
    ctx->sinkpad = gst_pad_new_from_template(sink_templ, sink_name);
    gst_pad_set_chain_function(ctx->sinkpad, GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_chain));
    gst_pad_set_event_function(ctx->sinkpad, GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_sink_event));
    gst_pad_set_query_function(ctx->sinkpad, GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_sink_query));
    gst_pad_set_active(ctx->sinkpad, TRUE);
    gst_element_add_pad(element, ctx->sinkpad);

    GstPadTemplate *src_templ = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(element), "src_%u");
    ctx->srcpad = gst_pad_new_from_template(src_templ, src_name);
    gst_pad_set_event_function(ctx->srcpad, GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_src_event));
    gst_pad_set_query_function(ctx->srcpad, GST_DEBUG_FUNCPTR(gst_dxvnpupipeline_src_query));
    gst_pad_set_active(ctx->srcpad, TRUE);
    gst_element_add_pad(element, ctx->srcpad);

    GstPad *ret_pad = ctx->sinkpad;
    self->channels[channel_id] = std::move(ctx);

    GST_INFO_OBJECT(self, "Created channel %d (%s / %s)", channel_id, sink_name, src_name);

    g_free(sink_name);
    g_free(src_name);
    return ret_pad;
}

// ---------------------------------------------------------------------------
// release_pad
// ---------------------------------------------------------------------------
static void gst_dxvnpupipeline_release_pad(GstElement *element, GstPad *pad) {
    auto *self = GST_DXVNPUPIPELINE(element);
    gint channel_id = get_sink_pad_index(pad);

    std::lock_guard<std::mutex> lock(self->channels_lock);
    auto it = self->channels.find(channel_id);
    if (it != self->channels.end()) {
        stop_channel(self, it->second.get());
        gst_element_remove_pad(element, it->second->sinkpad);
        gst_element_remove_pad(element, it->second->srcpad);
        self->channels.erase(it);
    }
}

// ---------------------------------------------------------------------------
// change_state
// ---------------------------------------------------------------------------
static GstStateChangeReturn
gst_dxvnpupipeline_change_state(GstElement *element, GstStateChange transition) {
    auto *self = GST_DXVNPUPIPELINE(element);

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!self->model_path || self->model_path[0] == '\0') {
            GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                ("model-path property must be set"), (nullptr));
            return GST_STATE_CHANGE_FAILURE;
        }
        if (!g_file_test(self->model_path, G_FILE_TEST_IS_REGULAR)) {
            GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                ("model-path file does not exist or is not a regular file: %s",
                 self->model_path), (nullptr));
            return GST_STATE_CHANGE_FAILURE;
        }
        break;

    case GST_STATE_CHANGE_PAUSED_TO_READY: {
        std::lock_guard<std::mutex> lock(self->channels_lock);
        for (auto &pair : self->channels)
            stop_channel(self, pair.second.get());
        break;
    }

    case GST_STATE_CHANGE_READY_TO_NULL: {
        std::lock_guard<std::mutex> lock(self->channels_lock);
        for (auto &pair : self->channels)
            pair.second->pipeline.reset();
        break;
    }

    default:
        break;
    }

    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

// ---------------------------------------------------------------------------
// setup_channel_pipeline — build MediaPipeline (dec→proc[VO]→infer)
// ---------------------------------------------------------------------------
static gboolean setup_channel_pipeline(GstDxVnpuPipeline *self,
                                        std::shared_ptr<DxVnpuChannelCtx> ctx,
                                        GstCaps *caps) {
    GstStructure *s = gst_caps_get_structure(caps, 0);
    const gchar *media_type = gst_structure_get_name(s);

    // Determine codec
    if (g_str_equal(media_type, "video/x-h264"))
        ctx->codec = dxvnpu::CODEC_H264;
    else if (g_str_equal(media_type, "video/x-h265"))
        ctx->codec = dxvnpu::CODEC_H265;
    else {
        GST_ERROR_OBJECT(self, "Channel %d: unsupported media type '%s'",
                         ctx->channel_id, media_type);
        return FALSE;
    }

    // Parse dimensions
    gint width = 0, height = 0;
    gst_structure_get_int(s, "width", &width);
    gst_structure_get_int(s, "height", &height);
    if (width <= 0) width = 1920;
    if (height <= 0) height = 1080;
    ctx->input_width = width;
    ctx->input_height = height;

    auto shapes = dxvnpu::GetModelInputShapes(self->model_path);
    int model_w = 640, model_h = 640;
    if (!shapes.empty() && shapes[0].second.size() >= 4) {
        auto &dims = shapes[0].second;
        if (dims[1] <= 4) {
            // NCHW: [N, C, H, W]
            model_h = static_cast<int>(dims[2]);
            model_w = static_cast<int>(dims[3]);
        } else {
            // NHWC: [N, H, W, C]
            model_h = static_cast<int>(dims[1]);
            model_w = static_cast<int>(dims[2]);
        }
    } else {
        GST_WARNING_OBJECT(self, "Could not read model input shape, using default %dx%d",
                           model_w, model_h);
    }

    try {
        dxvnpu::MediaPipelineBuilder builder;

        if (self->device_id >= 0)
            builder.SetDevice(self->device_id);

        // Decoder
        dxvnpu::DecoderConfig dec_cfg;
        dec_cfg.codec = ctx->codec;
        dec_cfg.stream_width = width;
        dec_cfg.stream_height = height;
        builder.AddDecoder("dec", dec_cfg);

        // Processor with VO output
        dxvnpu::ProcessorConfig proc_cfg;
        proc_cfg.input_width = width;
        proc_cfg.input_height = height;
        proc_cfg.input_format = dxvnpu::COLOR_FORMAT_YUV420SP;
        proc_cfg.input_fps = -1;
        proc_cfg.output_format = dxvnpu::COLOR_FORMAT_RGB888;
        proc_cfg.output_width = model_w;
        proc_cfg.output_height = model_h;
        proc_cfg.output_fps = -1;
        proc_cfg.use_device_buffer_output = true;
        proc_cfg.keep_aspect_ratio = self->keep_ratio;
        proc_cfg.use_vo_output = self->use_vnpu_hdmi;
        proc_cfg.vo_total_channel = self->max_hdmi_channels;
        builder.AddProcessor("proc", proc_cfg);

        // Inference
        dxvnpu::InferenceConfig infer_cfg;
        infer_cfg.model_path = self->model_path;
        infer_cfg.use_shared_model = true;
        infer_cfg.useORT = self->use_ort;
        infer_cfg.use_device_buffer_input = true;
        infer_cfg.drop_policy = dxvnpu::OLDEST;
        infer_cfg.postprocess_type = dxvnpu::POST_NONE;
        builder.AddInference("infer", infer_cfg);

        // Connect
        builder.Connect("dec", "proc");
        builder.Connect("proc", "infer");

        ctx->pipeline = builder.Build();
    } catch (const std::exception &e) {
        GST_ERROR_OBJECT(self, "Failed to create VNPU pipeline for channel %d: %s",
                         ctx->channel_id, e.what());
        return FALSE;
    }

    if (!ctx->pipeline) {
        GST_ERROR_OBJECT(self, "Build() returned null for channel %d", ctx->channel_id);
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Channel %d pipeline: dec→proc(VO)→infer, input=%dx%d, "
                    "model=%dx%d, device=%d, task_id=%d",
                    ctx->channel_id, width, height, model_w, model_h,
                    self->device_id, ctx->pipeline->GetTaskId());

    // Start output thread
    ctx->last_flow.store(GST_FLOW_OK, std::memory_order_relaxed);
    ctx->put_count.store(0, std::memory_order_relaxed);
    ctx->get_count.store(0, std::memory_order_relaxed);
    ctx->flushing.store(false, std::memory_order_relaxed);
    ctx->output_running.store(true, std::memory_order_relaxed);
    ctx->output_thread = std::make_unique<std::thread>(channel_output_loop, self, ctx);

    return TRUE;
}

// ---------------------------------------------------------------------------
// sink_event
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpupipeline_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event) {
    auto *self = GST_DXVNPUPIPELINE(parent);
    auto ctx = get_channel_from_sink(self, pad);
    if (!ctx) {
        gst_event_unref(event);
        return FALSE;
    }

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CAPS: {
        GstCaps *caps;
        gst_event_parse_caps(event, &caps);

        stop_channel(self, ctx.get());

        if (!setup_channel_pipeline(self, ctx, caps)) {
            gst_event_unref(event);
            return FALSE;
        }

        // Push tensor caps downstream
        GstCaps *src_caps = gst_caps_new_empty_simple("application/x-dxtensor");
        gboolean ret = gst_pad_push_event(ctx->srcpad, gst_event_new_caps(src_caps));
        gst_caps_unref(src_caps);
        gst_event_unref(event);
        return ret;
    }

    case GST_EVENT_EOS: {
        GST_INFO_OBJECT(self, "Channel %d: received EOS", ctx->channel_id);
        if (ctx->pipeline) {
            auto eos_pkt = std::make_shared<dxvnpu::DXBitStream>();
            eos_pkt->isEOS = true;

            constexpr int kEosTimeoutMs = 5000;
            int elapsed = 0;
            dxvnpu::Status s;
            do {
                s = ctx->pipeline->PutData("dec", eos_pkt, 100);
                elapsed += 100;
            } while (s == dxvnpu::Status::ERROR_TIMEOUT &&
                     ctx->output_running.load(std::memory_order_acquire) &&
                     elapsed < kEosTimeoutMs);

            if (s != dxvnpu::Status::OK) {
                GST_WARNING_OBJECT(self, "Channel %d: EOS PutData failed (status=%d)",
                                   ctx->channel_id, static_cast<int>(s));
            }
        } else {
            // No pipeline set up — forward EOS directly to downstream
            gst_pad_push_event(ctx->srcpad, gst_event_new_eos());
        }
        gst_event_unref(event);
        return TRUE;
    }

    case GST_EVENT_FLUSH_START: {
        ctx->flushing.store(true, std::memory_order_release);
        stop_thread(ctx.get());
        return gst_pad_push_event(ctx->srcpad, event);
    }

    case GST_EVENT_FLUSH_STOP: {
        ctx->flushing.store(false, std::memory_order_release);
        ctx->last_flow.store(GST_FLOW_OK, std::memory_order_relaxed);

        if (ctx->pipeline) {
            while (auto stale = ctx->pipeline->GetData("infer", 50)) {}
            ctx->output_running.store(true, std::memory_order_relaxed);
            ctx->output_thread = std::make_unique<std::thread>(
                channel_output_loop, self, ctx);
        }
        return gst_pad_push_event(ctx->srcpad, event);
    }

    default:
        return gst_pad_push_event(ctx->srcpad, event);
    }
}

// ---------------------------------------------------------------------------
// chain — feed bitstream into MediaPipeline, no buffer retention
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxvnpupipeline_chain(GstPad *pad, GstObject *parent,
                                                GstBuffer *buf) {
    auto *self = GST_DXVNPUPIPELINE(parent);
    auto ctx = get_channel_from_sink(self, pad);

    GST_LOG_OBJECT(self, "chain: ch=%d pts=%" GST_TIME_FORMAT,
                   ctx ? ctx->channel_id : -1,
                   GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    if (!ctx) {
        gst_buffer_unref(buf);
        return GST_FLOW_ERROR;
    }

    if (ctx->flushing.load(std::memory_order_acquire)) {
        gst_buffer_unref(buf);
        return GST_FLOW_FLUSHING;
    }

    GstFlowReturn lf = ctx->last_flow.load(std::memory_order_acquire);
    if (lf != GST_FLOW_OK) {
        gst_buffer_unref(buf);
        return lf;
    }

    auto pl = ctx->pipeline;
    if (!pl) {
        GST_ERROR_OBJECT(self, "Channel %d: pipeline not initialized", ctx->channel_id);
        gst_buffer_unref(buf);
        return GST_FLOW_NOT_NEGOTIATED;
    }

    // Zero-copy: wrap GstBuffer memory in shared_ptr
    auto *map_info = new GstMapInfo;
    if (!gst_buffer_map(buf, map_info, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Channel %d: failed to map input buffer", ctx->channel_id);
        delete map_info;
        gst_buffer_unref(buf);
        return GST_FLOW_ERROR;
    }

    gst_buffer_ref(buf);
    auto buf_deleter = [buf, map_info](void*) {
        gst_buffer_unmap(buf, map_info);
        gst_buffer_unref(buf);
        delete map_info;
    };

    auto pkt = std::make_shared<dxvnpu::DXBitStream>();
    pkt->codec = ctx->codec;
    pkt->timestamp = GST_BUFFER_PTS(buf);
    pkt->data = std::shared_ptr<void>(map_info->data, buf_deleter);
    pkt->data_size = map_info->size;

    auto status = pl->PutData("dec", pkt, 100);
    while (status == dxvnpu::Status::ERROR_TIMEOUT &&
           ctx->output_running.load(std::memory_order_acquire) &&
           !ctx->flushing.load(std::memory_order_acquire)) {
        status = pl->PutData("dec", pkt, 100);
    }

    if (status != dxvnpu::Status::OK) {
        GST_ERROR_OBJECT(self, "Channel %d: PutData failed (status=%d)",
                         ctx->channel_id, static_cast<int>(status));
        // pkt going out of scope will release the extra ref from line 639;
        // we still need to release the caller's ref (chain is transfer-full).
        gst_buffer_unref(buf);
        return ctx->flushing.load(std::memory_order_acquire)
                   ? GST_FLOW_FLUSHING : GST_FLOW_ERROR;
    }

    ctx->put_count.fetch_add(1, std::memory_order_relaxed);

    GST_TRACE_OBJECT(self, "Channel %d: PutData ok (pts=%" GST_TIME_FORMAT ", put=%zu)",
                     ctx->channel_id, GST_TIME_ARGS(GST_BUFFER_PTS(buf)),
                     ctx->put_count.load(std::memory_order_relaxed));

    gst_buffer_unref(buf);
    return ctx->last_flow.load(std::memory_order_acquire);
}

// ---------------------------------------------------------------------------
// channel_output_loop — per-channel thread: GetData → empty buf + FrameMeta → push
// ---------------------------------------------------------------------------
static void channel_output_loop(GstDxVnpuPipeline *self, std::shared_ptr<DxVnpuChannelCtx> ctx) {
    constexpr int kPollMs = 100;

    GST_DEBUG_OBJECT(self, "Channel %d output thread started", ctx->channel_id);

    while (ctx->output_running.load(std::memory_order_relaxed)) {
        auto pl = ctx->pipeline;
        if (!pl) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
            continue;
        }

        auto out = pl->GetData("infer", kPollMs);
        if (!out) continue;

        // EOS
        if (out->isEOS) {
            GST_INFO_OBJECT(self, "Channel %d: inference EOS (put=%zu, get=%zu)",
                            ctx->channel_id,
                            ctx->put_count.load(std::memory_order_relaxed),
                            ctx->get_count.load(std::memory_order_relaxed));
            gst_pad_push_event(ctx->srcpad, gst_event_new_eos());

            while (ctx->output_running.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
            break;
        }

        if (!out->isTensor()) continue;

        auto dxvnpu_tensors = std::dynamic_pointer_cast<dxvnpu::DXTensors>(out);
        if (!dxvnpu_tensors) continue;

        ctx->get_count.fetch_add(1, std::memory_order_relaxed);

        // Create empty buffer with FrameMeta
        GstBuffer *outbuf = gst_buffer_new();
        GST_BUFFER_PTS(outbuf) = out->timestamp;

        outbuf = dx_create_frame_meta(outbuf);
        DXFrameMeta *frame_meta = dx_get_frame_meta(outbuf);
        if (!frame_meta) {
            GST_ERROR_OBJECT(self, "Channel %d: failed to create frame metadata",
                             ctx->channel_id);
            gst_buffer_unref(outbuf);
            break;
        }

        // Use pipeline task_id as stream_id for OverlayRenderer routing
        frame_meta->_stream_id = ctx->pipeline->GetTaskId();
        frame_meta->_width = ctx->input_width;
        frame_meta->_height = ctx->input_height;

        // Copy tensor data
        dxs::DXTensors &dst = frame_meta->_output_tensors[self->inference_id];
        if (dxvnpu_tensors->getRawData() && dxvnpu_tensors->data_size > 0) {
            dst.allocate(dxvnpu_tensors->data_size);
            std::memcpy(dst.data_ptr(), dxvnpu_tensors->getRawData(),
                        dxvnpu_tensors->data_size);
        }

        // Copy tensor metadata
        for (const auto &meta : dxvnpu_tensors->tensor_list) {
            dxs::DXTensor t;
            t._name = meta.name;
            t._shape = meta.shape;
            t._type = static_cast<dxs::DataType>(meta.data_type);
            t._elemSize = meta.elem_size;
            if (dst.data_ptr()) {
                t._data = static_cast<uint8_t*>(dst.data_ptr()) + meta.data_offset;
            }
            dst._tensors.push_back(t);
        }

        GST_TRACE_OBJECT(self, "Channel %d: pushing tensor (ts=%" GST_TIME_FORMAT
                         ", get=%zu)",
                         ctx->channel_id, GST_TIME_ARGS(GST_BUFFER_PTS(outbuf)),
                         ctx->get_count.load(std::memory_order_relaxed));

        GstFlowReturn ret = gst_pad_push(ctx->srcpad, outbuf);
        if (ret != GST_FLOW_OK) {
            GST_WARNING_OBJECT(self, "Channel %d: push returned %s",
                               ctx->channel_id, gst_flow_get_name(ret));
            ctx->last_flow.store(ret, std::memory_order_release);

            while (ctx->output_running.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(std::chrono::milliseconds(kPollMs));
            break;
        }
    }

    ctx->output_running.store(false, std::memory_order_release);
    GST_DEBUG_OBJECT(self, "Channel %d output thread exiting (put=%zu, get=%zu)",
                     ctx->channel_id,
                     ctx->put_count.load(std::memory_order_relaxed),
                     ctx->get_count.load(std::memory_order_relaxed));
}

// ---------------------------------------------------------------------------
// src_event — upstream event from downstream (QOS, RECONFIGURE, etc.)
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpupipeline_src_event(GstPad *pad, GstObject *parent,
                                              GstEvent *event) {
    auto *self = GST_DXVNPUPIPELINE(parent);
    auto ctx = get_channel_from_src(self, pad);
    if (!ctx) {
        gst_event_unref(event);
        return FALSE;
    }
    return gst_pad_push_event(ctx->sinkpad, event);
}

// ---------------------------------------------------------------------------
// src_query — LATENCY etc. forwarded to paired sink pad's peer
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpupipeline_src_query(GstPad *pad, GstObject *parent,
                                              GstQuery *query) {
    auto *self = GST_DXVNPUPIPELINE(parent);
    auto ctx = get_channel_from_src(self, pad);
    if (!ctx)
        return FALSE;
    return gst_pad_peer_query(ctx->sinkpad, query);
}

// ---------------------------------------------------------------------------
// sink_query — ALLOCATION/CAPS etc. forwarded to paired src pad's peer
// ---------------------------------------------------------------------------
static gboolean gst_dxvnpupipeline_sink_query(GstPad *pad, GstObject *parent,
                                               GstQuery *query) {
    auto *self = GST_DXVNPUPIPELINE(parent);
    auto ctx = get_channel_from_sink(self, pad);
    if (!ctx)
        return FALSE;
    return gst_pad_peer_query(ctx->srcpad, query);
}
