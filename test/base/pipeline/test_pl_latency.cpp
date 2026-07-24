// Phase 4 — LATENCY query accumulation verification
// B5: dxrate adds frame_duration, dxinfer adds avg_latency, others pass through

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

using namespace dxtest;

static const char *CAPS_30FPS =
    "video/x-raw,format=I420,width=64,height=64,framerate=30/1";

// PL_latency_dxrate_adds: dxrate at 10fps adds ~100ms to min_latency
// Target: gst_dxrate_query L468-479
GST_START_TEST(PL_latency_dxrate_adds) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 10u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    GstQuery *q = gst_query_new_latency();
    gboolean res = gst_pad_peer_query(h.h->sinkpad, q);
    fail_unless(res, "LATENCY query must succeed");

    gboolean live;
    GstClockTime min_lat, max_lat;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);
    gst_query_unref(q);

    GstClockTime expected_frame_dur = GST_SECOND / 10;
    fail_unless(min_lat >= expected_frame_dur,
                "min_latency=%" G_GUINT64_FORMAT "ns must include frame_dur=%"
                G_GUINT64_FORMAT "ns (from dxrate 10fps)",
                (guint64)min_lat, (guint64)expected_frame_dur);
}
GST_END_TEST;

// PL_latency_transform_passthrough: dxscale + dxconvert don't add latency
// Target: GstBaseTransform default — passthrough latency query
GST_START_TEST(PL_latency_transform_passthrough) {
    GstHarness *h = gst_harness_new_parse(
        "dxscale width=32 height=32 ! dxconvert");
    gst_harness_set_src_caps_str(h, CAPS_30FPS);

    GstQuery *q = gst_query_new_latency();
    gboolean res = gst_pad_peer_query(h->sinkpad, q);
    fail_unless(res, "LATENCY query must succeed");

    gboolean live;
    GstClockTime min_lat, max_lat;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);
    gst_query_unref(q);

    GstClockTime frame_dur = GST_SECOND / 30;
    fail_unless(min_lat <= frame_dur,
                "dxscale+dxconvert should not add significant latency (got %"
                G_GUINT64_FORMAT "ns)", (guint64)min_lat);

    gst_harness_teardown(h);
}
GST_END_TEST;

// PL_latency_rate_accumulates: two dxrate elements → latency accumulates
// Target: each dxrate adds its own frame_duration
GST_START_TEST(PL_latency_rate_accumulates) {
    GstHarness *h = gst_harness_new_parse(
        "dxrate framerate=15 ! dxrate framerate=10");
    gst_harness_set_src_caps_str(h, CAPS_30FPS);

    GstQuery *q = gst_query_new_latency();
    gboolean res = gst_pad_peer_query(h->sinkpad, q);
    fail_unless(res, "LATENCY query must succeed");

    gboolean live;
    GstClockTime min_lat, max_lat;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);
    gst_query_unref(q);

    GstClockTime rate15_dur = GST_SECOND / 15;
    GstClockTime rate10_dur = GST_SECOND / 10;
    GstClockTime expected_min = rate15_dur + rate10_dur;

    fail_unless(min_lat >= expected_min,
                "two dxrate elements must accumulate latency: got %"
                G_GUINT64_FORMAT "ns, expected >= %" G_GUINT64_FORMAT "ns",
                (guint64)min_lat, (guint64)expected_min);

    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *pl_latency_suite(void) {
    Suite *s = suite_create("pl_latency");
    TCase *tc = tcase_create("latency_query");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_latency_dxrate_adds);
    tcase_add_test(tc, PL_latency_transform_passthrough);
    tcase_add_test(tc, PL_latency_rate_accumulates);
    return s;
}

GST_CHECK_MAIN(pl_latency);
