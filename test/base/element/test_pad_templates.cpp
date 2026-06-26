// Pad template caps tests — TDD red phase for refactor_plans_v2/04_test_design.md §2.2
//
// Defect: refactor_plans_v2/01_defect_report.md
//   - dxinfer (P1): sink/src GST_STATIC_CAPS_ANY (cpp:41-45)
//   - dxrate (P2): pad template GST_CAPS_ANY (cpp:156, 160)
//   - dxpostprocess (P2): pad template GST_STATIC_CAPS_ANY (cpp:21-25)
//   - dxmsgbroker (P2): sink template GST_CAPS_ANY (cpp:79)
//   - dxgather (P2): pad template GST_CAPS_ANY (cpp:12, 15)
//
// Contract: gst-dxstream-plugin/CLAUDE.md Part B.8 — pad templates must
// declare concrete media types so caps negotiation rejects invalid upstreams
// early. CAPS_ANY masks negotiation bugs and prevents element discovery via
// gst_element_factory_can_sink_caps().
//
// Oracle: for every pad template on the factory, gst_caps_is_any(caps) must
// be FALSE.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

static void assert_no_any_template(const char *element) {
    GstElementFactory *f = gst_element_factory_find(element);
    fail_unless(f != nullptr, "factory %s not found", element);

    const GList *templates = gst_element_factory_get_static_pad_templates(f);
    fail_unless(templates != nullptr, "%s: no pad templates", element);

    int n_checked = 0;
    for (const GList *l = templates; l != nullptr; l = l->next) {
        GstStaticPadTemplate *tmpl = (GstStaticPadTemplate *)l->data;
        GstCaps *caps = gst_static_caps_get(&tmpl->static_caps);
        fail_unless(caps != nullptr, "%s: template %s has null caps",
                    element, tmpl->name_template);
        gboolean is_any = gst_caps_is_any(caps);
        gst_caps_unref(caps);
        fail_if(is_any,
                "%s: pad template '%s' declares CAPS_ANY — Part B.8 violation",
                element, tmpl->name_template);
        n_checked++;
    }
    fail_unless(n_checked > 0, "%s: zero templates checked", element);
    gst_object_unref(f);
}

GST_START_TEST(TPL_dxinfer_not_any) {
    assert_no_any_template("dxinfer");
}
GST_END_TEST;

GST_START_TEST(TPL_dxrate_not_any) {
    assert_no_any_template("dxrate");
}
GST_END_TEST;

GST_START_TEST(TPL_dxpostprocess_not_any) {
    assert_no_any_template("dxpostprocess");
}
GST_END_TEST;

GST_START_TEST(TPL_dxmsgbroker_not_any) {
    assert_no_any_template("dxmsgbroker");
}
GST_END_TEST;

GST_START_TEST(TPL_dxgather_not_any) {
    assert_no_any_template("dxgather");
}
GST_END_TEST;

GST_START_TEST(TPL_dxpreprocess_not_any) {
    assert_no_any_template("dxpreprocess");
}
GST_END_TEST;

GST_START_TEST(TPL_dxosd_not_any) {
    assert_no_any_template("dxosd");
}
GST_END_TEST;

GST_START_TEST(TPL_dxtracker_not_any) {
    assert_no_any_template("dxtracker");
}
GST_END_TEST;

GST_START_TEST(TPL_dxscale_not_any) {
    assert_no_any_template("dxscale");
}
GST_END_TEST;

GST_START_TEST(TPL_dxconvert_not_any) {
    assert_no_any_template("dxconvert");
}
GST_END_TEST;

GST_START_TEST(TPL_dxmsgconv_not_any) {
    assert_no_any_template("dxmsgconv");
}
GST_END_TEST;

GST_START_TEST(TPL_dxinputselector_not_any) {
    assert_no_any_template("dxinputselector");
}
GST_END_TEST;

GST_START_TEST(TPL_dxoutputselector_not_any) {
    assert_no_any_template("dxoutputselector");
}
GST_END_TEST;

static Suite *pad_templates_suite(void) {
    Suite *s = suite_create("pad_templates");
    TCase *tc = tcase_create("caps_any_forbidden");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TPL_dxinfer_not_any);
    tcase_add_test(tc, TPL_dxrate_not_any);
    tcase_add_test(tc, TPL_dxpostprocess_not_any);
    tcase_add_test(tc, TPL_dxmsgbroker_not_any);
    tcase_add_test(tc, TPL_dxgather_not_any);
    tcase_add_test(tc, TPL_dxpreprocess_not_any);
    tcase_add_test(tc, TPL_dxosd_not_any);
    tcase_add_test(tc, TPL_dxtracker_not_any);
    tcase_add_test(tc, TPL_dxscale_not_any);
    tcase_add_test(tc, TPL_dxconvert_not_any);
    tcase_add_test(tc, TPL_dxmsgconv_not_any);
    tcase_add_test(tc, TPL_dxinputselector_not_any);
    tcase_add_test(tc, TPL_dxoutputselector_not_any);
    return s;
}

GST_CHECK_MAIN(pad_templates);
