#include "gst-dxmsgbroker.hpp"
#include "gst-dxmsgmeta.hpp"
#include "utils.hpp"

#include "dx_msgbrokerl_kafka.hpp"
#include "dx_msgbrokerl_mqtt.hpp"

enum class PropertyID { PROP_0, PROP_BROKER_NAME, PROP_CONN_INFO, PROP_CONFIG, PROP_TOPIC };

GST_DEBUG_CATEGORY_STATIC(gst_dxmsgbroker_debug_category);
#define GST_CAT_DEFAULT gst_dxmsgbroker_debug_category

G_DEFINE_TYPE(GstDxMsgBroker, gst_dxmsgbroker, GST_TYPE_BASE_SINK);

static void gst_dxmsgbroker_set_property(GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void gst_dxmsgbroker_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec);
static void gst_dxmsgbroker_finalize(GObject *object);

static GstStateChangeReturn
gst_dxmsgbroker_change_state(GstElement *element, GstStateChange transition);

static gboolean gst_dxmsgbroker_start(GstBaseSink *sink);
static gboolean gst_dxmsgbroker_stop(GstBaseSink *sink);
static GstFlowReturn gst_dxmsgbroker_render(GstBaseSink *sink,
                                            GstBuffer *buffer);
static gboolean gst_dxmsgbroker_event(GstBaseSink *sink, GstEvent *event);

constexpr char DXMSG_BAL_BROKER_NAME_MQTT[] = "mqtt";
constexpr char DXMSG_BAL_BROKER_NAME_KAFKA[] = "kafka";

static gboolean string_is_empty(const gchar *value) {
    return value == nullptr || value[0] == '\0';
}

/////////////////////////////////////////////////////////////////////////

static void gst_dxmsgbroker_class_init(GstDxMsgBrokerClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxmsgbroker_debug_category, "dxmsgbroker", 0,
                            "DXMsgbroker pulgin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *basesink_class = GST_BASE_SINK_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    /* GObjectClass method */
    gobject_class->set_property = gst_dxmsgbroker_set_property;
    gobject_class->get_property = gst_dxmsgbroker_get_property;
    gobject_class->finalize = gst_dxmsgbroker_finalize;

    /* install properties */
    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_BROKER_NAME),
        g_param_spec_string(
            "broker-name", "Broker Name",
            "The Name of message broker system(mqtt or kafka). Required.",
            "mqtt", G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_CONN_INFO),
        g_param_spec_string(
            "conn-info", "Connection Info",
            "Connection info in the format host:port. Required.", nullptr,
            G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_CONFIG),
        g_param_spec_string(
            "config", "Config",
            "Path to the broker configuration file. (optional).", nullptr,
            G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_TOPIC),
        g_param_spec_string("topic", "Topic",
                            "The topic name for publishing messages. Required.",
                            nullptr, G_PARAM_READWRITE));

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DX_VIDEORAW_CAPS_STR
                                                  "; video/x-raw")));

    /* GstElementClass method */
    element_class->change_state = gst_dxmsgbroker_change_state;

    /* GstBaseSinkClass method */
    basesink_class->start = GST_DEBUG_FUNCPTR(gst_dxmsgbroker_start);
    basesink_class->stop = GST_DEBUG_FUNCPTR(gst_dxmsgbroker_stop);
    basesink_class->render = GST_DEBUG_FUNCPTR(gst_dxmsgbroker_render);
    basesink_class->event = GST_DEBUG_FUNCPTR(gst_dxmsgbroker_event);

    gst_element_class_set_details_simple(element_class, "DXMsgBroker",
                                         "Generic", "DX Message Broker",
                                         "Sangil Jo <sijo@deepx.ai>");
}

static void gst_dxmsgbroker_init(GstDxMsgBroker *self) {
    self->_conn_info = nullptr;
    self->_config = nullptr;
    self->_topic = nullptr;
    self->_msgbroker_count = 0;
    self->_consecutive_failures = 0;

    self->_handle = nullptr;
    self->_broker_name = g_strdup(DXMSG_BAL_BROKER_NAME_MQTT);
    self->_connect_function = nullptr;
    self->_send_function = nullptr;
    self->_disconnect_function = nullptr;

    gst_base_sink_set_sync(GST_BASE_SINK(self), FALSE);
    gst_base_sink_set_async_enabled(GST_BASE_SINK(self), FALSE);
}

static void gst_dxmsgbroker_set_property(GObject *object, guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(object);

    switch (static_cast<PropertyID>(prop_id)) {
    case PropertyID::PROP_BROKER_NAME:
        g_free(self->_broker_name);
        self->_broker_name = g_value_dup_string(value);
        break;
    case PropertyID::PROP_CONN_INFO:
        if (self->_conn_info) {
            g_free(self->_conn_info);
        }
        self->_conn_info = g_value_dup_string(value);
        break;
    case PropertyID::PROP_CONFIG:
        if (self->_config) {
            g_free(self->_config);
        }
        self->_config = g_value_dup_string(value);
        break;
    case PropertyID::PROP_TOPIC:
        if (self->_topic) {
            g_free(self->_topic);
        }
        self->_topic = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgbroker_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(object);

    switch (static_cast<PropertyID>(prop_id)) {
    case PropertyID::PROP_BROKER_NAME:
        g_value_set_string(value, self->_broker_name);
        break;
    case PropertyID::PROP_CONN_INFO:
        g_value_set_string(value, self->_conn_info);
        break;
    case PropertyID::PROP_CONFIG:
        g_value_set_string(value, self->_config);
        break;
    case PropertyID::PROP_TOPIC:
        g_value_set_string(value, self->_topic);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgbroker_finalize(GObject *object) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(object);

    if (self->_broker_name) {
        g_free(self->_broker_name);
        self->_broker_name = nullptr;
    }
    if (self->_conn_info) {
        g_free(self->_conn_info);
        self->_conn_info = nullptr;
    }
    if (self->_config) {
        g_free(self->_config);
        self->_config = nullptr;
    }
    if (self->_topic) {
        g_free(self->_topic);
        self->_topic = nullptr;
    }
}

static GstStateChangeReturn
gst_dxmsgbroker_change_state(GstElement *element, GstStateChange transition) {
    return GST_ELEMENT_CLASS(gst_dxmsgbroker_parent_class)
              ->change_state(element, transition);
}

static gboolean gst_dxmsgbroker_start(GstBaseSink *sink) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(sink);

    if (g_strcmp0(self->_broker_name, DXMSG_BAL_BROKER_NAME_MQTT) == 0) {
        self->_connect_function = dxmsg_bal_connect_mqtt;
        self->_send_function = dxmsg_bal_send_mqtt;
        self->_disconnect_function = dxmsg_bal_disconnect_mqtt;
    } else if (g_strcmp0(self->_broker_name, DXMSG_BAL_BROKER_NAME_KAFKA) ==
               0) {
        self->_connect_function = dxmsg_bal_connect_kafka;
        self->_send_function = dxmsg_bal_send_kafka;
        self->_disconnect_function = dxmsg_bal_disconnect_kafka;
    } else {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("Invalid broker type: %s", self->_broker_name),
                          (NULL));
        return FALSE;
    }

    if (string_is_empty(self->_conn_info)) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("conn-info property is required but not set"),
                          (NULL));
        return FALSE;
    }

    if (string_is_empty(self->_topic)) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("topic property is required but not set"),
                          (NULL));
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Connecting to %s broker", self->_broker_name);
    self->_handle = self->_connect_function(self->_conn_info, self->_config);
    if (self->_handle == nullptr) {
        GST_ELEMENT_ERROR(self, RESOURCE, OPEN_READ_WRITE,
                          ("Failed to connect to %s broker", self->_broker_name),
                          (NULL));
        return FALSE;
    }
    GST_INFO_OBJECT(self, "Successfully connected to broker");

    self->_msgbroker_count = 0;
    return TRUE;
}

static gboolean gst_dxmsgbroker_stop(GstBaseSink *sink) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(sink);

    if (self->_handle && self->_disconnect_function) {
        GST_INFO_OBJECT(self, "Disconnecting from broker");
        DxMsg_Bal_Error_t error = self->_disconnect_function(self->_handle);
        if (error != DxMsg_Bal_Error::DXMSG_BAL_OK) {
            GST_WARNING_OBJECT(self, "Failed to disconnect from broker");
        }
        self->_handle = nullptr;
    }

    return TRUE;
}

static GstFlowReturn gst_dxmsgbroker_render(GstBaseSink *sink,
                                            GstBuffer *buffer) {
    GstDxMsgBroker *self = GST_DXMSGBROKER(sink);

    GST_LOG_OBJECT(self, "Render: msg #%" G_GUINT64_FORMAT, self->_msgbroker_count);
    self->_msgbroker_count++;

    static const guint MAX_CONSECUTIVE_FAILURES = 10;

    if (!self->_handle) {
        self->_consecutive_failures++;
        GST_WARNING_OBJECT(self,
            "Broker not connected, dropping message (%u/%u consecutive failures)",
            self->_consecutive_failures, MAX_CONSECUTIVE_FAILURES);
        if (self->_consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
            GST_ELEMENT_ERROR(self, RESOURCE, WRITE,
                ("Broker not connected for %u consecutive renders",
                 self->_consecutive_failures), (NULL));
            return GST_FLOW_ERROR;
        }
        return GST_FLOW_OK;
    }

    GstDxMsgMeta *meta =
        (GstDxMsgMeta *)gst_buffer_get_meta(buffer, GST_DXMSG_META_API_TYPE);

    if (meta) {
        const char *topic = self->_topic;
        const auto *payload = (DxMsgPayload *)meta->_payload;

        GST_LOG_OBJECT(self, "Publishing message (%u bytes) to topic: %s",
                         payload->_size, topic);
        DxMsg_Bal_Error_t error = self->_send_function(self->_handle, topic,
                                     payload->_data, payload->_size);
        if (error != DxMsg_Bal_Error::DXMSG_BAL_OK) {
            self->_consecutive_failures++;
            GST_WARNING_OBJECT(self,
                "Failed to publish message (%u/%u consecutive failures)",
                self->_consecutive_failures, MAX_CONSECUTIVE_FAILURES);
            if (self->_consecutive_failures >= MAX_CONSECUTIVE_FAILURES) {
                GST_ELEMENT_ERROR(self, RESOURCE, WRITE,
                    ("Broker publish failed %u consecutive times",
                     self->_consecutive_failures), (NULL));
                return GST_FLOW_ERROR;
            }
            return GST_FLOW_OK;
        }
    }

    self->_consecutive_failures = 0;
    return GST_FLOW_OK;
}

static gboolean gst_dxmsgbroker_event(GstBaseSink *sink, GstEvent *event) {
    return GST_BASE_SINK_CLASS(gst_dxmsgbroker_parent_class)
        ->event(sink, event);
}
