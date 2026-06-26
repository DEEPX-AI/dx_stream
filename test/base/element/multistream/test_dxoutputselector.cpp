// P4.2 — dxoutputselector contract tests
// Core: Direct GstElement subclass, 1:N branching. stream_id-based routing.
// No DXFrameMeta → drop. Unmapped stream_id → drop.
// Wrapped events are routed by stream_id, global EOS is consumed (dropped).

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

static GstBuffer *make_buf(GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

static GstBuffer *make_buf_with_meta(GstClockTime pts, int stream_id) {
    GstBuffer *b = make_buf(pts);
    make_frame_meta(b, stream_id, 4, 4);
    return b;
}

struct SelPipe {
    GstElement *pipe, *src, *sel;
    GstElement *queue[4], *sink[4];
    int n_out;

    void start() { gst_element_set_state(pipe, GST_STATE_PLAYING); }

    void push(GstBuffer *b) {
        gst_app_src_push_buffer(GST_APP_SRC(src), b);
    }

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
    g_object_set(p.src, "format", GST_FORMAT_TIME,
                 "is-live", FALSE, "caps", caps, nullptr);
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

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxoutputselector", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxoutputselector", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_outputsel_no_meta_drop: buffer without DXFrameMeta → drop
// Target: gst_dxoutputselector_chain_function L231-234
// MUT: remove L231-234 → null deref crash
GST_START_TEST(CE_outputsel_no_meta_drop) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    // push buffer without meta → drop
    p.push(make_buf(0));

    GstSample *s = p.pull(0, 500 * GST_MSECOND);
    fail_unless(s == nullptr, "no-meta buffer must be dropped");

    // push buffer with meta → arrives (verify pipeline is alive)
    p.push(make_buf_with_meta(100 * GST_MSECOND, 0));
    s = p.pull(0);
    fail_unless(s != nullptr, "meta buffer must arrive at sink0");
    gst_sample_unref(s);

    p.stop();
}
GST_END_TEST;

// CE_outputsel_unknown_stream_drop: unmapped stream_id → drop
// Target: gst_dxoutputselector_chain_function L238-242
// MUT: remove L238-242 → crash (map lookup failure)
GST_START_TEST(CE_outputsel_unknown_stream_drop) {
    SelPipe p = make_sel_pipe(1);  // only src_0 exists
    p.start();
    setup_sel_stream(p.sel, 0);

    // stream_id=99 → src_99 does not exist, so drop
    p.push(make_buf_with_meta(0, 99));

    GstSample *s = p.pull(0, 500 * GST_MSECOND);
    fail_unless(s == nullptr, "unknown stream_id buffer must be dropped");

    p.stop();
}
GST_END_TEST;

// CE_outputsel_correct_routing: correct srcpad routing per stream_id
// Target: gst_dxoutputselector_chain_function L237, L246
// MUT: remove L246 → buffer not delivered
GST_START_TEST(CE_outputsel_correct_routing) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    // stream_id=0 → arrives only at sink0
    p.push(make_buf_with_meta(0, 0));
    GstSample *s0 = p.pull(0);
    fail_unless(s0 != nullptr, "stream_id=0 must arrive at sink0");
    GstSample *s1 = p.pull(1, 300 * GST_MSECOND);
    fail_unless(s1 == nullptr, "stream_id=0 must NOT arrive at sink1");
    gst_sample_unref(s0);

    // stream_id=1 → arrives only at sink1
    p.push(make_buf_with_meta(100 * GST_MSECOND, 1));
    s1 = p.pull(1);
    fail_unless(s1 != nullptr, "stream_id=1 must arrive at sink1");
    s0 = p.pull(0, 300 * GST_MSECOND);
    fail_unless(s0 == nullptr, "stream_id=1 must NOT arrive at sink0");
    gst_sample_unref(s1);

    p.stop();
}
GST_END_TEST;

// CE_outputsel_wrapped_event_routing: wrapped event → routed to matching srcpad
// Target: gst_dxoutputselector_sink_event L139-166
// MUT: remove L149 (find_target_srcpad) → null deref
GST_START_TEST(CE_outputsel_wrapped_event_routing) {
    SelPipe p = make_sel_pipe(2);
    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 1);

    // attach event probe to src_1 pad
    GstPad *selSrc1 = gst_element_get_static_pad(p.sel, "src_1");
    fail_unless(selSrc1 != nullptr);
    EventCounter ec = {};
    attach_event_counter(selSrc1, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc1);

    // create wrapped CAPS event → send to sink pad
    GstCaps *caps = gst_caps_from_string(CAPS_STR);
    GstEvent *caps_ev = gst_event_new_caps(caps);
    gst_caps_unref(caps);

    GstStructure *ws = gst_structure_new("application/x-dx-wrapped-event",
        "stream-id", G_TYPE_INT, 1,
        "event", GST_TYPE_EVENT, caps_ev, NULL);
    GstEvent *wrapped = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, ws);
    gst_event_unref(caps_ev);

    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    gst_pad_send_event(sinkpad, wrapped);
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless(ec.n_caps >= 1,
                "wrapped CAPS must be unwrapped to src_1 (got %d caps events)",
                ec.n_caps);

    p.stop();
}
GST_END_TEST;

// CE_outputsel_global_eos_consumed: non-wrapped EOS → consumed (dropped)
// Target: gst_dxoutputselector_sink_event L172-175
// MUT: remove drop logic → EOS propagates downstream
GST_START_TEST(CE_outputsel_global_eos_consumed) {
    SelPipe p = make_sel_pipe(1);
    p.start();
    setup_sel_stream(p.sel, 0);

    // event probe on src_0
    GstPad *selSrc0 = gst_element_get_static_pad(p.sel, "src_0");
    EventCounter ec = {};
    attach_event_counter(selSrc0, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(selSrc0);

    // raw EOS → sink pad
    GstPad *sinkpad = gst_element_get_static_pad(p.sel, "sink");
    gst_pad_send_event(sinkpad, gst_event_new_eos());
    gst_object_unref(sinkpad);

    g_usleep(50000);
    fail_unless_equals_int(ec.n_eos, 0);

    p.stop();
}
GST_END_TEST;

// CE_outputsel_request_pad_registration: request pad → registered in _srcpads map
// Target: gst_dxoutputselector_request_pad L195, L202
// MUT: remove L202 → chain cannot find pad → drop
GST_START_TEST(CE_outputsel_request_pad_registration) {
    SelPipe p = make_sel_pipe(1);
    // only src_0 exists, request src_2 additionally
    GstPad *src2 = gst_element_get_request_pad(p.sel, "src_2");
    fail_unless(src2 != nullptr, "request src_2 must succeed");

    // link src_2 to queue→appsink
    GstElement *q2 = gst_element_factory_make("queue", "q2");
    GstElement *sink2 = gst_element_factory_make("appsink", "sink2");
    g_object_set(sink2, "sync", FALSE, nullptr);
    gst_bin_add_many(GST_BIN(p.pipe), q2, sink2, nullptr);
    gst_element_link(q2, sink2);

    GstPad *qSink = gst_element_get_static_pad(q2, "sink");
    gst_pad_link(src2, qSink);
    gst_object_unref(qSink);
    gst_object_unref(src2);

    p.start();
    setup_sel_stream(p.sel, 0);
    setup_sel_stream(p.sel, 2);

    // stream_id=2 → must arrive at src_2
    p.push(make_buf_with_meta(0, 2));
    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink2), 2 * GST_SECOND);
    fail_unless(s != nullptr, "stream_id=2 must arrive at src_2");
    gst_sample_unref(s);

    // must not arrive at src_0
    s = p.pull(0, 300 * GST_MSECOND);
    fail_unless(s == nullptr, "stream_id=2 must NOT arrive at src_0");

    p.stop();
}
GST_END_TEST;

static Suite *dxoutputselector_suite(void) {
    Suite *s = suite_create("dxoutputselector");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_outputsel_no_meta_drop);
    tcase_add_test(tc, CE_outputsel_unknown_stream_drop);
    tcase_add_test(tc, CE_outputsel_correct_routing);
    tcase_add_test(tc, CE_outputsel_wrapped_event_routing);
    tcase_add_test(tc, CE_outputsel_global_eos_consumed);
    tcase_add_test(tc, CE_outputsel_request_pad_registration);
    return s;
}

GST_CHECK_MAIN(dxoutputselector);
