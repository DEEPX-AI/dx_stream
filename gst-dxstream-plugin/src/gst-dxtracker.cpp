#include "gst-dxtracker.hpp"
#include "TrackerFactory.hpp"
#include "gst-dxmeta.hpp"
#include <glib.h>
#include <json-glib/json-glib.h>

enum { PROP_0, PROP_CONFIG_FILE_PATH, PROP_TRACKER_NAME, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxtracker_debug_category);
#define GST_CAT_DEFAULT gst_dxtracker_debug_category

static GstFlowReturn gst_dxtracker_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf);
static gboolean gst_dxtracker_start(GstBaseTransform *trans);
static gboolean gst_dxtracker_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxTracker, gst_dxtracker, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxtracker_debug_category, "gst-dxtracker", 0,
                            "debug category for gst-dxtracker element"))

static GstElementClass *parent_class = nullptr;

static void process_param(GstDxTracker *self, JsonObject *params,
                          const gchar *key) {
    JsonNode *value_node = json_object_get_member(params, key);
    GType value_type = json_node_get_value_type(value_node);
    gchar *value_str = nullptr;

    g_print("Processing key: %s, Type: %s\n", key, g_type_name(value_type));

    switch (value_type) {
    case G_TYPE_STRING:
        value_str = g_strdup(json_node_get_string(value_node));
        break;
    case G_TYPE_INT:
    case G_TYPE_INT64:
        value_str = g_strdup_printf("%ld", json_node_get_int(value_node));
        break;
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
        value_str = g_strdup_printf("%f", json_node_get_double(value_node));
        break;
    case G_TYPE_BOOLEAN:
        value_str =
            g_strdup(json_node_get_boolean(value_node) ? "true" : "false");
        break;
    default:
        return; // 지원하지 않는 타입은 무시
    }

    if (value_str) {
        self->_params[std::string(key)] = std::string(value_str);
        g_free(value_str);
    }
}

static void parse_config(GstDxTracker *self) {
    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        g_print("Config file does not exist: %s\n", self->_config_file_path);
        return;
    }

    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        g_print("Failed to parse config file: %s\n",
                error ? error->message : "unknown error");
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root_node = json_parser_get_root(parser);
    JsonObject *root_object = json_node_get_object(root_node);

    if (json_object_has_member(root_object, "tracker_name")) {
        const gchar *tracker_name =
            json_object_get_string_member(root_object, "tracker_name");
        g_object_set(self, "tracker-name", tracker_name, nullptr);
    }

    if (!json_object_has_member(root_object, "params")) {
        g_object_unref(parser);
        return;
    }

    JsonObject *params_object =
        json_object_get_object_member(root_object, "params");
    GList *keys = json_object_get_members(params_object);

    for (GList *iter = keys; iter != nullptr; iter = iter->next) {
        const gchar *key = static_cast<const gchar *>(iter->data);
        process_param(self, params_object, key);
    }
    g_list_free(keys);
    g_object_unref(parser);
}

static void dxtracker_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec) {
    GstDxTracker *self = GST_DXTRACKER(object);

    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        if (nullptr != self->_config_file_path)
            g_free(self->_config_file_path);
        self->_config_file_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;

    case PROP_TRACKER_NAME:
        self->_tracker_name = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxtracker_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec) {
    GstDxTracker *self = GST_DXTRACKER(object);

    switch (property_id) {
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;

    case PROP_TRACKER_NAME:
        g_value_set_string(value, self->_tracker_name);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static GstStateChangeReturn dxtracker_change_state(GstElement *element,
                                                   GstStateChange transition) {
    GstDxTracker *self = GST_DXTRACKER(element);
    GST_INFO_OBJECT(self, "Attempting to change state");
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxtracker_dispose(GObject *object) {
    GstDxTracker *self = GST_DXTRACKER(object);
    if (self->_config_file_path) {
        g_free(self->_config_file_path);
        self->_config_file_path = nullptr;
    }
    g_free(self->_tracker_name);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxtracker_class_init(GstDxTrackerClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxtracker_debug_category, "dxtracker", 0,
                            "DXTracker plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxtracker_set_property;
    gobject_class->get_property = dxtracker_get_property;
    gobject_class->dispose = dxtracker_dispose;

    static GParamSpec *obj_properties[N_PROPERTIES] = {
        nullptr,
    };

    obj_properties[PROP_CONFIG_FILE_PATH] =
        g_param_spec_string("config-file-path", "Config File Path",
                            "Path to the JSON config file containing the "
                            "tracking algorithm and parameters.",
                            nullptr, G_PARAM_READWRITE);

    obj_properties[PROP_TRACKER_NAME] = g_param_spec_string(
        "tracker-name", "Tracker Name",
        "Specifies the name of the tracking algorithm to use.", nullptr,
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

    gst_element_class_set_static_metadata(element_class, "DXTracker", "Generic",
                                          "Multi Object Tracking",
                                          "Song Yongjun <yjsong@deepx.ai>");

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxtracker_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxtracker_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxtracker_transform_ip);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxtracker_change_state;
}

static void gst_dxtracker_init(GstDxTracker *self) {
    self->_config_file_path = nullptr;
    self->_tracker_name = g_strdup("OC_SORT");
    self->_first_frame_processed = FALSE;
    self->_params.clear();
    self->_trackers.clear();
}

static gboolean gst_dxtracker_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    return TRUE;
}

static gboolean gst_dxtracker_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

Eigen::Matrix<float, Eigen::Dynamic, 7>
Vector2Matrix(std::vector<std::vector<float>> data) {
    // Create an Eigen::Matrix with the same number of rows as the data and 6
    // columns
    Eigen::Matrix<float, Eigen::Dynamic, 7> matrix(data.size(), data[0].size());

    // Iterate over the rows and columns of the data vector
    for (size_t i = 0; i < (size_t)data.size(); ++i) {
        for (size_t j = 0; j < (size_t)data[0].size(); ++j) {
            // Assign the value at position (i, j) of the matrix to the
            // corresponding value from the data vector
            matrix(i, j) = data[i][j];
        }
    }

    // Return the resulting matrix
    return matrix;
}

void track(GstDxTracker *self, DXFrameMeta *frame_meta, GstBuffer *buf) {
    if (self->_trackers.find(frame_meta->_stream_id) == self->_trackers.end()) {
        auto tracker = TrackerFactory::createTracker(self->_tracker_name);
        tracker->init(self->_params);
        self->_trackers[frame_meta->_stream_id] = std::move(tracker);
    }

    int objects_size = g_list_length(frame_meta->_object_meta_list);
    if (objects_size > 0) {
        std::vector<std::vector<float>> data;
        data.clear();
        for (int o = 0; o < objects_size; o++) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, o);

            std::vector<float> row;
            row.clear();
            row.push_back(static_cast<float>(object_meta->_box[0]));
            row.push_back(static_cast<float>(object_meta->_box[1]));
            row.push_back(static_cast<float>(object_meta->_box[2]));
            row.push_back(static_cast<float>(object_meta->_box[3]));
            row.push_back(static_cast<float>(object_meta->_confidence));
            row.push_back(static_cast<float>(object_meta->_label));
            row.push_back(static_cast<float>(o)); // input_idx
            data.push_back(row);
        }

        std::vector<Eigen::RowVectorXf> results =
            self->_trackers[frame_meta->_stream_id]->update(
                Vector2Matrix(data));

        for (Eigen::RowVectorXf result : results) {
            int idx = result(7);
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, idx);

            object_meta->_track_id = result(4);
        }

        for (int i = objects_size - 1; i >= 0; i--) {
            DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                frame_meta->_object_meta_list, i);
            if (object_meta->_track_id == -1) {
                GList *list = g_list_nth(frame_meta->_object_meta_list, i);
                frame_meta->_object_meta_list =
                    g_list_remove_link(frame_meta->_object_meta_list, list);
                g_list_free_1(list);
                gst_buffer_remove_meta(buf, (GstMeta *)object_meta);
            }
        }
    }
}

static GstFlowReturn gst_dxtracker_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf) {
    GstDxTracker *self = GST_DXTRACKER(trans);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }
    track(self, frame_meta, buf);

    return GST_FLOW_OK;
}
