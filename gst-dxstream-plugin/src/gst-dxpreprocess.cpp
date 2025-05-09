#include "gst-dxpreprocess.hpp"
#include <chrono>
#include <dlfcn.h>
#include <gst/video/video.h>
#include <iostream>
#include <json-glib/json-glib.h>
#include <libyuv.h>

enum {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_LIBRARY_FILE_PATH,
    PROP_FUNCTION_NAME,
    PROP_COLOR_FORMAT,
    PROP_PREPROCESS_ID,
    PROP_RESIZE_WIDTH,
    PROP_RESIZE_HEIGHT,
    PROP_KEEP_RATIO,
    PROP_PAD_VALUE,
    PROP_SECONDARY_MODE,
    PROP_TARGET_CLASS_ID,
    PROP_MIN_OBJECT_WIDTH,
    PROP_MIN_OBJECT_HEIGHT,
    PROP_INTERVAL,
    PROP_ROI,
    PROP_POOL_SIZE,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxpreprocess_debug_category);
#define GST_CAT_DEFAULT gst_dxpreprocess_debug_category

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf);
static gboolean gst_dxpreprocess_start(GstBaseTransform *trans);
static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans);
static gboolean gst_dxpreprocess_src_event(GstBaseTransform *trans,
                                           GstEvent *event);

G_DEFINE_TYPE_WITH_CODE(
    GstDxPreprocess, gst_dxpreprocess, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxpreprocess_debug_category, "gst-dxpreprocess",
                            0, "debug category for gst-dxpreprocess element"))

static GstElementClass *parent_class = NULL;

static void parse_config(GstDxPreprocess *self) {
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
            if (json_object_has_member(object, "preprocess_id")) {
                gint int_value =
                    json_object_get_int_member(object, "preprocess_id");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member preprocess_id has a "
                            "negative value (%d) "
                            "and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_preprocess_id = (guint)int_value;
            }
            if (json_object_has_member(object, "color_format")) {
                const gchar *color_format =
                    json_object_get_string_member(object, "color_format");
                if (g_strcmp0(color_format, "RGB") == 0 ||
                    g_strcmp0(color_format, "BGR") == 0) {
                    g_free(self->_color_format);
                    self->_color_format = g_strdup(color_format);
                } else {
                    g_warning("Invalid color mode: %s. Use RGB or BGR",
                              color_format);
                }
            }
            if (json_object_has_member(object, "resize_width")) {
                gint int_value =
                    json_object_get_int_member(object, "resize_width");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member resize_width has a negative "
                            "value (%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_resize_width = (guint)int_value;
            }
            if (json_object_has_member(object, "resize_height")) {
                gint int_value =
                    json_object_get_int_member(object, "resize_height");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member resize_height has a "
                            "negative value (%d) "
                            "and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_resize_height = (guint)int_value;
            }
            if (json_object_has_member(object, "pad_value")) {
                gint int_value =
                    json_object_get_int_member(object, "pad_value");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member pad_value has a negative "
                            "value (%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_pad_value = (guint)int_value;
            }
            if (json_object_has_member(object, "target_class_id")) {
                self->_target_class_id =
                    json_object_get_int_member(object, "target_class_id");
            }
            if (json_object_has_member(object, "min_object_width")) {
                gint int_value =
                    json_object_get_int_member(object, "min_object_width");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member min_object_width has a "
                            "negative value (%d) "
                            "and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_min_object_width = (guint)int_value;
            }
            if (json_object_has_member(object, "min_object_height")) {
                gint int_value =
                    json_object_get_int_member(object, "min_object_height");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member min_object_height has a "
                            "negative value "
                            "(%d) and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_min_object_height = (guint)int_value;
            }
            if (json_object_has_member(object, "interval")) {
                gint int_value = json_object_get_int_member(object, "interval");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member interval has a negative "
                            "value (%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_interval = (guint)int_value;
            }
            if (json_object_has_member(object, "keep_ratio")) {
                self->_keep_ratio =
                    json_object_get_boolean_member(object, "keep_ratio");
            }
            if (json_object_has_member(object, "secondary_mode")) {
                self->_secondary_mode =
                    json_object_get_boolean_member(object, "secondary_mode");
            }
            if (json_object_has_member(object, "pool_size")) {
                gint int_value =
                    json_object_get_int_member(object, "pool_size");
                if (int_value < 0) {
                    g_error("[dxpreprocess] Member pool_size has a negative "
                            "value (%d) and "
                            "cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_pool_size = (guint)int_value;
            }
            if (json_object_has_member(object, "roi")) {
                JsonArray *roi_array =
                    json_object_get_array_member(object, "roi");
                if (!roi_array) {
                    g_printerr("Error: 'roi' must be a JSON array.\n");
                    return;
                }
                guint length = json_array_get_length(roi_array);
                if (length != 4) {
                    g_printerr("Error: ROI must have exactly 4 integer values "
                               "(received %u values).\n",
                               length);
                    return;
                }
                gint temp_roi[4] = {-1, -1, -1, -1};
                for (guint i = 0; i < length; i++) {
                    JsonNode *node = json_array_get_element(roi_array, i);
                    if (!JSON_NODE_HOLDS_VALUE(node)) {
                        g_printerr(
                            "Error: ROI array contains a non-value node.\n");
                        return;
                    }
                    GType value_type = json_node_get_value_type(node);
                    if (value_type != G_TYPE_INT) {
                        g_printerr("Error: ROI array must contain only integer "
                                   "values.\n");
                        return;
                    }
                    temp_roi[i] = json_node_get_int(node);
                }
                for (guint i = 0; i < 4; i++) {
                    self->_roi[i] = temp_roi[i];
                }
            }

            g_object_unref(parser);
        }
    } else {
        g_print("Config file does not exist: %s\n", self->_config_file_path);
    }
}

static void dxpreprocess_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);

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

    case PROP_PREPROCESS_ID:
        self->_preprocess_id = g_value_get_uint(value);
        break;

    case PROP_COLOR_FORMAT: {
        guint color_value = g_value_get_uint(value);
        if (color_value == 0) {
            self->_color_format = g_strdup("RGB");
        } else if (color_value == 1) {
            self->_color_format = g_strdup("BGR");
        } else {
            g_warning("Invalid color mode: %d. Use RGB or BGR.", color_value);
        }
        break;
    }
    case PROP_RESIZE_WIDTH:
        self->_resize_width = g_value_get_uint(value);
        break;

    case PROP_RESIZE_HEIGHT:
        self->_resize_height = g_value_get_uint(value);
        break;

    case PROP_KEEP_RATIO:
        self->_keep_ratio = g_value_get_boolean(value);
        break;

    case PROP_PAD_VALUE:
        self->_pad_value = g_value_get_uint(value);
        break;

    case PROP_SECONDARY_MODE:
        self->_secondary_mode = g_value_get_boolean(value);
        break;

    case PROP_TARGET_CLASS_ID:
        self->_target_class_id = g_value_get_int(value);
        break;

    case PROP_MIN_OBJECT_WIDTH:
        self->_min_object_width = g_value_get_uint(value);
        break;

    case PROP_MIN_OBJECT_HEIGHT:
        self->_min_object_height = g_value_get_uint(value);
        break;

    case PROP_INTERVAL:
        self->_interval = g_value_get_uint(value);
        break;

    case PROP_ROI: {
        if (G_VALUE_HOLDS_STRING(value)) {
            const gchar *roi_str = g_value_get_string(value);
            gint roi_values[4];
            int count = sscanf(roi_str, "%d,%d,%d,%d", &roi_values[0],
                               &roi_values[1], &roi_values[2], &roi_values[3]);

            if (count != 4) {
                g_error("Invalid ROI format. Expected format: "
                        "roi=\"x1,y1,x2,y2\"\n");
                return;
            }

            for (size_t i = 0; i < 4; i++) {
                self->_roi[i] = roi_values[i];
            }
        }
        break;
    }

    case PROP_POOL_SIZE:
        self->_pool_size = g_value_get_uint(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxpreprocess_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);

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

    case PROP_PREPROCESS_ID:
        g_value_set_uint(value, self->_preprocess_id);
        break;

    case PROP_COLOR_FORMAT:
        if (g_strcmp0(self->_color_format, "RGB") == 0) {
            g_value_set_uint(value, 0);
        } else if (g_strcmp0(self->_color_format, "BGR") == 0) {
            g_value_set_uint(value, 1);
        } else {
            g_warning("Invalid color mode: %s. Use RGB or BGR.",
                      self->_color_format);
        }
        break;

    case PROP_RESIZE_WIDTH:
        g_value_set_uint(value, self->_resize_width);
        break;

    case PROP_RESIZE_HEIGHT:
        g_value_set_uint(value, self->_resize_height);
        break;

    case PROP_KEEP_RATIO:
        g_value_set_boolean(value, self->_keep_ratio);
        break;

    case PROP_PAD_VALUE:
        g_value_set_uint(value, self->_pad_value);
        break;

    case PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_secondary_mode);
        break;

    case PROP_TARGET_CLASS_ID:
        g_value_set_int(value, self->_target_class_id);
        break;

    case PROP_MIN_OBJECT_WIDTH:
        g_value_set_uint(value, self->_min_object_width);
        break;

    case PROP_MIN_OBJECT_HEIGHT:
        g_value_set_uint(value, self->_min_object_height);
        break;

    case PROP_INTERVAL:
        g_value_set_uint(value, self->_interval);
        break;

    case PROP_POOL_SIZE:
        g_value_set_uint(value, self->_pool_size);
        break;

    case PROP_ROI: {
        gchar roi_str[50];
        g_snprintf(roi_str, sizeof(roi_str), "%d,%d,%d,%d", self->_roi[0],
                   self->_roi[1], self->_roi[2], self->_roi[3]);

        g_value_set_string(value, roi_str);
        break;
    }

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static GstStateChangeReturn
dxpreprocess_change_state(GstElement *element, GstStateChange transition) {
    GstDxPreprocess *self = GST_DXPREPROCESS(element);
    GST_INFO_OBJECT(self, "Attempting to change state");
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxpreprocess_dispose(GObject *object) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);
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
    g_free(self->_color_format);

    self->_track_cnt.clear();
    self->_pool.deinitialize();

    if (self->_temp_output_buffer) {
        free(self->_temp_output_buffer);
        self->_temp_output_buffer = nullptr;
    }
#ifdef HAVE_LIBRGA
#else
    for (std::map<int, uint8_t *>::iterator it = self->_crop_frame.begin();
         it != self->_crop_frame.end(); ++it) {
        if (it->second != nullptr) {
            free(it->second);
        }
    }
    self->_crop_frame.clear();

    for (std::map<int, uint8_t *>::iterator it = self->_convert_frame.begin();
         it != self->_convert_frame.end(); ++it) {
        if (it->second != nullptr) {
            free(it->second);
        }
    }
    self->_convert_frame.clear();

    for (std::map<int, uint8_t *>::iterator it = self->_resized_frame.begin();
         it != self->_resized_frame.end(); ++it) {
        if (it->second != nullptr) {
            free(it->second);
        }
    }
    self->_resized_frame.clear();
#endif

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxpreprocess_finalize(GObject *object) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);

    self->_pool.deinitialize();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstCaps *gst_dxpreprocess_transform_caps(GstBaseTransform *trans,
                                                GstPadDirection direction,
                                                GstCaps *caps,
                                                GstCaps *filter) {
    GstCaps *new_caps = gst_caps_copy(caps);

    if (filter) {
        GstCaps *filtered_caps = gst_caps_intersect(new_caps, filter);
        gst_caps_unref(new_caps);
        new_caps = filtered_caps;
    }
    return new_caps;
}

static gboolean gst_dxpreprocess_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps, GstCaps *outcaps) {
    const GstStructure *structure = gst_caps_get_structure(incaps, 0);
    const gchar *format = gst_structure_get_string(structure, "format");

    if (!format) {
        g_warning("No format found in sink caps!");
        return FALSE;
    }
    return TRUE;
}

static void gst_dxpreprocess_class_init(GstDxPreprocessClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxpreprocess_debug_category, "dxpreprocess", 0,
                            "DXPreprocess plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxpreprocess_set_property;
    gobject_class->get_property = dxpreprocess_get_property;
    gobject_class->dispose = dxpreprocess_dispose;
    gobject_class->finalize = dxpreprocess_finalize;

    obj_properties[PROP_CONFIG_FILE_PATH] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the JSON config file containing the element's properties.",
        NULL, G_PARAM_READWRITE);

    obj_properties[PROP_LIBRARY_FILE_PATH] =
        g_param_spec_string("library-file-path", "Library File Path",
                            "Path to the custom preprocess library, if used",
                            NULL, G_PARAM_READWRITE);

    obj_properties[PROP_FUNCTION_NAME] = g_param_spec_string(
        "function-name", "Function Name",
        "Name of the custom preprocessing function to use. ", NULL,
        G_PARAM_READWRITE);

    obj_properties[PROP_PREPROCESS_ID] =
        g_param_spec_uint("preprocess-id", "Preprocess ID",
                          "Assigns an ID to the preprocessed input", 0, 10000,
                          0, G_PARAM_READWRITE);

    obj_properties[PROP_COLOR_FORMAT] =
        g_param_spec_uint("color-format", "Color Format",
                          "Specifies the color format for preprocessing. "
                          "[0: RGB, 1: BGR]",
                          0, 1, 0, G_PARAM_READWRITE);

    obj_properties[PROP_RESIZE_WIDTH] = g_param_spec_uint(
        "resize-width", "Resize Width", "Specifies the width for resizing.", 0,
        10000, 0, G_PARAM_READWRITE);

    obj_properties[PROP_RESIZE_HEIGHT] = g_param_spec_uint(
        "resize-height", "Resize Height", "Specifies the width for resizing.",
        0, 10000, 0, G_PARAM_READWRITE);

    obj_properties[PROP_KEEP_RATIO] = g_param_spec_boolean(
        "keep-ratio", "Keep Original Ratio",
        "Maintains the original aspect ratio during resizing", TRUE,
        G_PARAM_READWRITE);

    obj_properties[PROP_PAD_VALUE] = g_param_spec_uint(
        "pad-value", "PadValue", "Padding color value for R, G, B", 0, 255, 0,
        G_PARAM_READWRITE);

    obj_properties[PROP_SECONDARY_MODE] = g_param_spec_boolean(
        "secondary-mode", "Is Secondary Mode",
        "Enables Secondary Mode for processing object regions.", FALSE,
        G_PARAM_READWRITE);

    obj_properties[PROP_TARGET_CLASS_ID] =
        g_param_spec_int("target-class-id", "Target Class ID",
                         "Filters objects in Secondary Mode by class ID. ( -1 "
                         "processes all objects).",
                         -1, 10000, -1, G_PARAM_READWRITE);

    obj_properties[PROP_MIN_OBJECT_WIDTH] = g_param_spec_uint(
        "min-object-width", "Min Object Box Width",
        "Minimum object width for preprocessing in Secondary Mode", 0, 10000, 0,
        G_PARAM_READWRITE);

    obj_properties[PROP_MIN_OBJECT_HEIGHT] = g_param_spec_uint(
        "min-object-height", "Min Object Box Height",
        "Minimum object height for preprocessing in Secondary Mode", 0, 10000,
        0, G_PARAM_READWRITE);

    obj_properties[PROP_INTERVAL] = g_param_spec_uint(
        "interval", "Inference Interval",
        "Specifies the interval for preprocessing frames or objects.", 0, 10000,
        0, G_PARAM_READWRITE);

    obj_properties[PROP_POOL_SIZE] = g_param_spec_uint(
        "pool-size", "Pool Size for Input Tensors",
        "Number of preallocated memory blocks for input tensors.", 0, 1000, 0,
        G_PARAM_READWRITE);

    obj_properties[PROP_ROI] = g_param_spec_string(
        "roi", "Region of Interest",
        "Defines the ROI as a comma-separated string (x1,y1,x2,y2)",
        "-1,-1,-1,-1", G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new(
            "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
            gst_caps_from_string(
                "video/x-raw, format=(string){ RGB, I420, NV12 }")));

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new(
            "src", GST_PAD_SRC, GST_PAD_ALWAYS,
            gst_caps_from_string(
                "video/x-raw, format=(string){ RGB, I420, NV12 }")));

    gst_element_class_set_static_metadata(
        element_class, "DXPreprocess", "Generic", "Preprocesses network input",
        "Jo Sangil <sijo@deepx.ai>");

    base_transform_class->src_event =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_src_event);

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxpreprocess_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxpreprocess_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_transform_ip);
    base_transform_class->transform_caps =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_transform_caps);
    base_transform_class->set_caps =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_set_caps);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxpreprocess_change_state;
}

static void gst_dxpreprocess_init(GstDxPreprocess *self) {
    self->_config_file_path = NULL;
    self->_preprocess_id = 0;
    self->_color_format = g_strdup("RGB");
    self->_resize_width = 0;
    self->_resize_height = 0;
    self->_input_channel = 3;
    self->_keep_ratio = TRUE;
    self->_pad_value = 0;
    self->_secondary_mode = FALSE;
    self->_target_class_id = -1;
    self->_min_object_width = 0;
    self->_min_object_height = 0;
    self->_interval = 0;
    self->_cnt.clear();

    self->_acc_fps = 0;
    self->_frame_count_for_fps = 0;

    self->_roi[0] = -1;
    self->_roi[1] = -1;
    self->_roi[2] = -1;
    self->_roi[3] = -1;
    self->_track_cnt.clear();

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;
    self->_throttling_delay = 0;

    self->_temp_output_buffer = nullptr;

#ifdef HAVE_LIBRGA
#else
    self->_crop_frame = std::map<int, uint8_t *>();
    self->_convert_frame = std::map<int, uint8_t *>();
    self->_resized_frame = std::map<int, uint8_t *>();
#endif
    self->_pool_size = 1;
}

static gboolean gst_dxpreprocess_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
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

        self->_process_function =
            (bool (*)(DXFrameMeta *, DXObjectMeta *, void *))func_ptr;
    }

    self->_align_factor =
        (64 - ((self->_resize_width * self->_input_channel) % 64)) % 64;
    self->_pool.deinitialize();
    if (self->_resize_height > 0 && self->_resize_width > 0) {
        self->_pool.initialize(
            self->_resize_height * (self->_resize_width * self->_input_channel +
                                    self->_align_factor),
            self->_pool_size, static_cast<uint8_t>(self->_pad_value));
    } else {
        g_error("Invalid input size %d x %d", self->_resize_width,
                self->_resize_height);
        return FALSE;
    }

    if (self->_align_factor != 0) {
        self->_temp_output_buffer =
            malloc(self->_resize_height * self->_resize_width * 3);
        memset(self->_temp_output_buffer,
               static_cast<uint8_t>(self->_pad_value),
               self->_resize_height * self->_resize_width * 3);
    } else {
        self->_temp_output_buffer = nullptr;
    }

    return TRUE;
}

static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

#ifdef HAVE_LIBRGA
bool calculate_nv12_strides_short(int w, int h, int wa, int ha, int *ws,
                                  int *hs) {
    if (!ws || !hs || w <= 0 || h <= 0 || (h % 2 != 0) || wa < 0 || ha < 0) {
        return false;
    }
    *ws = (wa <= 1) ? w : (((w + wa - 1) / wa) * wa);
    *hs = (ha <= 1) ? h : (((h + ha - 1) / ha) * ha);

    return true;
}

bool preprocess(GstDxPreprocess *self, DXFrameMeta *frame_meta, void *output,
                cv::Rect *roi) {
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    if (!meta) {
        g_error("ERROR : video meta is nullptr! \n");
        return false;
    }

    if (self->_resize_width % 16 != 0 || self->_resize_height % 2 != 0) {
        g_error("ERROR : output W stride must be 16 (H stride 2) aligned ! \n");
        return true;
    }

    if (!output) {
        g_error("ERROR : output memory is nullptr! \n");
        return false;
    }

    if (g_strcmp0(frame_meta->_format, "NV12") != 0) {
        g_error("ERROR : not supported format (use NV12)! \n");
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_READ)) {
        g_error("ERROR : Failed to map GstBuffer (dxpreprocess) \n");
        return false;
    }
    int wstride, hstride;
    calculate_nv12_strides_short(frame_meta->_width, frame_meta->_height, 16,
                                 16, &wstride, &hstride);
    rga_buffer_t src_img = wrapbuffer_virtualaddr(
        reinterpret_cast<void *>(map.data), frame_meta->_width,
        frame_meta->_height, RK_FORMAT_YCbCr_420_SP, meta->stride[0], hstride);
    rga_buffer_t dst_img;
    if (g_strcmp0(self->_color_format, "RGB") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), self->_resize_width,
            self->_resize_height, RK_FORMAT_RGB_888);
    } else if (g_strcmp0(self->_color_format, "BGR") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), self->_resize_width,
            self->_resize_height, RK_FORMAT_BGR_888);
    } else {
        g_warning("Invalid color mode: %s. Use RGB or BGR.",
                  self->_color_format);
        return false;
    }

    int width = frame_meta->_width;
    int height = frame_meta->_height;

    im_rect src_rect, dst_rect;
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = frame_meta->_width;
    src_rect.height = frame_meta->_height;

    if (roi->width != 0 && roi->height != 0) {
        src_rect.x = std::max(roi->x % 2 == 0 ? roi->x : roi->x + 1, 0);
        src_rect.y = std::max(roi->y % 2 == 0 ? roi->y : roi->y + 1, 0);
        src_rect.width =
            std::max(roi->width % 2 == 0 ? roi->width : roi->width + 1, 0);
        if (src_rect.width + src_rect.x > frame_meta->_width) {
            src_rect.width = frame_meta->_width - src_rect.x;
        }
        src_rect.height =
            std::max(roi->height % 2 == 0 ? roi->height : roi->height + 1, 0);
        if (src_rect.height + src_rect.y > frame_meta->_height) {
            src_rect.height = frame_meta->_height - src_rect.y;
        }
        width = src_rect.width;
        height = src_rect.height;
    }

    if (self->_keep_ratio) {
        float dw, dh;
        uint16_t top, left;
        float ratioDest = (float)self->_resize_width / self->_resize_height;
        float ratioSrc = (float)width / height;
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = self->_resize_height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = self->_resize_width;
            newHeight = newWidth / ratioSrc;
        }

        dw = (self->_resize_width - newWidth) / 2.0;
        dh = (self->_resize_height - newHeight) / 2.0;

        top = (uint16_t)round(dh - 0.1);
        left = (uint16_t)round(dw - 0.1);

        dst_rect.x = left;
        dst_rect.y = top;
        dst_rect.width = newWidth;
        dst_rect.height = newHeight;
    } else {
        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = self->_resize_width;
        dst_rect.height = self->_resize_height;
    }

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        std::cerr << "check error: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
        return false;
    }

    if ((float)dst_rect.width / src_rect.width <= 0.125 ||
        (float)dst_rect.width / src_rect.width >= 8 ||
        (float)dst_rect.height / src_rect.height <= 0.125 ||
        (float)dst_rect.height / src_rect.height >= 8) {
        g_warning("DX Preprocess : scale check error, scale limit[1/8 ~ 8] \n");
        return false;
    }

    if (src_rect.width < 68 || src_rect.height < 2 || src_rect.width > 8176 ||
        src_rect.height > 8176) {
        g_warning("DX Preprocess : resolution check error, input range[68x2 ~ "
                  "8176x8176] \n");
        return false;
    }

    if (dst_rect.width < 68 || dst_rect.height < 2 || dst_rect.width > 8128 ||
        dst_rect.height > 8128) {
        g_warning("DX Preprocess : resolution check error, output range[68x2 ~ "
                  "8128x8128] \n");
        return false;
    }

    ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);

    gst_buffer_unmap(frame_meta->_buf, &map);
    if (ret != IM_STATUS_SUCCESS) {
        std::cerr << "RGA resize (imresize) failed: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
        return false;
    }
    return true;
}
#else
bool preprocess(GstDxPreprocess *self, DXFrameMeta *frame_meta, void *output,
                cv::Rect *roi) {
    int width = frame_meta->_width;
    int height = frame_meta->_height;

    if (self->_secondary_mode) {
        if (self->_crop_frame[frame_meta->_stream_id]) {
            free(self->_crop_frame[frame_meta->_stream_id]);
            self->_crop_frame[frame_meta->_stream_id] = nullptr;
        }
        if (self->_resized_frame[frame_meta->_stream_id]) {
            free(self->_resized_frame[frame_meta->_stream_id]);
            self->_resized_frame[frame_meta->_stream_id] = nullptr;
        }
        if (self->_convert_frame[frame_meta->_stream_id]) {
            free(self->_convert_frame[frame_meta->_stream_id]);
            self->_convert_frame[frame_meta->_stream_id] = nullptr;
        }
    }

    if (roi->width != 0 && roi->height != 0) {
        Crop(frame_meta->_buf, &self->_crop_frame[frame_meta->_stream_id],
             frame_meta->_width, frame_meta->_height, roi->x, roi->y,
             roi->width, roi->height, frame_meta->_format);
        width = roi->width;
        height = roi->height;
    }

    if (self->_keep_ratio) {
        float dw, dh;
        uint16_t top, bottom, left, right;
        float ratioDest = (float)self->_resize_width / self->_resize_height;
        float ratioSrc = (float)width / height;
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = self->_resize_height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = self->_resize_width;
            newHeight = newWidth / ratioSrc;
        }

        if (roi->width != 0 && roi->height != 0) {
            Resize(self->_crop_frame[frame_meta->_stream_id],
                   &self->_resized_frame[frame_meta->_stream_id], roi->width,
                   roi->height, newWidth, newHeight, frame_meta->_format);
        } else {
            Resize(frame_meta->_buf,
                   &self->_resized_frame[frame_meta->_stream_id],
                   frame_meta->_width, frame_meta->_height, newWidth, newHeight,
                   frame_meta->_format);
        }

        CvtColor(self->_resized_frame[frame_meta->_stream_id],
                 &self->_convert_frame[frame_meta->_stream_id], newWidth,
                 newHeight, frame_meta->_format, self->_color_format);
        cv::Mat temp = cv::Mat(newHeight, newWidth, CV_8UC3,
                               self->_convert_frame[frame_meta->_stream_id]);
        dw = (self->_resize_width - newWidth) / 2.0;
        dh = (self->_resize_height - newHeight) / 2.0;
        top = (uint16_t)round(dh - 0.1);
        bottom = (uint16_t)round(dh + 0.1);
        left = (uint16_t)round(dw - 0.1);
        right = (uint16_t)round(dw + 0.1);
        cv::Mat resizedFrame(self->_resize_height, self->_resize_width, CV_8UC3,
                             output);
        cv::copyMakeBorder(
            temp, resizedFrame, top, bottom, left, right, cv::BORDER_CONSTANT,
            cv::Scalar(self->_pad_value, self->_pad_value, self->_pad_value));
        temp.release();
    } else {
        if (roi->width != 0 && roi->height != 0) {
            Resize(self->_crop_frame[frame_meta->_stream_id],
                   &self->_resized_frame[frame_meta->_stream_id], width, height,
                   self->_resize_width, self->_resize_height,
                   frame_meta->_format);
        } else {
            Resize(frame_meta->_buf,
                   &self->_resized_frame[frame_meta->_stream_id], width, height,
                   self->_resize_width, self->_resize_height,
                   frame_meta->_format);
        }
        CvtColor(self->_resized_frame[frame_meta->_stream_id],
                 &self->_convert_frame[frame_meta->_stream_id],
                 self->_resize_width, self->_resize_height, frame_meta->_format,
                 self->_color_format);
        memcpy(output, self->_convert_frame[frame_meta->_stream_id],
               self->_resize_height * self->_resize_width * 3);
    }
    return true;
}
#endif

bool check_object_roi(float *box, int *roi) {
    if (int(box[0]) < roi[0])
        return false;
    if (int(box[1]) < roi[1])
        return false;
    if (int(box[2]) > roi[2])
        return false;
    if (int(box[3]) > roi[3])
        return false;
    return true;
}

static gboolean gst_dxpreprocess_src_event(GstBaseTransform *trans,
                                           GstEvent *event) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS: {

        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, NULL, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_OBJECT_LOCK(trans);
            self->_throttling_delay = diff;
            GST_OBJECT_UNLOCK(trans);
            // gst_event_unref(event);
            // return TRUE;
        }

        if (type == GST_QOS_TYPE_UNDERFLOW && diff > 0) {
            GST_OBJECT_LOCK(trans);
            self->_qos_timediff = diff;
            self->_qos_timestamp = timestamp;
            GST_OBJECT_UNLOCK(trans);
            // gst_event_unref(event);
        }
    }
        /* fall-through */
    default:
        break;
    }

    /** other events are handled in the default event handler */
    return GST_BASE_TRANSFORM_CLASS(parent_class)->src_event(trans, event);
}

bool check_object(GstDxPreprocess *self, DXFrameMeta *frame_meta,
                  DXObjectMeta *object_meta) {
    if (self->_target_class_id != -1 &&
        object_meta->_label != self->_target_class_id) {
        return false;
    }

    if (frame_meta->_roi[0] != -1 &&
        !check_object_roi(object_meta->_box, frame_meta->_roi)) {
        return false;
    }

    if (object_meta->_box[2] - object_meta->_box[0] < self->_min_object_width ||
        object_meta->_box[3] - object_meta->_box[1] <
            self->_min_object_height) {
        return false;
    }

    if (object_meta->_track_id != -1) {
        if (self->_track_cnt[frame_meta->_stream_id].count(
                object_meta->_track_id) > 0) {
            self->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] +=
                1;
        } else {
            self->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] =
                0;
        }

        if (self->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] <
            static_cast<int>(self->_interval)) {
            return false;
        }

        self->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] = 0;
    } else {
        // untracked
        if (self->_cnt[frame_meta->_stream_id] < self->_interval) {
            return false;
        }
    }
    return true;
}

void add_dummy_data(GstDxPreprocess *self, void *src, void *dst) {
    for (size_t y = 0; y < (size_t)self->_resize_height; y++) {
        memcpy(static_cast<uint8_t *>(dst) +
                   y * (static_cast<int>(self->_resize_width) *
                            static_cast<int>(self->_input_channel) +
                        self->_align_factor),
               static_cast<uint8_t *>(src) +
                   y * static_cast<int>(self->_resize_width) *
                       static_cast<int>(self->_input_channel),
               static_cast<int>(self->_resize_width) *
                   static_cast<int>(self->_input_channel));
    }
}

bool primary_process(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
    bool ret = true;
    if (self->_roi[0] != -1) {
        frame_meta->_roi[0] = std::max(self->_roi[0], 0);
        frame_meta->_roi[1] = std::max(self->_roi[1], 0);
        frame_meta->_roi[2] = std::min(self->_roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(self->_roi[3], frame_meta->_height - 1);
    }

    if (frame_meta->_input_memory_pool.find(self->_preprocess_id) !=
        frame_meta->_input_memory_pool.end()) {
        g_error("Preprocess ID %d already exists in the frame meta. check your "
                "pipeline",
                self->_preprocess_id);
        ret = false;
    }
    frame_meta->_input_memory_pool[self->_preprocess_id] =
        (MemoryPool *)&self->_pool;

    dxs::DXTensor tensor;
    tensor._data = self->_pool.allocate();
    tensor._type = dxs::DataType::UINT8;
    tensor._name = "input";
    tensor._shape.push_back(self->_resize_height);
    tensor._shape.push_back(self->_resize_width);
    tensor._shape.push_back(self->_input_channel);
    tensor._elemSize = 1;

    cv::Rect roi(cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                 cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));

    if (self->_align_factor && self->_temp_output_buffer) {
        if (self->_process_function != NULL) {
            if (!self->_process_function(frame_meta, nullptr,
                                         self->_temp_output_buffer)) {
                ret = false;
            }
        } else {
            if (!preprocess(self, frame_meta, self->_temp_output_buffer,
                            &roi)) {
                ret = false;
            }
        }
        add_dummy_data(self, self->_temp_output_buffer, tensor._data);
    } else {
        if (self->_process_function != NULL) {
            if (!self->_process_function(frame_meta, nullptr, tensor._data)) {
                ret = false;
            }
        } else {
            if (!preprocess(self, frame_meta, tensor._data, &roi)) {
                ret = false;
            }
        }
    }
    if (ret) {
        frame_meta->_input_tensor[self->_preprocess_id] = tensor;
    } else {
        self->_pool.deallocate(tensor._data);
        tensor._data = nullptr;
        frame_meta->_input_memory_pool[self->_preprocess_id] = nullptr;
    }
    return ret;
}

bool secondary_process(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
    if (self->_track_cnt.count(frame_meta->_stream_id) == 0) {
        self->_track_cnt[frame_meta->_stream_id] = std::map<int, int>();
    }

    int objects_size = g_list_length(frame_meta->_object_meta_list);
    for (int o = 0; o < objects_size; o++) {
        bool ret = true;
        DXObjectMeta *object_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, o);

        if (object_meta->_input_memory_pool.find(self->_preprocess_id) !=
            object_meta->_input_memory_pool.end()) {
            g_error("Preprocess ID %d already exists in the object meta. "
                    "check your "
                    "pipeline",
                    self->_preprocess_id);
        }
        object_meta->_input_memory_pool[self->_preprocess_id] =
            (MemoryPool *)&self->_pool;

        if (!check_object(self, frame_meta, object_meta)) {
            continue;
        }

        dxs::DXTensor tensor;
        tensor._data = self->_pool.allocate();
        tensor._type = dxs::DataType::UINT8;
        tensor._name = "input";
        tensor._shape.push_back(self->_resize_height);
        tensor._shape.push_back(self->_resize_width);
        tensor._shape.push_back(self->_input_channel);
        tensor._elemSize = 1;

        cv::Rect roi(
            cv::Point(std::max(int(object_meta->_box[0]), 0),
                      std::max(int(object_meta->_box[1]), 0)),
            cv::Point(
                std::min(int(object_meta->_box[2]), frame_meta->_width),
                std::min(int(object_meta->_box[3]), frame_meta->_height)));

        if (self->_align_factor && self->_temp_output_buffer) {
            if (self->_process_function != NULL) {
                if (!self->_process_function(frame_meta, object_meta,
                                             self->_temp_output_buffer)) {
                    ret = false;
                }
            } else {
                if (!preprocess(self, frame_meta, self->_temp_output_buffer,
                                &roi)) {
                    ret = false;
                }
            }
            add_dummy_data(self, self->_temp_output_buffer, tensor._data);
        } else {
            if (self->_process_function != NULL) {
                if (!self->_process_function(frame_meta, object_meta,
                                             tensor._data)) {
                    ret = false;
                }
            } else {
                if (!preprocess(self, frame_meta, tensor._data, &roi)) {
                    ret = false;
                }
            }
        }
        if (ret) {
            object_meta->_input_tensor[self->_preprocess_id] = tensor;
        } else {
            self->_pool.deallocate(tensor._data);
            tensor._data = nullptr;
            object_meta->_input_memory_pool[self->_preprocess_id] = nullptr;
        }
    }
    if (self->_cnt[frame_meta->_stream_id] < self->_interval) {
        self->_cnt[frame_meta->_stream_id] += 1;
    } else {
        self->_cnt[frame_meta->_stream_id] = 0;
    }
    return true;
}

bool gst_dxpreprocess_qos_process(GstDxPreprocess *self, GstBuffer *buf) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);

    if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
        return true;
    }

    if (self->_qos_timediff > 0) {

        GstClockTimeDiff earliest_time;

        if (self->_throttling_delay > 0) {
            earliest_time = self->_qos_timestamp + 2 * self->_qos_timediff +
                            self->_throttling_delay;
        } else {
            earliest_time = self->_qos_timestamp + self->_qos_timediff;
        }

        if (static_cast<GstClockTime>(earliest_time) > in_ts) {
            // gst_buffer_unref(buf);
            return true;
        }
    }
    return false;
}

bool check_primary_interval(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
    auto iter = self->_cnt.find(frame_meta->_stream_id);
    if (iter == self->_cnt.end()) {
        self->_cnt[frame_meta->_stream_id] = 0;
    }
    if (self->_secondary_mode) {
        return false;
    }
    if (self->_cnt[frame_meta->_stream_id] < self->_interval) {
        self->_cnt[frame_meta->_stream_id] += 1;
        return true;
    }
    self->_cnt[frame_meta->_stream_id] = 0;
    return false;
}

DXFrameMeta *get_frame_meta(GstBuffer *buf, GstBaseTransform *trans) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        if (!gst_buffer_is_writable(buf)) {
            buf = gst_buffer_make_writable(buf);
        }
        frame_meta =
            (DXFrameMeta *)gst_buffer_add_meta(buf, DX_FRAME_META_INFO, NULL);

        GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(trans);
        GstCaps *caps = gst_pad_get_current_caps(sinkpad);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        frame_meta->_name = gst_structure_get_name(s);
        frame_meta->_format = gst_structure_get_string(s, "format");
        gst_structure_get_int(s, "width", &frame_meta->_width);
        gst_structure_get_int(s, "height", &frame_meta->_height);
        gint num, denom;
        gst_structure_get_fraction(s, "framerate", &num, &denom);
        frame_meta->_frame_rate = (gfloat)num / (gfloat)denom;
        frame_meta->_stream_id = 0;
        frame_meta->_buf = buf;
        gst_caps_unref(caps);
    }
    return frame_meta;
}

void check_temp_buffers(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
#ifdef HAVE_LIBRGA
#else
    auto iter = self->_crop_frame.find(frame_meta->_stream_id);
    if (iter == self->_crop_frame.end()) {
        self->_crop_frame[frame_meta->_stream_id] = nullptr;
    }

    iter = self->_convert_frame.find(frame_meta->_stream_id);
    if (iter == self->_convert_frame.end()) {
        self->_convert_frame[frame_meta->_stream_id] = nullptr;
    }

    iter = self->_resized_frame.find(frame_meta->_stream_id);
    if (iter == self->_resized_frame.end()) {
        self->_resized_frame[frame_meta->_stream_id] = nullptr;
    }
#endif
}

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);

    DXFrameMeta *frame_meta = get_frame_meta(buf, trans);
    check_temp_buffers(self, frame_meta);

    if (check_primary_interval(self, frame_meta)) {
        return GST_FLOW_OK;
    }

    if (gst_dxpreprocess_qos_process(self, buf)) {
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
    }

    if (self->_secondary_mode) {
        if (!secondary_process(self, frame_meta)) {
            return GST_FLOW_ERROR;
        }
    } else {
        if (!primary_process(self, frame_meta)) {
            return GST_FLOW_ERROR;
        }
    }

    return GST_FLOW_OK;
}
