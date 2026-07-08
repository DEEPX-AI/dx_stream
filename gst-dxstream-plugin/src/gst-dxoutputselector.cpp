#include "gst-dxoutputselector.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils.hpp"
#include <new>
#include <vector>

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
static gboolean gst_dxoutputselector_sink_query(GstPad *pad, GstObject *parent,
                                                GstQuery *query);
static gboolean gst_dxoutputselector_src_event(GstPad *pad, GstObject *parent,
                                               GstEvent *event);
static gboolean gst_dxoutputselector_src_query(GstPad *pad, GstObject *parent,
                                               GstQuery *query);

G_DEFINE_TYPE(GstDxOutputSelector, gst_dxoutputselector, GST_TYPE_ELEMENT);
static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

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

static void dxoutputselector_finalize(GObject *object) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(object);
    self->_srcpads.~map();
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstStateChangeReturn
dxoutputselector_change_state(GstElement *element, GstStateChange transition) {
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
}

static void gst_dxoutputselector_class_init(GstDxOutputSelectorClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxoutputselector_debug_category,
                            "dxoutputselector", 0, "DXOutputSelector plugin");
    auto *element_class = GST_ELEMENT_CLASS(klass);
    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = dxoutputselector_dispose;
    gobject_class->finalize = dxoutputselector_finalize;

    gst_element_class_set_static_metadata(
        element_class, "DXOutputSelector", "Generic",
        "Routing N output stream from Input N Logical stream (1:N)",
        "Sangil Jo <sijo@deepx.ai>");

    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR));

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src_%u", GST_PAD_SRC, GST_PAD_REQUEST, GST_STATIC_CAPS("video/x-raw"));

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
    gst_pad_set_query_function(
        self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_sink_query));
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    new (&self->_srcpads) std::map<int, GstPad *>();
}

static GstPad *find_target_srcpad(GstDxOutputSelector *self, int stream_id) {
    if (stream_id == -1) {
        return nullptr;
    }

    GstPad *target = nullptr;
    GST_OBJECT_LOCK(self);
    auto it = self->_srcpads.find(stream_id);
    if (it != self->_srcpads.end() && it->second &&
        GST_PAD_IS_LINKED(it->second)) {
        target = GST_PAD(gst_object_ref(it->second));
    }
    GST_OBJECT_UNLOCK(self);
    return target;
}

static gboolean broadcast_event(GstDxOutputSelector *self, GstEvent *event) {
    std::vector<GstPad *> pads;
    GST_OBJECT_LOCK(self);
    for (auto &kv : self->_srcpads) {
        if (kv.second)
            pads.push_back(GST_PAD(gst_object_ref(kv.second)));
    }
    GST_OBJECT_UNLOCK(self);

    gboolean ret = TRUE;
    for (GstPad *p : pads) {
        ret &= gst_pad_push_event(p, gst_event_ref(event));
        gst_object_unref(p);
    }
    gst_event_unref(event);
    return ret;
}

static gboolean gst_dxoutputselector_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event) {
    std::ignore = pad;
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    // L2 wrapped downstream → unwrap → route to srcpad[stream_id]
    if (dx_event_is_wrapped_downstream(event)) {
        gint stream_id = -1;
        GstEvent *original = dx_event_unwrap(event, &stream_id);
        if (!original) {
            GST_WARNING_OBJECT(self, "Malformed wrapped event");
            return FALSE;
        }
        GstPad *target = find_target_srcpad(self, stream_id);
        if (!target) {
            GST_DEBUG_OBJECT(self,
                             "No srcpad for stream %d, dropping event %s",
                             stream_id, GST_EVENT_TYPE_NAME(original));
            gst_event_unref(original);
            return TRUE;
        }
        gboolean ret = gst_pad_push_event(target, original);
        gst_object_unref(target);
        return ret;
    }

    // Legacy wrapped event (transitional) — same routing
    if (GST_EVENT_TYPE(event) == GST_EVENT_CUSTOM_DOWNSTREAM) {
        const GstStructure *structure = gst_event_get_structure(event);
        if (structure &&
            gst_structure_has_name(structure, "application/x-dx-wrapped-event")) {
            gint stream_id = -1;
            GstEvent *original = nullptr;
            gst_structure_get_int(structure, "stream-id", &stream_id);
            gst_structure_get(structure, "event", GST_TYPE_EVENT, &original,
                              NULL);
            gst_event_unref(event);
            if (!original)
                return FALSE;
            GstPad *target = find_target_srcpad(self, stream_id);
            if (!target) {
                gst_event_unref(original);
                return TRUE;
            }
            gboolean ret = gst_pad_push_event(target, original);
            gst_object_unref(target);
            return ret;
        }
    }

    switch (GST_EVENT_TYPE(event)) {
    // L1A domain events — drop at boundary
    case GST_EVENT_STREAM_START:
    case GST_EVENT_CAPS:
    case GST_EVENT_SEGMENT:
    case GST_EVENT_EOS:
        GST_DEBUG_OBJECT(self, "Dropping L1A domain event %s",
                         GST_EVENT_TYPE_NAME(event));
        gst_event_unref(event);
        return TRUE;

    // L1B global events — broadcast to all srcpads
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_TAG:
    case GST_EVENT_GAP:
        return broadcast_event(self, event);

    case GST_EVENT_RECONFIGURE:
        // RECONFIGURE is upstream — if it arrives raw on sink, ignore
        // (normal path: src_event wraps as L2 upstream)
        GST_DEBUG_OBJECT(self, "Ignoring raw RECONFIGURE on sink (upstream event)");
        gst_event_unref(event);
        return TRUE;

    default:
        // Unknown: forward conservatively (책임 §2 "모르면 forward")
        return broadcast_event(self, event);
    }
}

static GstPad *gst_dxoutputselector_request_pad(GstElement *element,
                                                GstPadTemplate *templ,
                                                const gchar *name,
                                                const GstCaps *caps) {
    
    std::ignore = caps;

    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);

    gchar *pad_name = name ? g_strdup(name)
                           : g_strdup_printf("src_%" G_GSIZE_FORMAT, self->_srcpads.size());

    GstPad *srcpad = gst_pad_new_from_template(templ, pad_name);

    gint stream_id = get_src_pad_index(srcpad);
    gst_pad_set_active(srcpad, TRUE);
    gst_pad_set_event_function(srcpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_src_event));
    gst_pad_set_query_function(srcpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_src_query));
    gst_element_add_pad(element, srcpad);

    GST_OBJECT_LOCK(self);
    self->_srcpads[stream_id] = GST_PAD(gst_object_ref(srcpad));
    GST_OBJECT_UNLOCK(self);
    g_free(pad_name);
    return srcpad;
}

static void gst_dxoutputselector_release_pad(GstElement *element, GstPad *pad) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);
    gint stream_id = get_src_pad_index(pad);
    GST_OBJECT_LOCK(self);
    auto it = self->_srcpads.find(stream_id);
    if (it != self->_srcpads.end()) {
        gst_object_unref(it->second);
        self->_srcpads.erase(it);
    }
    GST_OBJECT_UNLOCK(self);
    gst_pad_set_active(pad, FALSE);
    gst_element_remove_pad(element, pad);
}

static GstFlowReturn gst_dxoutputselector_chain_function(GstPad *pad,
                                                         GstObject *parent,
                                                         GstBuffer *buffer) {
    
    std::ignore = pad;
    
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buffer)));

    const auto *frame_meta = dx_get_frame_meta(buffer);
    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No DXFrameMeta, dropping buffer");
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    GST_OBJECT_LOCK(self);
    auto it_pad = self->_srcpads.find(frame_meta->_stream_id);
    GstPad *target = nullptr;
    if (it_pad != self->_srcpads.end()) {
        target = GST_PAD(gst_object_ref(it_pad->second));
    }
    GST_OBJECT_UNLOCK(self);

    if (!target) {
        GST_WARNING_OBJECT(self, "No src pad for stream_id %d",
                           frame_meta->_stream_id);
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    GST_LOG_OBJECT(self, "Pushing buffer to stream %d", frame_meta->_stream_id);
    GstFlowReturn res = gst_pad_push(target, buffer);
    gst_object_unref(target);
    if (res != GST_FLOW_OK) {
        GST_WARNING_OBJECT(self, "Push failed stream [%d]: %s",
                         frame_meta->_stream_id, gst_flow_get_name(res));
        return res;
    }

    return GST_FLOW_OK;
}

static gboolean gst_dxoutputselector_src_event(GstPad *pad, GstObject *parent,
                                               GstEvent *event) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);
    gint stream_id = get_src_pad_index(pad);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS:
    case GST_EVENT_RECONFIGURE:
        // L2 wrap upstream — per-stream feedback routed via inputselector
        return gst_pad_push_event(
            self->_sinkpad, dx_event_wrap_upstream(stream_id, event));
    default:
        // L1C (SEEK / NAVIGATION / etc): forward to sink as-is
        return gst_pad_push_event(self->_sinkpad, event);
    }
}

static gboolean gst_dxoutputselector_src_query(GstPad *pad, GstObject *parent,
                                               GstQuery *query) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
    case GST_QUERY_ACCEPT_CAPS:
        // src emits video/x-raw — answer from template, don't proxy upstream dxvideoraw.
        return gst_pad_query_default(pad, parent, query);
    case GST_QUERY_LATENCY: {
        if (!gst_pad_peer_query(self->_sinkpad, query))
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
    case GST_QUERY_ALLOCATION:
        // Do NOT forward across the dxvideoraw boundary — upstream negotiated
        // application/x-dxvideoraw, downstream asks video/x-raw → caps
        // mismatch breaks pool negotiation. Answer locally (no pool).
        return gst_pad_query_default(pad, parent, query);
    default:
        return gst_pad_peer_query(self->_sinkpad, query);
    }
}

static gboolean gst_dxoutputselector_sink_query(GstPad *pad, GstObject *parent,
                                                GstQuery *query) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
    case GST_QUERY_ACCEPT_CAPS:
        // sink accepts dxvideoraw — answer from template, don't proxy.
        return gst_pad_query_default(pad, parent, query);
    default: {
        GstPad *target = nullptr;
        GST_OBJECT_LOCK(self);
        for (auto &pair : self->_srcpads) {
            if (GST_PAD_IS_LINKED(pair.second)) {
                target = GST_PAD(gst_object_ref(pair.second));
                break;
            }
        }
        GST_OBJECT_UNLOCK(self);
        if (target) {
            gboolean ret = gst_pad_peer_query(target, query);
            gst_object_unref(target);
            return ret;
        }
        return FALSE;
    }
    }
}
