#include "gst-dxrate.hpp"

#ifndef ABSDIFF
#define ABSDIFF(a, b) (((a) > (b)) ? (a) - (b) : (b) - (a))
#endif

#define GST_DXRATE_SCALED_TIME(self, count)                                    \
    gst_util_uint64_scale(count, GST_SECOND, self->_framerate)

#define DEFAULT_THROTTLE FALSE

enum { PROP_0, PROP_THROTTLE, PROP_FRAMERATE, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxrate_debug_category);
#define GST_CAT_DEFAULT gst_dxrate_debug_category

#define THROTTLE_DELAY_RATIO (0.999)

constexpr int MAGIC_LIMIT = 25;

static GstFlowReturn gst_dxrate_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf);
static void gst_dxrate_swap_prev(GstDxRate *self, GstBuffer *buffer,
                                 gint64 time);
static GstFlowReturn gst_dxrate_flush_prev(GstDxRate *self, gboolean duplicate,
                                           GstClockTime next_intime);

static gboolean gst_dxrate_start(GstBaseTransform *trans);
static gboolean gst_dxrate_stop(GstBaseTransform *trans);
static gboolean gst_dxrate_sink_event(GstBaseTransform *trans, GstEvent *event);

G_DEFINE_TYPE_WITH_CODE(
    GstDxRate, gst_dxrate, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxrate_debug_category, "gst-dxrate", 0,
                            "debug category for gst-dxrate element"))

static GstElementClass *parent_class = nullptr;

static void dxrate_set_property(GObject *object, guint property_id,
                                const GValue *value, GParamSpec *pspec) {
    GstDxRate *self = GST_DXRATE(object);

    GST_OBJECT_LOCK(self);

    switch (property_id) {
    case PROP_THROTTLE:
        self->_throttle = g_value_get_boolean(value);
        break;
    case PROP_FRAMERATE:
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
    case PROP_THROTTLE:
        g_value_set_boolean(value, self->_throttle);
        break;
    case PROP_FRAMERATE:
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
    GST_INFO_OBJECT(self, "Attempting to change state");
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxrate_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxrate_class_init(GstDxRateClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxrate_debug_category, "dxrate", 0,
                            "DXRate plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxrate_set_property;
    gobject_class->get_property = dxrate_get_property;
    gobject_class->dispose = dxrate_dispose;

    static GParamSpec *obj_properties[N_PROPERTIES] = {
        nullptr,
    };

    obj_properties[PROP_THROTTLE] =
        g_param_spec_boolean("throttle", "Throttle",
                             "Send Throttle type QoS events to upstream "
                             "Determines whether to send Throttle QoS Events "
                             "upstream on frame drops. ",
                             DEFAULT_THROTTLE, G_PARAM_READWRITE);

    obj_properties[PROP_FRAMERATE] = g_param_spec_uint(
        "framerate", "Framerate",
        "Sets the target framerate (FPS). This property must be configured. ",
        0, 10000, 0, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(
        element_class, "DXRate", "Generic",
        "control a frame rate of tensor streams in the pipeline",
        "Yongjun Song <yjsong@deepx.ai>");

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));

    element_class->change_state = dxrate_change_state;

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxrate_transform_ip);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxrate_sink_event);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxrate_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxrate_stop);
}

static GstFlowReturn gst_dxrate_push_buffer(GstDxRate *self, GstBuffer *outbuf,
                                            gboolean duplicate,
                                            GstClockTime next_intime) {
    GstFlowReturn res;
    GstClockTime push_ts;

    GST_BUFFER_OFFSET(outbuf) = self->_out;
    GST_BUFFER_OFFSET_END(outbuf) = self->_out + 1;
    GST_BUFFER_FLAG_UNSET(outbuf, GST_BUFFER_FLAG_DISCONT);

    if (duplicate)
        GST_BUFFER_FLAG_SET(outbuf, GST_BUFFER_FLAG_GAP);
    else
        GST_BUFFER_FLAG_UNSET(outbuf, GST_BUFFER_FLAG_GAP);

    push_ts = self->_next_ts;

    self->_out++;
    self->_out_frame_count++;

    if (self->_framerate) {
        GstClockTimeDiff duration;

        duration = GST_DXRATE_SCALED_TIME(self, self->_out_frame_count);

        self->_next_ts = self->_segment.base + self->_segment.start +
                         self->_base_ts + duration;

        GST_BUFFER_DURATION(outbuf) = self->_next_ts - push_ts;
    } else {
        g_assert(GST_BUFFER_PTS_IS_VALID(outbuf));
        g_assert(GST_BUFFER_DURATION_IS_VALID(outbuf));
        g_assert(GST_BUFFER_DURATION(outbuf) != 0);

        self->_next_ts = GST_BUFFER_PTS(outbuf) + GST_BUFFER_DURATION(outbuf);
    }

    GST_BUFFER_TIMESTAMP(outbuf) = push_ts - self->_segment.base;

    res = gst_pad_push(GST_BASE_TRANSFORM_SRC_PAD(self), outbuf);

    return res;
}

static GstFlowReturn gst_dxrate_flush_prev(GstDxRate *self, gboolean duplicate,
                                           GstClockTime next_intime) {

    if (!self->_prevbuf) {
        return GST_FLOW_OK;
    }

    GstBuffer *outbuf = gst_buffer_copy_deep(self->_prevbuf);

    return gst_dxrate_push_buffer(self, outbuf, duplicate, next_intime);
}

static void gst_dxrate_swap_prev(GstDxRate *self, GstBuffer *buffer,
                                 gint64 time) {

    if (self->_prevbuf)
        gst_buffer_unref(self->_prevbuf);
    self->_prevbuf = buffer != nullptr ? gst_buffer_ref(buffer) : nullptr;
    self->_prev_ts = time;
}

static void gst_dxrate_reset(GstDxRate *self) {

    self->_out = 0;
    self->_out_frame_count = 0;

    self->_base_ts = 0;
    self->_next_ts = GST_CLOCK_TIME_NONE;
    self->_last_ts = GST_CLOCK_TIME_NONE;

    gst_dxrate_swap_prev(self, nullptr, 0);
}

static void gst_dxrate_init(GstDxRate *self) {
    gst_dxrate_reset(self);

    self->_throttle = false;

    self->_framerate = 0;

    gst_segment_init(&self->_segment, GST_FORMAT_TIME);
}

static gboolean flush_loop(GstDxRate *self, GstClockTime limit) {
    gint count = 0;
    GstFlowReturn res = GST_FLOW_OK;

    while (res == GST_FLOW_OK && count <= MAGIC_LIMIT &&
           ((GST_CLOCK_TIME_IS_VALID(limit) &&
             GST_CLOCK_TIME_IS_VALID(self->_next_ts) &&
             self->_next_ts - self->_segment.base < limit) ||
            count < 1)) {
        res = gst_dxrate_flush_prev(self, count > 0, GST_CLOCK_TIME_NONE);
        count++;
    }

    return res == GST_FLOW_OK;
}

static gboolean gst_dxrate_sink_event(GstBaseTransform *trans,
                                      GstEvent *event) {
    GstDxRate *self = GST_DXRATE(trans);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_SEGMENT: {
        GstSegment segment;
        gint seqnum;

        gst_event_copy_segment(event, &segment);
        if (segment.format != GST_FORMAT_TIME) {
            return FALSE;
        }

        if (self->_prevbuf) {
            flush_loop(self, self->_segment.stop);
            gst_dxrate_swap_prev(self, nullptr, 0);
        }

        self->_base_ts = 0;
        self->_out_frame_count = 0;
        self->_next_ts = GST_CLOCK_TIME_NONE;

        gst_segment_copy_into(&segment, &self->_segment);

        seqnum = gst_event_get_seqnum(event);
        gst_event_unref(event);
        event = gst_event_new_segment(&segment);
        gst_event_set_seqnum(event, seqnum);

        break;
    }
    case GST_EVENT_SEGMENT_DONE:
    case GST_EVENT_EOS: {
        if (GST_CLOCK_TIME_IS_VALID(self->_segment.stop)) {
            flush_loop(self, self->_segment.stop);
        } else if (self->_prevbuf) {
            if (GST_BUFFER_DURATION_IS_VALID(self->_prevbuf)) {
                GstClockTime end_ts =
                    self->_next_ts + GST_BUFFER_DURATION(self->_prevbuf);
                flush_loop(self, end_ts);
            } else {
                gst_dxrate_flush_prev(self, FALSE, GST_CLOCK_TIME_NONE);
            }
        }
        break;
    }
    case GST_EVENT_FLUSH_STOP:
        gst_dxrate_reset(self);
        break;
    case GST_EVENT_GAP:
        gst_event_unref(event);
        return TRUE;
    default:
        break;
    }

    return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
}

static gboolean gst_dxrate_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    GstDxRate *self = GST_DXRATE(trans);
    gst_dxrate_reset(self);
    return TRUE;
}

static gboolean gst_dxrate_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    GstDxRate *self = GST_DXRATE(trans);
    gst_dxrate_reset(self);
    return TRUE;
}

static GstFlowReturn gst_dxrate_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf) {
    GstDxRate *self = GST_DXRATE(trans);

    if (self->_framerate == 0) {
        g_error("[dxrate] framerate must be set");
    }

    GstFlowReturn res = GST_BASE_TRANSFORM_FLOW_DROPPED;
    GstClockTime intime, in_ts, in_dur;

    if (G_UNLIKELY(self->_segment.rate < 0.0)) {
        GST_ERROR_OBJECT(self, "Unsupported reverse playback \n");
        return GST_FLOW_ERROR;
    }

    in_ts = GST_BUFFER_TIMESTAMP(buf);
    in_dur = GST_BUFFER_DURATION(buf);

    if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
        in_ts = self->_last_ts;
        if (G_UNLIKELY(!GST_CLOCK_TIME_IS_VALID(in_ts))) {
            GST_WARNING_OBJECT(self, "Discard an invalid buffer \n");
            return GST_BASE_TRANSFORM_FLOW_DROPPED;
        }
    }

    self->_last_ts = in_ts;
    if (GST_CLOCK_TIME_IS_VALID(in_dur))
        self->_last_ts += in_dur;

    intime = in_ts + self->_segment.base;

    if (self->_prevbuf == NULL) {
        gst_dxrate_swap_prev(self, buf, intime);
        if (!GST_CLOCK_TIME_IS_VALID(self->_next_ts)) {
            self->_next_ts = intime;
            self->_base_ts = in_ts - self->_segment.start;
            self->_out_frame_count = 0;
        }
    } else {
        GstClockTime prevtime;
        gint64 diff1 = 0, diff2 = 0;
        guint count = 0;

        prevtime = self->_prev_ts;

        if (intime < prevtime)
            return GST_BASE_TRANSFORM_FLOW_DROPPED;
        do {
            GstClockTime next_ts;
            if (!GST_BUFFER_DURATION_IS_VALID(self->_prevbuf))
                GST_BUFFER_DURATION(self->_prevbuf) =
                    intime > prevtime ? intime - prevtime : 0;

            next_ts = self->_base_ts + (self->_next_ts - self->_base_ts);

            diff1 = ABSDIFF(prevtime, next_ts);
            diff2 = ABSDIFF(intime, next_ts);

            if (diff1 <= diff2) {
                GstFlowReturn r;
                count++;

                if ((r = gst_dxrate_flush_prev(self, count > 1, intime)) !=
                    GST_FLOW_OK) {
                    return r;
                }
            }
        } while (diff1 < diff2);

        if (count == 0 && self->_throttle) {
            gst_dxrate_send_qos_throttle(self, intime);
        }

        gst_dxrate_swap_prev(self, buf, intime);
    }

    return res;
}
