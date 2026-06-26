// BaseTransform sink_event parent chain-up tests — TDD red phase for
// refactor_plans/event_query_state_audit.md §2.2
//
// Bug: dxosd / dxpreprocess / dxpostprocess sink_event handlers do their
// own work and then call `gst_pad_push_event(src_pad, event)` instead of
// `GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event)`.
//
// Effect: BaseTransform default sink_event — which copies SEGMENT into
// `trans->segment`, drives caps negotiation on CAPS, and updates internal
// QoS state — never runs. `trans->segment` stays in default state
// (format = GST_FORMAT_UNDEFINED, have_segment = FALSE).
//
// Test: push a TIME-format SEGMENT through the harness and read the
// element's BaseTransform internal segment directly. After the bug is
// fixed (parent chain-up restored) the segment is GST_FORMAT_TIME and
// `start` matches what we pushed.
//
// Currently FAILING for: dxosd, dxpreprocess, dxpostprocess.
// Currently PASSING for: dxrate, dxconvert, dxscale, dxtracker (positive
// controls — they already chain up).

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

using namespace dxtest;

static const GstClockTime SEG_START = 7 * GST_SECOND;
static const GstClockTime SEG_STOP  = 14 * GST_SECOND;

static void assert_segment_propagated(const char *element, const char *src_caps,
                                      const char *sink_caps,
                                      void (*setup_props)(GstElement *) = nullptr) {
    GstElement *e = gst_element_factory_make(element, nullptr);
    fail_unless(e != nullptr, "factory_make %s", element);
    if (setup_props) setup_props(e);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    fail_unless(h != nullptr, "Harness for %s", element);
    if (src_caps) gst_harness_set_src_caps_str(h, src_caps);
    if (sink_caps) gst_harness_set_sink_caps_str(h, sink_caps);

    GstEvent *stale_event = gst_harness_try_pull_event(h);
    while (stale_event != nullptr) {
        gst_event_unref(stale_event);
        stale_event = gst_harness_try_pull_event(h);
    }

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    seg.start = SEG_START;
    seg.stop  = SEG_STOP;
    seg.rate  = 1.0;
    gboolean ok = gst_harness_push_event(h, gst_event_new_segment(&seg));
    fail_unless(ok, "%s: SEGMENT push must succeed", element);

    GstBaseTransform *trans = GST_BASE_TRANSFORM(h->element);

    fail_unless_equals_int(trans->segment.format, GST_FORMAT_TIME);
    fail_unless(trans->segment.start == SEG_START,
                "%s: segment.start expected %" GST_TIME_FORMAT
                " got %" GST_TIME_FORMAT
                " — sink_event likely missing parent chain-up",
                element, GST_TIME_ARGS(SEG_START),
                GST_TIME_ARGS(trans->segment.start));
    fail_unless(trans->segment.stop == SEG_STOP,
                "%s: segment.stop expected %" GST_TIME_FORMAT
                " got %" GST_TIME_FORMAT,
                element, GST_TIME_ARGS(SEG_STOP),
                GST_TIME_ARGS(trans->segment.stop));
    fail_unless(trans->segment.rate == 1.0,
                "%s: segment.rate expected 1.0, got %f",
                element, trans->segment.rate);

    GstEvent *downstream_seg = gst_harness_try_pull_event(h);
    gboolean found_segment = FALSE;
    for (int i = 0; i < 10 && downstream_seg != nullptr; i++) {
        if (GST_EVENT_TYPE(downstream_seg) == GST_EVENT_SEGMENT) {
            const GstSegment *ds;
            gst_event_parse_segment(downstream_seg, &ds);
            fail_unless(ds->start == SEG_START,
                        "%s: downstream segment.start mismatch", element);
            fail_unless(ds->stop == SEG_STOP,
                        "%s: downstream segment.stop mismatch", element);
            fail_unless(ds->rate == 1.0,
                        "%s: downstream segment.rate mismatch", element);
            found_segment = TRUE;
            gst_event_unref(downstream_seg);
            break;
        }
        gst_event_unref(downstream_seg);
        downstream_seg = gst_harness_try_pull_event(h);
    }
    if (downstream_seg && !found_segment) gst_event_unref(downstream_seg);
    fail_unless(found_segment,
                "%s: SEGMENT must be forwarded downstream", element);

    gst_harness_teardown(h);
}

// ---- positive controls (already correct, must keep passing) ----

GST_START_TEST(SEG_dxrate_chainup) {
    auto setup = [](GstElement *e) { g_object_set(e, "framerate", 30u, nullptr); };
    assert_segment_propagated(
        "dxrate",
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1",
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

GST_START_TEST(SEG_dxconvert_chainup) {
    assert_segment_propagated(
        "dxconvert",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

GST_START_TEST(SEG_dxscale_chainup) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "width", 64u, "height", 64u, nullptr);
    };
    assert_segment_propagated(
        "dxscale",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

GST_START_TEST(SEG_dxtracker_chainup) {
    assert_segment_propagated(
        "dxtracker",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

// ---- bug targets (currently failing) ----

GST_START_TEST(SEG_dxosd_chainup) {
    assert_segment_propagated(
        "dxosd",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

GST_START_TEST(SEG_dxpreprocess_chainup) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "resize-width", 64u, "resize-height", 64u, nullptr);
    };
    assert_segment_propagated(
        "dxpreprocess",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        nullptr,
        setup);
}
GST_END_TEST;

GST_START_TEST(SEG_dxpostprocess_chainup) {
    assert_segment_propagated(
        "dxpostprocess",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

static Suite *segment_chainup_suite(void) {
    Suite *s = suite_create("segment_chainup");
    TCase *tc = tcase_create("base_transform");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, SEG_dxrate_chainup);
    tcase_add_test(tc, SEG_dxconvert_chainup);
    tcase_add_test(tc, SEG_dxscale_chainup);
    tcase_add_test(tc, SEG_dxtracker_chainup);
    tcase_add_test(tc, SEG_dxosd_chainup);
    tcase_add_test(tc, SEG_dxpreprocess_chainup);
    tcase_add_test(tc, SEG_dxpostprocess_chainup);
    return s;
}

GST_CHECK_MAIN(segment_chainup);
