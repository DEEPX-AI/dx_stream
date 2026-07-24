#include "gst-dxtracker.hpp"
#include "TrackerFactory.hpp"
#include "utils.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include <new>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <stdexcept>

enum class PropertyID { PROP_0, PROP_CONFIG_FILE_PATH, PROP_TRACKER_NAME, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxtracker_debug_category);
#define GST_CAT_DEFAULT gst_dxtracker_debug_category

static GstFlowReturn gst_dxtracker_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf);
static gboolean gst_dxtracker_start(GstBaseTransform *trans);
static gboolean gst_dxtracker_stop(GstBaseTransform *trans);
static gboolean gst_dxtracker_sink_event(GstBaseTransform *trans,
                                         GstEvent *event);
static gboolean gst_dxtracker_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query);
static gboolean gst_dxtracker_query(GstBaseTransform *trans,
                                    GstPadDirection direction,
                                    GstQuery *query);

G_DEFINE_TYPE(GstDxTracker, gst_dxtracker, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static void process_param(GstDxTracker *self, JsonObject *params,
                          const gchar *key) {
    JsonNode *value_node = json_object_get_member(params, key);
    GType value_type = json_node_get_value_type(value_node);
    gchar *value_str = nullptr;

    // g_print("Processing key: %s, Type: %s\n", key, g_type_name(value_type));

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
        GST_ERROR_OBJECT(self, "Config file does not exist: %s", self->_config_file_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading tracker config file: %s", self->_config_file_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        GST_ERROR_OBJECT(self, "Failed to parse config file: %s",
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
        auto *key = static_cast<const gchar *>(iter->data);
        process_param(self, params_object, key);
    }
    g_list_free(keys);
    GST_INFO_OBJECT(self, "Tracker config loaded: algorithm=%s, %zu parameters",
                    self->_tracker_name, self->_params.size());
    g_object_unref(parser);
}

static void dxtracker_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec) {
    GstDxTracker *self = GST_DXTRACKER(object);

    switch (property_id) {
    case static_cast<guint>(PropertyID::PROP_CONFIG_FILE_PATH):
        if (nullptr != self->_config_file_path)
            g_free(self->_config_file_path);
        self->_config_file_path = g_strdup(g_value_get_string(value));
        parse_config(self);
        break;

    case static_cast<guint>(PropertyID::PROP_TRACKER_NAME):
        g_free(self->_tracker_name);
        self->_tracker_name = g_value_dup_string(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxtracker_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec) {
    const GstDxTracker *self = GST_DXTRACKER(object);

    switch (property_id) {
    case static_cast<guint>(PropertyID::PROP_CONFIG_FILE_PATH):
        g_value_set_string(value, self->_config_file_path);
        break;

    case static_cast<guint>(PropertyID::PROP_TRACKER_NAME):
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

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        self->_trackers.clear();
        break;
    default:
        break;
    }

    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static void dxtracker_dispose(GObject *object) {
    GstDxTracker *self = GST_DXTRACKER(object);
    if (self->_config_file_path) {
        g_free(self->_config_file_path);
        self->_config_file_path = nullptr;
    }
    if (self->_tracker_name) {
        g_free(self->_tracker_name);
    }
    self->_tracker_name = nullptr;
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void dxtracker_finalize(GObject *object) {
    GstDxTracker *self = GST_DXTRACKER(object);
    self->_params.~map();
    self->_trackers.~map();
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_dxtracker_class_init(GstDxTrackerClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxtracker_debug_category, "dxtracker", 0,
                            "DXTracker plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxtracker_set_property;
    gobject_class->get_property = dxtracker_get_property;
    gobject_class->dispose = dxtracker_dispose;
    gobject_class->finalize = dxtracker_finalize;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<guint>(PropertyID::PROP_CONFIG_FILE_PATH)] =
        g_param_spec_string("config-file-path", "Config File Path",
                            "Path to the JSON config file containing the "
                            "tracking algorithm and parameters.",
                            nullptr, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_TRACKER_NAME)] = g_param_spec_string(
        "tracker-name", "Tracker Name",
        "Specifies the name of the tracking algorithm to use.", nullptr,
        G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<guint>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DX_VIDEORAW_CAPS_STR "; video/x-raw")));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DX_VIDEORAW_CAPS_STR "; video/x-raw")));

    gst_element_class_set_static_metadata(element_class, "DXTracker", "Generic",
                                          "Multi Object Tracking",
                                          "Yongjun Song <yjsong@deepx.ai>");

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxtracker_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxtracker_stop);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxtracker_sink_event);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxtracker_transform_ip);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxtracker_propose_allocation);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxtracker_query);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxtracker_change_state;
}

static void gst_dxtracker_init(GstDxTracker *self) {
    self->_config_file_path = nullptr;
    self->_tracker_name = g_strdup("OC_SORT");
    self->_first_frame_processed = FALSE;
    new (&self->_params) std::map<std::string, std::string, std::less<>>();
    new (&self->_trackers) std::map<int, std::unique_ptr<Tracker>>();
}

static gboolean gst_dxtracker_start(GstBaseTransform *trans) {
    GstDxTracker *self = GST_DXTRACKER(trans);
    GST_INFO_OBJECT(self, "Tracker starting with algorithm: %s", self->_tracker_name);
    return TRUE;
}

static gboolean gst_dxtracker_stop(GstBaseTransform *trans) {
    GstDxTracker *self = GST_DXTRACKER(trans);
    GST_INFO_OBJECT(self, "Tracker stopping (%zu active trackers)", self->_trackers.size());
    self->_trackers.clear();
    return TRUE;
}

static gboolean gst_dxtracker_sink_event(GstBaseTransform *trans,
                                          GstEvent *event) {
    GstDxTracker *self = GST_DXTRACKER(trans);

    if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP) {
        self->_trackers.clear();
    }

    return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
}

Eigen::Matrix<float, Eigen::Dynamic, 7>
Vector2Matrix(std::vector<std::vector<float>> data) {
    // Create an Eigen::Matrix with the same number of rows as the data and 6
    // columns
    Eigen::Matrix<float, Eigen::Dynamic, 7> matrix(data.size(), data[0].size());

    // Iterate over the rows and columns of the data vector
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[0].size(); ++j) {
            // Assign the value at position (i, j) of the matrix to the
            // corresponding value from the data vector
            matrix(i, j) = data[i][j];
        }
    }

    // Return the resulting matrix
    return matrix;
}

void track(GstDxTracker *self, DXFrameMeta *frame_meta) {
    if (self->_trackers.find(frame_meta->_stream_id) == self->_trackers.end()) {
        GST_INFO_OBJECT(self, "Initializing tracker for stream %d with algorithm: %s",
                        frame_meta->_stream_id, self->_tracker_name);
        auto tracker = TrackerFactory::createTracker(self->_tracker_name);
        if (!tracker) {
            GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                ("Unknown tracker algorithm: %s", self->_tracker_name), (NULL));
            return;
        }
        tracker->init(self->_params);
        self->_trackers[frame_meta->_stream_id] = std::move(tracker);
    }

    size_t objects_size = frame_meta->_object_meta_list.size();
    if (objects_size > 0) {
        GST_DEBUG_OBJECT(self, "Tracking %zu objects in stream %d",
                         objects_size, frame_meta->_stream_id);
        std::vector<std::vector<float>> data;
        data.clear();
        for (size_t o = 0; o < objects_size; o++) {
            const auto *object_meta = frame_meta->_object_meta_list[o];

            std::vector<float> row;
            row.clear();
            row.push_back(object_meta->_box[0]);
            row.push_back(object_meta->_box[1]);
            row.push_back(object_meta->_box[2]);
            row.push_back(object_meta->_box[3]);
            row.push_back(object_meta->_confidence);
            row.push_back(static_cast<float>(object_meta->_label));
            row.push_back(static_cast<float>(o)); // input_idx
            data.push_back(row);
        }

        std::vector<Eigen::RowVectorXf> results =
            self->_trackers[frame_meta->_stream_id]->update(
                Vector2Matrix(data));

        int assigned_count = 0;
        for (Eigen::RowVectorXf result : results) {
            auto idx = static_cast<int>(result(7));
            if (idx < 0 || idx >= static_cast<int>(objects_size)) {
                GST_WARNING_OBJECT(self, "Tracker returned invalid index %d (max=%zu)",
                                   idx, objects_size - 1);
                continue;
            }
            auto *object_meta = frame_meta->_object_meta_list[idx];

            object_meta->_track_id = static_cast<int>(result(4));
            assigned_count++;
        }

        int removed_count = 0;
        for (int i = static_cast<int>(objects_size) - 1; i >= 0; i--) {
            auto *object_meta = frame_meta->_object_meta_list[i];
            if (object_meta->_track_id == -1) {
                // Remove object from vector
                dx_release_obj_meta(object_meta);
                frame_meta->_object_meta_list.erase(frame_meta->_object_meta_list.begin() + i);
                removed_count++;
            }
        }
        
        GST_DEBUG_OBJECT(self, "Tracking results: %d IDs assigned, %d objects removed",
                         assigned_count, removed_count);
    }
}

static GstFlowReturn gst_dxtracker_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf) {
    GstDxTracker *self = GST_DXTRACKER(trans);

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)));
    auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No DXFrameMeta, passing through");
        return GST_FLOW_OK;
    }

    try {
        track(self, frame_meta);
    } catch (const std::exception &e) {
        GST_ELEMENT_ERROR(self, LIBRARY, FAILED,
                          ("Tracker exception: %s", e.what()), (NULL));
        return GST_FLOW_ERROR;
    }

    return GST_FLOW_OK;
}

static gboolean gst_dxtracker_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    GstCaps *qcaps = nullptr;
    gst_query_parse_allocation(query, &qcaps, nullptr);
    if (qcaps && dx_caps_is_videoraw(qcaps))
        gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}

static gboolean gst_dxtracker_query(GstBaseTransform *trans,
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
