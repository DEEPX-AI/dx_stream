#include "gst-dxinfer.hpp"
#include "utils.hpp"
#include <chrono>
#include <new>
#include "dx_dlfcn.h"
#include <json-glib/json-glib.h>
#include <map>

enum class PropertyID {
    PROP_0,
    PROP_PREPROC_ID,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    PROP_MODEL_PATH,
    PROP_CONFIG_PATH,
    PROP_USE_ORT,
    PROP_BACKEND,
    N_PROPERTIES
};

#define GST_TYPE_DXINFER_BACKEND (gst_dxinfer_backend_get_type())
static GType gst_dxinfer_backend_get_type() {
    static GType type = 0;
    if (g_once_init_enter(&type)) {
        static const GEnumValue values[] = {
            {static_cast<int>(BackendType::AUTO), "auto", "auto"},
            {static_cast<int>(BackendType::DXRT), "dxrt", "dxrt"},
            {static_cast<int>(BackendType::DXVNPU), "dxvnpu", "dxvnpu"},
            {0, NULL, NULL}
        };
        GType tmp = g_enum_register_static("GstDxInferBackend", values);
        g_once_init_leave(&type, tmp);
    }
    return type;
}

GST_DEBUG_CATEGORY_STATIC(gst_dxinfer_debug_category);
#define GST_CAT_DEFAULT gst_dxinfer_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; video/x-raw"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; video/x-raw"));

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);
static gboolean gst_dxinfer_sink_query(GstPad *pad, GstObject *parent,
                                       GstQuery *query);
static gboolean gst_dxinfer_src_query(GstPad *pad, GstObject *parent,
                                      GstQuery *query);

static gpointer push_thread_func(GstDxInfer *self);
static void drain_push_thread(GstDxInfer *self);

G_DEFINE_TYPE(GstDxInfer, gst_dxinfer, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static gboolean string_is_empty(const gchar *value) {
    return value == nullptr || value[0] == '\0';
}

static void parse_config(GstDxInfer *self) {
    if (string_is_empty(self->_config_path)) {
        return;
    }

    if (!g_file_test(self->_config_path, G_FILE_TEST_EXISTS)) {
        GST_ERROR_OBJECT(self, "[dxinfer] Config file does not exist: %s",
                         self->_config_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading config file: %s", self->_config_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, self->_config_path, &error)) {
        GST_ERROR_OBJECT(self, "[dxinfer] Failed to load config file: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    JsonObject *object = json_node_get_object(node);

    const gchar *model_path =
        json_object_get_string_member(object, "model_path");
    g_object_set(self, "model-path", model_path, nullptr);

    auto assign_uint_member = [&](const char *key, guint &target) {
        if (!json_object_has_member(object, key))
            return;
        gint64 val = json_object_get_int_member(object, key);
        if (val < 0) {
            GST_ERROR_OBJECT(self, "[dxinfer] Member %s has a negative value (%lld), ignoring.",
                             key, (long long)val);
            return;
        }
        target = static_cast<guint>(val);
    };

    assign_uint_member("preprocess_id", self->_preproc_id);
    assign_uint_member("inference_id", self->_infer_id);

    if (json_object_has_member(object, "secondary_mode")) {
        self->_secondary_mode =
            json_object_get_boolean_member(object, "secondary_mode");
    }

    if (json_object_has_member(object, "use_ort")) {
        self->_use_ort = json_object_get_boolean_member(object, "use_ort");
    }

    if (json_object_has_member(object, "backend")) {
        const gchar *backend_str = json_object_get_string_member(object, "backend");
        if (g_strcmp0(backend_str, "dxrt") == 0)
            self->_backend_type = BackendType::DXRT;
        else if (g_strcmp0(backend_str, "dxvnpu") == 0)
            self->_backend_type = BackendType::DXVNPU;
        else
            self->_backend_type = BackendType::AUTO;
    }

    GST_INFO_OBJECT(self, "Config loaded: model=%s, preproc_id=%u, infer_id=%u, secondary_mode=%d, use_ort=%d",
                    model_path, self->_preproc_id, self->_infer_id, self->_secondary_mode, self->_use_ort);
    g_object_unref(parser);
}

static void gst_dxinfer_set_property(GObject *object, guint property_id,
                                     const GValue *value,
                                     GParamSpec *pspec) {
    auto self = GST_DXINFER(object);

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_MODEL_PATH: {
        if (nullptr != self->_model_path)
            g_free(self->_model_path);
        self->_model_path = g_strdup(g_value_get_string(value));
        break;
    }
    case PropertyID::PROP_CONFIG_PATH: {
        if (nullptr != self->_config_path)
            g_free(self->_config_path);
        self->_config_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;
    }
    case PropertyID::PROP_PREPROC_ID: {
        self->_preproc_id = g_value_get_uint(value);
        break;
    }
    case PropertyID::PROP_INFER_ID: {
        self->_infer_id = g_value_get_uint(value);
        break;
    }
    case PropertyID::PROP_SECONDARY_MODE: {
        self->_secondary_mode = g_value_get_boolean(value);
        break;
    }
    case PropertyID::PROP_USE_ORT: {
        self->_use_ort = g_value_get_boolean(value);
        break;
    }
    case PropertyID::PROP_BACKEND: {
        self->_backend_type = static_cast<BackendType>(g_value_get_enum(value));
        break;
    }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gst_dxinfer_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec) {
    auto self = GST_DXINFER(object);
    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_MODEL_PATH:
        g_value_set_string(value, self->_model_path);
        break;
    case PropertyID::PROP_CONFIG_PATH:
        g_value_set_string(value, self->_config_path);
        break;
    case PropertyID::PROP_PREPROC_ID:
        g_value_set_uint(value, self->_preproc_id);
        break;
    case PropertyID::PROP_INFER_ID:
        g_value_set_uint(value, self->_infer_id);
        break;
    case PropertyID::PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_secondary_mode);
        break;
    case PropertyID::PROP_USE_ORT:
        g_value_set_boolean(value, self->_use_ort);
        break;
    case PropertyID::PROP_BACKEND:
        g_value_set_enum(value, static_cast<int>(self->_backend_type));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxinfer_dispose(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);
    if (self->_config_path) {
        g_free(self->_config_path);
        self->_config_path = nullptr;
    }
    if (self->_model_path) {
        g_free(self->_model_path);
        self->_model_path = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxinfer_finalize(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);

    // Drain push thread before destroying synchronization primitives.
    // Normal shutdown (PAUSED→READY) already drains, but abnormal paths
    // (e.g. FLUSH_STOP creating a thread in NULL state) may leave it alive.
    {
        std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
        self->_push_ctx.push_running = FALSE;
    }
    self->_push_ctx.cv.notify_all();
    drain_push_thread(self);

    if (self->_timing_ctx.recent_latencies) {
        g_queue_free(self->_timing_ctx.recent_latencies);
        self->_timing_ctx.recent_latencies = nullptr;
    }

    self->_backend.~unique_ptr();
    self->_push_ctx.push_queue.~queue();
    self->_push_ctx.push_lock.~mutex();
    self->_push_ctx.cv.~condition_variable();
    self->_eos_ctx.eos_lock.~mutex();
    self->_eos_ctx.stream_eos_arrived.~set();
    self->_eos_ctx.stream_pending_buffers.~map();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gboolean handle_null_to_ready(GstDxInfer *self) {
    if (string_is_empty(self->_model_path)) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("[dxinfer] model-path property is required but not set."),
                          (NULL));
        return FALSE;
    }

    if (!g_file_test(self->_model_path, G_FILE_TEST_IS_REGULAR)) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                          ("[dxinfer] model-path file does not exist or is not a regular file: %s",
                           self->_model_path),
                          (NULL));
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Loading model: %s (backend=%d, use_ort=%d)",
                    self->_model_path, static_cast<int>(self->_backend_type), self->_use_ort);

    self->_backend = InferBackendFactory::Create(self->_backend_type);
    if (!self->_backend) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                          ("[dxinfer] Failed to create inference backend (type=%d)",
                           static_cast<int>(self->_backend_type)),
                          (nullptr));
        return FALSE;
    }

    InferBackendOptions opts;
    opts.model_path = self->_model_path;
    opts.use_ort = static_cast<bool>(self->_use_ort);

    if (!self->_backend->Init(opts)) {
        GST_ELEMENT_ERROR(self, RESOURCE, FAILED,
                          ("[dxinfer] Failed to initialize %s backend",
                           self->_backend->GetName()),
                          (nullptr));
        self->_backend.reset();
        return FALSE;
    }

    self->_output_tensor_size = self->_backend->GetOutputBufferSize();
    GST_INFO_OBJECT(self, "Backend '%s' initialized", self->_backend->GetName());
    return TRUE;
}

static void handle_ready_to_paused(GstDxInfer *self) {
    GST_DEBUG_OBJECT(self, "Initializing runtime state");

    if (self->_backend) {
        self->_backend->Reset();
    }

    self->_eos_ctx.stream_eos_arrived.clear();
    self->_eos_ctx.stream_pending_buffers.clear();

    self->_timing_ctx.avg_latency = 0;
    self->_timing_ctx.throughput_count = 0;
    self->_timing_ctx.throughput_start = std::chrono::steady_clock::now();

    if (!self->_secondary_mode) {
        self->_push_ctx.push_running = TRUE;
        GST_INFO_OBJECT(self, "Starting push thread");
        self->_push_ctx.push_thread =
            g_thread_new("push-thread", (GThreadFunc)push_thread_func, self);
    }
}

static void handle_paused_to_playing(GstDxInfer *self) {
    GST_DEBUG_OBJECT(self, "Resuming data flow");
    self->_push_ctx.cv.notify_all();
}

static void handle_playing_to_paused(GstDxInfer *self) {
    GST_DEBUG_OBJECT(self, "Pausing data flow");
}

static GstStateChangeReturn dxinfer_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxInfer *self = GST_DXINFER(element);
    const gchar *transition_name = gst_state_change_get_name(transition);
    GST_DEBUG_OBJECT(self, "State transition: %s", transition_name);

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!handle_null_to_ready(self))
            return GST_STATE_CHANGE_FAILURE;
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        handle_ready_to_paused(self);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        handle_paused_to_playing(self);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(self, "Unblocking threads for shutdown");
        {
            std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.push_running = FALSE;
        }
        if (self->_backend) {
            self->_backend->Flush();
        }
        self->_push_ctx.cv.notify_all();
        break;
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        handle_playing_to_paused(self);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        if (!self->_secondary_mode) {
            drain_push_thread(self);
        } else if (self->_backend) {
            self->_backend->Reset();
        }
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        GST_DEBUG_OBJECT(self, "Releasing inference engine resources");
        self->_backend.reset();
        break;
    default:
        break;
    }

    GST_DEBUG_OBJECT(self, "State change completed: %d", result);
    return result;
}

static void gst_dxinfer_class_init(GstDxInferClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxinfer_debug_category, "dxinfer", 0,
                            "DXInfer plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = gst_dxinfer_set_property;
    gobject_class->get_property = gst_dxinfer_get_property;
    gobject_class->dispose = dxinfer_dispose;
    gobject_class->finalize = dxinfer_finalize;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<int>(PropertyID::PROP_MODEL_PATH)] =
        g_param_spec_string("model-path", "model file path",
                            "Path to the .dxnn model file used for inference.",
                            nullptr, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_CONFIG_PATH)] = g_param_spec_string(
        "config-file-path", "config path",
        "Path to the JSON config file containing the element's properties.",
        nullptr, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_PREPROC_ID)] = g_param_spec_uint(
        "preprocess-id", "pre process id",
        "Key of the input tensor in DXFrameMeta/DXObjectMeta to feed into inference "
        "(must match the preprocess-id used by the upstream dxpreprocess).", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_INFER_ID)] = g_param_spec_uint(
        "inference-id", "inference id",
        "Key under which inference output tensors are stored in DXFrameMeta/DXObjectMeta "
        "(the downstream dxpostprocess retrieves them by this ID).", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_SECONDARY_MODE)] = g_param_spec_boolean(
        "secondary-mode", "secondary mode",
        "Determines whether to operate in primary mode or secondary mode.",
        FALSE, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_USE_ORT)] = g_param_spec_boolean(
        "use-ort", "use ort",
        "Determines whether to use ONNX Runtime for inference.",
        TRUE, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_BACKEND)] = g_param_spec_enum(
        "backend", "Inference Backend",
        "Select inference backend (auto, dxrt, dxvnpu).",
        GST_TYPE_DXINFER_BACKEND, static_cast<int>(BackendType::AUTO),
        G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<int>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(element_class, "DXInfer", "Generic",
                                          "Performs inference",
                                          "Sangil Jo <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxinfer_change_state;
}

gboolean handle_custom_downstream_event(GstDxInfer *self, GstEvent *event) {
    gboolean res = TRUE;
    const GstStructure *s_check = gst_event_get_structure(event);
    if (gst_structure_has_name(s_check, "application/x-dx-wrapped-event")) {
        int stream_id = -1;
        GstEvent *original_event = nullptr;
        gst_structure_get_int(s_check, "stream-id", &stream_id);
        gst_structure_get(s_check, "event", GST_TYPE_EVENT, &original_event, NULL);
        const gboolean is_eos = original_event &&
            GST_EVENT_TYPE(original_event) == GST_EVENT_EOS;
        if (original_event) {
            gst_event_unref(original_event);
        }
        if (is_eos) {
            {
                std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
                self->_eos_ctx.stream_eos_arrived.insert(stream_id);
            }
            GST_DEBUG_OBJECT(self, "EOS Arrived From Stream [%d]", stream_id);

            if (!self->_secondary_mode) {
                std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
                self->_push_ctx.cv.wait(lock, [self, stream_id] {
                    std::lock_guard<std::mutex> eos_lk(self->_eos_ctx.eos_lock);
                    return self->_eos_ctx.stream_pending_buffers[stream_id] <= 0 ||
                           !self->_push_ctx.push_running;
                });
            }

            if (self->_push_ctx.push_running || self->_secondary_mode) {
                GST_DEBUG_OBJECT(self, "Push EOS From Stream [%d]", stream_id);
                res = gst_pad_push_event(self->_srcpad, event);
            } else {
                gst_event_unref(event);
            }
        } else {
            res = gst_pad_push_event(self->_srcpad, event);
        }
    } else {
        res = gst_pad_push_event(self->_srcpad, event);
    }
    return res;
}

static gboolean gst_dxinfer_sink_event(GstPad *pad, GstObject *parent,
                                       GstEvent *event) {
    GstDxInfer *self = GST_DXINFER(parent);

    gboolean res = TRUE;
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        GST_DEBUG_OBJECT(self, "Received EOS event");
        if (!self->_secondary_mode) {
            std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
            GST_DEBUG_OBJECT(self, "EOS: waiting for %zu queued buffers to drain",
                            self->_push_ctx.push_queue.size());
            self->_push_ctx.cv.wait(lock, [self] {
                return self->_push_ctx.push_queue.empty() ||
                       !self->_push_ctx.push_running;
            });
        }
        if (self->_push_ctx.push_running || self->_secondary_mode) {
            GST_DEBUG_OBJECT(self, "EOS: queue drained, forwarding downstream");
            res = gst_pad_push_event(self->_srcpad, event);
        } else {
            gst_event_unref(event);
        }
    } break;
    case GST_EVENT_FLUSH_START:
        if (!self->_secondary_mode) {
            std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.push_running = FALSE;
        }
        if (self->_backend) {
            self->_backend->Flush();
        }
        self->_push_ctx.cv.notify_all();
        res = gst_pad_event_default(pad, parent, event);
        break;
    case GST_EVENT_FLUSH_STOP:
        if (!self->_secondary_mode) {
            drain_push_thread(self);
            self->_push_ctx.push_running = TRUE;
            self->_push_ctx.push_thread =
                g_thread_new("push-thread", (GThreadFunc)push_thread_func, self);
        } else if (self->_backend) {
            self->_backend->Reset();
        }
        {
            std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
            self->_eos_ctx.stream_eos_arrived.clear();
            self->_eos_ctx.stream_pending_buffers.clear();
        }
        res = gst_pad_event_default(pad, parent, event);
        break;
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        res = handle_custom_downstream_event(self, event);
    } break;
    default:
        res = gst_pad_push_event(self->_srcpad, event);
        break;
    }
    return res;
}

static gboolean gst_dxinfer_sink_query(GstPad *pad, GstObject *parent,
                                       GstQuery *query) {
    GstDxInfer *self = GST_DXINFER(parent);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS: {
        GstCaps *filter = nullptr;
        gst_query_parse_caps(query, &filter);
        GstCaps *caps = gst_pad_get_pad_template_caps(pad);
        if (filter) {
            GstCaps *filtered = gst_caps_intersect_full(filter, caps,
                                                       GST_CAPS_INTERSECT_FIRST);
            gst_caps_unref(caps);
            caps = filtered;
        }
        gst_query_set_caps_result(query, caps);
        gst_caps_unref(caps);
        return TRUE;
    }
    case GST_QUERY_ACCEPT_CAPS: {
        GstCaps *caps = nullptr;
        gst_query_parse_accept_caps(query, &caps);
        GstCaps *templ = gst_pad_get_pad_template_caps(pad);
        gboolean accepted = caps && gst_caps_can_intersect(caps, templ);
        gst_caps_unref(templ);
        gst_query_set_accept_caps_result(query, accepted);
        return TRUE;
    }
    case GST_QUERY_ALLOCATION: {
        gboolean ret = gst_pad_peer_query(self->_srcpad, query);
        if (ret)
            gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
        return ret;
    }
    default:
        return gst_pad_peer_query(self->_srcpad, query);
    }
}

static gboolean gst_dxinfer_src_query(GstPad *pad, GstObject *parent,
                                      GstQuery *query) {
    std::ignore = pad;
    GstDxInfer *self = GST_DXINFER(parent);

    if (GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        gboolean res = gst_pad_peer_query(self->_sinkpad, query);
        if (res) {
            gboolean live;
            GstClockTime min_latency, max_latency;
            gst_query_parse_latency(query, &live, &min_latency, &max_latency);

            GstClockTime infer_latency =
                self->_timing_ctx.avg_latency * GST_MSECOND;
            min_latency += infer_latency;
            if (max_latency != GST_CLOCK_TIME_NONE)
                max_latency += infer_latency;

            GST_DEBUG_OBJECT(self, "LATENCY query: adding %" GST_TIME_FORMAT
                             " inference latency",
                             GST_TIME_ARGS(infer_latency));
            gst_query_set_latency(query, live, min_latency, max_latency);
        }
        return res;
    }

    return gst_pad_peer_query(self->_sinkpad, query);
}

static gboolean gst_dxinfer_src_event(GstPad *pad, GstObject *parent,
                                      GstEvent *event) {
    GstDxInfer *self = GST_DXINFER(parent);

    GstEvent *inner = nullptr;
    gboolean is_wrapped = dx_event_is_wrapped_upstream(event);
    if (is_wrapped) {
        inner = dx_event_peek_inner(event, nullptr);
    }
    GstEvent *qos_event = (is_wrapped && inner) ? inner : event;

    if (GST_EVENT_TYPE(qos_event) == GST_EVENT_QOS) {
        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(qos_event, &type, nullptr, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_DEBUG_OBJECT(self, "QoS THROTTLE event: diff=%" G_GINT64_FORMAT "ms", diff / 1000000);
            GST_OBJECT_LOCK(parent);
            if (self->_timing_ctx.throttling_delay != 0)
                self->_timing_ctx.throttling_delay = MIN(self->_timing_ctx.throttling_delay, diff);
            else
                self->_timing_ctx.throttling_delay = diff;
            GST_OBJECT_UNLOCK(parent);
        }

        if (type == GST_QOS_TYPE_UNDERFLOW) {
            GST_DEBUG_OBJECT(self, "QoS UNDERFLOW event: diff=%" G_GINT64_FORMAT "ms", diff / 1000000);
            GST_OBJECT_LOCK(parent);
            if (diff > 0) {
                self->_timing_ctx.qos_timediff = diff;
                self->_timing_ctx.qos_timestamp = timestamp;
            } else {
                self->_timing_ctx.qos_timediff = 0;
            }
            GST_OBJECT_UNLOCK(parent);
        }
    }

    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxinfer_init(GstDxInfer *self) {
    self->_sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxinfer_chain));
    gst_pad_set_event_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_sink_event));
    gst_pad_set_query_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_sink_query));
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_src_event));
    gst_pad_set_query_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_src_query));
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_model_path = nullptr;
    self->_config_path = nullptr;
    self->_secondary_mode = FALSE;
    self->_use_ort = TRUE;
    self->_backend_type = BackendType::AUTO;
    new (&self->_backend) std::unique_ptr<IInferBackend>();
    self->_output_tensor_size = 0;

    new (&self->_push_ctx.push_queue) std::queue<GstDxInferPushEntry>();
    new (&self->_push_ctx.push_lock) std::mutex();
    new (&self->_push_ctx.cv) std::condition_variable();
    self->_push_ctx.push_thread = nullptr;
    self->_push_ctx.push_running = FALSE;

    self->_timing_ctx.avg_latency = 0;
    self->_timing_ctx.recent_latencies = g_queue_new();
    self->_timing_ctx.prev_ts = 0;
    self->_timing_ctx.throttling_delay = 0;
    self->_timing_ctx.throttling_accum = 0;
    self->_timing_ctx.qos_timestamp = 0;
    self->_timing_ctx.qos_timediff = 0;
    self->_timing_ctx.throughput_count = 0;
    self->_timing_ctx.throughput_start = std::chrono::steady_clock::now();

    new (&self->_eos_ctx.eos_lock) std::mutex();
    new (&self->_eos_ctx.stream_eos_arrived) std::set<int>();
    new (&self->_eos_ctx.stream_pending_buffers) std::map<int, int>();
}

gint64 calculate_average(GQueue *queue) {
    if (g_queue_is_empty(queue)) {
        return 0;
    }

    gint64 sum = 0;
    guint count = 0;

    for (GList *node = queue->head; node != nullptr; node = node->next) {
        sum += GPOINTER_TO_INT(node->data);
        count++;
    }

    if (count == 0) {
        return 0;
    }

    return (gint)(sum / count);
}

static void update_metrics(GstDxInfer *self, gint64 latency_ms) {
    auto now = std::chrono::steady_clock::now();

    if (g_queue_get_length(self->_timing_ctx.recent_latencies) == 10) {
        g_queue_pop_head(self->_timing_ctx.recent_latencies);
    }
    g_queue_push_tail(self->_timing_ctx.recent_latencies, GINT_TO_POINTER(latency_ms));
    self->_timing_ctx.avg_latency = calculate_average(self->_timing_ctx.recent_latencies);

    self->_timing_ctx.throughput_count++;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - self->_timing_ctx.throughput_start).count();
    if (elapsed_ms >= 1000) {
        double fps = self->_timing_ctx.throughput_count * 1000.0 / elapsed_ms;
        GST_DEBUG_OBJECT(self, "Inference throughput: %.1f fps, latency: %" G_GINT64_FORMAT
                         "ms (avg: %" G_GINT64_FORMAT "ms)",
                         fps, latency_ms, self->_timing_ctx.avg_latency);
        self->_timing_ctx.throughput_count = 0;
        self->_timing_ctx.throughput_start = now;
    }
}

static gpointer push_thread_func(GstDxInfer *self) {
    while (self->_push_ctx.push_running) {
        GstBuffer *push_buf = nullptr;
        bool needs_get = false;
        {
            std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.cv.wait(lock, [self] {
                return !self->_push_ctx.push_running ||
                       !self->_push_ctx.push_queue.empty();
            });
            if (!self->_push_ctx.push_running)
                break;

            auto &entry = self->_push_ctx.push_queue.front();
            needs_get = entry.submitted;
            push_buf = entry.buffer;
        }

        if (!GST_IS_BUFFER(push_buf)) {
            GST_ERROR_OBJECT(self, "Invalid buffer in push thread");
            std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.push_queue.pop();
            self->_push_ctx.cv.notify_all();
            continue;
        }

        auto *frame_meta = dx_get_frame_meta(push_buf);
        if (!frame_meta) {
            GST_ERROR_OBJECT(self, "No DXFrameMeta in push buffer");
            gst_buffer_unref(push_buf);
            {
                std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
                self->_push_ctx.push_queue.pop();
                self->_push_ctx.cv.notify_all();
            }
            continue;
        }

        if (needs_get) {
            auto get_start = std::chrono::steady_clock::now();
            bool ok = self->_backend->Get(frame_meta->_output_tensors[self->_infer_id]);
            if (!ok) {
                GST_DEBUG_OBJECT(self, "Backend Get() returned false, exiting push loop");
                break;
            }
            auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - get_start).count();
            update_metrics(self, latency_ms);
        }

        int stream_id = frame_meta->_stream_id;
        GstFlowReturn ret = gst_pad_push(self->_srcpad, push_buf);
        // gst_pad_push takes buffer ownership regardless of return value

        {
            std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
            self->_eos_ctx.stream_pending_buffers[stream_id] -= 1;
        }

        {
            std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.push_queue.pop();
            self->_push_ctx.cv.notify_all();
        }

        if (ret != GST_FLOW_OK) {
            if (ret != GST_FLOW_FLUSHING) {
                GST_WARNING_OBJECT(self, "Push returned %s", gst_flow_get_name(ret));
            }
            self->_push_ctx.push_running = FALSE;
            self->_push_ctx.cv.notify_all();
            break;
        }
    }

    GST_INFO_OBJECT(self, "Push thread exiting");
    return nullptr;
}

static void drain_push_thread(GstDxInfer *self) {
    if (self->_push_ctx.push_thread) {
        g_thread_join(self->_push_ctx.push_thread);
        self->_push_ctx.push_thread = nullptr;
    }

    if (self->_backend) {
        self->_backend->Reset();
    }

    {
        std::lock_guard<std::mutex> push_lk(self->_push_ctx.push_lock);
        while (!self->_push_ctx.push_queue.empty()) {
            auto &entry = self->_push_ctx.push_queue.front();
            if (GST_IS_BUFFER(entry.buffer)) {
                auto *fm = dx_get_frame_meta(entry.buffer);
                if (fm) {
                    std::lock_guard<std::mutex> eos_lk(self->_eos_ctx.eos_lock);
                    self->_eos_ctx.stream_pending_buffers[fm->_stream_id] -= 1;
                }
                gst_buffer_unref(entry.buffer);
            }
            self->_push_ctx.push_queue.pop();
        }
        self->_push_ctx.cv.notify_all();
    }
}

static bool should_drop_buffer_due_to_qos(GstDxInfer *self, GstBuffer *buf) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);

    GST_OBJECT_LOCK(self);
    GstClockTimeDiff qos_timediff = self->_timing_ctx.qos_timediff;
    GstClockTime qos_timestamp = self->_timing_ctx.qos_timestamp;
    GstClockTimeDiff throttling_delay = self->_timing_ctx.throttling_delay;
    GST_OBJECT_UNLOCK(self);

    if (qos_timediff <= 0)
        return false;

    GstClockTimeDiff earliest_time;
    if (throttling_delay > 0) {
        earliest_time = qos_timestamp + 2 * qos_timediff + throttling_delay;
    } else {
        earliest_time = qos_timestamp + qos_timediff;
    }

    bool should_drop = static_cast<GstClockTime>(earliest_time) > in_ts;
    if (should_drop) {
        GST_LOG_OBJECT(self, "Dropping buffer due to QoS (ts=%" GST_TIME_FORMAT ")",
                         GST_TIME_ARGS(in_ts));
    }
    return should_drop;
}

static bool should_drop_buffer_due_to_throttling(GstDxInfer *self,
                                                 GstBuffer *buf) {
    if (self->_timing_ctx.throttling_delay <= 0)
        return false;

    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
    GstClockTimeDiff diff = in_ts - self->_timing_ctx.prev_ts;
    self->_timing_ctx.prev_ts = in_ts;
    self->_timing_ctx.throttling_accum += diff;

    GstClockTimeDiff delay =
        MAX(self->_timing_ctx.avg_latency * 1000, self->_timing_ctx.throttling_delay);
    if (self->_timing_ctx.throttling_accum < delay) {
        GST_LOG_OBJECT(self, "Dropping buffer due to throttling (delay=%" G_GINT64_FORMAT "ms)", delay / 1000);
        return true;
    }

    self->_timing_ctx.throttling_accum = 0;
    return false;
}

GstFlowReturn secondary_mode_infer(GstDxInfer *self, GstBuffer *buf, const DXFrameMeta *frame_meta) {
    GST_LOG_OBJECT(self, "Processing %zu objects in secondary mode", frame_meta->_object_meta_list.size());

    for (auto* object_meta : frame_meta->_object_meta_list) {
        auto iter = object_meta->_input_tensors.find(self->_preproc_id);
        if (iter != object_meta->_input_tensors.end()) {
            object_meta->_output_tensors[self->_infer_id] = dxs::DXTensors();
            object_meta->_output_tensors[self->_infer_id].allocate(self->_output_tensor_size);

            bool ok = self->_backend->Put(
                iter->second.data_ptr(),
                object_meta->_output_tensors[self->_infer_id].data_ptr());

            if (!ok) {
                if (self->_backend->IsFlushed()) {
                    GST_DEBUG_OBJECT(self, "Backend Put() flushed during secondary inference");
                    gst_buffer_unref(buf);
                    {
                        std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
                        self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] -= 1;
                    }
                    return GST_FLOW_FLUSHING;
                }
                GST_WARNING_OBJECT(self, "Backend Put() failed for object, skipping inference");
                continue;
            }

            if (!self->_backend->Get(object_meta->_output_tensors[self->_infer_id])) {
                if (self->_backend->IsFlushed()) {
                    GST_DEBUG_OBJECT(self, "Backend Get() flushed during secondary inference");
                    self->_backend->Reset();
                    gst_buffer_unref(buf);
                    {
                        std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
                        self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] -= 1;
                    }
                    return GST_FLOW_FLUSHING;
                }
                GST_WARNING_OBJECT(self, "Backend Get() failed for object, skipping");
            }
        }
    }

    int stream_id = frame_meta->_stream_id;
    GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);
    if (ret != GST_FLOW_OK) {
        GST_WARNING_OBJECT(self, "Failed to push buffer: %s", gst_flow_get_name(ret));
    }

    {
        std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
        self->_eos_ctx.stream_pending_buffers[stream_id] -= 1;
    }

    return ret;
}

GstFlowReturn primary_mode_infer(GstDxInfer *self, GstBuffer *buf, DXFrameMeta *frame_meta) {
    bool submitted = false;
    auto iter = frame_meta->_input_tensors.find(self->_preproc_id);
    if (iter != frame_meta->_input_tensors.end()) {
        frame_meta->_output_tensors[self->_infer_id] = dxs::DXTensors();
        frame_meta->_output_tensors[self->_infer_id].allocate(self->_output_tensor_size);

        submitted = self->_backend->Put(
            iter->second.data_ptr(),
            frame_meta->_output_tensors[self->_infer_id].data_ptr());

        if (!submitted) {
            if (self->_backend->IsFlushed()) {
                gst_buffer_unref(buf);
                std::lock_guard<std::mutex> lock(self->_eos_ctx.eos_lock);
                self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] -= 1;
                return GST_FLOW_FLUSHING;
            }
            GST_WARNING_OBJECT(self, "Backend Put() failed, passing through without inference");
        }
    }

    {
        std::lock_guard<std::mutex> lock(self->_push_ctx.push_lock);
        self->_push_ctx.push_queue.push({submitted, buf});
        self->_push_ctx.cv.notify_all();
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {

    std::ignore = pad;
    GstDxInfer *self = GST_DXINFER(parent);

    GST_LOG_OBJECT(self, "Chain: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    if (should_drop_buffer_due_to_qos(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    if (should_drop_buffer_due_to_throttling(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    auto *frame_meta = dx_get_frame_meta(buf);

    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No DXFrameMeta, dropping buffer");
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    { // NOSONAR - scope for lock
        std::unique_lock<std::mutex> lock(self->_eos_ctx.eos_lock);
        if (self->_eos_ctx.stream_eos_arrived.count(frame_meta->_stream_id) > 0) {
            GST_INFO_OBJECT(self, "EOS Already Arrived [%d] ", frame_meta->_stream_id);
            gst_buffer_unref(buf);
            return GST_FLOW_OK;
        }
        self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id]++;
    }

    if (self->_secondary_mode) {
        return secondary_mode_infer(self, buf, frame_meta);
    } else {
        return primary_mode_infer(self, buf, frame_meta);
    }
}
