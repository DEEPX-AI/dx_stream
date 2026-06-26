// LATENCY query contract tests — TDD red phase for refactor_plans/event_query_state_audit.md §2.1
//
// Plan §2.6: every processing element MUST add its self time (≥1 ns) to
// the upstream LATENCY value. Otherwise the sink computes base_time against
// a wrong latency and frames are dropped or over-buffered.
//
// Method: GstHarness pretends to be the upstream peer with a fixed
// upstream_latency. We issue a LATENCY query on the element's src pad. A
// compliant element returns a min_latency *strictly greater* than the
// upstream value because it adds its own processing/queue time.
//
// Currently FAILING for: dxpreprocess, dxpostprocess, dxosd, dxtracker,
// dxconvert, dxscale, dxmsgconv (BaseTransform default just forwards LATENCY).
// Currently PASSING for: dxrate (already overrides — kept as a positive
// control so the harness pattern itself is verified).

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static const GstClockTime UPSTREAM_LAT = 10 * GST_MSECOND;

struct LatencyResult {
    gboolean live;
    GstClockTime min_lat;
    GstClockTime max_lat;
};

static gboolean answer_non_live_latency(GstPad *pad, GstObject *parent,
                                        GstQuery *query) {
    (void)pad;
    (void)parent;
    if (GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        gst_query_set_latency(query, FALSE, UPSTREAM_LAT, GST_CLOCK_TIME_NONE);
        return TRUE;
    }
    return FALSE;
}

static LatencyResult query_full_latency(GstHarness *h) {
    GstPad *srcpad = gst_element_get_static_pad(h->element, "src");
    fail_unless(srcpad != nullptr);
    GstQuery *q = gst_query_new_latency();
    gboolean ok = gst_pad_query(srcpad, q);
    gst_object_unref(srcpad);
    fail_unless(ok, "LATENCY query must succeed");

    LatencyResult r = {};
    gst_query_parse_latency(q, &r.live, &r.min_lat, &r.max_lat);
    gst_query_unref(q);
    return r;
}

static void assert_self_time_added(const char *element, const char *src_caps,
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

    gst_pad_set_query_function(h->srcpad, answer_non_live_latency);
    LatencyResult r = query_full_latency(h);

    fail_unless(r.min_lat > UPSTREAM_LAT,
                "%s: min_latency must be > upstream (%" GST_TIME_FORMAT
                ") — element forgot to add its self time. got=%" GST_TIME_FORMAT,
                element, GST_TIME_ARGS(UPSTREAM_LAT), GST_TIME_ARGS(r.min_lat));

    if (GST_CLOCK_TIME_IS_VALID(r.max_lat)) {
        fail_unless(r.max_lat >= r.min_lat,
                    "%s: max_latency (%" GST_TIME_FORMAT ") must be >= min_latency ("
                    "%" GST_TIME_FORMAT ")",
                    element, GST_TIME_ARGS(r.max_lat), GST_TIME_ARGS(r.min_lat));
    }

    fail_unless(r.live == FALSE,
                "%s: live flag should be FALSE for non-live upstream", element);

    gst_harness_teardown(h);
}

// ---------------------------------------------------------------------------
// Positive control: dxrate already overrides LATENCY. If this fails the
// harness pattern itself is wrong.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxrate_adds_frame_duration) {
    auto setup = [](GstElement *e) { g_object_set(e, "framerate", 30u, nullptr); };
    assert_self_time_added(
        "dxrate",
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1",
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxosd — Dual BaseTransform, in-place. Self time = drawing/per-stream lookup.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxosd_self_time_added) {
    assert_self_time_added(
        "dxosd",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxpreprocess — Dual BaseTransform. Self time = preprocess kernel + tensor
// build. Needs resize-width/height to negotiate.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxpreprocess_self_time_added) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "resize-width", 64u, "resize-height", 64u, nullptr);
    };
    assert_self_time_added(
        "dxpreprocess",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        nullptr,
        setup);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxpostprocess — Dual BaseTransform, ANY caps.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxpostprocess_self_time_added) {
    assert_self_time_added(
        "dxpostprocess",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxtracker — Dual BaseTransform, ANY caps.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxtracker_self_time_added) {
    assert_self_time_added(
        "dxtracker",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxconvert — out-of-domain BaseTransform. Self time = color conversion.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxconvert_self_time_added) {
    assert_self_time_added(
        "dxconvert",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxscale — out-of-domain BaseTransform. Self time = resize kernel.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxscale_self_time_added) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "width", 64u, "height", 64u, nullptr);
    };
    assert_self_time_added(
        "dxscale",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// dxmsgconv — Dual caps-agnostic BaseTransform. Self time = meta conversion.
// ---------------------------------------------------------------------------
GST_START_TEST(LAT_dxmsgconv_self_time_added) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "library-file-path",
                     dxtest::resolve_lib_path("libdx_msgconvl.so").c_str(),
                     nullptr);
    };
    assert_self_time_added(
        "dxmsgconv",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

static Suite *latency_contract_suite(void) {
    Suite *s = suite_create("latency_contract");
    TCase *tc = tcase_create("self_time");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, LAT_dxrate_adds_frame_duration);
    tcase_add_test(tc, LAT_dxosd_self_time_added);
    tcase_add_test(tc, LAT_dxpreprocess_self_time_added);
    tcase_add_test(tc, LAT_dxpostprocess_self_time_added);
    tcase_add_test(tc, LAT_dxtracker_self_time_added);
    tcase_add_test(tc, LAT_dxconvert_self_time_added);
    tcase_add_test(tc, LAT_dxscale_self_time_added);
    tcase_add_test(tc, LAT_dxmsgconv_self_time_added);
    return s;
}

GST_CHECK_MAIN(latency_contract);
