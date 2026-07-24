// dxrate per-stream state test — TDD red phase for
// refactor_plans_v2/04_test_design.md §1.3
//
// Defect: 01_defect_report.md §7 P1 — dxrate's _prevbuf, _prev_ts, _next_ts,
// _base_ts, _out_frame_count, _segment are all element-global. In DOMAIN_MODE
// (dxinputselector → dxrate), two streams share the same rate engine and
// pollute each other.
//
// Minimal oracle: push 6 frames for stream0 (PTS 0,33,66,100,133,166 ms,
// frame_meta._stream_id=0) followed by 6 frames for stream1 (same PTS
// sequence, _stream_id=1). dxrate framerate=10 → expect roughly 2 outputs
// per stream (independent rate). Currently stream1 frames satisfy
// `intime < prev_ts` (because stream0 already advanced _prev_ts to ~166ms)
// → all stream1 frames dropped immediately. Output stream contains zero
// stream1 frames.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "gstdxstream/gst-dxframemeta.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"

using namespace dxtest;

static const char *CAPS = "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static GstBuffer *make_rate_buf(int stream_id, GstClockTime pts) {
    GstBuffer *b = make_video_buffer("RGB", 4, 4, pts);
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, 4, 4, "RGB", 30.0f);
    return b;
}

GST_START_TEST(PS_rate_two_streams_independent_timing) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    fail_unless(e != nullptr);
    g_object_set(e, "framerate", 10u, nullptr);

    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    fail_unless(h != nullptr);
    gst_harness_set_src_caps_str(h, CAPS);

    // stream0: 6 frames at 30fps, PTS 0..166ms
    for (int i = 0; i < 6; i++) {
        GstBuffer *b = make_rate_buf(0, i * (GST_SECOND / 30));
        gst_harness_push(h, b);
    }
    // stream1: same PTS range, stream_id=1
    for (int i = 0; i < 6; i++) {
        GstBuffer *b = make_rate_buf(1, i * (GST_SECOND / 30));
        gst_harness_push(h, b);
    }

    // Drain
    guint s0 = 0, s1 = 0;
    GstBuffer *out;
    while ((out = gst_harness_try_pull(h)) != nullptr) {
        DXFrameMeta *fm = dx_get_frame_meta(out);
        if (fm) {
            if (fm->_stream_id == 0) s0++;
            else if (fm->_stream_id == 1) s1++;
        }
        gst_buffer_unref(out);
    }

    // Both streams must produce output independently
    fail_unless(s0 >= 2,
                "stream 0 must produce >= 2 outputs (got %u)", s0);
    fail_unless(s1 >= 2,
                "stream 1 must produce >= 2 outputs (got %u)", s1);

    // Per-stream state is fully isolated (std::map<int, RateStreamState>).
    // With identical PTS sequences and segment, output counts must be equal
    // or differ by at most 1 (boundary timing).
    int diff = (int)s0 - (int)s1;
    if (diff < 0) diff = -diff;
    fail_unless(diff <= 1,
                "stream counts must be nearly equal with isolated state "
                "(%u vs %u, diff=%d) — cross-stream contamination detected",
                s0, s1, diff);

    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *dxrate_per_stream_suite(void) {
    Suite *s = suite_create("dxrate_per_stream");
    TCase *tc = tcase_create("per_stream");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PS_rate_two_streams_independent_timing);
    return s;
}

GST_CHECK_MAIN(dxrate_per_stream);
