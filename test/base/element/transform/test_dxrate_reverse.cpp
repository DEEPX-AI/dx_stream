// Phase 3 — dxrate reverse playback + NULL PTS edge cases
// B14: segment.rate<0 → GST_FLOW_ERROR (reverse playback unsupported)
// B13: GST_CLOCK_TIME_NONE PTS → buffer gracefully dropped

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

using namespace dxtest;

static const char *CAPS_30FPS =
    "video/x-raw,format=I420,width=320,height=240,framerate=30/1";
static const guint BUF_SIZE = 320 * 240 * 3 / 2;

// CE_rate_reverse_segment: segment.rate<0 → FLOW_ERROR on next buffer
// Target: gst_dxrate_transform_ip L438-441
// MUT: remove rate<0 check → buffer processed normally → fail
GST_START_TEST(CE_rate_reverse_segment) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 15u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    seg.rate = -1.0;
    seg.start = 0;
    seg.stop = 10 * GST_SECOND;
    seg.time = 0;
    seg.position = 10 * GST_SECOND;
    gst_harness_push_event(h.h, gst_event_new_segment(&seg));

    GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
    GST_BUFFER_PTS(b) = 9 * GST_SECOND;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_ERROR,
                "reverse playback must return FLOW_ERROR, got %s",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out == nullptr, "no output expected for reverse segment");
}
GST_END_TEST;

// CE_rate_null_pts_first: first buffer with NONE PTS → dropped (no _last_ts)
// Target: gst_dxrate_validate_and_get_timestamp L357-362
// MUT: remove invalid TS fallback → crash or process invalid → fail
GST_START_TEST(CE_rate_null_pts_first) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 15u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
    GST_BUFFER_PTS(b) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(b) = GST_CLOCK_TIME_NONE;
    GstFlowReturn r = gst_harness_push(h.h, b);
    // dxrate returns GST_BASE_TRANSFORM_FLOW_DROPPED internally,
    // but BaseTransform converts DROPPED→GST_FLOW_OK before returning to upstream.
    fail_unless(r == GST_FLOW_OK,
                "NULL PTS push must return GST_FLOW_OK (BaseTransform converts "
                "FLOW_DROPPED), got %s", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out == nullptr,
                "buffer with NONE PTS and no prior PTS must be dropped");
}
GST_END_TEST;

// CE_rate_null_pts_fallback: NONE PTS after valid → uses _last_ts fallback
// Target: gst_dxrate_validate_and_get_timestamp L358
// MUT: remove fallback to _last_ts → both buffers dropped → fail
GST_START_TEST(CE_rate_null_pts_fallback) {
    Harness h("dxrate",
              [](GstElement *e) { g_object_set(e, "framerate", 10u, nullptr); },
              "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    // First: push valid buffer (stored as prevbuf)
    GstBuffer *valid = gst_harness_create_buffer(h.h, 64*64*3/2);
    GST_BUFFER_PTS(valid) = 0;
    GST_BUFFER_DURATION(valid) = GST_SECOND / 30;
    GstFlowReturn r = gst_harness_push(h.h, valid);
    // BaseTransform converts FLOW_DROPPED → FLOW_OK before returning to upstream
    fail_unless(r == GST_FLOW_OK,
                "initial valid buffer must succeed (got %s)",
                gst_flow_get_name(r));

    // Then: push NULL PTS buffer — should be gracefully handled (dropped)
    GstBuffer *bad = gst_harness_create_buffer(h.h, 64*64*3/2);
    GST_BUFFER_PTS(bad) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(bad) = GST_CLOCK_TIME_NONE;
    r = gst_harness_push(h.h, bad);
    fail_unless(r == GST_FLOW_OK,
                "NULL PTS after valid must return GST_FLOW_OK (DROPPED is "
                "converted by BaseTransform), got %s", gst_flow_get_name(r));

    // Push another valid buffer to confirm dxrate continues working
    GstBuffer *next = gst_harness_create_buffer(h.h, 64*64*3/2);
    GST_BUFFER_PTS(next) = GST_SECOND;
    GST_BUFFER_DURATION(next) = GST_SECOND / 30;
    r = gst_harness_push(h.h, next);
    fail_unless(r == GST_FLOW_OK,
                "valid buffer after NULL PTS must succeed (got %s)",
                gst_flow_get_name(r));

    // Pull outputs — verify none has CLOCK_TIME_NONE PTS
    guint n_out = 0;
    GstBuffer *out;
    while ((out = gst_harness_try_pull(h.h)) != nullptr) {
        fail_unless(GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(out)),
                    "output buffer must never have NULL PTS");
        gst_buffer_unref(out);
        n_out++;
    }
    fail_unless(n_out > 0, "dxrate must produce at least one output buffer");
}
GST_END_TEST;

static Suite *dxrate_reverse_suite(void) {
    Suite *s = suite_create("dxrate_reverse");
    TCase *tc = tcase_create("edge_cases");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_rate_reverse_segment);
    tcase_add_test(tc, CE_rate_null_pts_first);
    tcase_add_test(tc, CE_rate_null_pts_fallback);
    return s;
}

GST_CHECK_MAIN(dxrate_reverse);
