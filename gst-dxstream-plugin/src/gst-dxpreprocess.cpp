#include "gst-dxpreprocess.hpp"
#include "format_convert.hpp"
#include <chrono>
#include <dlfcn.h>
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
            (cv::Mat(*)(DXFrameMeta *, DXObjectMeta *))func_ptr;
    }

    self->_align_factor =
        (64 - ((self->_resize_width * self->_input_channel) % 64)) % 64;
    self->_pool.deinitialize();
    if (self->_resize_height > 0 && self->_resize_width > 0) {
        self->_pool.initialize(self->_resize_height *
                                   (self->_resize_width * self->_input_channel +
                                    self->_align_factor),
                               self->_pool_size);
    }

    return TRUE;
}

static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

cv::Mat preprocess_image(GstDxPreprocess *self, DXFrameMeta *frame_meta,
                         cv::Rect *roi) {
    cv::Mat resizedFrame(self->_resize_height, self->_resize_width, CV_8UC3);
    uint8_t *convert_frame = nullptr;
    uint8_t *resized_frame = nullptr;
    if (self->_keep_ratio) {
        float dw, dh;
        uint16_t top, bottom, left, right;
        float ratioDest = (float)self->_resize_width / self->_resize_height;
        float ratioSrc = (float)frame_meta->_width / frame_meta->_height;
        if (roi) {
            ratioSrc = (float)roi->width / roi->height;
        }
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = self->_resize_height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = self->_resize_width;
            newHeight = newWidth / ratioSrc;
        }

        if (roi) {
            uint8_t *crop_frame = Crop(
                frame_meta->_buf, frame_meta->_width, frame_meta->_height,
                roi->x, roi->y, roi->width, roi->height, frame_meta->_format);
            resized_frame = Resize(crop_frame, roi->width, roi->height,
                                   newWidth, newHeight, frame_meta->_format);
            free(crop_frame);
        } else {
            resized_frame = Resize(frame_meta->_buf, frame_meta->_width,
                                   frame_meta->_height, newWidth, newHeight,
                                   frame_meta->_format);
        }
        convert_frame = CvtColor(resized_frame, newWidth, newHeight,
                                 frame_meta->_format, self->_color_format);
        cv::Mat temp = cv::Mat(newHeight, newWidth, CV_8UC3, convert_frame);
        dw = (self->_resize_width - newWidth) / 2.0;
        dh = (self->_resize_height - newHeight) / 2.0;
        top = (uint16_t)round(dh - 0.1);
        bottom = (uint16_t)round(dh + 0.1);
        left = (uint16_t)round(dw - 0.1);
        right = (uint16_t)round(dw + 0.1);
        // resizedFrame =
        //     cv::Mat(self->_resize_height, self->_resize_width, CV_8UC3);
        cv::copyMakeBorder(
            temp, resizedFrame, top, bottom, left, right, cv::BORDER_CONSTANT,
            cv::Scalar(self->_pad_value, self->_pad_value, self->_pad_value));
        temp.release();
    } else {
        resized_frame = Resize(frame_meta->_buf, frame_meta->_width,
                               frame_meta->_height, self->_resize_width,
                               self->_resize_height, frame_meta->_format);
        convert_frame =
            CvtColor(resized_frame, self->_resize_width, self->_resize_height,
                     frame_meta->_format, self->_color_format);
        // resizedFrame = cv::Mat(self->_resize_height, self->_resize_width,
        //                        CV_8UC3, convert_frame);
        memcpy(resizedFrame.data, convert_frame,
               self->_resize_height * self->_resize_width * 3);
    }
    // resizedFrame.release();
    free(resized_frame);
    free(convert_frame);
    return resizedFrame;
}

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

bool preproc(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
    bool processed = false;
    if (self->_roi[0] != -1) {
        frame_meta->_roi[0] = std::max(self->_roi[0], 0);
        frame_meta->_roi[1] = std::max(self->_roi[1], 0);
        frame_meta->_roi[2] = std::min(self->_roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(self->_roi[3], frame_meta->_height - 1);
    }

    if (self->_track_cnt.count(frame_meta->_stream_id) == 0) {
        self->_track_cnt[frame_meta->_stream_id] = std::map<int, int>();
    }

    cv::Mat resized_frame;

    if (self->_secondary_mode) {
        int objects_size = g_list_length(frame_meta->_object_meta_list);
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);

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

            cv::Rect crop_region(
                cv::Point(std::max(int(object_meta->_box[0]), 0),
                          std::max(int(object_meta->_box[1]), 0)),
                cv::Point(
                    std::min(int(object_meta->_box[2]), frame_meta->_width),
                    std::min(int(object_meta->_box[3]), frame_meta->_height)));

            if (self->_process_function == NULL) {
                resized_frame =
                    preprocess_image(self, frame_meta, &crop_region);
            } else {
                resized_frame =
                    self->_process_function(frame_meta, object_meta);
            }
            dxs::DXTensor new_tensor;
            new_tensor._data = self->_pool.allocate();

            add_dummy_data(self, resized_frame.data, new_tensor._data);

            new_tensor._type = dxs::DataType::UINT8;
            new_tensor._name = "input";
            new_tensor._shape.push_back(self->_resize_height);
            new_tensor._shape.push_back(self->_resize_width);
            new_tensor._shape.push_back(self->_input_channel);
            new_tensor._elemSize = 1;
            object_meta->_input_tensor[self->_preprocess_id] = new_tensor;
            processed = true;
        }
        if (self->_cnt[frame_meta->_stream_id] < self->_interval) {
            self->_cnt[frame_meta->_stream_id] += 1;
        } else {
            self->_cnt[frame_meta->_stream_id] = 0;
        }
    } else {
        if (frame_meta->_input_memory_pool.find(self->_preprocess_id) !=
            frame_meta->_input_memory_pool.end()) {
            g_error(
                "Preprocess ID %d already exists in the frame meta. check your "
                "pipeline",
                self->_preprocess_id);
        }
        frame_meta->_input_memory_pool[self->_preprocess_id] =
            (MemoryPool *)&self->_pool;

        if (self->_process_function == NULL) {
            if (frame_meta->_roi[0] != -1) {
                cv::Rect roi(
                    cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                    cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));

                resized_frame = preprocess_image(self, frame_meta, &roi);
            } else {
                resized_frame = preprocess_image(self, frame_meta, NULL);
            }
        } else {
            resized_frame = self->_process_function(frame_meta, nullptr);
        }
        dxs::DXTensor new_tensor;
        new_tensor._data = self->_pool.allocate();

        add_dummy_data(self, resized_frame.data, new_tensor._data);

        new_tensor._type = dxs::DataType::UINT8;
        new_tensor._name = "input";
        new_tensor._shape.push_back(self->_resize_height);
        new_tensor._shape.push_back(self->_resize_width);
        new_tensor._shape.push_back(self->_input_channel);
        new_tensor._elemSize = 1;
        frame_meta->_input_tensor[self->_preprocess_id] = new_tensor;
        processed = true;
    }
    return processed;
}

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);

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

    auto iter = self->_cnt.find(frame_meta->_stream_id);
    if (iter == self->_cnt.end()) {
        self->_cnt[frame_meta->_stream_id] = 0;
    }
    if (!self->_secondary_mode &&
        self->_cnt[frame_meta->_stream_id] < self->_interval) {
        self->_cnt[frame_meta->_stream_id] += 1;
        return GST_FLOW_OK;
    }

    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf); // nano second (10^-9)

    if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
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
            return GST_BASE_TRANSFORM_FLOW_DROPPED;
        }
    }

    // auto start = std::chrono::high_resolution_clock::now();

    preproc(self, frame_meta);

    // auto end = std::chrono::high_resolution_clock::now();
    // if (processed) {
    //     auto frameDuration =
    //         std::chrono::duration_cast<std::chrono::microseconds>(end -
    //         start);
    //     double frameTimeSec = frameDuration.count() / 1000000.0;
    //     self->_acc_fps += 1.0 / frameTimeSec;
    //     self->_frame_count_for_fps++;

    //     if (self->_frame_count_for_fps % 100 == 0 &&
    //         self->_frame_count_for_fps != 0) {
    //         gchar *name = NULL;
    //         g_object_get(G_OBJECT(self), "name", &name, NULL);
    //         g_print("[%s]\tFPS : %f \n", name,
    //                 self->_acc_fps / self->_frame_count_for_fps);
    //         self->_acc_fps = 0;
    //         self->_frame_count_for_fps = 0;
    //         g_free(name);
    //     }
    // }

    if (!self->_secondary_mode) {
        self->_cnt[frame_meta->_stream_id] = 0;
    }

    return GST_FLOW_OK;
}
