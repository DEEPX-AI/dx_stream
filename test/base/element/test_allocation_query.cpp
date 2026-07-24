// Phase 6 — Allocation query lock-in verification
// B11: propose/decide_allocation not overridden → default GstBaseTransform behavior
// Tests that elements work without custom allocation, establishing baseline for future changes

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_I420 =
    "video/x-raw,format=I420,width=64,height=64,framerate=30/1";

// CE_scale_allocation_default: dxscale uses default allocation (no custom propose/decide)
// Target: GstBaseTransform default allocation path
// MUT: if propose_allocation is added → query result changes → test detects regression
GST_START_TEST(CE_scale_allocation_default) {
    GstHarness *h = gst_harness_new("dxscale");
    g_object_set(h->element, "width", 32u, "height", 32u, nullptr);
    gst_harness_set_src_caps_str(h, CAPS_I420);

    GstCaps *alloc_caps = gst_caps_from_string(
        "video/x-raw,format=I420,width=32,height=32,framerate=30/1");
    GstQuery *q = gst_query_new_allocation(alloc_caps, FALSE);
    gst_caps_unref(alloc_caps);

    GstPad *sinkpad = gst_element_get_static_pad(h->element, "sink");
    gst_pad_query(sinkpad, q);
    gst_object_unref(sinkpad);

    guint n_pools = gst_query_get_n_allocation_pools(q);
    gst_query_unref(q);

    fail_unless(n_pools == 0,
                "default allocation: no pools proposed (got %u)", n_pools);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_convert_allocation_default: dxconvert uses default allocation
GST_START_TEST(CE_convert_allocation_default) {
    GstHarness *h = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=16,height=16,framerate=30/1");
    gst_harness_set_sink_caps_str(h,
        "video/x-raw,format=BGR,width=16,height=16,framerate=30/1");

    GstCaps *alloc_caps = gst_caps_from_string(
        "video/x-raw,format=BGR,width=16,height=16,framerate=30/1");
    GstQuery *q = gst_query_new_allocation(alloc_caps, FALSE);
    gst_caps_unref(alloc_caps);

    GstPad *sinkpad = gst_element_get_static_pad(h->element, "sink");
    gst_pad_query(sinkpad, q);
    gst_object_unref(sinkpad);

    guint n_pools = gst_query_get_n_allocation_pools(q);
    gst_query_unref(q);

    fail_unless(n_pools == 0,
                "default allocation: no pools proposed (got %u)", n_pools);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_rate_allocation_passthrough: dxrate uses passthrough allocation (in-place transform)
GST_START_TEST(CE_rate_allocation_passthrough) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 15u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_I420);

    GstCaps *alloc_caps = gst_caps_from_string(CAPS_I420);
    GstQuery *q = gst_query_new_allocation(alloc_caps, TRUE);
    gst_caps_unref(alloc_caps);

    GstPad *sinkpad = gst_element_get_static_pad(h.element(), "sink");
    gst_pad_query(sinkpad, q);
    gst_object_unref(sinkpad);

    guint n_pools = gst_query_get_n_allocation_pools(q);
    gst_query_unref(q);

    fail_unless(n_pools == 0,
                "passthrough allocation: no pools (got %u)", n_pools);
}
GST_END_TEST;

// CE_osd_allocation_default: dxosd uses default allocation (in-place transform)
GST_START_TEST(CE_osd_allocation_default) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=BGR,width=64,height=64,framerate=30/1");

    GstCaps *alloc_caps = gst_caps_from_string(
        "video/x-raw,format=BGR,width=64,height=64,framerate=30/1");
    GstQuery *q = gst_query_new_allocation(alloc_caps, TRUE);
    gst_caps_unref(alloc_caps);

    GstPad *sinkpad = gst_element_get_static_pad(h.element(), "sink");
    gst_pad_query(sinkpad, q);
    gst_object_unref(sinkpad);

    guint n_pools = gst_query_get_n_allocation_pools(q);
    gst_query_unref(q);

    fail_unless(n_pools == 0,
                "default allocation: no pools (got %u)", n_pools);
}
GST_END_TEST;

static Suite *allocation_query_suite(void) {
    Suite *s = suite_create("allocation_query");
    TCase *tc = tcase_create("allocation_lockin");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_scale_allocation_default);
    tcase_add_test(tc, CE_convert_allocation_default);
    tcase_add_test(tc, CE_rate_allocation_passthrough);
    tcase_add_test(tc, CE_osd_allocation_default);
    return s;
}

GST_CHECK_MAIN(allocation_query);
