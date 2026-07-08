#ifndef DXSTREAM_UTILS_HPP
#define DXSTREAM_UTILS_HPP

#include <gst/gst.h>

// ---------------------------------------------------------------------------
// dxvideoraw domain caps
// ---------------------------------------------------------------------------
// Domain-internal caps used between dxinputselector and dxoutputselector.
// Real per-buffer dimensions live in DXFrameMeta — caps only carries ranges.
#define DX_VIDEORAW_CAPS_STR                                                   \
    "application/x-dxvideoraw, "                                               \
    "width=(int)[1,2147483647], "                                              \
    "height=(int)[1,2147483647], "                                             \
    "framerate=(fraction)[0/1,2147483647/1]"

#define DX_VIDEORAW_MEDIA_TYPE "application/x-dxvideoraw"

// L2 wrapped event structure names.
// Downstream: per-stream lifecycle (STREAM_START/CAPS/SEGMENT/EOS/TAG/GAP).
// Upstream:   per-stream feedback   (QOS / RECONFIGURE).
#define DX_WRAPPED_EVENT_DOWNSTREAM "application/x-dx-wrapped-event"
#define DX_WRAPPED_EVENT_UPSTREAM   "application/x-dx-wrapped-event-upstream"

// ---------------------------------------------------------------------------
// Stream ID validation
// ---------------------------------------------------------------------------
#define DX_MAX_STREAMS 64

inline bool dx_validate_stream_id(GstElement *elem, int stream_id) {
    if (stream_id < 0 || stream_id >= DX_MAX_STREAMS) {
        GST_WARNING_OBJECT(elem,
            "Invalid stream_id %d (valid: 0-%d), dropping",
            stream_id, DX_MAX_STREAMS - 1);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Pad index helpers
// ---------------------------------------------------------------------------
inline gint get_sink_pad_index(GstPad *sinkpad) {
    gchar *pad_name = gst_pad_get_name(sinkpad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "sink_")) {
        pad_index = atoi(pad_name + 5);
    }
    g_free(pad_name);
    return pad_index;
}

inline gint get_src_pad_index(GstPad *srcpad) {
    gchar *pad_name = gst_pad_get_name(srcpad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "src_")) {
        pad_index = atoi(pad_name + 4);
    }
    g_free(pad_name);
    return pad_index;
}

// ---------------------------------------------------------------------------
// dxvideoraw caps inspection
// ---------------------------------------------------------------------------
inline gboolean dx_caps_is_videoraw(const GstCaps *caps) {
    if (!caps || gst_caps_is_empty(caps) || gst_caps_is_any(caps))
        return FALSE;
    const GstStructure *s = gst_caps_get_structure(caps, 0);
    return gst_structure_has_name(s, DX_VIDEORAW_MEDIA_TYPE);
}

// ---------------------------------------------------------------------------
// L2 wrapped event helpers
// ---------------------------------------------------------------------------
// Returns a new CUSTOM_DOWNSTREAM event carrying `inner` tagged with `stream_id`.
// Takes ownership of `inner`.
inline GstEvent *dx_event_wrap_downstream(gint stream_id, GstEvent *inner) {
    GstStructure *s = gst_structure_new(DX_WRAPPED_EVENT_DOWNSTREAM,
                                        "stream-id", G_TYPE_INT, stream_id,
                                        "event", GST_TYPE_EVENT, inner, NULL);
    gst_event_unref(inner);
    return gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
}

// Returns a new CUSTOM_UPSTREAM event carrying `inner` tagged with `stream_id`.
// Takes ownership of `inner`.
inline GstEvent *dx_event_wrap_upstream(gint stream_id, GstEvent *inner) {
    GstStructure *s = gst_structure_new(DX_WRAPPED_EVENT_UPSTREAM,
                                        "stream-id", G_TYPE_INT, stream_id,
                                        "event", GST_TYPE_EVENT, inner, NULL);
    gst_event_unref(inner);
    return gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM, s);
}

inline gboolean dx_event_is_wrapped_downstream(GstEvent *e) {
    if (GST_EVENT_TYPE(e) != GST_EVENT_CUSTOM_DOWNSTREAM)
        return FALSE;
    const GstStructure *s = gst_event_get_structure(e);
    return s && gst_structure_has_name(s, DX_WRAPPED_EVENT_DOWNSTREAM);
}

inline gboolean dx_event_is_wrapped_upstream(GstEvent *e) {
    if (GST_EVENT_TYPE(e) != GST_EVENT_CUSTOM_UPSTREAM)
        return FALSE;
    const GstStructure *s = gst_event_get_structure(e);
    return s && gst_structure_has_name(s, DX_WRAPPED_EVENT_UPSTREAM);
}

// Peek at inner event without taking ownership. Returns NULL if not wrapped.
inline GstEvent *dx_event_peek_inner(GstEvent *e, gint *stream_id_out) {
    const GstStructure *s = gst_event_get_structure(e);
    if (!s)
        return NULL;
    if (stream_id_out)
        gst_structure_get_int(s, "stream-id", stream_id_out);
    const GValue *v = gst_structure_get_value(s, "event");
    return v ? GST_EVENT(g_value_get_boxed(v)) : NULL;
}

// Unwrap: returns inner event (transfer full) and consumes outer.
// Caller owns returned event.
inline GstEvent *dx_event_unwrap(GstEvent *outer, gint *stream_id_out) {
    GstEvent *inner = dx_event_peek_inner(outer, stream_id_out);
    GstEvent *ret = inner ? gst_event_ref(inner) : NULL;
    gst_event_unref(outer);
    return ret;
}

// ---------------------------------------------------------------------------
// Event classification (2-layer event model)
// ---------------------------------------------------------------------------
enum DxEventLayer {
    DX_EVENT_L1A_DOMAIN,    // domain-internal standard (STREAM_START/CAPS/SEGMENT/EOS)
    DX_EVENT_L1B_GLOBAL,    // pipeline-wide standard (FLUSH/RECONFIGURE/TAG/GAP)
    DX_EVENT_L1C_UPSTREAM,  // upstream standard (SEEK/NAVIGATION)
    DX_EVENT_L2_DOWNSTREAM, // per-stream wrapped downstream
    DX_EVENT_L2_UPSTREAM,   // per-stream wrapped upstream
    DX_EVENT_UNKNOWN,
};

inline DxEventLayer dx_event_classify(GstEvent *e) {
    if (dx_event_is_wrapped_downstream(e))
        return DX_EVENT_L2_DOWNSTREAM;
    if (dx_event_is_wrapped_upstream(e))
        return DX_EVENT_L2_UPSTREAM;
    switch (GST_EVENT_TYPE(e)) {
    case GST_EVENT_STREAM_START:
    case GST_EVENT_CAPS:
    case GST_EVENT_SEGMENT:
    case GST_EVENT_EOS:
        return DX_EVENT_L1A_DOMAIN;
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_RECONFIGURE:
    case GST_EVENT_TAG:
    case GST_EVENT_GAP:
        return DX_EVENT_L1B_GLOBAL;
    case GST_EVENT_SEEK:
    case GST_EVENT_NAVIGATION:
    case GST_EVENT_QOS:
        return DX_EVENT_L1C_UPSTREAM;
    default:
        return DX_EVENT_UNKNOWN;
    }
}

#endif // DXSTREAM_UTILS_HPP
