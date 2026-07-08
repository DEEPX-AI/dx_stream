// dxrate wrapped SEGMENT (L2) unwrap test — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.9
//
// Defect: 01_defect_report.md §7 P2 (cpp:284-342) — sink_event only handles
// raw GST_EVENT_SEGMENT. CUSTOM_DOWNSTREAM L2 wrapped SEGMENT events (Part
// C.5) are not unwrapped, so dxrate's internal segment state never updates
// when used inside the dxvideoraw domain.
//
// Oracle: push a wrapped SEGMENT carrying TIME-format inner segment with
// start = SEG_START. The element's BaseTransform segment must reflect the
// inner segment (FORMAT_TIME, start == SEG_START). The raw GST_EVENT_SEGMENT
// path already works (positive control in test_dxsegment_chainup.cpp), so
// this isolates the unwrap path.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "utils.hpp"

static const GstClockTime SEG_START = 11 * GST_SECOND;

GST_START_TEST(CE_rate_domain_wrapped_segment_unwrap) {
    GstElement *e = gst_element_factory_make("dxrate", nullptr);
    fail_unless(e != nullptr);
    g_object_set(e, "framerate", 30u, nullptr);

    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    fail_unless(h != nullptr);
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    // Build inner SEGMENT and wrap as CUSTOM_DOWNSTREAM (Part C.5 L2).
    GstSegment inner_seg;
    gst_segment_init(&inner_seg, GST_FORMAT_TIME);
    inner_seg.start = SEG_START;
    GstEvent *inner_evt = gst_event_new_segment(&inner_seg);
    GstEvent *wrapped = dx_event_wrap_downstream(/*stream_id=*/0, inner_evt);

    fail_unless(gst_harness_push_event(h, wrapped),
                "wrapped SEGMENT push must succeed");

    GstBaseTransform *trans = GST_BASE_TRANSFORM(h->element);
    fail_unless_equals_int(trans->segment.format, GST_FORMAT_TIME);
    fail_unless(trans->segment.start == SEG_START,
                "dxrate did not unwrap L2 SEGMENT — segment.start "
                "expected %" GST_TIME_FORMAT " got %" GST_TIME_FORMAT,
                GST_TIME_ARGS(SEG_START),
                GST_TIME_ARGS(trans->segment.start));

    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *rate_l2_segment_suite(void) {
    Suite *s = suite_create("rate_l2_segment");
    TCase *tc = tcase_create("unwrap");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_rate_domain_wrapped_segment_unwrap);
    return s;
}

GST_CHECK_MAIN(rate_l2_segment);
