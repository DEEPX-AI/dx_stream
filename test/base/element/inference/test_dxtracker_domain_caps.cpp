// dxtracker pad template caps test — TDD red phase for
// refactor_plans/event_query_state_audit.md §3.7
//
// Plan README §"Element 배치 매트릭스" classifies dxtracker as Dual:
//   ✅ in-domain (application/x-dxvideoraw)
//   ✅ outside-domain (video/x-raw)
//
// Caps templates must enforce this — sink/src should accept dxvideoraw +
// video/x-raw, but NOTHING ELSE. The current implementation uses
// GST_CAPS_ANY (gst-dxtracker.cpp:213-217) so even nonsense like
// audio/x-raw or text/plain links successfully, defeating the
// "caps negotiation auto-blocks misplaced element" property the plan relies on.
//
// Currently FAILING: tpl is ANY → intersects everything.
// Target after fix: tpl is `application/x-dxvideoraw; video/x-raw,...`.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

static void assert_template(const char *padname) {
    GstElement *e = gst_element_factory_make("dxtracker", nullptr);
    fail_unless(e != nullptr);

    GstPadTemplate *tpl = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(e), padname);
    fail_unless(tpl != nullptr, "pad template %s missing", padname);

    GstCaps *caps = gst_pad_template_get_caps(tpl);
    fail_unless(caps != nullptr);

    fail_if(gst_caps_is_any(caps),
            "dxtracker %s template must NOT be ANY — domain caps negotiation "
            "cannot auto-block misplaced elements when ANY accepts everything",
            padname);

    GstCaps *dxvideoraw = gst_caps_from_string("application/x-dxvideoraw");
    fail_unless(gst_caps_can_intersect(caps, dxvideoraw),
                "dxtracker %s template must accept dxvideoraw (in-domain)",
                padname);
    gst_caps_unref(dxvideoraw);

    GstCaps *vraw = gst_caps_from_string("video/x-raw");
    fail_unless(gst_caps_can_intersect(caps, vraw),
                "dxtracker %s template must accept video/x-raw (out-of-domain)",
                padname);
    gst_caps_unref(vraw);

    GstCaps *audio = gst_caps_from_string("audio/x-raw");
    fail_if(gst_caps_can_intersect(caps, audio),
            "dxtracker %s template must NOT accept audio/x-raw "
            "(domain integrity)",
            padname);
    gst_caps_unref(audio);

    gst_object_unref(e);
}

GST_START_TEST(TRK_sink_template_constrained) {
    assert_template("sink");
}
GST_END_TEST;

GST_START_TEST(TRK_src_template_constrained) {
    assert_template("src");
}
GST_END_TEST;

static Suite *dxtracker_caps_suite(void) {
    Suite *s = suite_create("dxtracker_caps");
    TCase *tc = tcase_create("templates");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TRK_sink_template_constrained);
    tcase_add_test(tc, TRK_src_template_constrained);
    return s;
}

GST_CHECK_MAIN(dxtracker_caps);
