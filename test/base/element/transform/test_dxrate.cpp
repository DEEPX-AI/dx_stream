// P2.1 — dxrate contract tests (rewritten)
// Core: dxrate adjusts output framerate via frame drop/dup + PTS recalculation.
// All CE_rate TCs target specific functions/lines in the source.
// MUT: removing the targeted line must cause TC failure.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

#include <vector>

using namespace dxtest;

static const char *CAPS_30FPS =
    "video/x-raw,format=I420,width=320,height=240,framerate=30/1";
static const guint BUF_SIZE = 320 * 240 * 3 / 2;

struct DxRateHarness {
    GstHarness *h = nullptr;

    explicit DxRateHarness(guint framerate) {
        GstElement *e = gst_element_factory_make("dxrate", nullptr);
        fail_unless(e != nullptr);
        g_object_set(e, "framerate", framerate, nullptr);
        h = gst_harness_new_with_element(e, "sink", "src");
        gst_object_unref(e);
        fail_unless(h != nullptr, "Failed to create dxrate harness");
    }

    ~DxRateHarness() {
        if (h) gst_harness_teardown(h);
    }

    GstElement *element() const { return h->element; }
};

static void push_frames(GstHarness *h, guint n, guint fps) {
    GstClockTime dur = GST_SECOND / fps;
    for (guint i = 0; i < n; i++) {
        GstBuffer *b = gst_harness_create_buffer(h, BUF_SIZE);
        GST_BUFFER_PTS(b) = i * dur;
        GST_BUFFER_DURATION(b) = dur;
        gst_harness_push(h, b);
    }
}

static std::vector<GstClockTime> pull_all_pts(GstHarness *h) {
    std::vector<GstClockTime> v;
    GstBuffer *out;
    while ((out = gst_harness_try_pull(h)) != nullptr) {
        v.push_back(GST_BUFFER_PTS(out));
        gst_buffer_unref(out);
    }
    return v;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    guint fr = 999; gboolean th = TRUE;
    g_object_get(e, "framerate", &fr, "throttle", &th, nullptr);
    fail_unless_equals_int(fr, 0);
    fail_unless(th == FALSE);
    g_object_set(e, "framerate", 15u, "throttle", TRUE, nullptr);
    g_object_get(e, "framerate", &fr, "throttle", &th, nullptr);
    fail_unless_equals_int(fr, 15);
    fail_unless(th == TRUE);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    g_object_set(e, "framerate", 30u, nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_rate_drop: 30fps→10fps, 30 frames → output ~10
// Target: gst_dxrate_process_buffer L404 diff1<=diff2 → flush_prev
// MUT: remove diff comparison logic → all frames flushed → output=30 → fail
GST_START_TEST(CE_rate_drop) {
    DxRateHarness h(10u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 30, 30);
    push_eos(h.h);

    auto pts = pull_all_pts(h.h);
    fail_unless(pts.size() >= 8 && pts.size() <= 12,
                "30→10fps drop: expected ~10, got %zu", pts.size());
    fail_unless(pts.size() < 30,
                "drop logic must reduce output count (got %zu)", pts.size());
}
GST_END_TEST;

// CE_rate_pts_spacing: output PTS interval is 1/target_fps
// Target: gst_dxrate_push_buffer L186-212 (PTS recalculation: next_ts = base + duration)
// MUT: remove L212 (GST_BUFFER_TIMESTAMP set) → PTS unchanged from original → interval mismatch
GST_START_TEST(CE_rate_pts_spacing) {
    DxRateHarness h(10u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 30, 30);
    push_eos(h.h);

    auto pts = pull_all_pts(h.h);
    fail_unless(pts.size() >= 3, "need >= 3 output for spacing check");

    GstClockTime expected_interval = GST_SECOND / 10;
    for (size_t i = 1; i < pts.size() - 1; i++) {
        GstClockTimeDiff actual = (GstClockTimeDiff)(pts[i] - pts[i-1]);
        GstClockTimeDiff tolerance = (GstClockTimeDiff)(expected_interval / 5);
        GstClockTimeDiff diff_from_expected = actual > (GstClockTimeDiff)expected_interval
            ? actual - (GstClockTimeDiff)expected_interval
            : (GstClockTimeDiff)expected_interval - actual;
        fail_unless(diff_from_expected <= tolerance,
                    "PTS[%zu]->PTS[%zu]: interval=%" G_GINT64_FORMAT
                    "ns, expected ~%" G_GUINT64_FORMAT "ns (±20%%)",
                    i-1, i, (gint64)actual, (guint64)expected_interval);
    }
}
GST_END_TEST;

// CE_rate_eos_flush: EOS triggers flush of buffered frame
// Target: gst_dxrate_sink_event L312-323 (EOS → flush_loop/flush_prev)
// MUT: removing flush_loop call at L314 → last frame lost → output=0
GST_START_TEST(CE_rate_eos_flush) {
    DxRateHarness h(30u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
    GST_BUFFER_PTS(b) = 0;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    gst_harness_push(h.h, b);

    // first buffer is stored in prevbuf (DROPPED). push alone produces output 0.
    auto before_eos = pull_all_pts(h.h);
    fail_unless_equals_int(before_eos.size(), 0);

    // EOS must trigger flush so buffered frame is output
    push_eos(h.h);
    auto after_eos = pull_all_pts(h.h);
    fail_unless(after_eos.size() >= 1,
                "EOS must flush buffered frame, got %zu output", after_eos.size());
}
GST_END_TEST;

// CE_rate_first_buffer_stored: first buffer is stored, not output
// Target: gst_dxrate_transform_ip L451-454 (prevbuf==nullptr → store, DROPPED)
// MUT: remove first buffer store logic → first buffer also output → fail
GST_START_TEST(CE_rate_first_buffer_stored) {
    DxRateHarness h(30u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
    GST_BUFFER_PTS(b) = 0;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_OK);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out == nullptr,
                "first buffer must be stored internally, not output");
}
GST_END_TEST;

// CE_rate_framerate_zero: framerate=0 → state change failure
// Target: gst_dxrate_start (framerate==0 → GST_ELEMENT_ERROR)
// MUT: remove framerate==0 check → state change succeeds → fail
GST_START_TEST(CE_rate_framerate_zero) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    fail_unless(e != nullptr);
    g_object_set(e, "framerate", 0u, nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_rate_latency: LATENCY query adds 1/framerate
// Target: gst_dxrate_query L468-479 (frame_duration calculation + min_latency addition)
// MUT: remove L475 (min_latency += frame_duration) → min_lat unchanged → fail
GST_START_TEST(CE_rate_latency) {
    DxRateHarness h(10u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    gboolean live; GstClockTime min_lat, max_lat;
    gboolean r = query_latency_on_src(h.element(), &live, &min_lat, &max_lat);
    fail_unless(r, "latency query must succeed");

    GstClockTime expected_frame_dur = GST_SECOND / 10;
    fail_unless(min_lat >= expected_frame_dur,
                "min_latency=%" G_GUINT64_FORMAT "ns must include frame_duration=%"
                G_GUINT64_FORMAT "ns", (guint64)min_lat, (guint64)expected_frame_dur);
}
GST_END_TEST;

// CE_rate_flush_resets: FLUSH_STOP → internal counter reset → PTS restarts from new base
// Target: gst_dxrate_sink_event L327 (gst_dxrate_reset)
// MUT: remove L327 → previous state retained → PTS discontinuity or drop ratio error
GST_START_TEST(CE_rate_flush_resets) {
    DxRateHarness h(30u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 5, 30);
    int before_flush = gst_harness_buffers_received(h.h);

    push_flush(h.h);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_harness_push_event(h.h, gst_event_new_segment(&seg));

    // push again with new PTS base after flush
    GstClockTime dur = GST_SECOND / 30;
    for (int i = 0; i < 5; i++) {
        GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
        GST_BUFFER_PTS(b) = i * dur;
        GST_BUFFER_DURATION(b) = dur;
        GstFlowReturn r = gst_harness_push(h.h, b);
        fail_unless(r == GST_FLOW_OK,
                    "push after flush failed: %s", gst_flow_get_name(r));
    }

    push_eos(h.h);
    int total = gst_harness_buffers_received(h.h);
    fail_unless(total > before_flush,
                "after flush+new frames+EOS, must have more output (before=%d total=%d)",
                before_flush, total);
}
GST_END_TEST;

// CE_rate_same_fps: input==target fps → passthrough (all frames output)
// Target: gst_dxrate_process_buffer overall — same rate flushes all frames
// MUT: remove process_buffer entirely → output 0 → fail
GST_START_TEST(CE_rate_same_fps) {
    DxRateHarness h(30u);
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 10, 30);
    push_eos(h.h);

    auto pts = pull_all_pts(h.h);
    fail_unless(pts.size() >= 8 && pts.size() <= 12,
                "same fps (30→30): expected ~10 output, got %zu", pts.size());
}
GST_END_TEST;

static Suite *dxrate_suite(void) {
    Suite *s = suite_create("dxrate");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_rate_drop);
    tcase_add_test(tc, CE_rate_pts_spacing);
    tcase_add_test(tc, CE_rate_eos_flush);
    tcase_add_test(tc, CE_rate_first_buffer_stored);
    tcase_add_test(tc, CE_rate_framerate_zero);
    tcase_add_test(tc, CE_rate_latency);
    tcase_add_test(tc, CE_rate_flush_resets);
    tcase_add_test(tc, CE_rate_same_fps);
    return s;
}

GST_CHECK_MAIN(dxrate);
