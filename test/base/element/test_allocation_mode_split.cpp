// propose_allocation mode-split tests — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.8
//
// Defect: 01_defect_report.md
//   - dxpreprocess (P2, cpp:870)
//   - dxosd        (P2, cpp:283)
//   - dxtracker    (P2, cpp:391)
//
// All three call gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE,
// NULL) unconditionally inside propose_allocation. Contract (CLAUDE.md C.4 /
// C.6) requires DX_FRAME_META_API_TYPE only in DOMAIN_MODE (caps =
// video/dxvideoraw). In NORMAL_MODE (plain video/x-raw) it must not be
// proposed — upstream allocators that honor the proposal will needlessly
// attach the meta on every buffer.
//
// Oracle: with sink caps = video/x-raw, fire ALLOCATION query at sink; meta
// list must NOT contain DX_FRAME_META_API_TYPE.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "gst-dxframemeta.hpp"

static bool query_has_dxframe_meta(GstQuery *q) {
    guint n = gst_query_get_n_allocation_metas(q);
    for (guint i = 0; i < n; i++) {
        GType api = gst_query_parse_nth_allocation_meta(q, i, NULL);
        if (api == DX_FRAME_META_API_TYPE) return true;
    }
    return false;
}

static void assert_normal_mode_no_dxframe_meta(const char *element,
                                               const char *plain_caps,
                                               void (*setup_props)(GstElement *) = nullptr) {
    GstElement *e = gst_element_factory_make(element, nullptr);
    fail_unless(e != nullptr);
    if (setup_props) setup_props(e);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    fail_unless(h != nullptr);

    gst_harness_set_src_caps_str(h, plain_caps);

    GstCaps *acaps = gst_caps_from_string(plain_caps);
    GstQuery *q = gst_query_new_allocation(acaps, FALSE);
    gst_caps_unref(acaps);

    GstPad *sinkpad = gst_element_get_static_pad(h->element, "sink");
    gst_pad_query(sinkpad, q);
    gst_object_unref(sinkpad);

    bool has = query_has_dxframe_meta(q);
    gst_query_unref(q);

    fail_if(has,
            "%s NORMAL_MODE (plain video/x-raw): propose_allocation must NOT "
            "advertise DX_FRAME_META_API_TYPE — currently unconditional.",
            element);

    gst_harness_teardown(h);
}

GST_START_TEST(ALLOC_pre_normal_mode_no_dxframe_meta) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "resize-width", 64u, "resize-height", 64u, nullptr);
    };
    assert_normal_mode_no_dxframe_meta(
        "dxpreprocess",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1",
        setup);
}
GST_END_TEST;

GST_START_TEST(ALLOC_osd_normal_mode_no_dxframe_meta) {
    assert_normal_mode_no_dxframe_meta(
        "dxosd",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

GST_START_TEST(ALLOC_tracker_normal_mode_no_dxframe_meta) {
    assert_normal_mode_no_dxframe_meta(
        "dxtracker",
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
}
GST_END_TEST;

static Suite *allocation_mode_split_suite(void) {
    Suite *s = suite_create("allocation_mode_split");
    TCase *tc = tcase_create("normal_mode");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, ALLOC_pre_normal_mode_no_dxframe_meta);
    tcase_add_test(tc, ALLOC_osd_normal_mode_no_dxframe_meta);
    tcase_add_test(tc, ALLOC_tracker_normal_mode_no_dxframe_meta);
    return s;
}

GST_CHECK_MAIN(allocation_mode_split);
