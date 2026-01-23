#include "gst-dxoutputselector.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxoutputselector_debug_category);
#define GST_CAT_DEFAULT gst_dxoutputselector_debug_category

static GstFlowReturn gst_dxoutputselector_chain_function(GstPad *pad,
                                                         GstObject *parent,
                                                         GstBuffer *buf);
static void gst_dxoutputselector_release_pad(GstElement *element, GstPad *pad);
static GstPad *gst_dxoutputselector_request_pad(GstElement *element,
                                                GstPadTemplate *templ,
                                                const gchar *req_name,
                                                const GstCaps *caps);
static gboolean gst_dxoutputselector_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event);

G_DEFINE_TYPE(GstDxOutputSelector, gst_dxoutputselector, GST_TYPE_ELEMENT);
static GstElementClass *parent_class = nullptr;

static void dxoutputselector_dispose(GObject *object) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(object);
    
    for (auto &pair : self->_srcpads) {
        if (GST_IS_PAD(pair.second)) {
            gst_object_unref(pair.second);
            pair.second = nullptr;
        }
    }
    
    self->_srcpads.clear();
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static GstStateChangeReturn
dxoutputselector_change_state(GstElement *element, GstStateChange transition) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);

    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        break;
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

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void gst_dxoutputselector_class_init(GstDxOutputSelectorClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxoutputselector_debug_category,
                            "dxoutputselector", 0, "DXOutputSelector plugin");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = dxoutputselector_dispose;

    gst_element_class_set_static_metadata(
        element_class, "DXOutputSelector", "Generic",
        "Routing N output stream from Input N Logical stream (1:N)",
        "Jo Sangil <sijo@deepx.ai>");

    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src_%u", GST_PAD_SRC, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxoutputselector_request_pad);
    element_class->release_pad =
        GST_DEBUG_FUNCPTR(gst_dxoutputselector_release_pad);
    element_class->change_state = dxoutputselector_change_state;
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
}

static void gst_dxoutputselector_init(GstDxOutputSelector *self) {
    self->_sinkpad = gst_pad_new("sink", GST_PAD_SINK);
    gst_pad_set_chain_function(
        self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_chain_function));
    gst_pad_set_event_function(
        self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_sink_event));
    GST_PAD_SET_PROXY_CAPS(self->_sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    self->_srcpads.clear();
}

static GstPad *find_target_srcpad(GstDxOutputSelector *self, int stream_id) {
    if (stream_id == -1) {
        return nullptr;
    }

    auto it = self->_srcpads.find(stream_id);
    if (it != self->_srcpads.end() && it->second &&
        GST_PAD_IS_LINKED(it->second)) {
        return it->second;
    }
    return nullptr;
}

static gboolean gst_dxoutputselector_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);
    // g_print("OUTPUT_SELECTOR_RECEIVED_EVENT: %s \t %d \n", GST_EVENT_TYPE_NAME(event), self->_last_stream_id);
    gboolean res = TRUE;

    const GstEventType event_type = GST_EVENT_TYPE(event);

    switch (event_type) {
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        const GstStructure *structure = gst_event_get_structure(event);
        if (gst_structure_has_name(structure, "application/x-dx-wrapped-event")) {
            int stream_id = -1;
            GstEvent *original_event = nullptr;
            gst_structure_get_int(structure, "stream-id", &stream_id);
            gst_structure_get(structure, "event", GST_TYPE_EVENT, &original_event, NULL);

            GST_INFO_OBJECT(self, "Received custom event from stream [%d], type: %s", stream_id, GST_EVENT_TYPE_NAME(original_event));

            GstPad *target_pad = find_target_srcpad(self, stream_id);

            if (!target_pad) {
                GST_INFO_OBJECT(self, "No target srcpad found for stream [%d]", stream_id);
                gst_event_unref(original_event);
                gst_event_unref(event);
                return FALSE;
            }

            res = gst_pad_push_event(target_pad, original_event);
            if (!res) {
                GST_ERROR_OBJECT(self, "Failed to push Event\n");
                gst_event_unref(original_event);
                gst_event_unref(event);
                return FALSE;
            }
            gst_event_unref(event);
            return res;
        } else {
            GST_WARNING_OBJECT(self, "Custom event has no structure");
            gst_event_unref(event);
            res = FALSE;
        }
        break;
    }
    default: {
        GST_INFO_OBJECT(self, "Received Global EOS event from stream");
        gst_event_unref(event);
        return TRUE;
    }
    }

    return res;
}

static GstPad *gst_dxoutputselector_request_pad(GstElement *element,
                                                GstPadTemplate *templ,
                                                const gchar *name,
                                                const GstCaps *caps) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);

    gchar *pad_name = name ? g_strdup(name)
                           : g_strdup_printf("src_%ld", self->_srcpads.size());

    GstPad *srcpad = gst_pad_new_from_template(templ, pad_name);

    gint stream_id = get_src_pad_index(srcpad);
    gst_pad_set_active(srcpad, TRUE);
    GST_PAD_SET_PROXY_CAPS(srcpad);
    gst_element_add_pad(element, srcpad);

    self->_srcpads[stream_id] = GST_PAD(gst_object_ref(srcpad));
    g_free(pad_name);
    return srcpad;
}

static void gst_dxoutputselector_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static GstFlowReturn gst_dxoutputselector_chain_function(GstPad *pad,
                                                         GstObject *parent,
                                                         GstBuffer *buffer) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }

    auto it_pad = self->_srcpads.find(frame_meta->_stream_id);
    if (it_pad == self->_srcpads.end()) {
        GST_ERROR_OBJECT(self,
                         "Invalid stream_id: %u — no matching src pad found\n",
                         frame_meta->_stream_id);
        return GST_FLOW_ERROR;
    }

    GstFlowReturn res = gst_pad_push(self->_srcpads[frame_meta->_stream_id], buffer);
    if (res != GST_FLOW_OK) {
        GST_ERROR_OBJECT(self, "Failed to push buffer to stream [%d], res: %d", frame_meta->_stream_id, res);
        return res;
    }

    return GST_FLOW_OK;
}
