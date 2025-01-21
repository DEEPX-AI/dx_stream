#include "gst-dxpreprocess.hpp"
#include "format_convert.hpp"
#include "utils.hpp"
#include <dlfcn.h>
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

typedef enum { COLOR_RGB, COLOR_BGR, COLOR_LAST } ColorFormat;

GType color_mode_get_type() {
    static GType color_mode_type = 0;
    static const GEnumValue color_formats[] = {
        {COLOR_RGB, "RGB", "rgb"}, {COLOR_BGR, "BGR", "bgr"}, {0, NULL, NULL}};

    if (color_mode_type == 0) {
        color_mode_type = g_enum_register_static("ColorFormat", color_formats);
    }
    return color_mode_type;
}

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
                    g_error("Member preprocess_id has a negative value (%d) "
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
                    g_error("Member resize_width has a negative value (%d) and "
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
                    g_error("Member resize_height has a negative value (%d) "
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
                    g_error("Member pad_value has a negative value (%d) and "
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
                    g_error("Member min_object_width has a negative value (%d) "
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
                    g_error("Member min_object_height has a negative value "
                            "(%d) and cannot be "
                            "converted to unsigned.",
                            int_value);
                }
                self->_min_object_height = (guint)int_value;
            }
            if (json_object_has_member(object, "interval")) {
                gint int_value = json_object_get_int_member(object, "interval");
                if (int_value < 0) {
                    g_error("Member interval has a negative value (%d) and "
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
                    g_error("Member pool_size has a negative value (%d) and "
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
                    g_printerr("Expected 'roi' to be a JSON array.\n");
                    return;
                }

                guint length = json_array_get_length(roi_array);
                if (length != 4) {
                    g_printerr("ROI must have exactly 4 integer values.\n");
                    return;
                }

                for (guint i = 0; i < length; i++) {
                    JsonNode *node = json_array_get_element(roi_array, i);
                    if (!JSON_NODE_HOLDS_VALUE(node) ||
                        !json_node_get_value_type(node) == G_TYPE_INT) {
                        g_printerr(
                            "ROI array must contain only integer values.\n");
                        return;
                    }

                    int value = json_node_get_int(node);
                    self->_roi[i] = value;
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
        const gchar *color_mode = g_value_get_string(value);
        if (g_strcmp0(color_mode, "RGB") == 0 ||
            g_strcmp0(color_mode, "BGR") == 0) {
            g_free(self->_color_format);
            self->_color_format = g_strdup(color_mode);
        } else {
            g_warning("Invalid color mode: %s. Use RGB or BGR.", color_mode);
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
        if (!GST_VALUE_HOLDS_ARRAY(value)) {
            g_printerr("Expected a value array for ROI property.\n");
            return;
        }

        guint length = gst_value_array_get_size(value);
        if (length != 4) {
            g_printerr("ROI must have exactly 4 integer values.\n");
            return;
        }

        for (guint i = 0; i < length; i++) {
            const GValue *element = gst_value_array_get_value(value, i);
            if (!G_VALUE_HOLDS_INT(element)) {
                g_printerr("ROI array must contain only integers.\n");
                return;
            }
            self->_roi[i] = g_value_get_int(element);
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
        g_value_set_string(value, self->_color_format);
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
        GValue list = G_VALUE_INIT;
        g_value_init(&list, G_TYPE_VALUE_ARRAY);

        for (size_t i = 0; i < 4; i++) {
            GValue val = G_VALUE_INIT;
            g_value_init(&val, G_TYPE_INT);
            g_value_set_int(&val, self->_roi[i]);
            gst_value_array_append_value(&list, &val);
            g_value_unset(&val);
        }

        g_value_set_boxed(value, g_value_get_boxed(&list));
        g_value_unset(&list);
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
    self->_track_cnt.clear();
    self->_pool.deinitialize();

    for (std::map<int, MemoryPool>::iterator it = self->_surface_pool.begin();
         it != self->_surface_pool.end(); ++it) {
        it->second.deinitialize();
    }
    self->_surface_pool.clear();

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxpreprocess_finalize(GObject *object) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);

    self->_pool.deinitialize();

    for (std::map<int, MemoryPool>::iterator it = self->_surface_pool.begin();
         it != self->_surface_pool.end(); ++it) {
        it->second.deinitialize();
    }
    self->_surface_pool.clear();

    G_OBJECT_CLASS(parent_class)->finalize(object);
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
        g_param_spec_enum("color-format", "Color Format",
                          "Specifies the color format for preprocessing.",
                          color_mode_get_type(), COLOR_RGB, G_PARAM_READWRITE);

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

    obj_properties[PROP_ROI] = g_param_spec_value_array(
        "roi", "Region of Interest",
        "Defines the ROI (Region of Interest) for preprocessing. ",
        g_param_spec_int("roi", "ROI", "An ROI integer", G_MININT, G_MAXINT, 0,
                         G_PARAM_READWRITE),
        G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));
    gst_element_class_set_static_metadata(
        element_class, "DXPreprocess", "Generic", "Preprocesses network input",
        "Jo Sangil <sijo@deepx.ai>");

    base_transform_class->src_event =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_src_event);

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxpreprocess_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxpreprocess_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_transform_ip);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxpreprocess_change_state;
}

static void gst_dxpreprocess_init(GstDxPreprocess *self) {
    self->_config_file_path = NULL;
    self->_preprocess_id = 0;
    self->_color_format = "RGB";
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
    self->_cnt = 0;

    self->_roi[0] = -1;
    self->_roi[1] = -1;
    self->_roi[2] = -1;
    self->_roi[3] = -1;
    self->_track_cnt.clear();

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;
    self->_throttling_delay = 0;

    self->_pool_size = 1;
    // self->_align_factor =
    //     (64 - ((self->_resize_width * self->_input_channel) % 64)) % 64;
    // // self->_pool.initialize(
    // //     self->_resize_height *
    // //         (self->_resize_width * self->_input_channel +
    // self->_align_factor),
    // //     self->_pool_size);
    new (&self->_surface_pool) std::map<int, MemoryPool *>();
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
            (void (*)(cv::Mat, dxs::DXNetworkInput &, DXObjectMeta *))func_ptr;
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

void preprocess_image(GstDxPreprocess *self, cv::Mat *src,
                      dxs::DXNetworkInput &input_tensor, int align_factor) {
    cv::Mat resizedFrame(self->_resize_height, self->_resize_width, CV_8UC3);
    if (self->_keep_ratio) {
        float dw, dh;
        uint16_t top, bottom, left, right;
        float ratioDest = (float)resizedFrame.cols / resizedFrame.rows;
        float ratioSrc = (float)src->cols / src->rows;
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = resizedFrame.rows;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = resizedFrame.cols;
            newHeight = newWidth / ratioSrc;
        }
        cv::Mat temp = cv::Mat(newHeight, newWidth, CV_8UC3);
        cv::resize(*src, temp, cv::Size(newWidth, newHeight), 0, 0,
                   cv::INTER_LINEAR);
        dw = (resizedFrame.cols - temp.cols) / 2.0;
        dh = (resizedFrame.rows - temp.rows) / 2.0;
        top = (uint16_t)round(dh - 0.1);
        bottom = (uint16_t)round(dh + 0.1);
        left = (uint16_t)round(dw - 0.1);
        right = (uint16_t)round(dw + 0.1);
        cv::copyMakeBorder(
            temp, resizedFrame, top, bottom, left, right, cv::BORDER_CONSTANT,
            cv::Scalar(self->_pad_value, self->_pad_value, self->_pad_value));
        temp.release();
    } else {
        cv::resize(*src, resizedFrame,
                   cv::Size(self->_resize_width, self->_resize_height), 0, 0,
                   cv::INTER_LINEAR);
    }
    for (int y = 0; y < self->_resize_height; y++) {
        memcpy(
            input_tensor._data +
                y * (self->_resize_width * self->_input_channel + align_factor),
            resizedFrame.data + y * self->_resize_width * self->_input_channel,
            self->_resize_width * self->_input_channel);
    }
    resizedFrame.release();
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

void preproc(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
    if (frame_meta->_input_memory_pool.find(self->_preprocess_id) !=
        frame_meta->_input_memory_pool.end()) {
        g_error("Preprocess ID %d already exists in the frame meta. check your "
                "pipeline",
                self->_preprocess_id);
    }
    frame_meta->_input_memory_pool[self->_preprocess_id] =
        (MemoryPool *)&self->_pool;

    if (frame_meta->_surface_pool == nullptr) {
        if (self->_surface_pool.find(frame_meta->_stream_id) ==
            self->_surface_pool.end()) {
            self->_surface_pool[frame_meta->_stream_id] = MemoryPool();
            self->_surface_pool[frame_meta->_stream_id].initialize(
                sizeof(uint8_t) * frame_meta->_width * frame_meta->_height * 3,
                self->_pool_size);
        }

        frame_meta->_surface_pool =
            (MemoryPool *)&self->_surface_pool[frame_meta->_stream_id];
    }
    if (frame_meta->_rgb_surface.data == nullptr) {
        frame_meta->_rgb_surface =
            cv::Mat(frame_meta->_height, frame_meta->_width, CV_8UC3,
                    frame_meta->_surface_pool->allocate());
        set_surface(frame_meta);
    }

    cv::Mat originFrame = convert_color(frame_meta, self->_color_format);

    if (self->_roi[0] != -1) {
        frame_meta->_roi[0] = std::max(self->_roi[0], 0);
        frame_meta->_roi[1] = std::max(self->_roi[1], 0);
        frame_meta->_roi[2] = std::min(self->_roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(self->_roi[3], frame_meta->_height - 1);
    }

    if (self->_track_cnt.count(frame_meta->_stream_id) == 0) {
        self->_track_cnt[frame_meta->_stream_id] = std::map<int, int>();
    }

    if (self->_secondary_mode) {
        int objects_size = g_list_length(frame_meta->_object_meta_list);
        frame_meta->_input_object_tensor[self->_preprocess_id] =
            std::map<void *, dxs::DXNetworkInput>();
        frame_meta->_input_object_tensor[self->_preprocess_id].clear();
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);

            if (self->_target_class_id != -1 &&
                object_meta->_label != self->_target_class_id) {
                continue;
            }

            if (frame_meta->_roi[0] != -1 &&
                !check_object_roi(object_meta->_box, frame_meta->_roi)) {
                continue;
            }

            if (object_meta->_track_id != -1) {
                if (self->_track_cnt[frame_meta->_stream_id].count(
                        object_meta->_track_id) > 0) {
                    self->_track_cnt[frame_meta->_stream_id]
                                    [object_meta->_track_id] += 1;
                } else {
                    self->_track_cnt[frame_meta->_stream_id]
                                    [object_meta->_track_id] = 0;
                }

                if (self->_track_cnt[frame_meta->_stream_id]
                                    [object_meta->_track_id] <
                    self->_interval) {
                    continue;
                }

                self->_track_cnt[frame_meta->_stream_id]
                                [object_meta->_track_id] = 0;
            } else {
                // untracked
                if (self->_cnt < self->_interval) {
                    continue;
                }
            }

            cv::Rect crop_region(
                cv::Point(std::max(int(object_meta->_box[0]), 0),
                          std::max(int(object_meta->_box[1]), 0)),
                cv::Point(
                    std::min(int(object_meta->_box[2]), frame_meta->_width),
                    std::min(int(object_meta->_box[3]), frame_meta->_height)));
            if (crop_region.width < self->_min_object_width ||
                crop_region.height < self->_min_object_height) {
                continue;
            }

            cv::Mat crop_img = originFrame(crop_region);

            frame_meta->_input_object_tensor[self->_preprocess_id]
                                            [(void *)object_meta] =
                dxs::DXNetworkInput(self->_pool.get_block_size(),
                                    self->_pool.allocate());

            if (self->_process_function == NULL) {
                preprocess_image(
                    self, &crop_img,
                    frame_meta->_input_object_tensor[self->_preprocess_id]
                                                    [(void *)object_meta],
                    self->_align_factor);
            } else {
                self->_process_function(
                    originFrame,
                    frame_meta->_input_object_tensor[self->_preprocess_id]
                                                    [(void *)object_meta],
                    object_meta);
            }
        }
        if (self->_cnt < self->_interval) {
            self->_cnt += 1;
            return;
        } else {
            self->_cnt = 0;
        }
    } else {
        frame_meta->_input_tensor[self->_preprocess_id] = dxs::DXNetworkInput(
            self->_pool.get_block_size(), self->_pool.allocate());
        if (frame_meta->_roi[0] != -1) {
            cv::Rect roi(cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                         cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));
            cv::Mat RoIFrame = originFrame(roi);

            if (self->_process_function == NULL) {
                preprocess_image(
                    self, &RoIFrame,
                    frame_meta->_input_tensor[self->_preprocess_id],
                    self->_align_factor);
            } else {
                self->_process_function(
                    RoIFrame, frame_meta->_input_tensor[self->_preprocess_id],
                    nullptr);
            }
        } else {
            if (self->_process_function == NULL) {
                preprocess_image(
                    self, &originFrame,
                    frame_meta->_input_tensor[self->_preprocess_id],
                    self->_align_factor);
            } else {
                self->_process_function(
                    originFrame,
                    frame_meta->_input_tensor[self->_preprocess_id], nullptr);
            }
        }
    }
}

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);

    if (!self->_secondary_mode && self->_cnt < self->_interval) {
        self->_cnt += 1;
        return GST_FLOW_OK;
    }

    gint64 start_time, end_time, latency;
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

        if (earliest_time > in_ts) {
            // gst_buffer_unref(buf);
            return GST_BASE_TRANSFORM_FLOW_DROPPED;
        }
    }

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        if (!gst_buffer_is_writable(buf)) {
            gst_buffer_make_writable(buf);
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

    preproc(self, frame_meta);

    if (!self->_secondary_mode) {
        self->_cnt = 0;
    }

    return GST_FLOW_OK;
}
