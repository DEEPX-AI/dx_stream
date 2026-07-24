#include "gst-dxpostprocess.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils.hpp"
#include "dx_dlfcn.h"
#include <json-glib/json-glib.h>

enum class PropertyID {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_LIBRARY_FILE_PATH,
    PROP_FUNCTION_NAME,
    PROP_INFER_ID,
    PROP_SECONDARY_MODE,
    N_PROPERTIES
};

GST_DEBUG_CATEGORY_STATIC(gst_dxpostprocess_debug_category);
#define GST_CAT_DEFAULT gst_dxpostprocess_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; video/x-raw"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; video/x-raw"));

static GstFlowReturn gst_dxpostprocess_transform_ip(GstBaseTransform *trans,
                                                    GstBuffer *buf);
static gboolean gst_dxpostprocess_start(GstBaseTransform *trans);
static gboolean gst_dxpostprocess_stop(GstBaseTransform *trans);
static gboolean gst_dxpostprocess_sink_event(GstBaseTransform *trans,
                                             GstEvent *event);
static gboolean gst_dxpostprocess_propose_allocation(GstBaseTransform *trans,
                                                     GstQuery *decide_query,
                                                     GstQuery *query);
static gboolean gst_dxpostprocess_query(GstBaseTransform *trans,
                                        GstPadDirection direction,
                                        GstQuery *query);

G_DEFINE_TYPE(GstDxPostprocess, gst_dxpostprocess, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static gboolean string_is_empty(const gchar *value) {
    return value == nullptr || value[0] == '\0';
}

static void parse_config(GstDxPostprocess *self) {
    if (string_is_empty(self->_config_file_path)) {
        return;
    }

    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        GST_ERROR_OBJECT(self, "Config file does not exist: %s", self->_config_file_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading postprocess config file: %s", self->_config_file_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        GST_ERROR_OBJECT(self, "[dxpostprocess] Failed to load config file: %s",
                         error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    JsonObject *object = json_node_get_object(node);

    auto set_string_property = [&](const char *json_key, const char *gobj_key) {
        if (json_object_has_member(object, json_key)) {
            const gchar *val = json_object_get_string_member(object, json_key);
            g_object_set(self, gobj_key, val, nullptr);
        }
    };

    set_string_property("library_file_path", "library-file-path");
    set_string_property("function_name", "function-name");

    if (json_object_has_member(object, "inference_id")) {
        gint64 val = json_object_get_int_member(object, "inference_id");
        if (val < 0) {
            GST_ERROR_OBJECT(self, "[dxpostprocess] Member inference_id has a negative value "
                             "(%ld), ignoring.", val);
        } else {
            self->_infer_id = static_cast<guint>(val);
        }
    }

    if (json_object_has_member(object, "secondary_mode")) {
        self->_secondary_mode =
            json_object_get_boolean_member(object, "secondary_mode");
    }

    g_object_unref(parser);
}

static void dxpostprocess_set_property(GObject *object, guint property_id,
                                       const GValue *value, GParamSpec *pspec) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(object);

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        if (nullptr != self->_config_file_path)
            g_free(self->_config_file_path);
        self->_config_file_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;

    case PropertyID::PROP_LIBRARY_FILE_PATH:
        if (self->_library_file_path) {
            g_free(self->_library_file_path);
        }
        self->_library_file_path = g_value_dup_string(value);
        break;

    case PropertyID::PROP_FUNCTION_NAME:
        if (self->_function_name) {
            g_free(self->_function_name);
        }
        self->_function_name = g_value_dup_string(value);
        break;

    case PropertyID::PROP_SECONDARY_MODE:
        self->_secondary_mode = g_value_get_boolean(value);
        break;

    case PropertyID::PROP_INFER_ID: {
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
    const GstDxPostprocess *self = GST_DXPOSTPROCESS(object);

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;

    case PropertyID::PROP_LIBRARY_FILE_PATH:
        g_value_set_string(value, self->_library_file_path);
        break;

    case PropertyID::PROP_FUNCTION_NAME:
        g_value_set_string(value, self->_function_name);
        break;

    case PropertyID::PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_secondary_mode);
        break;

    case PropertyID::PROP_INFER_ID:
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
    GST_DEBUG_OBJECT(self, "State change: %s",
                     gst_state_change_get_name(transition));

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY: {
        const gboolean has_library_path = !string_is_empty(self->_library_file_path);
        const gboolean has_function_name = !string_is_empty(self->_function_name);

        if (has_library_path != has_function_name) {
            GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                              ("[dxpostprocess] library-file-path and function-name must be set together."),
                              (NULL));
            return GST_STATE_CHANGE_FAILURE;
        }

        if (!self->_library_handle && has_library_path && has_function_name) {
            self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
            if (!self->_library_handle) {
                GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                                  ("Error opening library '%s': %s",
                                   self->_library_file_path, dlerror()),
                                  (NULL));
                return GST_STATE_CHANGE_FAILURE;
            }
            void *func_ptr = dlsym(self->_library_handle, self->_function_name);
            if (!func_ptr) {
                GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                                  ("Function '%s' not found in '%s': %s",
                                   self->_function_name,
                                   self->_library_file_path, dlerror()),
                                  ("Hint: Run 'nm -D %s | grep %s' to list available functions.",
                                   self->_library_file_path,
                                   self->_function_name));
                dlclose(self->_library_handle);
                self->_library_handle = nullptr;
                return GST_STATE_CHANGE_FAILURE;
            }
            self->_postproc_function =
                (void (*)(GstBuffer *, std::vector<dxs::DXTensor>, DXFrameMeta *,
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
        if (self->_library_handle) {
            dlclose(self->_library_handle);
            self->_library_handle = nullptr;
            self->_postproc_function = nullptr;
        }
        break;
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_DEBUG_OBJECT(self, "State change completed: %s",
                     gst_element_state_change_return_get_name(result));
    return result;
}

static void dxpostprocess_dispose(GObject *object) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(object);
    if (self->_config_file_path) {
        g_free(self->_config_file_path);
        self->_config_file_path = nullptr;
    }
    if (self->_library_file_path) {
        g_free(self->_library_file_path);
        self->_library_file_path = nullptr;
    }
    if (self->_function_name) {
        g_free(self->_function_name);
        self->_function_name = nullptr;
    }
    if (self->_library_handle) {
        dlclose(self->_library_handle);
        self->_library_handle = nullptr;
    }
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxpostprocess_class_init(GstDxPostprocessClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxpostprocess_debug_category, "dxpostprocess",
                            0, "DXPostprocess plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxpostprocess_set_property;
    gobject_class->get_property = dxpostprocess_get_property;
    gobject_class->dispose = dxpostprocess_dispose;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<int>(PropertyID::PROP_CONFIG_FILE_PATH)] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the configuration file", nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<int>(PropertyID::PROP_LIBRARY_FILE_PATH)] = g_param_spec_string(
        "library-file-path", "Library File Path",
        "Path to the shared library file", nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<int>(PropertyID::PROP_FUNCTION_NAME)] = g_param_spec_string(
        "function-name", "Function Name", "Name of the function to be used",
        nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<int>(PropertyID::PROP_INFER_ID)] =
        g_param_spec_uint("inference-id", "inference id", "set inference id", 0,
                          1000, 0, G_PARAM_READWRITE);

    obj_properties[static_cast<int>(PropertyID::PROP_SECONDARY_MODE)] = g_param_spec_boolean(
        "secondary-mode", "secondary mode", "is secondary inference mode",
        FALSE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<int>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(
        element_class, "DXPostprocess", "Generic",
        "Postprocesses inference results", "Sangil Jo <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxpostprocess_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxpostprocess_stop);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxpostprocess_sink_event);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxpostprocess_transform_ip);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxpostprocess_propose_allocation);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxpostprocess_query);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxpostprocess_change_state;
}

static void gst_dxpostprocess_init(GstDxPostprocess *self) {
    self->_config_file_path = nullptr;
    self->_library_file_path = nullptr;
    self->_function_name = nullptr;
    self->_library_handle = nullptr;
    self->_postproc_function = nullptr;

    self->_acc_fps = 0;
    self->_frame_count_for_fps = 0;
}

static gboolean gst_dxpostprocess_start(GstBaseTransform *trans) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(trans);
    GST_INFO_OBJECT(self, "Postprocessor starting (secondary_mode=%d)", self->_secondary_mode);
    return TRUE;
}

static gboolean gst_dxpostprocess_stop(GstBaseTransform *trans) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(trans);
    GST_INFO_OBJECT(self, "Postprocessor stopping");
    return TRUE;
}

static gboolean gst_dxpostprocess_sink_event(GstBaseTransform *trans,
                                             GstEvent *event) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(trans);

    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        GST_DEBUG_OBJECT(self, "Received EOS event");
    }

    return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
}

static gboolean process_secondary_mode(GstBuffer *buf,
                                       DXFrameMeta *frame_meta,
                                       const GstDxPostprocess *self) {
    gboolean had_error = FALSE;
    size_t objects_size = frame_meta->_object_meta_list.size();
    for (size_t o = 0; o < objects_size; o++) {
        DXObjectMeta *object_meta = frame_meta->_object_meta_list[o];
        auto iter = object_meta->_output_tensors.find(self->_infer_id);
        if (iter == object_meta->_output_tensors.end())
            continue;

        if (iter->second._tensors.empty())
            continue;

        try {
            self->_postproc_function(buf, iter->second._tensors, frame_meta, object_meta);
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(self, "Postprocess function threw exception in secondary mode: %s", e.what());
            had_error = TRUE;
        } catch (...) {
            GST_ERROR_OBJECT(self, "Postprocess function threw unknown exception in secondary mode");
            had_error = TRUE;
        }
    }
    return !had_error;
}

static GstFlowReturn gst_dxpostprocess_transform_ip(GstBaseTransform *trans,
                                                    GstBuffer *buf) {
    GstDxPostprocess *self = GST_DXPOSTPROCESS(trans);
    GST_LOG_OBJECT(self, "DXPostprocess Transform IP called");

    if (G_UNLIKELY(!self->_postproc_function)) {
        GST_ELEMENT_ERROR(self, LIBRARY, INIT,
            ("Postprocess function not loaded"), (NULL));
        return GST_FLOW_ERROR;
    }

    auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No DXFrameMeta, passing through");
        return GST_FLOW_OK;
    }

    if (self->_secondary_mode) {
        GST_LOG_OBJECT(self, "Processing in secondary mode");
        if (!process_secondary_mode(buf, frame_meta, self)) {
            GST_ELEMENT_ERROR(self, LIBRARY, FAILED,
                              ("Postprocess function '%s' failed in secondary mode",
                               self->_function_name),
                              (NULL));
            return GST_FLOW_ERROR;
        }
    } else {
        GST_LOG_OBJECT(self, "Processing in primary mode");
        auto iter = frame_meta->_output_tensors.find(self->_infer_id);
        if (iter != frame_meta->_output_tensors.end()) {
            try {
                self->_postproc_function(buf, iter->second._tensors, frame_meta, nullptr);
            } catch (const std::exception &e) {
                GST_ELEMENT_ERROR(self, LIBRARY, FAILED,
                                  ("Postprocess function '%s' threw exception: %s",
                                   self->_function_name, e.what()),
                                  (NULL));
                return GST_FLOW_ERROR;
            } catch (...) {
                GST_ELEMENT_ERROR(self, LIBRARY, FAILED,
                                  ("Postprocess function '%s' threw unknown exception",
                                   self->_function_name),
                                  (NULL));
                return GST_FLOW_ERROR;
            }
        }
    }

    return GST_FLOW_OK;
}

static gboolean gst_dxpostprocess_propose_allocation(GstBaseTransform *trans,
                                                     GstQuery *decide_query,
                                                     GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}

static gboolean gst_dxpostprocess_query(GstBaseTransform *trans,
                                        GstPadDirection direction,
                                        GstQuery *query) {
    if (direction == GST_PAD_SRC && GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        if (!GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query))
            return FALSE;
        gboolean live;
        GstClockTime min_lat, max_lat;
        gst_query_parse_latency(query, &live, &min_lat, &max_lat);
        const GstClockTime self_lat = 1 * GST_USECOND;
        min_lat += self_lat;
        if (max_lat != GST_CLOCK_TIME_NONE)
            max_lat += self_lat;
        gst_query_set_latency(query, live, min_lat, max_lat);
        return TRUE;
    }
    return GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query);
}