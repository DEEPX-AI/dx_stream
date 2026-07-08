#include "gst-dxinputselector.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils.hpp"
#include <algorithm>
#include <new>
#include <vector>

GST_DEBUG_CATEGORY_STATIC(gst_dxinputselector_debug_category);
#define GST_CAT_DEFAULT gst_dxinputselector_debug_category

enum class PropertyID { PROP_0, PROP_MAX_QUEUE_SIZE };

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink_%u", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS("video/x-raw"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR));

static GstFlowReturn gst_dxinputselector_aggregate(GstAggregator *agg,
                                                    gboolean timeout);
static gboolean gst_dxinputselector_sink_event(GstAggregator *agg,
                                                GstAggregatorPad *pad,
                                                GstEvent *event);
static gboolean gst_dxinputselector_src_event(GstAggregator *agg,
                                               GstEvent *event);
static GstBuffer *gst_dxinputselector_clip(GstAggregator *agg,
                                            GstAggregatorPad *pad,
                                            GstBuffer *buf);
static gboolean gst_dxinputselector_src_query(GstAggregator *agg,
                                               GstQuery *query);
static gboolean gst_dxinputselector_sink_query(GstAggregator *agg,
                                                GstAggregatorPad *pad,
                                                GstQuery *query);
static void gst_dxinputselector_finalize(GObject *object);

G_DEFINE_TYPE(GstDxInputSelector, gst_dxinputselector, GST_TYPE_AGGREGATOR);

static GstAggregatorClass *parent_class = nullptr;  // NOSONAR

static void gst_dxinputselector_set_property(GObject *object, guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec) {
    auto *self = GST_DXINPUTSELECTOR(object);
    if (static_cast<PropertyID>(prop_id) == PropertyID::PROP_MAX_QUEUE_SIZE) {
        self->_max_queue_size = g_value_get_uint(value);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void gst_dxinputselector_get_property(GObject *object, guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec) {
    const auto *self = GST_DXINPUTSELECTOR(object);
    if (static_cast<PropertyID>(prop_id) == PropertyID::PROP_MAX_QUEUE_SIZE) {
        g_value_set_uint(value, self->_max_queue_size);
    } else {
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void send_wrapped_eos(GstAggregator *agg, gint stream_id) {
    GstEvent *wrapped = dx_event_wrap_downstream(stream_id, gst_event_new_eos());
    GST_INFO_OBJECT(agg, "Push wrapped EOS for stream [%d]", stream_id);
    gst_pad_push_event(GST_AGGREGATOR_SRC_PAD(agg), wrapped);
}

static GstBuffer *
gst_dxinputselector_clip(GstAggregator *agg, GstAggregatorPad *pad,
                          GstBuffer *buf) {
    if (!GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(buf))) {
        gst_buffer_unref(buf);
        return NULL;
    }

    if (!dx_get_frame_meta(buf)) {
        buf = dx_create_frame_meta(buf);
        DXFrameMeta *meta = dx_get_frame_meta(buf);
        if (!meta) {
            GST_ERROR_OBJECT(agg, "Failed to create frame metadata for stream %d",
                             get_sink_pad_index(GST_PAD(pad)));
            return buf;
        }
        GstCaps *caps = gst_pad_get_current_caps(GST_PAD(pad));
        meta->_stream_id = get_sink_pad_index(GST_PAD(pad));
        if (caps) {
            const GstStructure *s = gst_caps_get_structure(caps, 0);
            meta->_name = gst_structure_get_name(s);
            const gchar *fmt = gst_structure_get_string(s, "format");
            meta->_format = fmt ? fmt : "";
            gst_structure_get_int(s, "width", &meta->_width);
            gst_structure_get_int(s, "height", &meta->_height);
            gint num, denom;
            if (gst_structure_get_fraction(s, "framerate", &num, &denom))
                meta->_frame_rate = (gfloat)num / (gfloat)denom;
            gst_caps_unref(caps);
        }
    }
    return buf;
}

static gboolean
gst_dxinputselector_sink_event(GstAggregator *agg, GstAggregatorPad *pad,
                                GstEvent *event) {
    gint stream_id = get_sink_pad_index(GST_PAD(pad));

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_STREAM_START:
    case GST_EVENT_CAPS:
    case GST_EVENT_SEGMENT:
        // Aggregator default eats these (B.6). L1A domain events are emitted
        // by GstAggregator's negotiate (push_mandatory_events) before the
        // first buffer. Here we only emit the per-stream L2 wrap.
        gst_pad_push_event(GST_AGGREGATOR_SRC_PAD(agg),
                           dx_event_wrap_downstream(stream_id,
                                                    gst_event_ref(event)));
        return GST_AGGREGATOR_CLASS(parent_class)->sink_event(agg, pad, event);

    case GST_EVENT_EOS:
        return GST_AGGREGATOR_CLASS(parent_class)->sink_event(agg, pad, event);

    case GST_EVENT_STREAM_GROUP_DONE:
        gst_event_unref(event);
        return TRUE;

    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
        // L1B: aggregator default handles upstream/downstream broadcast
        return GST_AGGREGATOR_CLASS(parent_class)->sink_event(agg, pad, event);

    default:
        // TAG / GAP / unknown → wrap per-stream so outputselector can route
        gst_pad_push_event(GST_AGGREGATOR_SRC_PAD(agg),
                           dx_event_wrap_downstream(stream_id,
                                                    gst_event_ref(event)));
        return GST_AGGREGATOR_CLASS(parent_class)->sink_event(agg, pad, event);
    }
}

static GstFlowReturn
gst_dxinputselector_aggregate(GstAggregator *agg, gboolean /*timeout*/) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(agg);

    GstClockTime min_pts = GST_CLOCK_TIME_NONE;
    GstAggregatorPad *min_pad = nullptr;
    std::vector<std::pair<GstAggregatorPad *, GstBuffer *>> peeked;
    gboolean all_done = TRUE;
    std::vector<gint> new_eos_streams;

    GST_OBJECT_LOCK(agg);
    for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
        GstAggregatorPad *pad = GST_AGGREGATOR_PAD(l->data);
        gint stream_id = get_sink_pad_index(GST_PAD(pad));

        if (gst_aggregator_pad_is_eos(pad)) {
            GstBuffer *buf = gst_aggregator_pad_peek_buffer(pad);
            if (buf) {
                peeked.emplace_back(pad, buf);
                all_done = FALSE;
            } else if (self->_stream_eos_sent.count(stream_id) == 0) {
                self->_stream_eos_sent.insert(stream_id);
                new_eos_streams.push_back(stream_id);
            }
            continue;
        }

        all_done = FALSE;
        GstBuffer *buf = gst_aggregator_pad_peek_buffer(pad);
        if (!buf) {
            GST_OBJECT_UNLOCK(agg);
            for (auto &p : peeked) gst_buffer_unref(p.second);
            return GST_AGGREGATOR_FLOW_NEED_DATA;
        }
        peeked.emplace_back(pad, buf);
    }
    GST_OBJECT_UNLOCK(agg);

    for (gint sid : new_eos_streams) {
        send_wrapped_eos(agg, sid);
    }

    if (all_done && peeked.empty()) {
        GST_DEBUG_OBJECT(self, "All streams EOS, returning GST_FLOW_EOS");
        return GST_FLOW_EOS;
    }

    for (auto &p : peeked) {
        GstClockTime pts = GST_BUFFER_PTS(p.second);
        if (min_pts == GST_CLOCK_TIME_NONE ||
            (GST_CLOCK_TIME_IS_VALID(pts) && pts < min_pts)) {
            min_pts = pts;
            min_pad = p.first;
        }
    }

    for (auto &p : peeked) gst_buffer_unref(p.second);

    if (!min_pad)
        return GST_AGGREGATOR_FLOW_NEED_DATA;

    GstBuffer *buf = gst_aggregator_pad_pop_buffer(min_pad);
    if (!buf)
        return GST_AGGREGATOR_FLOW_NEED_DATA;

    GST_LOG_OBJECT(self, "Pushing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)));
    return gst_aggregator_finish_buffer(agg, buf);
}

static gboolean gst_dxinputselector_start(GstAggregator *agg) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(agg);
    self->_stream_eos_sent.clear();
    return TRUE;
}

static gboolean gst_dxinputselector_stop(GstAggregator *agg) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(agg);
    self->_stream_eos_sent.clear();
    return TRUE;
}

static void gst_dxinputselector_release_pad(GstElement *element, GstPad *pad) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(element);
    gint stream_id = get_sink_pad_index(pad);
    GST_OBJECT_LOCK(self);
    self->_stream_eos_sent.erase(stream_id);
    GST_OBJECT_UNLOCK(self);
    GST_ELEMENT_CLASS(parent_class)->release_pad(element, pad);
}

static void gst_dxinputselector_class_init(GstDxInputSelectorClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxinputselector_debug_category,
                            "dxinputselector", 0, "DXInputSelector plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);
    auto *agg_class = GST_AGGREGATOR_CLASS(klass);

    gobject_class->set_property = gst_dxinputselector_set_property;
    gobject_class->get_property = gst_dxinputselector_get_property;
    gobject_class->finalize = gst_dxinputselector_finalize;

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_MAX_QUEUE_SIZE),
        g_param_spec_uint(
            "max-queue-size", "Max Queue Size",
            "Maximum number of buffers per stream queue", 1, G_MAXUINT, 10,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                          GST_PARAM_MUTABLE_READY)));

    gst_element_class_set_static_metadata(
        element_class, "DXInputSelector", "Generic",
        "Input Selection from Multi Channel Streams (N:1)",
        "Sangil Jo <sijo@deepx.ai>");

    gst_element_class_add_static_pad_template_with_gtype(
        element_class, &sink_template, GST_TYPE_AGGREGATOR_PAD);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    parent_class = GST_AGGREGATOR_CLASS(g_type_class_peek_parent(klass));
    element_class->release_pad = GST_DEBUG_FUNCPTR(gst_dxinputselector_release_pad);
    agg_class->aggregate = GST_DEBUG_FUNCPTR(gst_dxinputselector_aggregate);
    agg_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxinputselector_sink_event);
    agg_class->src_event = GST_DEBUG_FUNCPTR(gst_dxinputselector_src_event);
    agg_class->clip = GST_DEBUG_FUNCPTR(gst_dxinputselector_clip);
    agg_class->start = GST_DEBUG_FUNCPTR(gst_dxinputselector_start);
    agg_class->stop = GST_DEBUG_FUNCPTR(gst_dxinputselector_stop);
    agg_class->src_query = GST_DEBUG_FUNCPTR(gst_dxinputselector_src_query);
    agg_class->sink_query = GST_DEBUG_FUNCPTR(gst_dxinputselector_sink_query);
}

static void gst_dxinputselector_init(GstDxInputSelector *self) {
    // GObject zero-allocates the instance without invoking C++ constructors,
    // so the std::set member must be constructed in place before any use.
    // Skipping this crashes on MSVC (null tree sentinel) the first time the
    // set is touched (e.g. _stream_eos_sent.clear() in start()).
    new (&self->_stream_eos_sent) std::set<int>();
    self->_max_queue_size = 2;
}

static void gst_dxinputselector_finalize(GObject *object) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(object);
    self->_stream_eos_sent.~set();
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static gboolean gst_dxinputselector_src_event(GstAggregator *agg,
                                               GstEvent *event) {
    if (dx_event_is_wrapped_upstream(event)) {
        gint stream_id = -1;
        GstEvent *original = dx_event_unwrap(event, &stream_id);
        if (!original || stream_id < 0) {
            if (original)
                gst_event_unref(original);
            return FALSE;
        }
        GstPad *target = nullptr;
        GST_OBJECT_LOCK(agg);
        for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
            GstPad *pad = GST_PAD(l->data);
            if (get_sink_pad_index(pad) == stream_id) {
                target = GST_PAD(gst_object_ref(pad));
                break;
            }
        }
        GST_OBJECT_UNLOCK(agg);
        if (target) {
            gboolean ret = gst_pad_push_event(target, original);
            gst_object_unref(target);
            return ret;
        }
        gst_event_unref(original);
        return FALSE;
    }

    // Legacy wrapped events (kept transitionally) — accept both naming layouts.
    if (GST_EVENT_TYPE(event) == GST_EVENT_CUSTOM_UPSTREAM) {
        const GstStructure *s = gst_event_get_structure(event);
        if (s && gst_structure_has_name(s, "application/x-dx-wrapped-event")) {
            gint stream_id = -1;
            GstEvent *original = nullptr;
            gst_structure_get_int(s, "stream-id", &stream_id);
            gst_structure_get(s, "event", GST_TYPE_EVENT, &original, NULL);
            gst_event_unref(event);

            if (!original || stream_id < 0) {
                if (original)
                    gst_event_unref(original);
                return FALSE;
            }
            GstPad *target = nullptr;
            GST_OBJECT_LOCK(agg);
            for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
                GstPad *pad = GST_PAD(l->data);
                if (get_sink_pad_index(pad) == stream_id) {
                    target = GST_PAD(gst_object_ref(pad));
                    break;
                }
            }
            GST_OBJECT_UNLOCK(agg);
            if (target) {
                gboolean ret = gst_pad_push_event(target, original);
                gst_object_unref(target);
                return ret;
            }
            gst_event_unref(original);
            return FALSE;
        }
    }

    return GST_AGGREGATOR_CLASS(parent_class)->src_event(agg, event);
}

// Estimate frame_duration from the first sinkpad that has a valid framerate.
// Returns 0 if no framerate could be determined.
static GstClockTime estimate_frame_duration(GstAggregator *agg) {
    GstClockTime dur = 0;
    GST_OBJECT_LOCK(agg);
    for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
        GstPad *pad = GST_PAD(l->data);
        GstCaps *caps = gst_pad_get_current_caps(pad);
        if (!caps) continue;
        const GstStructure *s = gst_caps_get_structure(caps, 0);
        gint num = 0, denom = 1;
        if (s && gst_structure_get_fraction(s, "framerate", &num, &denom) &&
            num > 0 && denom > 0) {
            dur = gst_util_uint64_scale_int(GST_SECOND, denom, num);
            gst_caps_unref(caps);
            break;
        }
        gst_caps_unref(caps);
    }
    GST_OBJECT_UNLOCK(agg);
    return dur;
}

static gboolean gst_dxinputselector_src_query(GstAggregator *agg,
                                               GstQuery *query) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(agg);

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_LATENCY: {
        if (!GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query))
            return FALSE;
        gboolean live;
        GstClockTime min_lat, max_lat;
        gst_query_parse_latency(query, &live, &min_lat, &max_lat);
        GstClockTime frame_dur = estimate_frame_duration(agg);
        if (frame_dur > 0) {
            GstClockTime self_buf =
                (GstClockTime)self->_max_queue_size * frame_dur;
            min_lat += self_buf;
            if (GST_CLOCK_TIME_IS_VALID(max_lat))
                max_lat += self_buf;
        }
        gst_query_set_latency(query, live, min_lat, max_lat);
        return TRUE;
    }
    case GST_QUERY_ALLOCATION: {
        // Forward to first sink's peer
        std::vector<GstPad *> pads;
        GST_OBJECT_LOCK(agg);
        for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
            pads.push_back(GST_PAD(gst_object_ref(l->data)));
        }
        GST_OBJECT_UNLOCK(agg);
        for (GstPad *p : pads) {
            if (gst_pad_peer_query(p, query)) {
                for (GstPad *pp : pads) gst_object_unref(pp);
                return TRUE;
            }
        }
        for (GstPad *p : pads) gst_object_unref(p);
        return GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query);
    }
    default:
        return GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query);
    }
}

static gboolean gst_dxinputselector_sink_query(GstAggregator *agg,
                                                GstAggregatorPad *pad,
                                                GstQuery *query) {
    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_ALLOCATION: {
        gboolean ret =
            GST_AGGREGATOR_CLASS(parent_class)->sink_query(agg, pad, query);
        if (ret)
            gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
        return ret;
    }
    default:
        return GST_AGGREGATOR_CLASS(parent_class)->sink_query(agg, pad, query);
    }
}
