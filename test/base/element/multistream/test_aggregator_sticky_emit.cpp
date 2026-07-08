// Aggregator subclass sticky event emission tests
//
// GstAggregator default sink_event eats STREAM_START/CAPS/SEGMENT (B.6).
// Subclasses must ensure these appear on srcpad before the first buffer.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "event_probe.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"

#include <map>
#include <mutex>

using namespace dxtest;

static const char *CAPS_RGB =
    "video/x-raw,format=RGB,width=64,height=64,framerate=30/1";

struct AggPipe {
    GstElement *pipe = nullptr;
    GstElement *agg = nullptr;
    GstElement *sink = nullptr;

    void start() { gst_element_set_state(pipe, GST_STATE_PLAYING); }

    void stop() {
        if (!pipe) return;
        gst_element_set_state(pipe, GST_STATE_NULL);
        if (agg) gst_object_unref(agg);
        if (sink) gst_object_unref(sink);
        gst_object_unref(pipe);
        pipe = agg = sink = nullptr;
    }
};

struct PadSequenceTrace {
    EventTrace events;
    std::mutex mu;
    gint next_index = 0;
    gint first_buffer_index = -1;
    std::map<GstEventType, gint> first_event_index;

    static GstPadProbeReturn probe_cb(GstPad *, GstPadProbeInfo *info,
                                      gpointer ud) {
        auto *self = static_cast<PadSequenceTrace *>(ud);
        std::lock_guard<std::mutex> lk(self->mu);
        gint idx = self->next_index++;

        if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) != 0) {
            GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
            GstEventType type = GST_EVENT_TYPE(ev);
            self->events.counts[type]++;
            self->events.order.push_back(type);
            if (self->first_event_index.count(type) == 0)
                self->first_event_index[type] = idx;
        }

        if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) != 0 &&
            self->first_buffer_index < 0) {
            self->first_buffer_index = idx;
        }

        return GST_PAD_PROBE_OK;
    }

    void attach(GstPad *pad) {
        gst_pad_add_probe(
            pad,
            (GstPadProbeType)(GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                              GST_PAD_PROBE_TYPE_BUFFER),
            probe_cb, this, nullptr);
    }

    bool before_first_buffer(GstEventType type) {
        std::lock_guard<std::mutex> lk(mu);
        auto it = first_event_index.find(type);
        return it != first_event_index.end() && first_buffer_index >= 0 &&
               it->second < first_buffer_index;
    }
};

static AggPipe make_agg_pipe(const char *agg_name) {
    AggPipe p;
    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false caps=%s ! %s name=agg ! appsink name=sink sync=false async=false",
        CAPS_RGB, agg_name);
    p.pipe = gst_parse_launch(launch, &err);
    g_free(launch);

    fail_unless(p.pipe != nullptr && err == nullptr,
                "Failed to create pipeline for %s", agg_name);
    if (err) g_error_free(err);

    p.agg = gst_bin_get_by_name(GST_BIN(p.pipe), "agg");
    p.sink = gst_bin_get_by_name(GST_BIN(p.pipe), "sink");
    fail_unless(p.agg != nullptr, "%s: aggregator not found", agg_name);
    fail_unless(p.sink != nullptr, "%s: appsink not found", agg_name);
    return p;
}

static void assert_bus_ok(GstElement *pipe, const char *agg_name) {
    GstBus *bus = gst_element_get_bus(pipe);
    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus, 0, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING));
    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
        gchar *dbg = nullptr;
        gst_message_parse_error(msg, &err, &dbg);
        gchar *text = g_strdup_printf("%s: pipeline error: %s%s%s", agg_name,
                                      err ? err->message : "unknown",
                                      dbg ? " (" : "", dbg ? dbg : "");
        if (dbg) {
            gchar *tmp = g_strconcat(text, ")", nullptr);
            g_free(text);
            text = tmp;
        }
        if (err) g_error_free(err);
        g_free(dbg);
        gst_message_unref(msg);
        gst_object_unref(bus);
        fail("%s", text);
        g_free(text);
        return;
    }
    if (msg) gst_message_unref(msg);
    gst_object_unref(bus);
}

static void assert_aggregator_sticky(const char *agg_name,
                                     void (*push_buffer)(GstElement *pipe)) {
    AggPipe p = make_agg_pipe(agg_name);
    GstPad *agg_srcpad = gst_element_get_static_pad(p.agg, "src");
    fail_unless(agg_srcpad != nullptr, "%s: src pad not found", agg_name);

    PadSequenceTrace trace;
    trace.attach(agg_srcpad);

    p.start();
    push_buffer(p.pipe);

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(p.sink),
                                                     3 * GST_SECOND);
    fail_unless(sample != nullptr, "%s: first buffer must reach appsink",
                agg_name);
    gst_sample_unref(sample);
    assert_bus_ok(p.pipe, agg_name);

    fail_unless(trace.events.has(GST_EVENT_STREAM_START),
                "%s: STREAM_START must appear on src pad", agg_name);
    fail_unless(trace.events.has(GST_EVENT_CAPS),
                "%s: CAPS must appear on src pad", agg_name);
    fail_unless(trace.events.has(GST_EVENT_SEGMENT),
                "%s: SEGMENT must appear on src pad", agg_name);

    fail_unless(trace.events.order_before(GST_EVENT_STREAM_START, GST_EVENT_CAPS),
                "%s: STREAM_START must come before CAPS", agg_name);
    fail_unless(trace.events.order_before(GST_EVENT_CAPS, GST_EVENT_SEGMENT),
                "%s: CAPS must come before SEGMENT", agg_name);

    fail_unless(trace.before_first_buffer(GST_EVENT_STREAM_START),
                "%s: STREAM_START must appear before first buffer", agg_name);
    fail_unless(trace.before_first_buffer(GST_EVENT_CAPS),
                "%s: CAPS must appear before first buffer", agg_name);
    fail_unless(trace.before_first_buffer(GST_EVENT_SEGMENT),
                "%s: SEGMENT must appear before first buffer", agg_name);
    gst_object_unref(agg_srcpad);
    p.stop();
}

static void push_gather_buffer(GstElement *pipe) {
    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    fail_unless(src != nullptr, "dxgather: source not found");

    GstBuffer *buf = make_video_buffer("RGB", 64, 64, 0);
    make_frame_meta(buf, 0, 64, 64, "RGB");
    fail_unless(gst_app_src_push_buffer(GST_APP_SRC(src), buf) == GST_FLOW_OK,
                "dxgather: push_buffer failed");
    fail_unless(gst_app_src_end_of_stream(GST_APP_SRC(src)) == GST_FLOW_OK,
                "dxgather: EOS failed");
    gst_object_unref(src);
}

GST_START_TEST(STICKY_dxgather_emits_sticky_events) {
    assert_aggregator_sticky("dxgather", push_gather_buffer);
}
GST_END_TEST;

static void push_inputselector_buffer(GstElement *pipe) {
    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    fail_unless(src != nullptr, "dxinputselector: source not found");

    GstBuffer *buf = make_video_buffer("RGB", 64, 64, 0);
    fail_unless(gst_app_src_push_buffer(GST_APP_SRC(src), buf) == GST_FLOW_OK,
                "dxinputselector: push_buffer failed");
    fail_unless(gst_app_src_end_of_stream(GST_APP_SRC(src)) == GST_FLOW_OK,
                "dxinputselector: EOS failed");
    gst_object_unref(src);
}

GST_START_TEST(STICKY_dxinputselector_emits_sticky_events) {
    assert_aggregator_sticky("dxinputselector", push_inputselector_buffer);
}
GST_END_TEST;

static Suite *aggregator_sticky_emit_suite(void) {
    Suite *s = suite_create("aggregator_sticky_emit");
    TCase *tc = tcase_create("sticky_events");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, STICKY_dxgather_emits_sticky_events);
    tcase_add_test(tc, STICKY_dxinputselector_emits_sticky_events);
    return s;
}

GST_CHECK_MAIN(aggregator_sticky_emit);
