#include "gst-dxmsgconv.hpp"
#include "gst-dxmeta.hpp"
#include "gst-dxmsgmeta.hpp"
#include <dlfcn.h>
#include <json-glib/json-glib.h>

enum {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_LIBRARY_FILE_PATH,
    PROP_MESSAGE_INTERVAL
};

GST_DEBUG_CATEGORY_STATIC(gst_dxmsgconv_debug_category);
#define GST_CAT_DEFAULT gst_dxmsgconv_debug_category

static GstFlowReturn gst_dxmsgconv_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf);
static gboolean gst_dxmsgconv_start(GstBaseTransform *trans);
static gboolean gst_dxmsgconv_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxMsgConv, gst_dxmsgconv, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxmsgconv_debug_category, "dxmsgconv", 0,
                            "debug category for dxmsgconv element"))

static GstElementClass *parent_class = nullptr;

static GstStateChangeReturn dxmsgconv_change_state(GstElement *element,
                                                   GstStateChange transition) {
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    return result;
}

static void dxmsgconv_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void parse_config(GstDxMsgConv *self) {
    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        g_print("Config file does not exist: %s\n", self->_config_file_path);
        return;
    }

    JsonParser *parser = json_parser_new();
    GError *error = nullptr;
    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        g_warning("Failed to load config file: %s", error->message);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    if (!node) {
        g_warning("Config file has no root node");
        g_object_unref(parser);
        return;
    }

    JsonObject *object = json_node_get_object(node);

    if (json_object_has_member(object, "library_file_path")) {
        const gchar *path =
            json_object_get_string_member(object, "library_file_path");
        g_object_set(self, "library-file-path", path, nullptr);
    }

    if (json_object_has_member(object, "message_interval")) {
        gint interval = json_object_get_int_member(object, "message_interval");
        g_object_set(self, "message-interval", interval, nullptr);
    }

    g_object_unref(parser);
}

static void gst_dxmsgconv_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec) {
    GstDxMsgConv *self = GST_DXMSGCONV(object);

    switch (prop_id) {
    case PROP_CONFIG_FILE_PATH:
        g_free(self->_config_file_path);
        self->_config_file_path = g_value_dup_string(value);
        parse_config(self);
        break;
    case PROP_LIBRARY_FILE_PATH:
        g_free(self->_library_file_path);
        self->_library_file_path = g_value_dup_string(value);
        break;
    case PROP_MESSAGE_INTERVAL:
        self->_message_interval = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgconv_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec) {
    GstDxMsgConv *self = GST_DXMSGCONV(object);

    switch (prop_id) {
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;
    case PROP_LIBRARY_FILE_PATH:
        g_value_set_string(value, self->_library_file_path);
        break;
    case PROP_MESSAGE_INTERVAL:
        g_value_set_int(value, self->_message_interval);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgconv_class_init(GstDxMsgConvClass *klass) {
    GObjectClass *gobject_class = (GObjectClass *)klass;
    GstElementClass *element_class = (GstElementClass *)klass;

    gobject_class->dispose = dxmsgconv_dispose;
    gobject_class->set_property = gst_dxmsgconv_set_property;
    gobject_class->get_property = gst_dxmsgconv_get_property;

    g_object_class_install_property(
        gobject_class, PROP_CONFIG_FILE_PATH,
        g_param_spec_string("config-file-path", "Config File Path",
                            "Path to the configuration file containing private "
                            "properties for message formats. (optional).",
                            nullptr, G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, PROP_LIBRARY_FILE_PATH,
        g_param_spec_string(
            "library-file-path", "Library File Path",
            "Path to the custom message converter library. Required.", nullptr,
            G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, PROP_MESSAGE_INTERVAL,
        g_param_spec_int(
            "message-interval", "Message Interval",
            "Frame interval at which message is converted (optional).", 1,
            10000, 1, G_PARAM_READWRITE));

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxmsgconv_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxmsgconv_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_transform_ip);

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxmsgconv_change_state;

    gst_element_class_set_details_simple(element_class, "DXMsgConv", "Generic",
                                         "DX Message Converter",
                                         "JB Lim <jblim@dxsolution.kr>");
}

static void gst_dxmsgconv_init(GstDxMsgConv *self) {
    GST_TRACE_OBJECT(self, "init");

    self->_seq_id = 0;
    self->_config_file_path = nullptr;
    self->_library_file_path = nullptr;
    self->_library_handle = nullptr;
    self->_message_interval = 1;
}

static gboolean gst_dxmsgconv_start(GstBaseTransform *trans) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    GST_DEBUG_OBJECT(self, "start");

    if (self->_library_file_path == nullptr) {
        GST_ERROR_OBJECT(self, "dxmsgconv custom library is not set\n");
        return FALSE;
    }

    self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
    if (!self->_library_handle) {
        GST_ERROR_OBJECT(self, "dxmsgconv custom library: %s\n", dlerror());
        return FALSE;
    }
    self->_create_context_function = (DXMsg_CreateContextFptr)dlsym(
        self->_library_handle, "dxmsg_create_context");
    self->_delete_context_function = (DXMsg_DeleteContextFptr)dlsym(
        self->_library_handle, "dxmsg_delete_context");
    self->_convert_payload_function = (DXMsg_ConvertPayloadFptr)dlsym(
        self->_library_handle, "dxmsg_convert_payload");

    if (!self->_create_context_function || 
        !self->_delete_context_function ||
        !self->_convert_payload_function) {
        GST_ERROR_OBJECT(self, "dxmsgconv loading functions: %s\n", dlerror());
        if (self->_library_handle) {
            dlclose(self->_library_handle);
            self->_library_handle = nullptr;
        }
        return FALSE;
    }

    self->_context = self->_create_context_function();

    return TRUE;
}

static gboolean gst_dxmsgconv_stop(GstBaseTransform *trans) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    GST_DEBUG_OBJECT(trans, "stop");

    if (self->_context) {
        self->_delete_context_function(self->_context);
        self->_context = nullptr;
    }

    if (self->_library_handle) {
        dlclose(self->_library_handle);
        self->_library_handle = nullptr;
    }
    return TRUE;
}

void convert(GstDxMsgConv *self, DXFrameMeta *frame_meta, GstBuffer *buf) {
    if (self->_message_interval == 0 ||
        (self->_seq_id % self->_message_interval) == 0) {
        GstDxMsgMetaInfo meta_info;
        meta_info._frame_meta = frame_meta;
        meta_info._seq_id = self->_seq_id;
        meta_info._input_info = &self->_input_info;

        DxMsgPayload *payload =
            self->_convert_payload_function(self->_context, &meta_info);

        dx_add_payload_to_buffer(buf, payload);

    } else {
        GST_DEBUG_OBJECT(self, "skip seq:%lu, _message_interval: %d",
                         self->_seq_id, self->_message_interval);
    }
}

static GstFlowReturn gst_dxmsgconv_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);

    gst_video_info_from_caps(&self->_input_info, gst_pad_get_current_caps(GST_BASE_TRANSFORM_SINK_PAD(trans)));

    self->_seq_id++;

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }
    convert(self, frame_meta, buf);

    return GST_FLOW_OK;
}
