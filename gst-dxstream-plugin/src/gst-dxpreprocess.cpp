#include "gst-dxpreprocess.hpp"
#include "preprocessors/preprocessor_factory.h"
#include <chrono>
#include <dlfcn.h>
#include <gst/video/video.h>
#include <iostream>
#include <json-glib/json-glib.h>
#include <libyuv.h>

enum class PropertyID {
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

G_DEFINE_TYPE(GstDxPreprocess, gst_dxpreprocess, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

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
        out_roi[i] = static_cast<gint>(json_node_get_int(node));
    }
    return TRUE;
}

static void parse_config(GstDxPreprocess *self) {
    if (!g_file_test(self->_config.file_path, G_FILE_TEST_EXISTS)) {
        g_error("[dxpreprocess] Config file does not exist: %s\n",
                self->_config.file_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading preprocess config file: %s", self->_config.file_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;
    if (!json_parser_load_from_file(parser, self->_config.file_path, &error)) {
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
        gint64 val = json_object_get_int_member(object, json_key);
        if (val < 0) {
            g_error("[dxpreprocess] Member %s has a negative value (%ld).",
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

    set_uint("preprocess_id", self->_preprocess.id, "preprocess_id");
    set_uint("resize_width", self->_preprocess.width, "resize_width");
    set_uint("resize_height", self->_preprocess.height, "resize_height");
    set_uint("pad_value", self->_preprocess.pad_value, "pad_value");
    set_uint("min_object_width", self->_object_filter.min_width, "min_object_width");
    set_uint("min_object_height", self->_object_filter.min_height,
             "min_object_height");
    set_uint("interval", self->_frame_ctrl.interval, "interval");

    if (json_object_has_member(object, "color_format")) {
        const gchar *fmt =
            json_object_get_string_member(object, "color_format");
        if (g_strcmp0(fmt, "RGB") == 0 || g_strcmp0(fmt, "BGR") == 0) {
            g_free(self->_preprocess.color_format);
            self->_preprocess.color_format = g_strdup(fmt);
        } else {
            g_warning("Invalid color mode: %s. Use RGB or BGR", fmt);
        }
    }

    if (json_object_has_member(object, "target_class_id")) {
        gint64 val = json_object_get_int_member(object, "target_class_id");
        if (val < G_MININT || val > G_MAXINT) {
            g_error("[dxpreprocess] target_class_id value out of range");
        }
        self->_object_filter.target_class_id = static_cast<gint>(val);
    }

    if (json_object_has_member(object, "roi")) {
        JsonArray *roi_array = json_object_get_array_member(object, "roi");
        std::array<gint, 4> roi;
        if (validate_roi(roi_array, roi.data())) {
            memcpy(self->_object_filter.roi, roi.data(), sizeof(roi));
        }
    }

    set_boolean("keep_ratio", self->_preprocess.keep_ratio);
    set_boolean("secondary_mode", self->_object_filter.secondary_mode);
    set_boolean("transpose", self->_preprocess.transpose);

    g_object_unref(parser);
}

static void dxpreprocess_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        if (nullptr != self->_config.file_path)
            g_free(self->_config.file_path);
        self->_config.file_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;

    case PropertyID::PROP_LIBRARY_FILE_PATH:
        if (self->_config.library_path) {
            g_free(self->_config.library_path);
        }
        self->_config.library_path = g_value_dup_string(value);
        break;

    case PropertyID::PROP_FUNCTION_NAME:
        if (self->_config.function_name) {
            g_free(self->_config.function_name);
        }
        self->_config.function_name = g_value_dup_string(value);
        break;

    case PropertyID::PROP_PREPROCESS_ID:
        self->_preprocess.id = g_value_get_uint(value);
        break;

    case PropertyID::PROP_COLOR_FORMAT: {
        g_free(self->_preprocess.color_format);
        guint color_value = g_value_get_uint(value);
        if (color_value == 0) {
            self->_preprocess.color_format = g_strdup("RGB");
        } else if (color_value == 1) {
            self->_preprocess.color_format = g_strdup("BGR");
        } else {
            g_warning("Invalid color mode: %d. Use RGB or BGR.", color_value);
        }
        break;
    }
    case PropertyID::PROP_RESIZE_WIDTH:
        self->_preprocess.width = g_value_get_uint(value);
        break;

    case PropertyID::PROP_RESIZE_HEIGHT:
        self->_preprocess.height = g_value_get_uint(value);
        break;

    case PropertyID::PROP_KEEP_RATIO:
        self->_preprocess.keep_ratio = g_value_get_boolean(value);
        break;

    case PropertyID::PROP_PAD_VALUE:
        self->_preprocess.pad_value = g_value_get_uint(value);
        break;

    case PropertyID::PROP_SECONDARY_MODE:
        self->_object_filter.secondary_mode = g_value_get_boolean(value);
        break;

    case PropertyID::PROP_TRANSPOSE:
        self->_preprocess.transpose = g_value_get_boolean(value);
        break;

    case PropertyID::PROP_TARGET_CLASS_ID:
        self->_object_filter.target_class_id = g_value_get_int(value);
        break;

    case PropertyID::PROP_MIN_OBJECT_WIDTH:
        self->_object_filter.min_width = g_value_get_uint(value);
        break;

    case PropertyID::PROP_MIN_OBJECT_HEIGHT:
        self->_object_filter.min_height = g_value_get_uint(value);
        break;

    case PropertyID::PROP_INTERVAL:
        self->_frame_ctrl.interval = g_value_get_uint(value);
        break;

    case PropertyID::PROP_ROI: {
        if (G_VALUE_HOLDS_STRING(value)) {
            const gchar *roi_str = g_value_get_string(value);
            std::array<gint, 4> roi_values;
            int count = sscanf(roi_str, "%d,%d,%d,%d", &roi_values[0],
                               &roi_values[1], &roi_values[2], &roi_values[3]);

            if (count != 4) {
                g_error("Invalid ROI format. Expected format: "
                        "roi=\"x1,y1,x2,y2\"\n");
                return;
            }

            for (size_t i = 0; i < 4; i++) {
                self->_object_filter.roi[i] = roi_values[i];
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

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config.file_path);
        break;

    case PropertyID::PROP_LIBRARY_FILE_PATH:
        g_value_set_string(value, self->_config.library_path);
        break;

    case PropertyID::PROP_FUNCTION_NAME:
        g_value_set_string(value, self->_config.function_name);
        break;

    case PropertyID::PROP_PREPROCESS_ID:
        g_value_set_uint(value, self->_preprocess.id);
        break;

    case PropertyID::PROP_COLOR_FORMAT:
        if (g_strcmp0(self->_preprocess.color_format, "RGB") == 0) {
            g_value_set_uint(value, 0);
        } else if (g_strcmp0(self->_preprocess.color_format, "BGR") == 0) {
            g_value_set_uint(value, 1);
        } else {
            g_warning("Invalid color mode: %s. Use RGB or BGR.",
                      self->_preprocess.color_format);
        }
        break;

    case PropertyID::PROP_RESIZE_WIDTH:
        g_value_set_uint(value, self->_preprocess.width);
        break;

    case PropertyID::PROP_RESIZE_HEIGHT:
        g_value_set_uint(value, self->_preprocess.height);
        break;

    case PropertyID::PROP_KEEP_RATIO:
        g_value_set_boolean(value, self->_preprocess.keep_ratio);
        break;

    case PropertyID::PROP_PAD_VALUE:
        g_value_set_uint(value, self->_preprocess.pad_value);
        break;

    case PropertyID::PROP_SECONDARY_MODE:
        g_value_set_boolean(value, self->_object_filter.secondary_mode);
        break;

    case PropertyID::PROP_TRANSPOSE:
        g_value_set_boolean(value, self->_preprocess.transpose);
        break;

    case PropertyID::PROP_TARGET_CLASS_ID:
        g_value_set_int(value, self->_object_filter.target_class_id);
        break;

    case PropertyID::PROP_MIN_OBJECT_WIDTH:
        g_value_set_uint(value, self->_object_filter.min_width);
        break;

    case PropertyID::PROP_MIN_OBJECT_HEIGHT:
        g_value_set_uint(value, self->_object_filter.min_height);
        break;

    case PropertyID::PROP_INTERVAL:
        g_value_set_uint(value, self->_frame_ctrl.interval);
        break;

    case PropertyID::PROP_ROI: {
        std::string roi_str = std::to_string(self->_object_filter.roi[0]) + "," +
                              std::to_string(self->_object_filter.roi[1]) + "," +
                              std::to_string(self->_object_filter.roi[2]) + "," +
                              std::to_string(self->_object_filter.roi[3]);

        g_value_set_string(value, roi_str.c_str());
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
    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        self->_stream.info.clear();
        break;
    default:
        break;
    }
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxpreprocess_dispose(GObject *object) {
    GstDxPreprocess *self = GST_DXPREPROCESS(object);
    if (self->_config.file_path) {
        g_free(self->_config.file_path);
        self->_config.file_path = nullptr;
    }
    if (self->_config.library_path) {
        g_free(self->_config.library_path);
        self->_config.library_path = nullptr;
    }
    if (self->_config.function_name) {
        g_free(self->_config.function_name);
        self->_config.function_name = nullptr;
    }
    if (self->_plugin.library_handle) {
        dlclose(self->_plugin.library_handle);
        self->_plugin.library_handle = nullptr;
    }
    g_free(self->_preprocess.color_format);

    // Clean up preprocessor pointer (now automatically managed by unique_ptr)
    self->_plugin.preprocessor.reset();

    // _transpose_data is now std::vector, automatically cleaned up
    self->_preprocess.transpose_data.clear();

    self->_frame_ctrl.track_cnt.clear();

    // Frame buffers are now std::vector, automatically cleaned up
    self->_buffers.crop.clear();
    self->_buffers.convert.clear();
    self->_buffers.resized.clear();

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxpreprocess_finalize(GObject *object) {
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstCaps *gst_dxpreprocess_transform_caps(GstBaseTransform *trans,
                                                GstPadDirection direction,
                                                GstCaps *caps,
                                                GstCaps *filter) {
    
    std::ignore = trans;
    std::ignore = direction;

    auto *new_caps = gst_caps_copy(caps);

    if (filter) {
        auto *filtered_caps = gst_caps_intersect(new_caps, filter);
        gst_caps_unref(new_caps);
        new_caps = filtered_caps;
    }
    return new_caps;
}

static gboolean gst_dxpreprocess_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps, GstCaps *outcaps) {
    
    std::ignore = trans;
    std::ignore = outcaps;
    std::ignore = incaps;
    
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
    // Only register on first caps event per stream_id.
    // Dynamic resolution change within a running stream is not supported.
    if (incaps && self->_stream.info.find(stream_id) == self->_stream.info.end()) {
        gst_video_info_init(&self->_stream.info[stream_id]);
        if (!gst_video_info_from_caps(&self->_stream.info[stream_id], incaps)) {
            GST_WARNING_OBJECT(self, "Failed to parse caps for stream %d", stream_id);
            self->_stream.info.erase(stream_id);
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
            if (original_event) {
                gst_event_unref(original_event);
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

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxpreprocess_set_property;
    gobject_class->get_property = dxpreprocess_get_property;
    gobject_class->dispose = dxpreprocess_dispose;
    gobject_class->finalize = dxpreprocess_finalize;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<guint>(PropertyID::PROP_CONFIG_FILE_PATH)] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the JSON config file containing the element's properties.",
        nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_LIBRARY_FILE_PATH)] =
        g_param_spec_string("library-file-path", "Library File Path",
                            "Path to the custom preprocess library, if used",
                            nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_FUNCTION_NAME)] = g_param_spec_string(
        "function-name", "Function Name",
        "Name of the custom preprocessing function to use. ", nullptr,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_PREPROCESS_ID)] =
        g_param_spec_uint("preprocess-id", "Preprocess ID",
                          "Assigns an ID to the preprocessed input", 0, 10000,
                          0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_COLOR_FORMAT)] =
        g_param_spec_uint("color-format", "Color Format",
                          "Specifies the color format for preprocessing. "
                          "[0: RGB, 1: BGR]",
                          0, 1, 0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_RESIZE_WIDTH)] = g_param_spec_uint(
        "resize-width", "Resize Width", "Specifies the width for resizing.", 0,
        10000, 0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_RESIZE_HEIGHT)] = g_param_spec_uint(
        "resize-height", "Resize Height", "Specifies the width for resizing.",
        0, 10000, 0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_KEEP_RATIO)] = g_param_spec_boolean(
        "keep-ratio", "Keep Original Ratio",
        "Maintains the original aspect ratio during resizing", TRUE,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_PAD_VALUE)] = g_param_spec_uint(
        "pad-value", "PadValue", "Padding color value for R, G, B", 0, 255, 0,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_SECONDARY_MODE)] = g_param_spec_boolean(
        "secondary-mode", "Is Secondary Mode",
        "Enables Secondary Mode for processing object regions.", FALSE,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_TRANSPOSE)] = g_param_spec_boolean(
        "transpose", "Is Transpose",
        "Enables Transpose for processing object regions.", FALSE,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_TARGET_CLASS_ID)] =
        g_param_spec_int("target-class-id", "Target Class ID",
                         "Filters objects in Secondary Mode by class ID. ( -1 "
                         "processes all objects).",
                         -1, 10000, -1, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_MIN_OBJECT_WIDTH)] = g_param_spec_uint(
        "min-object-width", "Min Object Box Width",
        "Minimum object width for preprocessing in Secondary Mode", 0, 10000, 0,
        G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_MIN_OBJECT_HEIGHT)] = g_param_spec_uint(
        "min-object-height", "Min Object Box Height",
        "Minimum object height for preprocessing in Secondary Mode", 0, 10000,
        0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_INTERVAL)] = g_param_spec_uint(
        "interval", "Inference Interval",
        "Specifies the interval for preprocessing frames or objects.", 0, 10000,
        0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_ROI)] = g_param_spec_string(
        "roi", "Region of Interest",
        "Defines the ROI as a comma-separated string (x1,y1,x2,y2)",
        "-1,-1,-1,-1", G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<int>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

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
    self->_config.file_path = nullptr;
    self->_preprocess.id = 0;
    self->_preprocess.color_format = g_strdup("RGB");
    self->_preprocess.width = 0;
    self->_preprocess.height = 0;
    self->_preprocess.channel = 3;
    self->_preprocess.keep_ratio = TRUE;
    self->_preprocess.pad_value = 0;
    self->_object_filter.secondary_mode = FALSE;
    self->_object_filter.target_class_id = -1;
    self->_object_filter.min_width = 0;
    self->_object_filter.min_height = 0;
    self->_frame_ctrl.interval = 0;
    self->_preprocess.transpose = FALSE;
    self->_preprocess.transpose_data.clear();

    self->_frame_ctrl.cnt.clear();

    self->_frame_ctrl.acc_fps = 0;
    self->_frame_ctrl.frame_count = 0;

    self->_object_filter.roi[0] = -1;
    self->_object_filter.roi[1] = -1;
    self->_object_filter.roi[2] = -1;
    self->_object_filter.roi[3] = -1;
    self->_frame_ctrl.track_cnt.clear();

    self->_stream.last_id = 0;
    self->_stream.info.clear();

    self->_qos.timestamp = 0;
    self->_qos.timediff = 0;
    self->_qos.throttling_delay = 0;

    self->_plugin.preprocessor = nullptr;

    self->_buffers.crop.clear();
    self->_buffers.convert.clear();
    self->_buffers.resized.clear();
}

static gboolean gst_dxpreprocess_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    if (!self->_plugin.library_handle && self->_config.library_path &&
        self->_config.function_name) {
        self->_plugin.library_handle = dlopen(self->_config.library_path, RTLD_LAZY);
        if (!self->_plugin.library_handle) {
            g_print("Error opening library: %s\n", dlerror());
            return FALSE;
        }
        void *func_ptr = dlsym(self->_plugin.library_handle, self->_config.function_name);
        if (!func_ptr) {
            g_print("Error finding function: %s\n", dlerror());
            if (self->_plugin.library_handle) {
                dlclose(self->_plugin.library_handle);
                self->_plugin.library_handle = nullptr;
            }
            return FALSE;
        }

        self->_plugin.process_function =
            (bool (*)(GstBuffer *, DXFrameMeta *, DXObjectMeta *, void *))func_ptr;
        if (!self->_plugin.process_function) {
            g_print("Error: Process function is nullptr\n");
            return FALSE;
        }
    }

    if (self->_preprocess.height > 0 && self->_preprocess.width > 0) {
        if (self->_preprocess.transpose) {
            try {
                size_t size = self->_preprocess.height * self->_preprocess.width * self->_preprocess.channel;
                self->_preprocess.transpose_data.resize(size);
            } catch (const std::bad_alloc& e) {
                g_error("Failed to allocate memory for transpose data: %s", e.what());
                return FALSE;
            }
        }
    } else {
        g_error("Invalid input size %d x %d", self->_preprocess.width,
                self->_preprocess.height);
        return FALSE;
    }

    if (!self->_plugin.preprocessor) {
        self->_plugin.preprocessor = PreprocessorFactory::create_preprocessor(self);
        if (!self->_plugin.preprocessor) {
            GST_ERROR_OBJECT(self, "Failed to create preprocessor instance");
            return FALSE;
        }
        GST_INFO_OBJECT(self, "Preprocessor instance created successfully");
    }

    GST_INFO_OBJECT(self, "Preprocessor ready (size=%dx%d, secondary_mode=%d, interval=%u)",
                    self->_preprocess.width, self->_preprocess.height,
                    self->_object_filter.secondary_mode, self->_frame_ctrl.interval);
    return TRUE;
}

static gboolean gst_dxpreprocess_stop(GstBaseTransform *trans) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    GST_INFO_OBJECT(self, "Preprocessor stopping");
    return TRUE;
}

static gboolean gst_dxpreprocess_src_event(GstBaseTransform *trans,
                                           GstEvent *event) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);
    if (GST_EVENT_TYPE(event) == GST_EVENT_QOS) {
        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, nullptr, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_OBJECT_LOCK(trans);
            self->_qos.throttling_delay = diff;
            GST_OBJECT_UNLOCK(trans);
        }

        if (type == GST_QOS_TYPE_UNDERFLOW && diff > 0) {
            GST_OBJECT_LOCK(trans);
            self->_qos.timediff = diff;
            self->_qos.timestamp = timestamp;
            GST_OBJECT_UNLOCK(trans);
        }
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
    GstClockTimeDiff qos_timediff = self->_qos.timediff;
    GstClockTime qos_timestamp = self->_qos.timestamp;
    GstClockTimeDiff throttling_delay = self->_qos.throttling_delay;
    GST_OBJECT_UNLOCK(self);

    if (qos_timediff > 0) {
        GstClockTimeDiff earliest_time;
        if (throttling_delay > 0) {
            earliest_time = qos_timestamp + 2 * qos_timediff + throttling_delay;
        } else {
            earliest_time = qos_timestamp + qos_timediff;
        }

        if (static_cast<GstClockTime>(earliest_time) > in_ts) {
            GST_DEBUG_OBJECT(self, "Dropping buffer due to QoS (ts=%" GST_TIME_FORMAT ")",
                           GST_TIME_ARGS(in_ts));
            return true;
        }
    }
    return false;
}

static GstFlowReturn gst_dxpreprocess_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buf) {
    GstDxPreprocess *self = GST_DXPREPROCESS(trans);

    GST_DEBUG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    if (gst_dxpreprocess_qos_process(self, buf)) {
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
    }

    buf = self->_plugin.preprocessor->check_frame_meta(buf);

    if (self->_object_filter.secondary_mode) {
        GST_DEBUG_OBJECT(self, "Processing in secondary mode");
        if (!self->_plugin.preprocessor->secondary_process(buf)) {
            GST_ERROR_OBJECT(self, "Secondary preprocessing failed");
            return GST_FLOW_ERROR;
        }
    } else {
        GST_DEBUG_OBJECT(self, "Processing in primary mode");
        if (!self->_plugin.preprocessor->primary_process(buf)) {
            GST_ERROR_OBJECT(self, "Primary preprocessing failed");
            return GST_FLOW_ERROR;
        }
    }

    return GST_FLOW_OK;
}
