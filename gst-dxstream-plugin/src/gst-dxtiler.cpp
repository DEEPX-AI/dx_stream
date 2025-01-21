#include "gst-dxtiler.hpp"
#include "gst-dxmeta.hpp"
#include "utils.hpp"
#include <cmath>
#include <json-glib/json-glib.h>
#include <libyuv.h>
#include <opencv2/opencv.hpp>

enum {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_COLS,
    PROP_ROWS,
    N_PROPERTIES
};
static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxtiler_debug_category);
#define GST_CAT_DEFAULT gst_dxtiler_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, format=(string){BGRx}"));

static GstFlowReturn gst_dxtiler_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);

G_DEFINE_TYPE(GstDxTiler, gst_dxtiler, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = NULL;

static void dxtiler_set_property(GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec) {
    GstDxTiler *self = GST_DXTILER(object);
    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        if (self->_config_file_path) {
            g_free(self->_config_file_path);
        }
        self->_config_file_path = g_value_dup_string(value);
        if (g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
            JsonParser *parser = json_parser_new();
            GError *error = NULL;

            if (json_parser_load_from_file(parser, self->_config_file_path,
                                           &error)) {
                JsonNode *root = json_parser_get_root(parser);
                JsonObject *_object = json_node_get_object(root);
                if (json_object_has_member(_object, "width")) {
                    int width = json_object_get_int_member(_object, "width");
                    g_object_set(self, "width", width, NULL);
                }
                if (json_object_has_member(_object, "height")) {
                    int height = json_object_get_int_member(_object, "height");
                    g_object_set(self, "height", height, NULL);
                }
                if (json_object_has_member(_object, "cols")) {
                    int cols = json_object_get_int_member(_object, "cols");
                    g_object_set(self, "cols", cols, NULL);
                }
                if (json_object_has_member(_object, "rows")) {
                    int rows = json_object_get_int_member(_object, "rows");
                    g_object_set(self, "rows", rows, NULL);
                }
            } else {
                g_print("Error loading JSON file: %s\n", error->message);
                g_error_free(error);
            }

            g_object_unref(parser);
        } else {
            g_print("Config file does not exist: %s\n",
                    self->_config_file_path);
        }
        break;

    case PROP_WIDTH:
        self->_width = g_value_get_int(value);
        break;

    case PROP_HEIGHT:
        self->_height = g_value_get_int(value);
        break;

    case PROP_COLS:
        self->_cols = g_value_get_int(value);
        break;

    case PROP_ROWS:
        self->_rows = g_value_get_int(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxtiler_get_property(GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec) {
    GstDxTiler *self = GST_DXTILER(object);

    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;

    case PROP_COLS:
        g_value_set_int(value, self->_cols);
        break;

    case PROP_ROWS:
        g_value_set_int(value, self->_rows);
        break;

    case PROP_WIDTH:
        g_value_set_int(value, self->_width);
        break;

    case PROP_HEIGHT:
        g_value_set_int(value, self->_height);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static GstStateChangeReturn dxtiler_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxTiler *self = GST_DXTILER(element);
    GST_INFO_OBJECT(self, "Attempting to change state");
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    if (result == GST_STATE_CHANGE_FAILURE)
        return result;

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED: {
        std::unique_lock<std::mutex> lock(self->_buffer_lock);
        self->_outbuffer = gst_buffer_new_allocate(
            NULL, self->_width * self->_cols * self->_height * self->_rows * 4,
            NULL);
        GstMapInfo map_output;
        gst_buffer_map(self->_outbuffer, &map_output, GST_MAP_WRITE);
        memset(map_output.data, 0, map_output.size);
        gst_buffer_unmap(self->_outbuffer, &map_output);

    } break;
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
    return result;
}

static void dxtiler_dispose(GObject *object) {
    GstDxTiler *self = GST_DXTILER(object);
    if (self->_config_file_path) {
        g_free(self->_config_file_path);
        self->_config_file_path = NULL;
    }
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static gboolean gst_dxtiler_sink_event(GstPad *pad, GstObject *parent,
                                       GstEvent *event) {
    GstDxTiler *self = GST_DXTILER(parent);
    // GST_LOG("Received event: %s", GST_EVENT_TYPE_NAME(event));

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        GST_INFO("Received EOS event");
        break;
    }
    case GST_EVENT_CAPS: {
        gboolean result;
        self->_caps = gst_caps_new_simple(
            "video/x-raw", "format", G_TYPE_STRING, "BGRx", "width", G_TYPE_INT,
            (gint)self->_width * self->_cols, "height", G_TYPE_INT,
            (gint)self->_height * self->_rows, "framerate", GST_TYPE_FRACTION,
            0, 1, "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1, NULL);
        if (!self->_caps) {
            g_printerr("Failed to create caps\n");
            return FALSE;
        }
        GstEvent *caps_event = gst_event_new_caps(self->_caps);
        if (!caps_event) {
            g_printerr("Failed to create caps event\n");
            return FALSE;
        }
        result = gst_pad_push_event(self->_srcpad, caps_event);
        if (!result) {
            g_printerr("Failed to push caps event to src pad\n");
            return FALSE;
        }
        return result;
    }
    case GST_EVENT_SEGMENT: {
        GstSegment segment;

        gst_event_copy_segment(event, &segment);

        g_print("Received SEGMENT event: start %" GST_TIME_FORMAT
                " stop %" GST_TIME_FORMAT "\n",
                GST_TIME_ARGS(segment.start), GST_TIME_ARGS(segment.stop));
        break;
    }
    case GST_EVENT_FLUSH_START:
        GST_INFO("Received FLUSH_START event");
        break;
    case GST_EVENT_FLUSH_STOP:
        GST_INFO("Received FLUSH_STOP event");
        break;
    default:
        break;
    }
    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxtiler_class_init(GstDxTilerClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxtiler_debug_category, "dxtiler", 0,
                            "dxtiler plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxtiler_set_property;
    gobject_class->get_property = dxtiler_get_property;
    gobject_class->dispose = dxtiler_dispose;

    obj_properties[PROP_CONFIG_FILE_PATH] = g_param_spec_string(
        "config-file-path", "Config File Path",
        "Path to the JSON config file containing the element's properties.",
        NULL, G_PARAM_READWRITE);

    obj_properties[PROP_WIDTH] = g_param_spec_int(
        "width", "Width", "Sets the width of each tile in the grid.", 0, 10000,
        1920, G_PARAM_READWRITE);

    obj_properties[PROP_HEIGHT] = g_param_spec_int(
        "height", "Height", "Sets the height of each tile in the grid.", 0,
        10000, 1080, G_PARAM_READWRITE);

    obj_properties[PROP_COLS] = g_param_spec_int(
        "cols", "TILE Cols", "Sets the number of columns in the grid.", 0,
        10000, 1, G_PARAM_READWRITE);

    obj_properties[PROP_ROWS] = g_param_spec_int(
        "rows", "TILE Rows", "Sets the number of rows in the grid.", 0, 10000,
        1, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "dxtiler", "Generic",
                                          "Make tiled display buffer",
                                          "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxtiler_change_state;
}

static void gst_dxtiler_init(GstDxTiler *self) {
    GstPad *sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(sinkpad, GST_DEBUG_FUNCPTR(gst_dxtiler_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxtiler_sink_event));
    GST_PAD_SET_PROXY_CAPS(sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_active(self->_srcpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_config_file_path = NULL;
    self->_width = 1920;
    self->_height = 1080;
    self->_cols = 1;
    self->_rows = 1;
    self->_last_pts = 0;
}

void copyMatToGstBuffer(const cv::Mat &mat, const GstMapInfo &map, int offset,
                        int stride) {
    int row_size = mat.cols * mat.elemSize();
    for (int row = 0; row < mat.rows; ++row) {
        memcpy(map.data + offset + row * stride, mat.ptr(row), row_size);
    }
}

static GstFlowReturn gst_dxtiler_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {
    GstDxTiler *self = GST_DXTILER(parent);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        g_error("No DXFrameMeta in GstBuffer \n");
    }

    if (self->_cols * self->_rows <= frame_meta->_stream_id) {
        g_error("output tile size lower than input streams %d \n",
                frame_meta->_stream_id);
    }

    int totalWidth = self->_cols * self->_width;
    int totalHeight = self->_rows * self->_height;

    int stride = totalWidth * 4;

    float ratioDest = (float)self->_width / self->_height;
    float ratioSrc = (float)frame_meta->_width / frame_meta->_height;
    int newWidth, newHeight;

    if (ratioSrc < ratioDest) {
        newHeight = self->_height;
        newWidth = newHeight * ratioSrc;
    } else {
        newWidth = self->_width;
        newHeight = newWidth / ratioSrc;
    }

    cv::Mat resizeFrame;
    cv::resize(frame_meta->_rgb_surface, resizeFrame,
               cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);

    int top = (int)round((self->_height - newHeight) / 2.0);
    int bottom = self->_height - newHeight - top;
    int left = (int)round((self->_width - newWidth) / 2.0);
    int right = self->_width - newWidth - left;

    cv::copyMakeBorder(resizeFrame, resizeFrame, top, bottom, left, right,
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    cv::Mat bgrxFrame(self->_height, self->_width, CV_8UC4);
    libyuv::RAWToARGB(resizeFrame.data, resizeFrame.step, bgrxFrame.data,
                      bgrxFrame.step, self->_width, self->_height);

    int row = frame_meta->_stream_id / self->_cols;
    int col = frame_meta->_stream_id % self->_cols;
    int offset = (row * self->_height * stride) + (col * self->_width * 4);

    {
        std::unique_lock<std::mutex> lock(self->_buffer_lock);
        GstMapInfo map_output;
        if (!gst_buffer_is_writable(self->_outbuffer)) {
            gst_buffer_make_writable(self->_outbuffer);
        }
        gst_buffer_map(self->_outbuffer, &map_output, GST_MAP_WRITE);
        copyMatToGstBuffer(bgrxFrame, map_output, offset, stride);
        gst_buffer_unmap(self->_outbuffer, &map_output);
    }

    if (self->_last_pts < GST_BUFFER_PTS(buf)) {
        GstBuffer *outbuffer = gst_buffer_copy_deep(self->_outbuffer);
        GST_BUFFER_PTS(outbuffer) = GST_BUFFER_PTS(buf);
        self->_last_pts = GST_BUFFER_PTS(outbuffer);
        GstFlowReturn ret = gst_pad_push(self->_srcpad, outbuffer);

        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
        }
        gst_buffer_unref(buf);
        return ret;
    } else {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }
}