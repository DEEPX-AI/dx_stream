#include "gst-dxpostprocess.hpp"
#include "gst-dxmeta.hpp"
#include <dlfcn.h>
#include <json-glib/json-glib.h>

enum {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_LIBRARY_FILE_PATH,
    PROP_FUNCTION_NAME,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxpostprocess_debug_category);
#define GST_CAT_DEFAULT gst_dxpostprocess_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxpostprocess_transform_ip(GstBaseTransform *trans,
                                                    GstBuffer *buf);
static gboolean gst_dxpostprocess_start(GstBaseTransform *trans);
static gboolean gst_dxpostprocess_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxPostprocess, gst_dxpostprocess, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxpostprocess_debug_category,
                            "gst-dxpostprocess", 0,
                            "debug category for gst-dxpostprocess element"))

static GstElementClass *parent_class = NULL;

static void parse_config(GstDxPostprocess *self) {
    if (g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        JsonParser *parser = json_parser_new();
        GError *error = NULL;
        if (json_parser_load_from_file(parser, self->_config_file_path,
                                       &error)) {
            JsonNode *node = json_parser_get_root(parser);
            JsonObject *object = json_node_get_object(node);
            if (json_object_has_member(object, "library_file_path")) {
                const gchar *library_file_path =
                    json_object_get_string_member(object, "library_file_path");
                g_object_set(self, "library-file-path", library_file_path,
                             NULL);
            }
            if (json_object_has_member(object, "function_name")) {
                const gchar *function_name =
                    json_object_get_string_member(object, "function_name");
                g_object_set(self, "function-name", function_name, NULL);
            }
            if (json_object_has_member(object, "inference_id")) {
                gint int_value =
                    json_object_get_int_member(object, "inference_id");
                if (int_value < 0) {
                    g_error("Member inference_id has a negative value (%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_infer_id = (guint)int_value;
            }
            if (json_object_has_member(object, "secondary_mode")) {
                self->_secondary_mode =
                    json_object_get_boolean_member(object, "secondary_mode");
            }
            g_object_unref(parser);
        }
    } else {
        g_print("Config file does not exist: %s\n", self->_config_file_path);
    }
}

static void dxpostprocess_set_property(GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(object);

    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        if (nullptr != self->_config_file_path)
            g_free(self->_config_file_path);
        self->_config_file_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;

    case PROP_LIBRARY_FILE_PATH:
        if (self->_library_file_path) {
            g_free(self->_library_file_path);
        }
        self->_library_file_path = g_value_dup_string(value);
        break;

    case PROP_FUNCTION_NAME:
        if (self->_function_name) {
            g_free(self->_function_name);
        }
        self->_function_name = g_value_dup_string(value);
        break;

    case PROP_SECONDARY_MODE:
        self->_secondary_mode = g_value_get_boolean(value);
        break;

    case PROP_INFER_ID: {
        self->_infer_id = g_value_get_uint(value);
        break;
    }

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxpostprocess_get_property(GObject *object, guint property_id,
                                       GValue *value, GParamSpec *pspec) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(object);

    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;

    case PROP_LIBRARY_FILE_PATH:
        g_value_set_string(value, self->_library_file_path);
        break;

    case PROP_FUNCTION_NAME:
        g_value_set_string(value, self->_function_name);
        break;

    case PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_secondary_mode);
        break;

    case PROP_INFER_ID:
        g_value_set_uint(value, self->_infer_id);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static GstStateChangeReturn
dxpostprocess_change_state(GstElement *element, GstStateChange transition) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(element);
    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        if (!self->_library_handle && self->_library_file_path &&
            self->_function_name) {
            self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
            if (!self->_library_handle) {
                g_print("Error opening library: %s\n", dlerror());
            }
            void *func_ptr = dlsym(self->_library_handle, self->_function_name);
            if (!func_ptr) {
                g_print("Error finding function: %s\n", dlerror());
                dlclose(self->_library_handle);
                self->_library_handle = NULL;
            }
            self->_postproc_function =
                (void (*)(std::vector<dxs::DXTensor>, DXFrameMeta *,
                          DXObjectMeta *))func_ptr;
        }
        break;
    }
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        break;
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxpostprocess_dispose(GObject *object) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(object);
    if (self->_config_file_path) {
        g_free(self->_config_file_path);
        self->_config_file_path = NULL;
    }
    if (self->_library_file_path) {
        g_free(self->_library_file_path);
        self->_library_file_path = NULL;
    }
    if (self->_function_name) {
        g_free(self->_function_name);
        self->_function_name = NULL;
    }
    if (self->_library_handle) {
        dlclose(self->_library_handle);
        self->_library_handle = NULL;
    }
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxpostprocess_class_init(GstDxPostprocessClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxpostprocess_debug_category, "dxpostprocess",
                            0, "DXPostprocess plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxpostprocess_set_property;
    gobject_class->get_property = dxpostprocess_get_property;
    gobject_class->dispose = dxpostprocess_dispose;

    obj_properties[PROP_CONFIG_FILE_PATH] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the configuration file", NULL, G_PARAM_READWRITE);

    obj_properties[PROP_LIBRARY_FILE_PATH] = g_param_spec_string(
        "library-file-path", "Library File Path",
        "Path to the shared library file", NULL, G_PARAM_READWRITE);

    obj_properties[PROP_FUNCTION_NAME] = g_param_spec_string(
        "function-name", "Function Name", "Name of the function to be used",
        NULL, G_PARAM_READWRITE);

    obj_properties[PROP_INFER_ID] =
        g_param_spec_uint("inference-id", "inference id", "set inference id", 0,
                          1000, 0, G_PARAM_READWRITE);

    obj_properties[PROP_SECONDARY_MODE] = g_param_spec_boolean(
        "secondary-mode", "secondary mode", "is secondary inference mode",
        FALSE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(
        element_class, "DXPostprocess", "Generic",
        "Postprocesses inference results", "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxpostprocess_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxpostprocess_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxpostprocess_transform_ip);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxpostprocess_change_state;
}

static void gst_dxpostprocess_init(GstDxPostprocess *self) {
    self->_config_file_path = NULL;
    self->_library_file_path = NULL;
    self->_function_name = NULL;
    self->_library_handle = NULL;
    self->_postproc_function = NULL;

    self->_acc_fps = 0;
    self->_frame_count_for_fps = 0;
}

static gboolean gst_dxpostprocess_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    return TRUE;
}

static gboolean gst_dxpostprocess_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

static GstFlowReturn gst_dxpostprocess_transform_ip(GstBaseTransform *trans,
                                                    GstBuffer *buf) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(trans);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        g_error("No DXFrameMeta in GstBuffer !! \n");
    }

    bool processed = false;
    auto start = std::chrono::high_resolution_clock::now();
    if (self->_secondary_mode) {
        int objects_size = g_list_length(frame_meta->_object_meta_list);
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);
            auto iter = object_meta->_output_tensor.find(self->_infer_id);
            if (iter != object_meta->_output_tensor.end()) {
                self->_postproc_function(iter->second, frame_meta, object_meta);
                processed = true;
            }
        }
    } else {
        auto iter = frame_meta->_output_tensor.find(self->_infer_id);
        if (iter != frame_meta->_output_tensor.end()) {
            self->_postproc_function(iter->second, frame_meta, nullptr);
            processed = true;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    if (processed) {
        auto frameDuration =
            std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double frameTimeSec = frameDuration.count() / 1000000.0;
        self->_acc_fps += 1.0 / frameTimeSec;
        self->_frame_count_for_fps++;

        if (self->_frame_count_for_fps % 100 == 0 &&
            self->_frame_count_for_fps != 0) {
            gchar *name = NULL;
            g_object_get(G_OBJECT(self), "name", &name, NULL);
            g_print("[%s]\tFPS : %f \n", name,
                    self->_acc_fps / self->_frame_count_for_fps);
            self->_acc_fps = 0;
            self->_frame_count_for_fps = 0;
            g_free(name);
        }
    }

    return GST_FLOW_OK;
}