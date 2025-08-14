#include "gst-dxinfer.hpp"
#include <chrono>
#include <dlfcn.h>
#include <json-glib/json-glib.h>
#include <map>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <vector>

enum {
    PROP_0,
    PROP_PREPROC_ID,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    PROP_MODEL_PATH,
    PROP_CONFIG_PATH,
    N_PROPERTIES
};

GST_DEBUG_CATEGORY_STATIC(gst_dxinfer_debug_category);
#define GST_CAT_DEFAULT gst_dxinfer_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);

static gpointer push_thread_func(GstDxInfer *self);

G_DEFINE_TYPE(GstDxInfer, gst_dxinfer, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;

// Helper function for semantic version comparison
bool version_less_than(const std::string& version1, const std::string& version2) {
    std::vector<int> v1_parts, v2_parts;
    
    // Parse version strings
    std::stringstream ss1(version1), ss2(version2);
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

static void parse_config(GstDxInfer *self) {
    if (!g_file_test(self->_config_path, G_FILE_TEST_EXISTS)) {
        g_error("[dxinfer] Config file does not exist: %s\n",
                self->_config_path);
        return;
    }

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
        gint val = json_object_get_int_member(object, key);
        if (val < 0) {
            g_error("[dxinfer] Member %s has a negative value (%d) and cannot "
                    "be converted to unsigned.",
                    key, val);
        }
        target = static_cast<guint>(val);
    };

    assign_uint_member("preprocess_id", self->_preproc_id);
    assign_uint_member("inference_id", self->_infer_id);

    if (json_object_has_member(object, "secondary_mode")) {
        self->_secondary_mode =
            json_object_get_boolean_member(object, "secondary_mode");
    }

    g_object_unref(parser);
}

static void gst_dxinfer_set_property(GObject *object, guint property_id,
                                     const GValue *value,
                                     GParamSpec *app_spec) {
    auto self =
        G_TYPE_CHECK_INSTANCE_CAST((object), GST_TYPE_DXINFER, GstDxInfer);
    if ((object == nullptr) || (value == nullptr) || (app_spec == nullptr)) {
        g_error("[dxinfer] properties can not be null pointer !! ");
    }

    switch (property_id) {
    case PROP_MODEL_PATH: {
        if (nullptr != self->_model_path)
            g_free(self->_model_path);
        self->_model_path = g_strdup(g_value_get_string(value));
        break;
    }
    case PROP_CONFIG_PATH: {
        if (nullptr != self->_config_path)
            g_free(self->_config_path);
        self->_config_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;
    }
    case PROP_PREPROC_ID: {
        self->_preproc_id = g_value_get_uint(value);
        break;
    }
    case PROP_INFER_ID: {
        self->_infer_id = g_value_get_uint(value);
        break;
    }
    case PROP_SECONDARY_MODE: {
        self->_secondary_mode = g_value_get_boolean(value);
        break;
    }
    default:
        break;
    }
}

static void gst_dxinfer_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *app_spec) {
    auto self =
        G_TYPE_CHECK_INSTANCE_CAST((object), GST_TYPE_DXINFER, GstDxInfer);
    if ((object == nullptr) || (value == nullptr) || (app_spec == nullptr)) {
        g_error("[dxinfer] to get property, It can not be null pointer !! ");
    }
    switch (property_id) {
    case PROP_MODEL_PATH:
        g_value_set_string(value, self->_model_path);
        break;
    case PROP_CONFIG_PATH:
        g_value_set_string(value, self->_config_path);
        break;
    case PROP_PREPROC_ID:
        g_value_set_uint(value, self->_preproc_id);
        break;
    case PROP_INFER_ID:
        g_value_set_uint(value, self->_infer_id);
        break;
    case PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_secondary_mode);
        break;
    default:
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

    while (!self->_push_queue.empty()) {
        if (GST_IS_BUFFER(self->_push_queue.front().second)) {
            gst_buffer_unref(self->_push_queue.front().second);
        }
        self->_push_queue.pop();
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxinfer_finalize(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);

    if (self->_recent_latencies) {
        g_queue_free(self->_recent_latencies);
        self->_recent_latencies = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

void convert_tensor(std::vector<std::shared_ptr<dxrt::Tensor>> src,
                    dxs::DXTensors &output) {
    for (size_t i = 0; i < src.size(); i++) {
        dxs::DXTensor new_tensor;
        new_tensor._name = src[i]->name();
        new_tensor._shape = src[i]->shape();
        new_tensor._type = static_cast<dxs::DataType>(src[i]->type());
        new_tensor._data = src[i]->data();
        new_tensor._phyAddr = src[i]->phy_addr();
        new_tensor._elemSize = src[i]->elem_size();
        output._tensors.push_back(new_tensor);
    }
}

static void handle_null_to_ready(GstDxInfer *self) {
    if (self->_model_path == nullptr) {
        g_error("[dxinfer] Model Path Must be setted : %s\n",
                self->_model_path);
        return;
    }

    self->_ie = std::make_shared<dxrt::InferenceEngine>(self->_model_path);
    self->_output_tensor_size = self->_ie->GetOutputSize();
    std::string version = dxrt::Configuration::GetInstance().GetVersion();
    if (!version_meets_minimum(version, "3.0.0")) {
        g_error("[dxinfer] DXRT library version is too low. (required: >= 3.0.0, current: %s)\n", version.c_str());
        return;
    }
    std::string model_version = self->_ie->GetModelVersion();
    if (!version_meets_minimum(model_version, "7")) {
        g_error("[dxinfer] Model version is too low. (required: >= 7, current: %s , Use DX-COM v2.0.0 or higher)\n", model_version.c_str());
        return;
    }
}

static void handle_ready_to_paused(GstDxInfer *self) {
    if (self->_secondary_mode)
        return;

    if (!self->_push_running) {
        self->_push_running = TRUE;
        self->_push_thread =
            g_thread_new("push-thread", (GThreadFunc)push_thread_func, self);
    }
}

static void handle_playing_to_paused(GstDxInfer *self) {
    if (self->_secondary_mode)
        return;

    self->_push_running = FALSE;
    self->_cv.notify_all();

    if (self->_ie && self->_last_req_id != 0) {
        self->_ie->Wait(self->_last_req_id);
    }

    {
        std::unique_lock<std::mutex> lock(self->_push_lock);
        while (!self->_push_queue.empty()) {
            if (GST_IS_BUFFER(self->_push_queue.front().second)) {
                gst_buffer_unref(self->_push_queue.front().second);
            }
            self->_push_queue.pop();
        }
    }
}

static void handle_paused_to_ready(GstDxInfer *self) {
    if (self->_secondary_mode)
        return;

    self->_cv.notify_all();
    g_thread_join(self->_push_thread);
}

static void handle_paused_to_playing(GstDxInfer *self) {
    self->_push_running = TRUE;
    self->_cv.notify_all();
}

static GstStateChangeReturn dxinfer_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxInfer *self = GST_DXINFER(element);
    GST_INFO_OBJECT(self, "Attempting to change state");

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
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void gst_dxinfer_class_init(GstDxInferClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxinfer_debug_category, "dxinfer", 0,
                            "DXInfer plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = gst_dxinfer_set_property;
    gobject_class->get_property = gst_dxinfer_get_property;
    gobject_class->dispose = dxinfer_dispose;
    gobject_class->finalize = dxinfer_finalize;

    static GParamSpec *obj_properties[N_PROPERTIES] = {
        nullptr,
    };

    obj_properties[PROP_MODEL_PATH] =
        g_param_spec_string("model-path", "model file path",
                            "Path to the .dxnn model file used for inference.",
                            nullptr, G_PARAM_READWRITE);
    obj_properties[PROP_CONFIG_PATH] = g_param_spec_string(
        "config-file-path", "config path",
        "Path to the JSON config file containing the element's properties.",
        nullptr, G_PARAM_READWRITE);
    obj_properties[PROP_PREPROC_ID] = g_param_spec_uint(
        "preprocess-id", "pre process id",
        "Specifies the ID of the input tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[PROP_INFER_ID] = g_param_spec_uint(
        "inference-id", "inference id",
        "Specifies the ID of the output tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[PROP_SECONDARY_MODE] = g_param_spec_boolean(
        "secondary-mode", "secondary mode",
        "Determines whether to operate in primary mode or secondary mode.",
        FALSE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

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

static gboolean gst_dxinfer_sink_event(GstPad *pad, GstObject *parent,
                                       GstEvent *event) {
    GstDxInfer *self = GST_DXINFER(parent);
    // g_print("Received event: %s \n", GST_EVENT_TYPE_NAME(event));
    gboolean res = TRUE;
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        if (!self->_secondary_mode) {
            self->_global_eos = true;
            self->_cv.notify_all();
        } else {
            res = gst_pad_push_event(self->_srcpad, event);
        }
    } break;
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
        res = gst_pad_event_default(pad, parent, event);
        break;
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        const GstStructure *s_check = gst_event_get_structure(event);
        if (gst_structure_has_name(s_check,
                                   "application/x-dx-logical-stream-eos")) {
            int stream_id = -1;
            gst_structure_get_int(s_check, "stream-id", &stream_id);
            self->_stream_eos_arrived.insert(stream_id);
            res = TRUE;
            gst_event_unref(event);
        } else {
            res = gst_pad_push_event(self->_srcpad, event);
        }
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

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS: {

        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, nullptr, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_OBJECT_LOCK(parent);
            if (self->_throttling_delay != 0)
                /* set to more tight framerate */
                self->_throttling_delay = MIN(self->_throttling_delay, diff);
            else
                self->_throttling_delay = diff;
            GST_OBJECT_UNLOCK(parent);
            gst_event_unref(event);
            return TRUE;
        }

        if (type == GST_QOS_TYPE_UNDERFLOW && diff > 0) {
            GST_OBJECT_LOCK(parent);

            self->_qos_timediff = diff;
            self->_qos_timestamp = timestamp;

            GST_OBJECT_UNLOCK(parent);
            // gst_event_unref(event);
        }
        break;
    }
    default:
        break;
    }
    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxinfer_init(GstDxInfer *self) {
    GstPad *sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(sinkpad, GST_DEBUG_FUNCPTR(gst_dxinfer_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_sink_event));
    // GST_PAD_SET_PROXY_CAPS(sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_src_event));
    // gst_pad_set_active(self->_srcpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_model_path = nullptr;
    self->_config_path = nullptr;
    self->_secondary_mode = FALSE;
    self->_ie = nullptr;
    self->_output_tensor_size = 0;

    self->_push_queue = std::queue<std::pair<int, GstBuffer *>>();

    self->_avg_latency = 0;
    self->_recent_latencies = g_queue_new();
    self->_prev_ts = 0;
    self->_throttling_delay = 0;
    self->_throttling_accum = 0;

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;

    self->_global_eos = false;
    self->_stream_eos_arrived.clear();
    self->_stream_pending_buffers = std::map<int, int>();
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
    GstStructure *s_route_info =
        gst_structure_new("application/x-dx-route-info", "stream-id",
                          G_TYPE_INT, stream_id, NULL);
    GstEvent *route_info = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s_route_info);
    // g_print("INFER PUSH_LOGICAL_EOS: %d \n", stream_id);
    res = gst_pad_push_event(self->_srcpad, route_info);
    if (!res) {
        gst_event_unref(route_info); 
        return;
    }

    GstStructure *s_logical_eos =
        gst_structure_new("application/x-dx-logical-stream-eos", "stream-id",
                          G_TYPE_INT, stream_id, NULL);
    GstEvent *logical_eos_event =
        gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s_logical_eos);
    res = gst_pad_push_event(self->_srcpad, logical_eos_event);
    if (!res) {
        gst_event_unref(logical_eos_event);
    }
}

static gpointer push_thread_func(GstDxInfer *self) {
    while (self->_push_running) {
        GstBuffer *push_buf = nullptr;
        int req_id = -1;
        {
            std::unique_lock<std::mutex> lock(self->_push_lock);
            self->_cv.wait(lock, [self] {
                return self->_global_eos || !self->_push_running ||
                       !self->_push_queue.empty();
            });

            if (self->_global_eos && self->_push_queue.size() == 0) {
                GstEvent *eos_event = gst_event_new_eos();
                if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                    GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                }
                break;
            }

            if (!self->_push_running && self->_push_queue.size() == 0) {
                break;
            }

            push_buf = self->_push_queue.front().second;
            req_id = self->_push_queue.front().first;
            self->_push_queue.pop();
            self->_cv.notify_all();
        }

        DXFrameMeta *frame_meta = (DXFrameMeta *)gst_buffer_get_meta(
            push_buf, DX_FRAME_META_API_TYPE);

        if (req_id != -1) {
            auto outputs = self->_ie->Wait(req_id);
            convert_tensor(outputs,
                           frame_meta->_output_tensors[self->_infer_id]);
        }

        if (push_buf) {
            GstFlowReturn ret = gst_pad_push(self->_srcpad, push_buf);
            if (ret != GST_FLOW_OK) {
                GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
            }

            {
                std::unique_lock<std::mutex> lock(self->_eos_lock);
                self->_stream_pending_buffers[frame_meta->_stream_id] -= 1;
                if (self->_stream_eos_arrived.count(frame_meta->_stream_id) &&
                    self->_stream_pending_buffers[frame_meta->_stream_id] ==
                        0) {
                    self->_stream_eos_arrived.erase(frame_meta->_stream_id);
                    push_logical_eos(self, frame_meta->_stream_id);
                }
            }
        }
    }
    self->_cv.notify_all();
    return nullptr;
}

static bool should_drop_buffer_due_to_qos(GstDxInfer *self, GstBuffer *buf) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
    if (self->_qos_timediff <= 0)
        return false;

    GstClockTimeDiff earliest_time;
    if (self->_throttling_delay > 0) {
        earliest_time = self->_qos_timestamp + 2 * self->_qos_timediff +
                        self->_throttling_delay;
    } else {
        earliest_time = self->_qos_timestamp + self->_qos_timediff;
    }

    return static_cast<GstClockTime>(earliest_time) > in_ts;
}

static bool should_drop_buffer_due_to_throttling(GstDxInfer *self,
                                                 GstBuffer *buf) {
    if (self->_throttling_delay <= 0)
        return false;

    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
    GstClockTimeDiff diff = in_ts - self->_prev_ts;
    self->_throttling_accum += diff;

    GstClockTimeDiff delay =
        MAX(self->_avg_latency * 1000, self->_throttling_delay);
    if (self->_throttling_accum < delay) {
        self->_prev_ts = in_ts;
        return true;
    }

    self->_prev_ts = in_ts;
    return false;
}

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {
    GstDxInfer *self = GST_DXINFER(parent);

    if (should_drop_buffer_due_to_qos(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    if (should_drop_buffer_due_to_throttling(self, buf)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    gint64 latency = (gint64)self->_ie->GetLatency();

    if (g_queue_get_length(self->_recent_latencies) == 10) {
        g_queue_pop_head(self->_recent_latencies);
    }
    g_queue_push_tail(self->_recent_latencies, GINT_TO_POINTER(latency));
    self->_avg_latency = calculate_average(self->_recent_latencies);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);

    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_ERROR;
    }

    {
        std::unique_lock<std::mutex> lock(self->_eos_lock);
        self->_stream_pending_buffers[frame_meta->_stream_id]++;
    }

    if (self->_secondary_mode) {
        int objects_size = g_list_length(frame_meta->_object_meta_list);
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);
            auto iter = object_meta->_input_tensors.find(self->_preproc_id);
            if (iter != object_meta->_input_tensors.end()) {
                object_meta->_output_tensors[self->_infer_id] =
                    dxs::DXTensors();
                object_meta->_output_tensors[self->_infer_id]._mem_size =
                    self->_output_tensor_size;
                object_meta->_output_tensors[self->_infer_id]._data =
                    malloc(self->_output_tensor_size);
                auto outputs = self->_ie->Run(
                    iter->second._data, nullptr,
                    object_meta->_output_tensors[self->_infer_id]._data);
                convert_tensor(outputs,
                               object_meta->_output_tensors[self->_infer_id]);
            }
        }

        GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);
        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
        }

        {
            std::unique_lock<std::mutex> lock(self->_eos_lock);
            self->_stream_pending_buffers[frame_meta->_stream_id] -= 1;
            if (self->_stream_eos_arrived.count(frame_meta->_stream_id) &&
                self->_stream_pending_buffers[frame_meta->_stream_id] == 0) {
                self->_stream_eos_arrived.erase(frame_meta->_stream_id);
                push_logical_eos(self, frame_meta->_stream_id);
            }
        }

        return ret;
    } else {
        int req_id = -1;
        auto iter = frame_meta->_input_tensors.find(self->_preproc_id);
        if (iter != frame_meta->_input_tensors.end()) {
            frame_meta->_output_tensors[self->_infer_id] = dxs::DXTensors();
            frame_meta->_output_tensors[self->_infer_id]._mem_size =
                self->_output_tensor_size;
            frame_meta->_output_tensors[self->_infer_id]._data =
                malloc(self->_output_tensor_size);
            req_id = self->_ie->RunAsync(
                iter->second._data, nullptr,
                frame_meta->_output_tensors[self->_infer_id]._data);
            self->_last_req_id = req_id;
        }

        {
            std::unique_lock<std::mutex> lock(self->_push_lock);
            self->_cv.wait(lock, [self] {
                return self->_global_eos || !self->_push_running ||
                       self->_push_queue.size() <= MAX_PUSH_QUEUE_SIZE;
            });

            if (!self->_push_running) {
                gst_buffer_unref(buf);
                return GST_FLOW_OK;
            }

            self->_push_queue.push(std::make_pair(req_id, buf));
            self->_cv.notify_all();
        }
    }
    return GST_FLOW_OK;
}
