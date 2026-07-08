// dxinputselector release_pad cleanup test — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.4
//
// Defect: 01_defect_report.md §1 P2 — `_stream_eos_sent` is only cleared in
// start()/stop() (cpp:220, 231). Aggregator default release_pad does not
// touch this set. Therefore, after request_pad → push EOS → release_pad →
// request_pad (same name, same stream_id), the second EOS will NOT generate
// a wrapped EOS event because count(stream_id) > 0.
//
// Realistic scenario: two pads (sink_0, sink_1). sink_0 EOS partial → wrapped
// EOS for stream 0 → release sink_0 (sink_1 still alive, so aggregator does
// NOT return global GST_FLOW_EOS). Re-request sink_0 → new EOS → must
// produce a second wrapped EOS for stream 0.
//
// (The "release and re-request after global EOS" scenario violates GStreamer
//  principle — once global EOS pushed downstream, the pipeline is done.)

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "utils.hpp"
#include <cstring>

static guint g_wrapped_eos_stream0_count = 0;

static GstPadProbeReturn count_wrapped_eos_probe(GstPad * /*pad*/,
                                                  GstPadProbeInfo *info,
                                                  gpointer /*user_data*/) {
    GstEvent *e = GST_PAD_PROBE_INFO_EVENT(info);
    if (e && dx_event_is_wrapped_downstream(e)) {
        gint sid = -1;
        GstEvent *inner = dx_event_peek_inner(e, &sid);
        if (inner && GST_EVENT_TYPE(inner) == GST_EVENT_EOS && sid == 0) {
            g_atomic_int_inc((gint *)&g_wrapped_eos_stream0_count);
        }
    }
    return GST_PAD_PROBE_OK;
}

static void send_pad_preamble(GstPad *pad, const gchar *stream_label) {
    fail_unless(gst_pad_send_event(pad, gst_event_new_stream_start(stream_label)));
    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
    fail_unless(gst_pad_send_event(pad, gst_event_new_caps(caps)));
    gst_caps_unref(caps);
    GstSegment seg; gst_segment_init(&seg, GST_FORMAT_TIME);
    fail_unless(gst_pad_send_event(pad, gst_event_new_segment(&seg)));
}

static void push_keepalive_buffer(GstPad *pad, GstClockTime pts) {
    GstBuffer *b = gst_buffer_new_allocate(nullptr, 4 * 4 * 3, nullptr);
    GstMapInfo m; gst_buffer_map(b, &m, GST_MAP_WRITE);
    memset(m.data, 0x80, m.size); gst_buffer_unmap(b, &m);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    GstFlowReturn fr = gst_pad_chain(pad, b);
    fail_unless(fr == GST_FLOW_OK, "gst_pad_chain returned %s",
                gst_flow_get_name(fr));
}

GST_START_TEST(CE_inputsel_release_pad_clears_stream_eos_sent) {
    g_wrapped_eos_stream0_count = 0;

    GstElement *agg = gst_element_factory_make("dxinputselector", "agg");
    fail_unless(agg != nullptr);
    GstElement *sink = gst_element_factory_make("fakesink", "sink");
    fail_unless(sink != nullptr);
    g_object_set(sink, "async", FALSE, "sync", FALSE, NULL);

    GstElement *pipeline = gst_pipeline_new("pipe");
    gst_bin_add_many(GST_BIN(pipeline), agg, sink, NULL);
    fail_unless(gst_element_link(agg, sink));

    // Request pads BEFORE state change so aggregator activates them during
    // PAUSED transition (chain function needs an active pad).
    GstPad *sink1 = gst_element_get_request_pad(agg, "sink_1");
    fail_unless(sink1 != nullptr);
    GstPad *sink0a = gst_element_get_request_pad(agg, "sink_0");
    fail_unless(sink0a != nullptr, "request_pad sink_0 [cycle 1]");

    fail_unless(gst_element_set_state(pipeline, GST_STATE_PLAYING) !=
                GST_STATE_CHANGE_FAILURE);

    GstPad *srcpad = gst_element_get_static_pad(agg, "src");
    fail_unless(srcpad != nullptr);
    gulong pid = gst_pad_add_probe(srcpad,
                                   GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
                                   count_wrapped_eos_probe, nullptr, nullptr);

    // Keep sink_1 alive throughout — prevents global GST_FLOW_EOS so the
    // aggregator src task stays running.
    send_pad_preamble(sink1, "s1-keepalive");

    // ---- cycle 1: preamble sink_0 → keepalive buffer on sink_1 → EOS on sink_0 ----
    send_pad_preamble(sink0a, "s0-a");
    // Feed sink_1 so aggregate() doesn't stall on NEED_DATA, allowing
    // sink_0's EOS branch to be reached.
    push_keepalive_buffer(sink1, 0);
    fail_unless(gst_pad_send_event(sink0a, gst_event_new_eos()));

    for (int i = 0; i < 100 &&
         g_atomic_int_get((gint *)&g_wrapped_eos_stream0_count) < 1; i++)
        g_usleep(10 * 1000);

    fail_unless(g_atomic_int_get((gint *)&g_wrapped_eos_stream0_count) >= 1,
                "cycle 1: at least one wrapped EOS for stream 0 expected, "
                "got %u",
                g_wrapped_eos_stream0_count);
    guint after_cycle1 = g_wrapped_eos_stream0_count;

    gst_element_release_request_pad(agg, sink0a);
    gst_object_unref(sink0a);

    // ---- cycle 2: re-request sink_0 → preamble → EOS ----
    GstPad *sink0b = gst_element_get_request_pad(agg, "sink_0");
    fail_unless(sink0b != nullptr, "request_pad sink_0 [cycle 2]");
    send_pad_preamble(sink0b, "s0-b");
    push_keepalive_buffer(sink1, GST_SECOND);
    fail_unless(gst_pad_send_event(sink0b, gst_event_new_eos()));

    for (int i = 0; i < 100 &&
         g_atomic_int_get((gint *)&g_wrapped_eos_stream0_count) <= (gint)after_cycle1;
         i++)
        g_usleep(10 * 1000);

    fail_unless(g_wrapped_eos_stream0_count > after_cycle1,
                "cycle 2: second wrapped EOS for stream 0 expected — "
                "release_pad did not clear _stream_eos_sent (count stayed "
                "at %u)",
                g_wrapped_eos_stream0_count);

    gst_element_release_request_pad(agg, sink0b);
    gst_object_unref(sink0b);
    gst_element_release_request_pad(agg, sink1);
    gst_object_unref(sink1);
    gst_pad_remove_probe(srcpad, pid);
    gst_object_unref(srcpad);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}
GST_END_TEST;

static Suite *inputsel_release_pad_suite(void) {
    Suite *s = suite_create("inputsel_release_pad");
    TCase *tc = tcase_create("release");
    tcase_set_timeout(tc, 15.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_inputsel_release_pad_clears_stream_eos_sent);
    return s;
}

GST_CHECK_MAIN(inputsel_release_pad);
