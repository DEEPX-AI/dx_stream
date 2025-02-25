#include "gst-dxrouter.hpp"
#include "gst-dxmeta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxrouter_debug_category);
#define GST_CAT_DEFAULT gst_dxrouter_debug_category

struct _GstDxRouterClass {
    GstElementClass parent_class;
};

static gchar *generate_pad_name(gint pad_index) {
    return g_strdup_printf("src_%d", pad_index);
}

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src_%u", GST_PAD_SRC, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

static GstPad *gst_dxrouter_request_pad(GstElement *element,
                                        GstPadTemplate *templ,
                                        const gchar *name, const GstCaps *caps);
static void gst_dxrouter_release_pad(GstElement *element, GstPad *pad);

static GstFlowReturn gst_dxrouter_chain_function(GstPad *pad, GstObject *parent,
                                                 GstBuffer *buffer);
static gboolean dxrouter_sink_event(GstPad *pad, GstObject *parent,
                                    GstEvent *event);

gint get_src_pad_index(GstPad *src_pad) {
    const gchar *pad_name = gst_pad_get_name(src_pad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "src_")) {
        pad_index = atoi(pad_name + 4);
    }
    return pad_index;
}

// Class initialization
G_DEFINE_TYPE(GstDxRouter, gst_dxrouter, GST_TYPE_ELEMENT)

static void gst_dxrouter_class_init(GstDxRouterClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxrouter_debug_category, "dxrouter", 0,
                            "DXRouter plugin");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    gst_element_class_set_metadata(
        element_class, "Dynamic Router", "Generic",
        "Routes buffers to dynamic src pads based on metadata",
        "Developer <developer@example.com>");

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxrouter_request_pad);
    element_class->release_pad = GST_DEBUG_FUNCPTR(gst_dxrouter_release_pad);
}

// Instance initialization
static void gst_dxrouter_init(GstDxRouter *self) {
    self->_sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxrouter_chain_function));
    gst_pad_set_event_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(dxrouter_sink_event));
    GST_PAD_SET_PROXY_CAPS(self->_sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    self->_srcpads.clear();
}

// Handle sink pad events
static gboolean dxrouter_sink_event(GstPad *pad, GstObject *parent,
                                    GstEvent *event) {
    gboolean res = TRUE;
    if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_START ||
        GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP ||
        GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        res = gst_pad_event_default(pad, parent, event);
    } else {
        gst_event_unref(event);
    }

    return res;
}

// Request new src pad
static GstPad *gst_dxrouter_request_pad(GstElement *element,
                                        GstPadTemplate *templ,
                                        const gchar *name,
                                        const GstCaps *caps) {
    GstDxRouter *self = GST_DXROUTER(element);

    gchar *pad_name =
        name ? g_strdup(name) : generate_pad_name(self->_srcpads.size());
    GstPad *new_pad = gst_pad_new_from_template(templ, pad_name);

    gint stream_id = get_src_pad_index(new_pad);
    self->_srcpads[stream_id] = GST_PAD(gst_object_ref(new_pad));
    gst_pad_set_active(new_pad, TRUE);
    gst_element_add_pad(element, new_pad);

    GST_DEBUG_OBJECT(self, "Created new src pad: %s", pad_name);
    g_free(pad_name);
    return new_pad;
}

// Release src pad
static void gst_dxrouter_release_pad(GstElement *element, GstPad *pad) {
    GstDxRouter *self = GST_DXROUTER(element);

    gint stream_id = get_src_pad_index(pad);
    self->_srcpads.erase(stream_id);

    gst_element_remove_pad(element, pad);
    GST_DEBUG_OBJECT(self, "Released src pad");
}

static gboolean forward_events(GstPad *pad, GstEvent **event,
                               gpointer user_data) {
    GstPad *srcpad = GST_PAD_CAST(user_data);

    gst_pad_push_event(srcpad, gst_event_ref(*event));

    return TRUE;
}

// Process incoming buffers
static GstFlowReturn gst_dxrouter_chain_function(GstPad *pad, GstObject *parent,
                                                 GstBuffer *buffer) {
    GstDxRouter *self = GST_DXROUTER(parent);

    GstFlowReturn result = GST_FLOW_OK;

    if (self->_srcpads.empty()) {
        GST_ERROR_OBJECT(self, "No src pads available to push buffer");
        return GST_FLOW_ERROR;
    }

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }

    GstPad *target_pad = self->_srcpads[frame_meta->_stream_id];

    if (GST_IS_PAD(target_pad)) {
        gst_pad_sticky_events_foreach(pad, forward_events, target_pad);

        GstBuffer *buffer_copy = gst_buffer_copy(buffer);
        result = gst_pad_push(target_pad, buffer_copy);

        if (result != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer to pad %s",
                             GST_PAD_NAME(target_pad));
        }
    }

    gst_buffer_unref(buffer);
    return result;
}
