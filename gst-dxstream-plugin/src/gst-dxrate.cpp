#include "gst-dxrate.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "utils.hpp"
#include <array>
#include <map>
#include <tuple>

#ifndef ABSDIFF
#define ABSDIFF(a, b) (((a) > (b)) ? (a) - (b) : (b) - (a))
#endif

#define GST_DXRATE_SCALED_TIME(self, count)                                    \
    gst_util_uint64_scale(count, GST_SECOND, self->_framerate)

#define DEFAULT_THROTTLE FALSE

enum class PropertyID { PROP_0, PROP_THROTTLE, PROP_FRAMERATE, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxrate_debug_category);
#define GST_CAT_DEFAULT gst_dxrate_debug_category

#define THROTTLE_DELAY_RATIO (0.999)

constexpr int MAGIC_LIMIT = 25;

// Per-stream rate engine state. One instance per DXFrameMeta._stream_id (or
// stream_id=0 in NORMAL_MODE). All timing fields previously element-global.
struct RateStreamState {
    GstBuffer *prevbuf = nullptr;
    GstSegment segment;
    guint64 out_frame_count = 0;
    guint64 base_ts = 0;
    guint64 prev_ts = 0;
    guint64 next_ts = GST_CLOCK_TIME_NONE;
    guint64 last_ts = GST_CLOCK_TIME_NONE;

    RateStreamState() { gst_segment_init(&segment, GST_FORMAT_TIME); }
    ~RateStreamState() {
        if (prevbuf) gst_buffer_unref(prevbuf);
    }

    RateStreamState(const RateStreamState&) = delete;
    RateStreamState& operator=(const RateStreamState&) = delete;

    RateStreamState(RateStreamState&& o) noexcept
        : prevbuf(o.prevbuf), segment(o.segment),
          out_frame_count(o.out_frame_count), base_ts(o.base_ts),
          prev_ts(o.prev_ts), next_ts(o.next_ts), last_ts(o.last_ts) {
        o.prevbuf = nullptr;
    }
    RateStreamState& operator=(RateStreamState&& o) noexcept {
        if (this != &o) {
            if (prevbuf) gst_buffer_unref(prevbuf);
            prevbuf = o.prevbuf;  o.prevbuf = nullptr;
            segment = o.segment;
            out_frame_count = o.out_frame_count;
            base_ts = o.base_ts;
            prev_ts = o.prev_ts;
            next_ts = o.next_ts;
            last_ts = o.last_ts;
        }
        return *this;
    }
};

using RateStreamMap = std::map<int, RateStreamState>;

static inline RateStreamMap &streams_map(GstDxRate *self) {
    return *reinterpret_cast<RateStreamMap *>(self->_streams);
}

static inline RateStreamState &get_state(GstDxRate *self, int stream_id) {
    return streams_map(self)[stream_id];
}

static int buffer_stream_id(GstBuffer *buf) {
    if (!buf) return 0;
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    if (fm && fm->_stream_id >= 0 && fm->_stream_id < DX_MAX_STREAMS)
        return fm->_stream_id;
    return 0;
}

static GstFlowReturn gst_dxrate_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf);
static void gst_dxrate_swap_prev(GstDxRate *self, RateStreamState &st,
                                 GstBuffer *buffer, gint64 time);
static GstFlowReturn gst_dxrate_flush_prev(GstDxRate *self, RateStreamState &st,
                                           gboolean duplicate,
                                           GstClockTime next_intime);
static gboolean gst_dxrate_validate_and_get_timestamp(GstDxRate *self,
                                                       RateStreamState &st,
                                                       GstBuffer *buf,
                                                       GstClockTime *out_intime);
static void gst_dxrate_handle_first_buffer(GstDxRate *self, RateStreamState &st,
                                           GstBuffer *buf, GstClockTime intime,
                                           GstClockTime in_ts);
static GstFlowReturn gst_dxrate_process_buffer(GstDxRate *self,
                                                RateStreamState &st,
                                                GstBuffer *buf,
                                                GstClockTime intime);

static gboolean gst_dxrate_start(GstBaseTransform *trans);
static gboolean gst_dxrate_stop(GstBaseTransform *trans);
static gboolean gst_dxrate_sink_event(GstBaseTransform *trans, GstEvent *event);
static gboolean gst_dxrate_query(GstBaseTransform *trans, GstPadDirection direction,
                                 GstQuery *query);
static gboolean gst_dxrate_propose_allocation(GstBaseTransform *trans,
                                              GstQuery *decide_query,
                                              GstQuery *query);

G_DEFINE_TYPE(GstDxRate, gst_dxrate, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static void dxrate_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec) {
    GstDxRate *self = GST_DXRATE(object);

    GST_OBJECT_LOCK(self);

    switch (property_id) {
    case static_cast<guint>(PropertyID::PROP_THROTTLE):
        self->_throttle = g_value_get_boolean(value);
        break;
    case static_cast<guint>(PropertyID::PROP_FRAMERATE):
        self->_framerate = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }

    GST_OBJECT_UNLOCK(self);
}

static void dxrate_get_property(GObject *object, guint property_id,
                                GValue *value, GParamSpec *pspec) {
    GstDxRate *self = GST_DXRATE(object);

    GST_OBJECT_LOCK(self);

    switch (property_id) {
    case static_cast<guint>(PropertyID::PROP_THROTTLE):
        g_value_set_boolean(value, self->_throttle);
        break;
    case static_cast<guint>(PropertyID::PROP_FRAMERATE):
        g_value_set_uint(value, self->_framerate);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }

    GST_OBJECT_UNLOCK(self);
}

static void gst_dxrate_send_qos_throttle(GstDxRate *self,
                                         GstClockTime timestamp) {
    GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(&self->_parent_instance);
    GstClockTimeDiff delay;
    GstEvent *event;

    delay = GST_DXRATE_SCALED_TIME(self, 1);
    delay = (GstClockTimeDiff)(((gdouble)delay) * THROTTLE_DELAY_RATIO);

    event = gst_event_new_qos(GST_QOS_TYPE_THROTTLE, 0.9, delay, timestamp);

    gst_pad_push_event(sinkpad, event);
}

static GstStateChangeReturn dxrate_change_state(GstElement *element,
                                                GstStateChange transition) {
    GstDxRate *self = GST_DXRATE(element);
    const gchar *transition_name = gst_state_change_get_name(transition);
    GST_DEBUG_OBJECT(self, "State transition: %s", transition_name);
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_DEBUG_OBJECT(self, "State change completed: %s",
                     gst_element_state_change_return_get_name(result));
    return result;
}

static void dxrate_finalize(GObject *object) {
    GstDxRate *self = GST_DXRATE(object);
    if (self->_streams) {
        delete reinterpret_cast<RateStreamMap *>(self->_streams);
        self->_streams = nullptr;
    }
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_dxrate_class_init(GstDxRateClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxrate_debug_category, "dxrate", 0,
                            "DXRate plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxrate_set_property;
    gobject_class->get_property = dxrate_get_property;
    gobject_class->finalize = dxrate_finalize;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<guint>(PropertyID::PROP_THROTTLE)] =
        g_param_spec_boolean("throttle", "Throttle",
                             "Send Throttle type QoS events to upstream "
                             "Determines whether to send Throttle QoS Events "
                             "upstream on frame drops. ",
                             DEFAULT_THROTTLE, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_FRAMERATE)] = g_param_spec_uint(
        "framerate", "Framerate",
        "Sets the target framerate (FPS). This property must be configured. ",
        0, 10000, 0, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<guint>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));

    auto *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(
        element_class, "DXRate", "Generic",
        "control a frame rate of tensor streams in the pipeline",
        "Yongjun Song <yjsong@deepx.ai>");

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DX_VIDEORAW_CAPS_STR
                                                  "; video/x-raw")));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DX_VIDEORAW_CAPS_STR
                                                  "; video/x-raw")));

    element_class->change_state = dxrate_change_state;

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxrate_transform_ip);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxrate_sink_event);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxrate_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxrate_stop);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxrate_query);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxrate_propose_allocation);
}

static GstFlowReturn gst_dxrate_push_buffer(GstDxRate *self,
                                            RateStreamState &st,
                                            GstBuffer *outbuf,
                                            gboolean duplicate,
                                            GstClockTime next_intime) {
    std::ignore = next_intime;
    GstFlowReturn res;
    GstClockTime push_ts;

    GST_BUFFER_OFFSET(outbuf) = self->_out;
    GST_BUFFER_OFFSET_END(outbuf) = self->_out + 1;
    GST_BUFFER_FLAG_UNSET(outbuf, GST_BUFFER_FLAG_DISCONT);

    if (duplicate)
        GST_BUFFER_FLAG_SET(outbuf, GST_BUFFER_FLAG_GAP);
    else
        GST_BUFFER_FLAG_UNSET(outbuf, GST_BUFFER_FLAG_GAP);

    push_ts = st.next_ts;

    self->_out++;
    st.out_frame_count++;

    if (self->_framerate) {
        GstClockTimeDiff duration;

        duration = GST_DXRATE_SCALED_TIME(self, st.out_frame_count);

        st.next_ts = st.segment.base + st.segment.start +
                     st.base_ts + duration;

        GST_BUFFER_DURATION(outbuf) = st.next_ts - push_ts;
    } else {
        if (!GST_BUFFER_PTS_IS_VALID(outbuf) || !GST_BUFFER_DURATION_IS_VALID(outbuf) ||
            GST_BUFFER_DURATION(outbuf) == 0) {
            GST_ELEMENT_ERROR(self, STREAM, FORMAT,
                              ("Buffer has invalid PTS or duration in passthrough mode"), (NULL));
            gst_buffer_unref(outbuf);
            return GST_FLOW_ERROR;
        }

        st.next_ts = GST_BUFFER_PTS(outbuf) + GST_BUFFER_DURATION(outbuf);
    }

    GST_BUFFER_TIMESTAMP(outbuf) = push_ts - st.segment.base;

    res = gst_pad_push(GST_BASE_TRANSFORM_SRC_PAD(self), outbuf);

    return res;
}

static GstFlowReturn gst_dxrate_flush_prev(GstDxRate *self, RateStreamState &st,
                                           gboolean duplicate,
                                           GstClockTime next_intime) {

    if (!st.prevbuf) {
        return GST_FLOW_OK;
    }

    GstBuffer *outbuf = gst_buffer_copy_deep(st.prevbuf);

    return gst_dxrate_push_buffer(self, st, outbuf, duplicate, next_intime);
}

static void gst_dxrate_swap_prev(GstDxRate *self, RateStreamState &st,
                                 GstBuffer *buffer, gint64 time) {
    std::ignore = self;
    if (st.prevbuf)
        gst_buffer_unref(st.prevbuf);
    st.prevbuf = buffer != nullptr ? gst_buffer_ref(buffer) : nullptr;
    st.prev_ts = time;
}

static void gst_dxrate_reset_state(GstDxRate *self, RateStreamState &st) {
    st.out_frame_count = 0;
    st.base_ts = 0;
    st.next_ts = GST_CLOCK_TIME_NONE;
    st.last_ts = GST_CLOCK_TIME_NONE;
    gst_dxrate_swap_prev(self, st, nullptr, 0);
}

static void gst_dxrate_reset_all(GstDxRate *self) {
    self->_out = 0;
    if (self->_streams) {
        for (auto &kv : streams_map(self)) {
            gst_dxrate_reset_state(self, kv.second);
        }
        streams_map(self).clear();
    }
}

static void gst_dxrate_init(GstDxRate *self) {
    self->_streams = new RateStreamMap();
    self->_out = 0;
    self->_throttle = false;
    self->_framerate = 0;
}

static gboolean flush_loop(GstDxRate *self, RateStreamState &st,
                           GstClockTime limit) {
    gint count = 0;
    GstFlowReturn res = GST_FLOW_OK;

    while (res == GST_FLOW_OK && count <= MAGIC_LIMIT &&
           ((GST_CLOCK_TIME_IS_VALID(limit) &&
             GST_CLOCK_TIME_IS_VALID(st.next_ts) &&
             st.next_ts - st.segment.base < limit) ||
            count < 1)) {
        res = gst_dxrate_flush_prev(self, st, count > 0, GST_CLOCK_TIME_NONE);
        count++;
    }

    return res == GST_FLOW_OK;
}

// Apply a SEGMENT to the given stream's state. Mirrors the original
// SEGMENT-handling block but scoped to one stream.
static void apply_segment(GstDxRate *self, RateStreamState &st,
                          const GstSegment &segment) {
    if (st.prevbuf) {
        flush_loop(self, st, st.segment.stop);
        gst_dxrate_swap_prev(self, st, nullptr, 0);
    }
    st.base_ts = 0;
    st.out_frame_count = 0;
    st.next_ts = GST_CLOCK_TIME_NONE;
    gst_segment_copy_into(&segment, &st.segment);
}

static gboolean gst_dxrate_sink_event(GstBaseTransform *trans,
                                      GstEvent *event) {
    GstDxRate *self = GST_DXRATE(trans);

    // L2 wrapped SEGMENT (CUSTOM_DOWNSTREAM, Part C.5): unwrap and apply to
    // the per-stream state of the wrapped stream_id. The wrapped event itself
    // is still forwarded by the BaseTransform parent class.
    if (dx_event_is_wrapped_downstream(event)) {
        gint sid = -1;
        GstEvent *inner = dx_event_peek_inner(event, &sid);
        if (inner && GST_EVENT_TYPE(inner) == GST_EVENT_SEGMENT) {
            GstSegment segment;
            gst_event_copy_segment(inner, &segment);
            if (segment.format == GST_FORMAT_TIME) {
                int stream_id = (sid >= 0) ? sid : 0;
                apply_segment(self, get_state(self, stream_id), segment);
                // NOTE: multi-stream domain mode — last arriving stream's
                // segment wins for trans->segment. Per-stream timing uses
                // RateStreamState::segment, so this only affects BaseTransform
                // internals (clip/QoS). Acceptable when streams share the
                // same segment base, which is the common case.
                gst_segment_copy_into(&segment, &trans->segment);
            }
        }
        return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
    }

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_SEGMENT: {
        GstSegment segment;
        gint seqnum;

        gst_event_copy_segment(event, &segment);
        if (segment.format != GST_FORMAT_TIME) {
            GST_WARNING_OBJECT(self, "Non-TIME segment, forwarding as-is");
            break;
        }

        // Raw SEGMENT (NORMAL_MODE or domain L1A): apply to stream_id=0 as
        // default. Per-stream wrapped SEGMENT (above) handles DOMAIN_MODE.
        apply_segment(self, get_state(self, 0), segment);

        seqnum = gst_event_get_seqnum(event);
        gst_event_unref(event);
        event = gst_event_new_segment(&segment);
        gst_event_set_seqnum(event, seqnum);

        break;
    }
    case GST_EVENT_SEGMENT_DONE:
    case GST_EVENT_EOS: {
        for (auto &kv : streams_map(self)) {
            RateStreamState &st = kv.second;
            if (GST_CLOCK_TIME_IS_VALID(st.segment.stop)) {
                flush_loop(self, st, st.segment.stop);
            } else if (st.prevbuf) {
                if (GST_BUFFER_DURATION_IS_VALID(st.prevbuf)) {
                    GstClockTime end_ts =
                        st.next_ts + GST_BUFFER_DURATION(st.prevbuf);
                    flush_loop(self, st, end_ts);
                } else {
                    gst_dxrate_flush_prev(self, st, FALSE, GST_CLOCK_TIME_NONE);
                }
            }
        }
        break;
    }
    case GST_EVENT_FLUSH_STOP:
        gst_dxrate_reset_all(self);
        break;
    case GST_EVENT_GAP:
        break;
    default:
        break;
    }

    return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
}

static gboolean gst_dxrate_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    GstDxRate *self = GST_DXRATE(trans);

    if (self->_framerate == 0) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("[dxrate] framerate property must be set to a non-zero value. "
                           "Example: dxrate framerate=30"),
                          (NULL));
        return FALSE;
    }

    gst_dxrate_reset_all(self);
    return TRUE;
}

static gboolean gst_dxrate_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    GstDxRate *self = GST_DXRATE(trans);
    gst_dxrate_reset_all(self);
    return TRUE;
}

static gboolean gst_dxrate_validate_and_get_timestamp(GstDxRate *self,
                                                       RateStreamState &st,
                                                       GstBuffer *buf,
                                                       GstClockTime *out_intime) {
    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);
    GstClockTime in_dur = GST_BUFFER_DURATION(buf);

    if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
        in_ts = st.last_ts;
        if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
            GST_WARNING_OBJECT(self, "Discarding buffer with invalid timestamp");
            return FALSE;
        }
    }

    st.last_ts = in_ts;
    if (GST_CLOCK_TIME_IS_VALID(in_dur))
        st.last_ts += in_dur;

    *out_intime = in_ts + st.segment.base;
    return TRUE;
}

static void gst_dxrate_handle_first_buffer(GstDxRate *self, RateStreamState &st,
                                           GstBuffer *buf, GstClockTime intime,
                                           GstClockTime in_ts) {
    gst_dxrate_swap_prev(self, st, buf, intime);
    if (!GST_CLOCK_TIME_IS_VALID(st.next_ts)) {
        st.next_ts = intime;
        st.base_ts = in_ts - st.segment.start;
        st.out_frame_count = 0;
    }
}

static GstFlowReturn gst_dxrate_process_buffer(GstDxRate *self,
                                                RateStreamState &st,
                                                GstBuffer *buf,
                                                GstClockTime intime) {
    GstClockTime prevtime = st.prev_ts;
    gint64 diff1 = 0;
    gint64 diff2 = 0;
    guint count = 0;

    if (intime < prevtime)
        return GST_BASE_TRANSFORM_FLOW_DROPPED;

    do {
        GstClockTime next_ts;
        if (!GST_BUFFER_DURATION_IS_VALID(st.prevbuf))
            GST_BUFFER_DURATION(st.prevbuf) =
                intime > prevtime ? intime - prevtime : 0;

        next_ts = st.base_ts + (st.next_ts - st.base_ts);

        diff1 = ABSDIFF(prevtime, next_ts);
        diff2 = ABSDIFF(intime, next_ts);

        if (diff1 <= diff2) {
            GstFlowReturn r;
            count++;

            if ((r = gst_dxrate_flush_prev(self, st, count > 1, intime)) !=
                GST_FLOW_OK) {
                return r;
            }
        }
    } while (diff1 < diff2);

    if (count == 0 && self->_throttle) {
        gst_dxrate_send_qos_throttle(self, intime);
    }

    gst_dxrate_swap_prev(self, st, buf, intime);
    return GST_BASE_TRANSFORM_FLOW_DROPPED;
}

static GstFlowReturn gst_dxrate_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf) {
    GstDxRate *self = GST_DXRATE(trans);

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    if (self->_framerate == 0) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("[dxrate] framerate property must be set to a non-zero value. "
                           "Example: dxrate framerate=30"),
                          (NULL));
        return GST_FLOW_ERROR;
    }

    int stream_id = buffer_stream_id(buf);
    RateStreamState &st = get_state(self, stream_id);

    if (G_UNLIKELY(st.segment.rate < 0.0)) {
        GST_ERROR_OBJECT(self, "Unsupported reverse playback");
        return GST_FLOW_ERROR;
    }

    GstClockTime intime;
    if (!gst_dxrate_validate_and_get_timestamp(self, st, buf, &intime)) {
        GST_DEBUG_OBJECT(self, "Dropping buffer: invalid timestamp");
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
    }

    GstClockTime in_ts = GST_BUFFER_TIMESTAMP(buf);

    if (st.prevbuf == nullptr) {
        GST_DEBUG_OBJECT(self, "First buffer received for stream %d", stream_id);
        gst_dxrate_handle_first_buffer(self, st, buf, intime, in_ts);
        return GST_BASE_TRANSFORM_FLOW_DROPPED;
    }

    return gst_dxrate_process_buffer(self, st, buf, intime);
}

static gboolean gst_dxrate_query(GstBaseTransform *trans, GstPadDirection direction,
                                 GstQuery *query) {
    GstDxRate *self = GST_DXRATE(trans);

    if (direction == GST_PAD_SRC && GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        if (!GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query))
            return FALSE;

        if (self->_framerate > 0) {
            gboolean live;
            GstClockTime min_latency, max_latency;
            gst_query_parse_latency(query, &live, &min_latency, &max_latency);

            GstClockTime frame_duration =
                gst_util_uint64_scale(GST_SECOND, 1, self->_framerate);
            min_latency += frame_duration;
            if (max_latency != GST_CLOCK_TIME_NONE)
                max_latency += frame_duration;

            gst_query_set_latency(query, live, min_latency, max_latency);
        }
        return TRUE;
    }

    return GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query);
}

static gboolean gst_dxrate_propose_allocation(GstBaseTransform *trans,
                                              GstQuery *decide_query,
                                              GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}
