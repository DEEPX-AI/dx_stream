#include "gst-dxinfer.hpp"
#include <chrono>
#include <dlfcn.h>
#include <json-glib/json-glib.h>
#include <map>
#include <opencv2/opencv.hpp>

enum {
    PROP_0,
    PROP_PREPROC_ID,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    PROP_MODEL_PATH,
    PROP_CONFIG_PATH,
    PROP_POOL_SIZE,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxinfer_debug_category);
#define GST_CAT_DEFAULT gst_dxinfer_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);
static gpointer inference_thread_func(GstDxInfer *self);

static gpointer push_thread_func(GstDxInfer *self);

G_DEFINE_TYPE(GstDxInfer, gst_dxinfer, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = NULL;

static void parse_config(GstDxInfer *self) {
    if (g_file_test(self->_config_path, G_FILE_TEST_EXISTS)) {
        JsonParser *parser = json_parser_new();
        GError *error = NULL;
        if (json_parser_load_from_file(parser, self->_config_path, &error)) {
            JsonNode *node = json_parser_get_root(parser);
            JsonObject *object = json_node_get_object(node);
            const gchar *model_path =
                json_object_get_string_member(object, "model_path");
            g_object_set(self, "model-path", model_path, NULL);
            if (json_object_has_member(object, "preprocess_id")) {
                gint int_value =
                    json_object_get_int_member(object, "preprocess_id");
                if (int_value < 0) {
                    g_error("[dxinfer] Member preprocess_id has a negative "
                            "value (%d) "
                            "and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_preproc_id = (guint)int_value;
            }
            if (json_object_has_member(object, "inference_id")) {
                gint int_value =
                    json_object_get_int_member(object, "inference_id");
                if (int_value < 0) {
                    g_error("[dxinfer] Member inference_id has a negative "
                            "value (%d) "
                            "and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_infer_id = (guint)int_value;
            }
            if (json_object_has_member(object, "secondary_mode")) {
                self->_secondary_mode =
                    json_object_get_boolean_member(object, "secondary_mode");
            }
            if (json_object_has_member(object, "pool_size")) {
                gint int_value =
                    json_object_get_int_member(object, "pool_size");
                if (int_value < 0) {
                    g_error("[dxinfer] Member pool_size has a negative value "
                            "(%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_pool_size = (guint)int_value;
            }
            g_object_unref(parser);
        }
    } else {
        g_error("[dxinfer] Config file does not exist: %s\n",
                self->_config_path);
    }
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
    case PROP_POOL_SIZE: {
        self->_pool_size = g_value_get_uint(value);
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
    case PROP_POOL_SIZE:
        g_value_set_uint(value, self->_pool_size);
        break;
    default:
        break;
    }
}

static void dxinfer_dispose(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);
    if (self->_config_path) {
        g_free(self->_config_path);
        self->_config_path = NULL;
    }
    if (self->_model_path) {
        g_free(self->_model_path);
        self->_model_path = NULL;
    }

    self->_pool.deinitialize();

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxinfer_finalize(GObject *object) {
    GstDxInfer *self = GST_DXINFER(object);

    self->_pool.deinitialize();
    self->_ie.reset();

    if (self->_recent_latencies) {
        g_queue_free(self->_recent_latencies);
        self->_recent_latencies = nullptr;
    }

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

struct CallbackInput {
    DXFrameMeta *frame_meta;
    DXObjectMeta *object_meta;
    GstDxInfer *self;

    CallbackInput() : frame_meta(NULL), object_meta(NULL), self(NULL) {}

    CallbackInput(DXFrameMeta *_frame_meta, DXObjectMeta *_object_meta,
                  GstDxInfer *_self)
        : frame_meta(_frame_meta), object_meta(_object_meta), self(_self) {}

    CallbackInput &operator=(const CallbackInput &other) {
        frame_meta = other.frame_meta;
        object_meta = other.object_meta;
        self = other.self;
        return *this;
    }

    CallbackInput(const CallbackInput &other)
        : frame_meta(other.frame_meta), object_meta(other.object_meta),
          self(other.self) {}

    ~CallbackInput() {
        frame_meta = NULL;
        self = NULL;
    }
};

std::vector<dxs::DXTensor>
convert_tensor(std::vector<shared_ptr<dxrt::Tensor>> src) {
    std::vector<dxs::DXTensor> output;
    for (size_t i = 0; i < src.size(); i++) {
        dxs::DXTensor new_tensor;
        new_tensor._name = src[i]->name();
        new_tensor._shape = src[i]->shape();
        new_tensor._type = static_cast<dxs::DataType>(src[i]->type());
        new_tensor._data = src[i]->data();
        new_tensor._phyAddr = src[i]->phy_addr();
        new_tensor._elemSize = src[i]->elem_size();
        output.push_back(new_tensor);
    }

    return output;
}

static GstStateChangeReturn dxinfer_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxInfer *self = GST_DXINFER(element);
    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        if (self->_model_path != nullptr) {
            self->_ie =
                std::make_shared<dxrt::InferenceEngine>(self->_model_path);
            std::function<int(std::vector<std::shared_ptr<dxrt::Tensor>>,
                              void *)>
                postProcCallBack = [&](std::vector<shared_ptr<dxrt::Tensor>>
                                           outputs,
                                       void *arg) {
                    CallbackInput *callback_input =
                        static_cast<CallbackInput *>(arg);

                    guint infer_id = callback_input->self->_infer_id;
                    if (callback_input->self->_secondary_mode) {
                        if (!callback_input->object_meta) {
                            g_error("DXObjectMeta Not found in Inference "
                                    "Callback \n");
                        }
                        // object
                        callback_input->object_meta->_output_tensor[infer_id] =
                            convert_tensor(outputs);
                        // callback_input->self->_infer_count++;
                    } else {
                        // frame
                        callback_input->frame_meta->_output_tensor[infer_id] =
                            convert_tensor(outputs);

                        std::unique_lock<std::mutex> lock(
                            callback_input->self->_push_lock);
                        if (callback_input->self->_push_running) {
                            callback_input->self->_push_queue.push_back(
                                callback_input->frame_meta->_buf);
                        }
                    }

                    callback_input->self->_push_cv.notify_one();

                    // callback_input->self->_frame_count_for_fps++;
                    // if (callback_input->self->_frame_count_for_fps % 100 ==
                    // 0) {
                    //     auto end = std::chrono::high_resolution_clock::now();
                    //     auto frameDuration = std::chrono::duration_cast<
                    //         std::chrono::microseconds>(
                    //         end - callback_input->self->_start_time);
                    //     double frameTimeSec = frameDuration.count() /
                    //     1000000.0; double fps = 100.0 / frameTimeSec; gchar
                    //     *name = NULL;
                    //     g_object_get(G_OBJECT(callback_input->self), "name",
                    //                  &name, NULL);
                    //     g_print("[%s]\tFPS : %f \n", name, fps);
                    //     callback_input->self->_start_time =
                    //         std::chrono::high_resolution_clock::now();
                    //     callback_input->self->_frame_count_for_fps = 0;
                    //     g_free(name);
                    // }
                    delete callback_input;
                    return 0;
                };
            self->_ie->RegisterCallback(postProcCallBack);
        }
        if (self->_ie != nullptr) {
            self->_pool.deinitialize();
            self->_pool.initialize(self->_ie->GetOutputSize(),
                                   self->_pool_size);
        }
    } break;
    case GST_STATE_CHANGE_READY_TO_PAUSED: {
        if (!self->_secondary_mode) {
            if (!self->_running) {
                self->_running = TRUE;
                self->_thread =
                    g_thread_new("inference-thread",
                                 (GThreadFunc)inference_thread_func, self);
            }
            if (!self->_push_running) {
                self->_push_running = TRUE;
                self->_push_thread = g_thread_new(
                    "push-thread", (GThreadFunc)push_thread_func, self);
            }
        }
    } break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING: {
        self->_start_time = std::chrono::high_resolution_clock::now();
        self->_frame_count_for_fps = 0;
        break;
    }
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY: {
        if (!self->_secondary_mode) {
            {
                if (self->_running) {
                    self->_running = FALSE;
                }
            }
            self->_cv.notify_one();
            g_thread_join(self->_thread);
            self->_ie->Wait(self->_last_req_id);
            if (self->_push_running) {
                self->_push_running = FALSE;
            }
            self->_push_cv.notify_one();
            g_thread_join(self->_push_thread);
        }
        while (!self->_buffer_queue.empty()) {
            GstBuffer *buffer = self->_buffer_queue.front();
            self->_buffer_queue.pop();
            gst_buffer_unref(buffer);
        }
        while (!self->_push_queue.empty()) {
            GstBuffer *buffer = self->_push_queue.front();
            self->_push_queue.erase(self->_push_queue.begin());
            gst_buffer_unref(buffer);
        }
        self->_cv.notify_one();
        self->_push_cv.notify_one();
    } break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        break;
    default:
        break;
    }
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    if (result == GST_STATE_CHANGE_FAILURE)
        return result;
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

    obj_properties[PROP_MODEL_PATH] =
        g_param_spec_string("model-path", "model file path",
                            "Path to the .dxnn model file used for inference.",
                            NULL, G_PARAM_READWRITE);
    obj_properties[PROP_CONFIG_PATH] = g_param_spec_string(
        "config-file-path", "config path",
        "Path to the JSON config file containing the element's properties.",
        NULL, G_PARAM_READWRITE);
    obj_properties[PROP_PREPROC_ID] = g_param_spec_uint(
        "preprocess-id", "pre process id",
        "Specifies the ID of the input tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[PROP_INFER_ID] = g_param_spec_uint(
        "inference-id", "inference id",
        "Specifies the ID of the output tensor to be used for inference.", 0,
        10000, 0, G_PARAM_READWRITE);
    obj_properties[PROP_POOL_SIZE] =
        g_param_spec_uint("pool-size", "Output Tensor Memory Pool SIZE",
                          "Specifies the number of preallocated memory blocks "
                          "for output tensors.",
                          0, 1000, 0, G_PARAM_READWRITE);
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
    // GST_LOG("Received event: %s", GST_EVENT_TYPE_NAME(event));

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        self->_get_eos = true;
        if (!self->_secondary_mode) {
            {
                std::unique_lock<std::mutex> lock(self->_queue_lock);
                self->_cv.wait(
                    lock, [self] { return self->_buffer_queue.size() == 0; });
            }
            self->_ie->Wait(self->_last_req_id);

            {
                std::unique_lock<std::mutex> lock(self->_push_lock);
                self->_push_cv.wait(
                    lock, [self] { return self->_push_queue.size() == 0; });
            }
        }
        break;
    }
    default:
        break;
    }
    return gst_pad_event_default(pad, parent, event);
}

static gboolean gst_dxinfer_src_event(GstPad *pad, GstObject *parent,
                                      GstEvent *event) {
    GstDxInfer *self = GST_DXINFER(parent);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS: {

        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, NULL, &diff, &timestamp);

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
    GST_PAD_SET_PROXY_CAPS(sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxinfer_src_event));
    gst_pad_set_active(self->_srcpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_model_path = NULL;
    self->_config_path = NULL;
    self->_secondary_mode = FALSE;
    self->_ie = nullptr;
    self->_pool_size = 1;

    self->_buffer_queue = std::queue<GstBuffer *>();
    self->_push_queue = std::vector<GstBuffer *>();

    self->_avg_latency = 0;
    self->_recent_latencies = g_queue_new();
    self->_prev_ts = 0;
    self->_throttling_delay = 0;
    self->_throttling_accum = 0;
    self->_buffer_cnt = 0;

    self->_infer_count = 0;

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;

    self->_get_eos = false;
}

void inference_async(GstDxInfer *self, DXFrameMeta *frame_meta) {
    if (self->_secondary_mode) {
        int infer_objects_cnt = 0;
        int objects_size = g_list_length(frame_meta->_object_meta_list);
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);
            auto iter = object_meta->_input_tensor.find(self->_preproc_id);
            if (iter != object_meta->_input_tensor.end()) {
                CallbackInput *callback_input =
                    new CallbackInput(frame_meta, object_meta, self);
                object_meta->_output_memory_pool[self->_infer_id] =
                    (MemoryPool *)&self->_pool;
                self->_ie->Run(
                    iter->second._data, static_cast<void *>(callback_input),
                    self->_pool.allocate());
                infer_objects_cnt++;
            }
        }
        // while(self->_infer_count < infer_objects_cnt) {
        //     g_print("sdf");
        // }
        // self->_infer_count = 0;
        // self->_ie->Wait(self->_last_req_id);
    } else {
        auto iter = frame_meta->_input_tensor.find(self->_preproc_id);
        if (iter != frame_meta->_input_tensor.end()) {
            CallbackInput *callback_input =
                new CallbackInput(frame_meta, nullptr, self);
            frame_meta->_output_memory_pool[self->_infer_id] =
                (MemoryPool *)&self->_pool;
            self->_last_req_id = self->_ie->RunAsync(
                iter->second._data, static_cast<void *>(callback_input),
                self->_pool.allocate());
        } else {
            self->_ie->Wait(self->_last_req_id);
            std::unique_lock<std::mutex> lock(self->_push_lock);
            if (self->_push_running) {
                self->_push_queue.push_back(frame_meta->_buf);
            }
        }
    }
}

gint64 calculate_average(GQueue *queue) {
    if (g_queue_is_empty(queue)) {
        return 0.0;
    }

    gint64 sum = 0;
    guint count = 0;

    for (GList *node = queue->head; node != NULL; node = node->next) {
        sum += GPOINTER_TO_INT(node->data);
        count++;
    }

    return (gint)sum / count;
}

static gpointer push_thread_func(GstDxInfer *self) {
    while (self->_push_running) {
        if (!self->_get_eos && self->_push_queue.size() <= MAX_QUEUE_SIZE - 1) {
            g_usleep(100);
            continue;
        }
        if (self->_push_queue.empty()) {
            g_usleep(100);
            continue;
        }
        GstBuffer *push_buf = nullptr;
        {
            std::unique_lock<std::mutex> lock(self->_push_lock);
            auto smallest_it = std::min_element(
                self->_push_queue.begin(), self->_push_queue.end(),
                [](GstBuffer *a, GstBuffer *b) {
                    return GST_BUFFER_PTS(a) < GST_BUFFER_PTS(b);
                });
            push_buf = gst_buffer_ref(*smallest_it);
            gst_buffer_unref(*smallest_it);
            self->_push_queue.erase(smallest_it);
        }
        self->_push_cv.notify_one();

        if (push_buf) {
            GstFlowReturn ret = gst_pad_push(self->_srcpad, push_buf);
            if (ret != GST_FLOW_OK) {
                GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
                break;
            }
        }
    }
    return nullptr;
}

static gpointer inference_thread_func(GstDxInfer *self) {
    while (self->_running) {
        if (self->_buffer_queue.empty()) {
            self->_cv.notify_one();
            g_usleep(100);
            continue;
        }

        GstBuffer *buf = nullptr;
        {
            std::unique_lock<std::mutex> lock(self->_queue_lock);
            buf = self->_buffer_queue.front();
            self->_buffer_queue.pop();
        }
        self->_cv.notify_one();

        if (buf) {
            // QOS
            GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
            if (self->_qos_timediff > 0) {
                GstClockTimeDiff earliest_time;
                if (self->_throttling_delay > 0) {
                    earliest_time = self->_qos_timestamp +
                                    2 * self->_qos_timediff +
                                    self->_throttling_delay;
                } else {
                    earliest_time = self->_qos_timestamp + self->_qos_timediff;
                }
                if (static_cast<GstClockTime>(earliest_time) > in_ts) {
                    gst_buffer_unref(buf);
                    continue;
                }
            }

            if (self->_throttling_delay > 0) {
                GstClockTimeDiff diff = in_ts - self->_prev_ts;
                self->_throttling_accum += diff;

                GstClockTimeDiff delay =
                    MAX(self->_avg_latency * 1000, self->_throttling_delay);
                if (self->_throttling_accum < delay) {
                    // GstClockTimeDiff duration = GST_BUFFER_DURATION(buf);
                    // gdouble avg_rate = gst_guint64_to_gdouble(duration) /
                    //                    gst_guint64_to_gdouble(delay);
                    self->_prev_ts = in_ts;
                    gst_buffer_unref(buf);
                    continue;
                }
            }
            self->_prev_ts = in_ts;

            DXFrameMeta *frame_meta =
                (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
            if (!frame_meta) {
                GST_ERROR_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
                gst_buffer_unref(buf);
                continue;
            }
            inference_async(self, frame_meta);
            gint64 latency = (gint64)self->_ie->GetLatency();

            if (g_queue_get_length(self->_recent_latencies) == 10) {
                g_queue_pop_head(self->_recent_latencies);
            }
            g_queue_push_tail(self->_recent_latencies,
                              GINT_TO_POINTER(latency));
            self->_avg_latency = calculate_average(self->_recent_latencies);
        }
        self->_cv.notify_one();
    }
    return nullptr;
}

static GstFlowReturn gst_dxinfer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {
    GstDxInfer *self = GST_DXINFER(parent);
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }
    if (self->_secondary_mode) {
        inference_async(self, frame_meta);
        GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);
        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer:%d\n ", ret);
        }
        return ret;
    } else {
        {
            std::unique_lock<std::mutex> lock(self->_queue_lock);
            self->_cv.wait(lock, [self] {
                return self->_buffer_queue.size() < MAX_QUEUE_SIZE &&
                       self->_push_queue.size() < MAX_QUEUE_SIZE;
            });
            if (self->_running) {
                self->_buffer_queue.push(buf);
            }
        }
        self->_cv.notify_one();
    }

    return GST_FLOW_OK;
}
