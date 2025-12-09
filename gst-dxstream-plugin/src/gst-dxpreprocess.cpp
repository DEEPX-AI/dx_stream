#include "gst-dxpreprocess.hpp"
#include "preprocessors/preprocessor_factory.h"
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
    PROP_TRANSPOSE,
    N_PROPERTIES
};

GST_DEBUG_CATEGORY_STATIC(gst_dxpreprocess_debug_category);
#define GST_CAT_DEFAULT gst_dxpreprocess_debug_category

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf);
static gboolean gst_dxpreprocess_start(GstBaseTransform *trans);
static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans);
static gboolean gst_dxpreprocess_src_event(GstBaseTransform *trans,
                                           GstEvent *event);
static gboolean gst_dxpreprocess_sink_event(GstBaseTransform *trans,
                                            GstEvent *event);

G_DEFINE_TYPE_WITH_CODE(
    GstDxPreprocess, gst_dxpreprocess, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxpreprocess_debug_category, "gst-dxpreprocess",
                            0, "debug category for gst-dxpreprocess element"))

static GstElementClass *parent_class = nullptr;

static gboolean validate_roi(JsonArray *roi_array, gint *out_roi) {
    if (!roi_array || json_array_get_length(roi_array) != 4) {
        g_printerr("Error: ROI must have exactly 4 integer values.\n");
        return FALSE;
    }
    for (guint i = 0; i < 4; i++) {
        JsonNode *node = json_array_get_element(roi_array, i);
        if (!JSON_NODE_HOLDS_VALUE(node) ||
            json_node_get_value_type(node) != G_TYPE_INT) {
            g_printerr("Error: ROI array must contain only integer values.\n");
            return FALSE;
        }
        out_roi[i] = json_node_get_int(node);
    }
    return TRUE;
}

static void parse_config(GstDxPreprocess *self) {
    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        g_error("[dxpreprocess] Config file does not exist: %s\n",
                self->_config_file_path);
        return;
    }

    JsonParser *parser = json_parser_new();
    GError *error = nullptr;
    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        g_error("[dxpreprocess] Failed to load config file: %s",
                error->message);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    JsonObject *object = json_node_get_object(node);

    auto set_string = [&](const char *json_key, const char *gobj_key) {
        if (json_object_has_member(object, json_key)) {
            const gchar *val = json_object_get_string_member(object, json_key);
            g_object_set(self, gobj_key, val, nullptr);
        }
    };

    auto set_uint = [&](const char *json_key, guint &target,
                        const char *err_name) {
        if (!json_object_has_member(object, json_key))
            return;
        gint val = json_object_get_int_member(object, json_key);
        if (val < 0) {
            g_error("[dxpreprocess] Member %s has a negative value (%d).",
                    err_name, val);
        }
        target = static_cast<guint>(val);
    };

    auto set_boolean = [&](const char *json_key, gboolean &target) {
        if (json_object_has_member(object, json_key)) {
            target = json_object_get_boolean_member(object, json_key);
        }
    };

    set_string("library_file_path", "library-file-path");
    set_string("function_name", "function-name");

    set_uint("preprocess_id", self->_preprocess_id, "preprocess_id");
    set_uint("resize_width", self->_resize_width, "resize_width");
    set_uint("resize_height", self->_resize_height, "resize_height");
    set_uint("pad_value", self->_pad_value, "pad_value");
    set_uint("min_object_width", self->_min_object_width, "min_object_width");
    set_uint("min_object_height", self->_min_object_height,
             "min_object_height");
    set_uint("interval", self->_interval, "interval");

    if (json_object_has_member(object, "color_format")) {
        const gchar *fmt =
            json_object_get_string_member(object, "color_format");
        if (g_strcmp0(fmt, "RGB") == 0 || g_strcmp0(fmt, "BGR") == 0) {
            g_free(self->_color_format);
            self->_color_format = g_strdup(fmt);
        } else {
            g_warning("Invalid color mode: %s. Use RGB or BGR", fmt);
        }
    }

    if (json_object_has_member(object, "target_class_id")) {
        self->_target_class_id =
            json_object_get_int_member(object, "target_class_id");
    }

    if (json_object_has_member(object, "roi")) {
        JsonArray *roi_array = json_object_get_array_member(object, "roi");
        gint roi[4];
        if (validate_roi(roi_array, roi)) {
            memcpy(self->_roi, roi, sizeof(roi));
        }
    }

    set_boolean("keep_ratio", self->_keep_ratio);
    set_boolean("secondary_mode", self->_secondary_mode);
    set_boolean("transpose", self->_transpose);

    g_object_unref(parser);
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
        g_free(self->_color_format);
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

    case PROP_TRANSPOSE:
        self->_transpose = g_value_get_boolean(value);
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

    case PROP_TRANSPOSE:
        g_value_set_boolean(value, self->_transpose);
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
    g_free(self->_color_format);

    if (self->_preprocessor) {
        delete self->_preprocessor;
        self->_preprocessor = nullptr;
    }

    if (self->_transpose_data) {
        free(self->_transpose_data);
        self->_transpose_data = nullptr;
    }

    self->_track_cnt.clear();

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

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxpreprocess_finalize(GObject *object) {
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

void set_input_info(GstDxPreprocess *self, GstEvent *event, int stream_id) {
    GstCaps *incaps = nullptr;
    gst_event_parse_caps(event, &incaps);
    if (incaps) {
        if (self->_input_info.find(stream_id) == self->_input_info.end()) {
            gst_video_info_init(&self->_input_info[stream_id]);
            gst_video_info_from_caps(
                &self->_input_info[stream_id], incaps);
        }
    }
}

static gboolean gst_dxpreprocess_sink_event(GstBaseTransform *trans,
                                            GstEvent *event) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    GstPad *src_pad = GST_BASE_TRANSFORM_SRC_PAD(trans);
    GST_INFO_OBJECT(self, "Received event [%s] ", GST_EVENT_TYPE_NAME(event));
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        // for inputselector event
        const GstStructure *s_check = gst_event_get_structure(event);
        if (gst_structure_has_name(s_check, "application/x-dx-wrapped-event")) {
            int stream_id = -1;
            GstEvent *original_event = nullptr;
            gst_structure_get_int(s_check, "stream-id", &stream_id);
            gst_structure_get(s_check, "event", GST_TYPE_EVENT, &original_event, NULL);
            if (original_event && GST_EVENT_TYPE(original_event) == GST_EVENT_CAPS) {
                set_input_info(self, original_event, stream_id);
            }
        }
    } break;
    case GST_EVENT_CAPS: {
        // for single stream
        set_input_info(self, event, 0);
    } break;
    default:
        break;
    }
    return gst_pad_push_event(src_pad, event);
}

static void gst_dxpreprocess_class_init(GstDxPreprocessClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxpreprocess_debug_category, "dxpreprocess", 0,
                            "DXPreprocess plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxpreprocess_set_property;
    gobject_class->get_property = dxpreprocess_get_property;
    gobject_class->dispose = dxpreprocess_dispose;
    gobject_class->finalize = dxpreprocess_finalize;

    static GParamSpec *obj_properties[N_PROPERTIES] = {
        nullptr,
    };

    obj_properties[PROP_CONFIG_FILE_PATH] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the JSON config file containing the element's properties.",
        nullptr, G_PARAM_READWRITE);

    obj_properties[PROP_LIBRARY_FILE_PATH] =
        g_param_spec_string("library-file-path", "Library File Path",
                            "Path to the custom preprocess library, if used",
                            nullptr, G_PARAM_READWRITE);

    obj_properties[PROP_FUNCTION_NAME] = g_param_spec_string(
        "function-name", "Function Name",
        "Name of the custom preprocessing function to use. ", nullptr,
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

    obj_properties[PROP_TRANSPOSE] = g_param_spec_boolean(
        "transpose", "Is Transpose",
        "Enables Transpose for processing object regions.", FALSE,
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
    base_transform_class->sink_event =
        GST_DEBUG_FUNCPTR(gst_dxpreprocess_sink_event);
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
    self->_config_file_path = nullptr;
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
    self->_transpose = FALSE;
    self->_transpose_data = nullptr;

    self->_cnt.clear();

    self->_acc_fps = 0;
    self->_frame_count_for_fps = 0;

    self->_roi[0] = -1;
    self->_roi[1] = -1;
    self->_roi[2] = -1;
    self->_roi[3] = -1;
    self->_track_cnt.clear();

    self->_last_stream_id = 0;
    self->_input_info.clear();

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;
    self->_throttling_delay = 0;

    self->_preprocessor = nullptr;

    self->_crop_frame = std::map<int, uint8_t *>();
    self->_convert_frame = std::map<int, uint8_t *>();
    self->_resized_frame = std::map<int, uint8_t *>();
}

static gboolean gst_dxpreprocess_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    if (!self->_library_handle && self->_library_file_path &&
        self->_function_name) {
        self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
        if (!self->_library_handle) {
            g_print("Error opening library: %s\n", dlerror());
            return FALSE;
        }
        void *func_ptr = dlsym(self->_library_handle, self->_function_name);
        if (!func_ptr) {
            g_print("Error finding function: %s\n", dlerror());
            if (self->_library_handle) {
                dlclose(self->_library_handle);
                self->_library_handle = nullptr;
            }
            return FALSE;
        }

        self->_process_function =
            (bool (*)(GstBuffer *, DXFrameMeta *, DXObjectMeta *, void *))func_ptr;
        if (!self->_process_function) {
            g_print("Error: Process function is nullptr\n");
            return FALSE;
        }
    }

    if (self->_resize_height > 0 && self->_resize_width > 0) {
        if (self->_transpose) {
            self->_transpose_data = (uint8_t *)malloc(
                self->_resize_height * self->_resize_width * self->_input_channel);
            if (!self->_transpose_data) {
                g_error("Failed to allocate memory for transpose data");
                return FALSE;
            }
        }
    } else {
        g_error("Invalid input size %d x %d", self->_resize_width,
                self->_resize_height);
        return FALSE;
    }

    if (!self->_preprocessor) {
        self->_preprocessor = PreprocessorFactory::create_preprocessor(self);
        if (!self->_preprocessor) {
            GST_ERROR_OBJECT(self, "Failed to create preprocessor instance");
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

static gboolean gst_dxpreprocess_src_event(GstBaseTransform *trans,
                                           GstEvent *event) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS: {

        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, nullptr, &diff, &timestamp);

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

bool gst_dxpreprocess_qos_process(GstDxPreprocess *self, GstBuffer *buf) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);

    if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
        return true;
    }

    GST_OBJECT_LOCK(self);
    GstClockTimeDiff qos_timediff = self->_qos_timediff;
    GstClockTime qos_timestamp = self->_qos_timestamp;
    GstClockTimeDiff throttling_delay = self->_throttling_delay;
    GST_OBJECT_UNLOCK(self);

    if (qos_timediff > 0) {
        GstClockTimeDiff earliest_time;
        if (throttling_delay > 0) {
            earliest_time = qos_timestamp + 2 * qos_timediff + throttling_delay;
        } else {
            earliest_time = qos_timestamp + qos_timediff;
        }

        if (static_cast<GstClockTime>(earliest_time) > in_ts) {
            return true;
        }
    }
    return false;
}

DXFrameMeta *get_frame_meta(GstBuffer *buf, GstBaseTransform *trans) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        frame_meta = (DXFrameMeta *)gst_buffer_add_meta(buf, DX_FRAME_META_INFO,
                                                        nullptr);

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
        gst_caps_unref(caps);
    }
    return frame_meta;
}

void check_temp_buffers(GstDxPreprocess *self, DXFrameMeta *frame_meta) {
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
}

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);

    if (gst_dxpreprocess_qos_process(self, buf)) {
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
    }

    self->_preprocessor->check_frame_meta(buf);

    if (self->_secondary_mode) {
        if (!self->_preprocessor->secondary_process(buf)) {
            return GST_FLOW_ERROR;
        }
    } else {
        if (!self->_preprocessor->primary_process(buf)) {
            return GST_FLOW_ERROR;
        }
    }

    return GST_FLOW_OK;
}
