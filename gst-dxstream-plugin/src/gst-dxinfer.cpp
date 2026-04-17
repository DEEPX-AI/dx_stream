#include "gst-dxinfer.hpp"
#include <chrono>
#include <dlfcn.h>
#include <json-glib/json-glib.h>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

enum class PropertyID {
    PROP_0,
    PROP_PREPROC_ID,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    PROP_MODEL_PATH,
    PROP_CONFIG_PATH,
    PROP_USE_ORT,
    N_PROPERTIES
};

GST_DEBUG_CATEGORY_STATIC(gst_dxinfer_debug_category);
#define GST_CAT_DEFAULT gst_dxinfer_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);

static gpointer push_thread_func(GstDxInfer *self);

G_DEFINE_TYPE(GstDxInfer, gst_dxinfer, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

// Helper function for semantic version comparison
bool version_less_than(const std::string& version1, const std::string& version2) {
    std::vector<int> v1_parts;
    std::vector<int> v2_parts;
    
    // Parse version strings
    std::stringstream ss1(version1);
    std::stringstream ss2(version2);
    std::string part;
    
    while (std::getline(ss1, part, '.')) {
        v1_parts.push_back(std::stoi(part));
    }
    
    while (std::getline(ss2, part, '.')) {
        v2_parts.push_back(std::stoi(part));
    }
    
    // Pad with zeros if needed
    while (v1_parts.size() < v2_parts.size()) {
        v1_parts.push_back(0);
    }
    while (v2_parts.size() < v1_parts.size()) {
        v2_parts.push_back(0);
    }
    
    // Compare parts
    for (size_t i = 0; i < v1_parts.size(); ++i) {
        if (v1_parts[i] < v2_parts[i]) return true;
        if (v1_parts[i] > v2_parts[i]) return false;
    }
    
    return false; // versions are equal
}

// Helper function to check if version meets minimum requirement
bool version_meets_minimum(const std::string& current_version, const std::string& minimum_version) {
    return !version_less_than(current_version, minimum_version);
}

// Convert dxrt tensor metadata into dxs::DXTensors (data already in user buffer)
static void convert_tensor(const dxrt::TensorPtrs &src, dxs::DXTensors &output) {
    for (size_t i = 0; i < src.size(); i++) {
        dxs::DXTensor t;
        t._name = src[i]->name();
        t._shape = src[i]->shape();
        t._type = static_cast<dxs::DataType>(src[i]->type());
        t._data = src[i]->data();
        t._phyAddr = src[i]->phy_addr();
        t._elemSize = src[i]->elem_size();
        output._tensors.push_back(t);
    }
}

static void parse_config(GstDxInfer *self) {
    if (!g_file_test(self->_config_path, G_FILE_TEST_EXISTS)) {
        g_error("[dxinfer] Config file does not exist: %s\n",
                self->_config_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading config file: %s", self->_config_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, self->_config_path, &error)) {
        g_error("[dxinfer] Failed to load config file: %s", error->message);
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
            g_error("[dxinfer] Member %s has a negative value (%lld) and cannot "
                    "be converted to unsigned.",
                    key, (long long)val);
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

    if (self->_ie && self->_last_req_id != 0) {
        self->_ie->Wait(self->_last_req_id);
    }

    while (!self->_push_ctx.push_queue.empty()) {
        if (GST_IS_BUFFER(self->_push_ctx.push_queue.front().second)) {
            gst_buffer_unref(self->_push_ctx.push_queue.front().second);
        }
        self->_push_ctx.push_queue.pop();
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxinfer_finalize(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);

    if (self->_timing_ctx.recent_latencies) {
        g_queue_free(self->_timing_ctx.recent_latencies);
        self->_timing_ctx.recent_latencies = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void handle_null_to_ready(GstDxInfer *self) {
    if (self->_model_path == nullptr) {
        g_error("[dxinfer] Model Path Must be setted : %s\n",
                self->_model_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading model: %s (use_ort=%d)", self->_model_path, self->_use_ort);
    self->_infer_option = std::make_shared<dxrt::InferenceOption>();
    self->_infer_option->useORT = self->_use_ort;

    try {
        self->_ie = std::make_shared<dxrt::InferenceEngine>(
            self->_model_path, *(self->_infer_option));
    } catch (const dxrt::Exception &e) {
        GST_ELEMENT_ERROR(self, RESOURCE, FAILED,
                          ("[dxinfer] Failed to load InferenceEngine: %s", e.what()),
                          (nullptr));
        return;
    }

    self->_output_tensor_size = self->_ie->GetOutputSize();

    std::string version = dxrt::Configuration::GetInstance().GetVersion();
    if (!version_meets_minimum(version, "3.0.0")) {
        g_error("[dxinfer] DXRT library version is too low. (required: >= 3.0.0, current: %s)\n", version.c_str());
        return;
    }
    // std::string model_version = self->_ie->GetModelVersion();
    // if (!version_meets_minimum(model_version, "7")) {
    //     g_error("[dxinfer] Model version is too low. (required: >= 7, current: %s , Use DX-COM v2.0.0 or higher)\n", model_version.c_str());
    //     return;
    // }

    self->_num_devices = dxrt::DevicePool::GetInstance().GetDeviceCount();

    GST_INFO_OBJECT(self, "Inference engine initialized successfully (DXRT version: %s, Devices: %zu)", version.c_str(), self->_num_devices);
}

static void handle_ready_to_paused(GstDxInfer *self) {
    if (!self->_push_ctx.push_running) {
        self->_push_ctx.push_running = TRUE;
        if (!self->_secondary_mode) {
            GST_INFO_OBJECT(self, "Starting push thread");
            self->_push_ctx.push_thread =
                g_thread_new("push-thread", (GThreadFunc)push_thread_func, self);
        }
    }
}

static void handle_paused_to_playing(GstDxInfer *self) {
    if (!self->_secondary_mode) {
        self->_push_ctx.push_running = TRUE;
        if (!self->_push_ctx.push_thread) {
            self->_push_ctx.push_thread =
                g_thread_new("push-thread", (GThreadFunc)push_thread_func, self);
        }
    }
    self->_push_ctx.cv.notify_all();
}

static void handle_playing_to_paused(GstDxInfer *self) {
    GST_INFO_OBJECT(self, "Stopping push thread");
    self->_push_ctx.push_running = FALSE;
    self->_push_ctx.cv.notify_all();

    if (self->_push_ctx.push_thread && !self->_secondary_mode) {
        g_thread_join(self->_push_ctx.push_thread);
        self->_push_ctx.push_thread = nullptr;
        GST_INFO_OBJECT(self, "Push thread stopped");
    }
}

static void handle_paused_to_ready(GstDxInfer *self) {
    if (self->_secondary_mode)
        return;

    { // NOSONAR - scope for lock
        std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
        if (!self->_push_ctx.push_queue.empty()) {
            GST_WARNING_OBJECT(self, "Push queue not empty after thread completion: %zu items", 
                             self->_push_ctx.push_queue.size());
            while (!self->_push_ctx.push_queue.empty()) {
                auto& front = self->_push_ctx.push_queue.front();
                int existing_req_id = front.first;
                if (existing_req_id != -1) {
                    GST_DEBUG_OBJECT(self, "Waiting for inference request %d in cleanup", existing_req_id);
                    auto outputs = self->_ie->Wait(existing_req_id);
                }
                if (GST_IS_BUFFER(front.second)) {
                    gst_buffer_unref(front.second);
                }
                self->_push_ctx.push_queue.pop();
            }
        }
    }
    
    self->_last_req_id = 0;
}

static GstStateChangeReturn dxinfer_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxInfer *self = GST_DXINFER(element);
    const gchar *transition_name = gst_state_change_get_name(transition);
    GST_INFO_OBJECT(self, "State transition: %s", transition_name);

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        handle_null_to_ready(self);
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        handle_ready_to_paused(self);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        handle_paused_to_playing(self);
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        handle_playing_to_paused(self);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        handle_paused_to_ready(self);
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        GST_INFO_OBJECT(self, "Cleaning up inference engine");
        break;
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
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
        "Specifies the ID of the input tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_INFER_ID)] = g_param_spec_uint(
        "inference-id", "inference id",
        "Specifies the ID of the output tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_SECONDARY_MODE)] = g_param_spec_boolean(
        "secondary-mode", "secondary mode",
        "Determines whether to operate in primary mode or secondary mode.",
        FALSE, G_PARAM_READWRITE);
    obj_properties[static_cast<int>(PropertyID::PROP_USE_ORT)] = g_param_spec_boolean(
        "use-ort", "use ort",
        "Determines whether to use ONNX Runtime for inference.",
        TRUE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<int>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(element_class, "DXInfer", "Generic",
                                          "Performs inference",
                                          "Jo Sangil <sijo@deepx.ai>");

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
            size_t buffer_size = 0;
            { // NOSONAR - scope for lock
                std::unique_lock<std::mutex> lock(self->_eos_ctx.eos_lock);
                buffer_size = self->_eos_ctx.stream_pending_buffers[stream_id];
            }

            GST_INFO_OBJECT(self, "EOS Arrived From Stream [%d] ", stream_id);
            self->_eos_ctx.stream_eos_arrived.insert(stream_id);

            if (buffer_size == 0) {
                GST_INFO_OBJECT(self, "Push EOS From Stream [%d] ", stream_id);
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
        GST_INFO_OBJECT(self, "Received EOS event");
        size_t buffer_size = 0;
        { // NOSONAR - scope for lock
            std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
            buffer_size = self->_push_ctx.push_queue.size();
        }

        if (buffer_size > 0) {
            self->_eos_ctx.global_eos = true;
            self->_push_ctx.cv.notify_all();
        } else {
            res = gst_pad_push_event(self->_srcpad, event);
        }
    } break;
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
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

static gboolean gst_dxinfer_src_event(GstPad *pad, GstObject *parent,
                                      GstEvent *event) {
    GstDxInfer *self = GST_DXINFER(parent);

    if (GST_EVENT_TYPE(event) == GST_EVENT_QOS) {
        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, nullptr, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_DEBUG_OBJECT(self, "QoS THROTTLE event: diff=%" G_GINT64_FORMAT "ms", diff / 1000000);
            GST_OBJECT_LOCK(parent);
            if (self->_timing_ctx.throttling_delay != 0)
                /* set to more tight framerate */
                self->_timing_ctx.throttling_delay = MIN(self->_timing_ctx.throttling_delay, diff);
            else
                self->_timing_ctx.throttling_delay = diff;
            GST_OBJECT_UNLOCK(parent);
            gst_event_unref(event);
            return TRUE;
        }

        if (type == GST_QOS_TYPE_UNDERFLOW && diff > 0) {
            GST_DEBUG_OBJECT(self, "QoS UNDERFLOW event: diff=%" G_GINT64_FORMAT "ms", diff / 1000000);
            GST_OBJECT_LOCK(parent);

            self->_timing_ctx.qos_timediff = diff;
            self->_timing_ctx.qos_timestamp = timestamp;

            GST_OBJECT_UNLOCK(parent);
        }
    }

    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxinfer_init(GstDxInfer *self) {
    GstPad *sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(sinkpad, GST_DEBUG_FUNCPTR(gst_dxinfer_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_sink_event));
    gst_element_add_pad(GST_ELEMENT(self), sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_src_event));
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_model_path = nullptr;
    self->_config_path = nullptr;
    self->_secondary_mode = FALSE;
    self->_use_ort = TRUE;
    self->_ie = nullptr;
    self->_num_devices = 1;
    self->_output_tensor_size = 0;

    // Push context initialization
    self->_push_ctx.push_queue = std::queue<std::pair<int, GstBuffer *>>();
    self->_push_ctx.push_thread = nullptr;
    self->_push_ctx.push_running = FALSE;

    // Timing context initialization
    self->_timing_ctx.avg_latency = 0;
    self->_timing_ctx.recent_latencies = g_queue_new();
    self->_timing_ctx.prev_ts = 0;
    self->_timing_ctx.throttling_delay = 0;
    self->_timing_ctx.throttling_accum = 0;
    self->_timing_ctx.qos_timestamp = 0;
    self->_timing_ctx.qos_timediff = 0;

    // EOS context initialization
    self->_eos_ctx.global_eos = false;
    self->_eos_ctx.stream_eos_arrived.clear();
    self->_eos_ctx.stream_pending_buffers = std::map<int, int>();
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

void push_logical_eos(GstDxInfer *self, int stream_id) {
    gboolean res = TRUE;

    GstEvent *eos_event = gst_event_new_eos();
    GstStructure *s = gst_structure_new("application/x-dx-wrapped-event",
                                        "stream-id", G_TYPE_INT, stream_id,
                                        "event", GST_TYPE_EVENT, eos_event,
                                        NULL);

    GstEvent *wrapped_event = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
    GST_INFO_OBJECT(self, "Push EOS From Stream [%d] ", stream_id);
    res = gst_pad_push_event(self->_srcpad, wrapped_event);
    if (!res) {
        GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
        gst_event_unref(eos_event); 
    }
}

static gpointer push_thread_func(GstDxInfer *self) {
    while (self->_push_ctx.push_running) {
        GstBuffer *push_buf = nullptr;
        int req_id = -1;
        { // NOSONAR - scope for lock
            std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
            self->_push_ctx.cv.wait(lock, [self] {
                return self->_eos_ctx.global_eos || !self->_push_ctx.push_running ||
                       !self->_push_ctx.push_queue.empty();
            });

            if (self->_eos_ctx.global_eos && self->_push_ctx.push_queue.empty()) {
                GstEvent *eos_event = gst_event_new_eos();
                GST_INFO_OBJECT(self, "Push Global EOS");
                if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                    GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                }
                break;
            }

            if (!self->_push_ctx.push_running) {
                GST_INFO_OBJECT(self, "Push thread shutdown requested");
                break;
            }

            push_buf = self->_push_ctx.push_queue.front().second;
            req_id = self->_push_ctx.push_queue.front().first;
            self->_push_ctx.push_queue.pop();
            self->_push_ctx.cv.notify_all();
        }

        if (!GST_IS_BUFFER(push_buf)) {
            GST_ERROR_OBJECT(self, "Invalid buffer in push thread");
            continue;
        }

        auto *frame_meta = dx_get_frame_meta(push_buf);

        if (req_id != -1) {
            auto outputs = self->_ie->Wait(req_id);
            convert_tensor(outputs, frame_meta->_output_tensors[self->_infer_id]);
        }
        GstFlowReturn ret = gst_pad_push(self->_srcpad, push_buf);
        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
        }

        { // NOSONAR - scope for lock
            std::unique_lock<std::mutex> lock(self->_eos_ctx.eos_lock);
            self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] -= 1;
            if (self->_eos_ctx.stream_eos_arrived.count(frame_meta->_stream_id) &&
                self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] ==
                    0) {
                self->_eos_ctx.stream_eos_arrived.erase(frame_meta->_stream_id);
                push_logical_eos(self, frame_meta->_stream_id);
            }
        }
    }

    GST_INFO_OBJECT(self, "Cleaning up %zu remaining buffers with inference completion", 
                              self->_push_ctx.push_queue.size());
                
    while (!self->_push_ctx.push_queue.empty()) {
        auto& front = self->_push_ctx.push_queue.front();
        int existing_req_id = front.first;
        GstBuffer* existing_buf = front.second;
        
        if (existing_req_id != -1) {
            GST_DEBUG_OBJECT(self, "Waiting for inference request %d in cleanup", existing_req_id);
            auto outputs = self->_ie->Wait(existing_req_id);
        }
        
        if (GST_IS_BUFFER(existing_buf)) {
            gst_buffer_unref(existing_buf);
        }
        self->_push_ctx.push_queue.pop();
    }
    
    GST_INFO_OBJECT(self, "Push thread cleanup completed");
    
    self->_push_ctx.cv.notify_all();
    return nullptr;
}

static bool should_drop_buffer_due_to_qos(const GstDxInfer *self, GstBuffer *buf) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
    if (self->_timing_ctx.qos_timediff <= 0)
        return false;

    GstClockTimeDiff earliest_time;
    if (self->_timing_ctx.throttling_delay > 0) {
        earliest_time = self->_timing_ctx.qos_timestamp + 2 * self->_timing_ctx.qos_timediff +
                        self->_timing_ctx.throttling_delay;
    } else {
        earliest_time = self->_timing_ctx.qos_timestamp + self->_timing_ctx.qos_timediff;
    }

    bool should_drop = static_cast<GstClockTime>(earliest_time) > in_ts;
    if (should_drop) {
        GST_DEBUG_OBJECT(self, "Dropping buffer due to QoS (ts=%" GST_TIME_FORMAT ")",
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
    self->_timing_ctx.throttling_accum += diff;

    GstClockTimeDiff delay =
        MAX(self->_timing_ctx.avg_latency * 1000, self->_timing_ctx.throttling_delay);
    if (self->_timing_ctx.throttling_accum < delay) {
        self->_timing_ctx.prev_ts = in_ts;
        GST_DEBUG_OBJECT(self, "Dropping buffer due to throttling (delay=%" G_GINT64_FORMAT "ms)", delay / 1000);
        return true;
    }

    self->_timing_ctx.prev_ts = in_ts;
    return false;
}

GstFlowReturn secondary_mode_infer(GstDxInfer *self, GstBuffer *buf, const DXFrameMeta *frame_meta) {
    // Implementation for secondary mode inference
    if (!self->_push_ctx.push_running) {
        GST_DEBUG_OBJECT(self, "Dropping buffer in secondary mode due to shutdown");
        gst_buffer_unref(buf);
        return GST_FLOW_FLUSHING;
    }

    GST_DEBUG_OBJECT(self, "Processing %zu objects in secondary mode", frame_meta->_object_meta_list.size());
    for (auto* object_meta : frame_meta->_object_meta_list) {
        if (!self->_push_ctx.push_running) {
            GST_DEBUG_OBJECT(self, "Stopping inference loop due to shutdown");
            gst_buffer_unref(buf);
            return GST_FLOW_FLUSHING;
        }

        auto iter = object_meta->_input_tensors.find(self->_preproc_id);
        if (iter != object_meta->_input_tensors.end()) {
            object_meta->_output_tensors[self->_infer_id] = dxs::DXTensors();
            object_meta->_output_tensors[self->_infer_id].allocate(self->_output_tensor_size);

            auto outputs = self->_ie->Run(
                iter->second.data_ptr(), nullptr,
                object_meta->_output_tensors[self->_infer_id].data_ptr());
            convert_tensor(outputs, object_meta->_output_tensors[self->_infer_id]);
        }
    }

    if (!self->_push_ctx.push_running) {
        GST_DEBUG_OBJECT(self, "Dropping buffer before push due to shutdown");
        gst_buffer_unref(buf);
        return GST_FLOW_FLUSHING;
    }

    GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);
    if (ret != GST_FLOW_OK) {
        GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
    }

    { // NOSONAR - scope for lock
        std::unique_lock<std::mutex> lock(self->_eos_ctx.eos_lock);
        self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] -= 1;
        if (self->_eos_ctx.stream_eos_arrived.count(frame_meta->_stream_id) &&
            self->_eos_ctx.stream_pending_buffers[frame_meta->_stream_id] == 0) {
            self->_eos_ctx.stream_eos_arrived.erase(frame_meta->_stream_id);
            push_logical_eos(self, frame_meta->_stream_id);
        }
    }

    return ret;
}

GstFlowReturn primary_mode_infer(GstDxInfer *self, GstBuffer *buf, DXFrameMeta *frame_meta) {
    int req_id = -1;
    auto iter = frame_meta->_input_tensors.find(self->_preproc_id);
    if (iter != frame_meta->_input_tensors.end()) {
        frame_meta->_output_tensors[self->_infer_id] = dxs::DXTensors();
        frame_meta->_output_tensors[self->_infer_id].allocate(self->_output_tensor_size);

        req_id = self->_ie->RunAsync(
            iter->second.data_ptr(), nullptr,
            frame_meta->_output_tensors[self->_infer_id].data_ptr());
        GST_DEBUG_OBJECT(self, "Submitting async inference request %d", req_id);
        self->_last_req_id = req_id;
    }

    { // NOSONAR - scope for lock
        std::unique_lock<std::mutex> lock(self->_push_ctx.push_lock);
        self->_push_ctx.cv.wait(lock, [self] {
            return self->_eos_ctx.global_eos || !self->_push_ctx.push_running ||
                    self->_push_ctx.push_queue.size() <= MAX_PUSH_QUEUE_SIZE * self->_num_devices;
        });

        if (!self->_push_ctx.push_running) {
            if (req_id != -1) {
                auto outputs = self->_ie->Wait(req_id);
            }
            gst_buffer_unref(buf);
            return GST_FLOW_FLUSHING;
        } else {
            GST_DEBUG_OBJECT(self, "Queue size after wait: %zu", self->_push_ctx.push_queue.size());
        }

        GST_DEBUG_OBJECT(self, "Queueing buffer with request ID %d", req_id);
        self->_push_ctx.push_queue.push(std::make_pair(req_id, buf));
        self->_push_ctx.cv.notify_all();
    }
    return GST_FLOW_OK;
}

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {

    std::ignore = pad;
    GstDxInfer *self = GST_DXINFER(parent);

    if (should_drop_buffer_due_to_qos(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    if (should_drop_buffer_due_to_throttling(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    auto latency = (gint64)self->_ie->GetLatency();

    if (g_queue_get_length(self->_timing_ctx.recent_latencies) == 10) {
        g_queue_pop_head(self->_timing_ctx.recent_latencies);
    }
    g_queue_push_tail(self->_timing_ctx.recent_latencies, GINT_TO_POINTER(latency));
    self->_timing_ctx.avg_latency = calculate_average(self->_timing_ctx.recent_latencies);
    
    GST_DEBUG_OBJECT(self, "Inference latency: %" G_GINT64_FORMAT "ms, avg: %" G_GINT64_FORMAT "ms",
                     latency, self->_timing_ctx.avg_latency);

    auto *frame_meta = dx_get_frame_meta(buf);

    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_ERROR;
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
