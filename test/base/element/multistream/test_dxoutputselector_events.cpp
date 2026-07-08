// Phase 7 — dxoutputselector event classification tests
// Core: Direct GstElement subclass. Must correctly classify events into
// L1A (drop), L1B (broadcast), L2 (route by stream-id), and upstream wrapping.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_STR =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static GstBuffer *make_buf_with_meta(GstClockTime pts, int stream_id) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, 4, 4);
    return b;
}

static void setup_sel_stream(GstElement *sel, int stream_id) {
    char padname[32];
    snprintf(padname, sizeof(padname), "src_%d", stream_id);
    GstPad *srcpad = gst_element_get_static_pad(sel, padname);
    if (!srcpad) return;
    char sid[32];
    snprintf(sid, sizeof(sid), "stream%d", stream_id);
    gst_pad_push_event(srcpad, gst_event_new_stream_start(sid));
    GstCaps *caps = gst_caps_from_string(CAPS_STR);
    gst_pad_push_event(srcpad, gst_event_new_caps(caps));
    gst_caps_unref(caps);
    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_pad_push_event(srcpad, gst_event_new_segment(&seg));
    gst_object_unref(srcpad);
}

struct SelPipe {
    GstElement *pipe, *src, *sel;
    GstElement *queue[4], *sink[4];
    int n_out;

    void start() { gst_element_set_state(pipe, GST_STATE_PLAYING); }
    void push(GstBuffer *b) { gst_app_src_push_buffer(GST_APP_SRC(src), b); }
    GstSample *pull(int i, GstClockTime t = 2 * GST_SECOND) {
        return gst_app_sink_try_pull_sample(GST_APP_SINK(sink[i]), t);
    }
    void stop() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }
};

static SelPipe make_sel_pipe(int n_out) {
    SelPipe p = {};
    p.n_out = n_out;
    p.pipe = gst_pipeline_new(nullptr);
    p.src = gst_element_factory_make("appsrc", "src");
    p.sel = gst_element_factory_make("dxoutputselector", "sel");
    GstCaps *caps = gst_caps_from_string(CAPS_STR);
    g_object_set(p.src, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    gst_caps_unref(caps);
    gst_bin_add_many(GST_BIN(p.pipe), p.src, p.sel, nullptr);
    gst_element_link(p.src, p.sel);
    for (int i = 0; i < n_out; i++) {
        char qn[32], sn[32], pn[32];
        snprintf(qn, sizeof(qn), "q%d", i);
        snprintf(sn, sizeof(sn), "sink%d", i);
        snprintf(pn, sizeof(pn), "src_%d", i);
        p.queue[i] = gst_element_factory_make("queue", qn);
        p.sink[i] = gst_element_factory_make("appsink", sn);
        g_object_set(p.sink[i], "sync", FALSE, "async", FALSE, nullptr);
        gst_bin_add_many(GST_BIN(p.pipe), p.queue[i], p.sink[i], nullptr);
        gst_element_link(p.queue[i], p.sink[i]);
        GstPad *selSrc = gst_element_get_request_pad(p.sel, pn);
        GstPad *qSink = gst_element_get_static_pad(p.queue[i], "sink");
        gst_pad_link(selSrc, qSink);
        gst_object_unref(selSrc);
        gst_object_unref(qSink);
    }
    return p;
}

// ---------------------------------------------------------------------------
// CE_outputsel_l1a_stream_start_dropped
// Target: gst_dxoutputselector_sink_event L194-202 (L1A domain events dropped)
// MUT: remove STREAM_START from L196 → downstream receives unexpected STREAM_START
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1a_stream_start_dropped) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    EventCounter ec = {};
    attach_event_counter(selSrc0, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);

    // Inject raw STREAM_START on sink — must be dropped (L1A)
    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_stream_start("domain-internal"));
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless_equals_int(ec.n_stream_start, 0);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_l1a_caps_dropped
// Target: same L194-202 (CAPS in L1A set)
// MUT: remove CAPS from L197 → domain caps leak to downstream
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1a_caps_dropped) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    int initial_caps = 0;
    EventCounter ec = {};
    attach_event_counter(selSrc0, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);
    g_usleep(20000);
    initial_caps = ec.n_caps;

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    GstCaps *c = gst_caps_from_string("application/x-dxvideoraw,width=1,height=1,framerate=0/1,format=ANY");
    gst_pad_send_event(sinkpad, gst_event_new_caps(c));
    gst_caps_unref(c);
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless_equals_int(ec.n_caps, initial_caps);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_l1a_segment_dropped
// Target: same (SEGMENT in L1A set)
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1a_segment_dropped) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    EventCounter ec = {};
    attach_event_counter(selSrc0, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);
    g_usleep(20000);
    int initial_seg = ec.n_segment;

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_pad_send_event(sinkpad, gst_event_new_segment(&seg));
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless_equals_int(ec.n_segment, initial_seg);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_l1b_flush_broadcast
// Target: gst_dxoutputselector_sink_event L205-211 (broadcast_event for L1B)
// MUT: replace broadcast_event with drop → downstream never sees FLUSH
// Verified on 2 src pads — both must receive FLUSH_START.
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1b_flush_broadcast) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    GstPad *selSrc1 = gst_element_get_static_pad(p.sel, "src_1");
    EventCounter ec0 = {}, ec1 = {};
    GstPadProbeType flush_probe = static_cast<GstPadProbeType>(
        GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_EVENT_FLUSH);
    attach_event_counter(selSrc0, &ec0, flush_probe);
    attach_event_counter(selSrc1, &ec1, flush_probe);
    gst_object_unref(selSrc0);
    gst_object_unref(selSrc1);

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_flush_start());
    gst_pad_send_event(sinkpad, gst_event_new_flush_stop(TRUE));
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless(ec0.n_flush_start >= 1,
                "FLUSH_START must reach src_0 (got %d)", ec0.n_flush_start);
    fail_unless(ec1.n_flush_start >= 1,
                "FLUSH_START must reach src_1 (got %d)", ec1.n_flush_start);
    fail_unless(ec0.n_flush_stop >= 1,
                "FLUSH_STOP must reach src_0 (got %d)", ec0.n_flush_stop);
    fail_unless(ec1.n_flush_stop >= 1,
                "FLUSH_STOP must reach src_1 (got %d)", ec1.n_flush_stop);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_l1b_tag_broadcast
// Target: same L205-211 (TAG in L1B set)
// MUT: remove TAG from L209 → downstream never receives tags
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1b_tag_broadcast) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    GstPad *selSrc1 = gst_element_get_static_pad(p.sel, "src_1");
    EventCounter ec0 = {}, ec1 = {};
    attach_event_counter(selSrc0, &ec0, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    attach_event_counter(selSrc1, &ec1, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);
    gst_object_unref(selSrc1);

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    GstTagList *tags = gst_tag_list_new(GST_TAG_TITLE, "broadcast-test", NULL);
    gst_pad_send_event(sinkpad, gst_event_new_tag(tags));
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless(ec0.n_tag >= 1, "TAG must reach src_0 (got %d)", ec0.n_tag);
    fail_unless(ec1.n_tag >= 1, "TAG must reach src_1 (got %d)", ec1.n_tag);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_l1b_gap_broadcast
// Target: same (GAP in L1B set)
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_l1b_gap_broadcast) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    GstPad *selSrc1 = gst_element_get_static_pad(p.sel, "src_1");
    EventCounter ec0 = {}, ec1 = {};
    attach_event_counter(selSrc0, &ec0, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    attach_event_counter(selSrc1, &ec1, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);
    gst_object_unref(selSrc1);

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_gap(0, 100 * GST_MSECOND));
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless(ec0.n_gap >= 1, "GAP must reach src_0 (got %d)", ec0.n_gap);
    fail_unless(ec1.n_gap >= 1, "GAP must reach src_1 (got %d)", ec1.n_gap);

    p.stop();
}
GST_END_TEST;


// ---------------------------------------------------------------------------
// CE_outputsel_src_event_qos_wraps_upstream
// Target: gst_dxoutputselector_src_event L293-308
//   - QoS on src pad → wrapped as upstream event with stream_id
// MUT: remove L302-303 → upstream never sees per-stream QoS
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_src_event_qos_wraps_upstream) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    // Attach upstream event probe on sink pad
    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    struct { int n_wrapped_upstream; } tracker = {0};
    gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
        [](GstPad *, GstPadProbeInfo *info, gpointer ud) -> GstPadProbeReturn {
            auto *t = static_cast<decltype(&tracker)>(ud);
            GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
            if (GST_EVENT_TYPE(ev) == GST_EVENT_CUSTOM_UPSTREAM) {
                const GstStructure *s = gst_event_get_structure(ev);
                if (s && (gst_structure_has_name(s, "application/x-dx-wrapped-event-upstream") ||
                          gst_structure_has_name(s, "application/x-dx-wrapped-event"))) {
                    t->n_wrapped_upstream++;
                }
            }
            return GST_PAD_PROBE_OK;
        }, &tracker, nullptr);
    gst_object_unref(sinkpad);

    // Send QoS on src_1 pad
    GstPad *selSrc1 = gst_element_get_static_pad(p.sel, "src_1");
    GstEvent *qos = gst_event_new_qos(GST_QOS_TYPE_UNDERFLOW, 1.0,
                                       100 * GST_MSECOND, 1 * GST_SECOND);
    gst_pad_send_event(selSrc1, qos);
    gst_object_unref(selSrc1);

    g_usleep(50000);
    fail_unless(tracker.n_wrapped_upstream >= 1,
                "QoS on src_1 must be wrapped upstream (got %d)",
                tracker.n_wrapped_upstream);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_sink_query_caps_answered
// Target: gst_dxoutputselector_sink_query L324-340
//   - CAPS/ACCEPT_CAPS: answered from template (dxvideoraw)
// MUT: remove L330 → CAPS query falls through to srcpad peer → wrong caps
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_sink_query_caps_answered) {
    GstElement *sel = gst_element_factory_make("dxoutputselector", nullptr);
    fail_unless(sel != nullptr);

    // Request a src pad so the element is usable
    GstPad *src0 = gst_element_get_request_pad(sel, "src_0");
    fail_unless(src0 != nullptr);

    GstPad *sinkpad = gst_element_get_static_pad(sel, "sink");
    GstQuery *q = gst_query_new_caps(nullptr);
    gboolean ret = gst_pad_query(sinkpad, q);
    fail_unless(ret, "CAPS query on sink must succeed");

    GstCaps *result;
    gst_query_parse_caps_result(q, &result);
    fail_unless(result != nullptr && !gst_caps_is_empty(result),
                "sink CAPS query must return non-empty caps");

    GstCaps *dxvideoraw = gst_caps_from_string("application/x-dxvideoraw");
    fail_unless(gst_caps_can_intersect(result, dxvideoraw),
                "sink CAPS must include dxvideoraw domain");
    gst_caps_unref(dxvideoraw);

    gst_query_unref(q);
    gst_object_unref(sinkpad);
    gst_object_unref(src0);
    gst_object_unref(sel);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_src_query_caps_answered
// Target: gst_dxoutputselector_src_query L310-322
//   - CAPS: answered from template (video/x-raw)
// MUT: remove L316 → CAPS query proxied to sink → returns dxvideoraw
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_src_query_caps_answered) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    GstQuery *q = gst_query_new_caps(nullptr);
    gboolean ret = gst_pad_query(selSrc0, q);
    fail_unless(ret, "CAPS query on src_0 must succeed");

    GstCaps *result;
    gst_query_parse_caps_result(q, &result);
    fail_unless(result != nullptr && !gst_caps_is_empty(result),
                "src CAPS query must return non-empty caps");

    GstCaps *video_raw = gst_caps_from_string("video/x-raw");
    fail_unless(gst_caps_can_intersect(result, video_raw),
                "src_0 CAPS must include video/x-raw");
    gst_caps_unref(video_raw);

    gst_query_unref(q);
    gst_object_unref(selSrc0);

    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_outputsel_pad_release
// Target: gst_dxoutputselector_release_pad L244-254
//   - Removes pad from _srcpads map; deactivates pad; removes from element
// MUT: remove L248-249 → pad stays in map → routing to released pad crashes
// ---------------------------------------------------------------------------
GST_START_TEST(CE_outputsel_pad_release) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    // Route buffer to src_1 — works
    p.push(make_buf_with_meta(0, 1));
    GstSample *s = p.pull(1);
    fail_unless(s != nullptr, "stream_id=1 must arrive before release");
    gst_sample_unref(s);

    // Release src_1
    GstPad *src1 = gst_element_get_static_pad(p.sel, "src_1");
    fail_unless(src1 != nullptr);
    gst_element_release_request_pad(p.sel, src1);
    gst_object_unref(src1);

    // After release, buffer to stream_id=1 → dropped (no crash)
    p.push(make_buf_with_meta(100 * GST_MSECOND, 1));
    s = p.pull(1, 300 * GST_MSECOND);
    fail_unless(s == nullptr, "stream_id=1 after pad release must be dropped");

    // src_0 still works
    p.push(make_buf_with_meta(200 * GST_MSECOND, 0));
    s = p.pull(0);
    fail_unless(s != nullptr, "stream_id=0 must still work after src_1 release");
    gst_sample_unref(s);

    p.stop();
}
GST_END_TEST;

static Suite *dxoutputselector_events_suite(void) {
    Suite *s = suite_create("dxoutputselector_events");
    TCase *tc = tcase_create("event_classification");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_outputsel_l1a_stream_start_dropped);
    tcase_add_test(tc, CE_outputsel_l1a_caps_dropped);
    tcase_add_test(tc, CE_outputsel_l1a_segment_dropped);
    tcase_add_test(tc, CE_outputsel_l1b_flush_broadcast);
    tcase_add_test(tc, CE_outputsel_l1b_tag_broadcast);
    tcase_add_test(tc, CE_outputsel_l1b_gap_broadcast);
    tcase_add_test(tc, CE_outputsel_src_event_qos_wraps_upstream);
    tcase_add_test(tc, CE_outputsel_sink_query_caps_answered);
    tcase_add_test(tc, CE_outputsel_src_query_caps_answered);
    tcase_add_test(tc, CE_outputsel_pad_release);
    return s;
}

GST_CHECK_MAIN(dxoutputselector_events);
