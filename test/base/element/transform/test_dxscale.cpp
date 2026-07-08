// P2.3 — dxscale contract tests (rewritten)
// Core: dxscale is a kernel-based video resizer. Output resolution set by width/height properties.
// Same size → gst_copy_video_frame (meta copy skip), resize → kernel + meta copy.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

using namespace dxtest;

static const char *CAPS_RGB_320 =
    "video/x-raw,format=RGB,width=320,height=240,framerate=30/1";

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    guint w = 999, h2 = 999;
    g_object_get(e, "width", &w, "height", &h2, nullptr);
    fail_unless_equals_int(w, 0);
    fail_unless_equals_int(h2, 0);
    g_object_set(e, "width", 320u, "height", 240u, nullptr);
    g_object_get(e, "width", &w, "height", &h2, nullptr);
    fail_unless_equals_int(w, 320);
    fail_unless_equals_int(h2, 240);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_scale_rejects_domain_caps: dxscale is normal-mode only — domain caps must NOT
// intersect sink/src templates so misplaced dxscale inside dxinputselector→dxoutputselector
// fails caps negotiation at link-time rather than silently corrupting per-buffer dims.
// Target: gst-dxscale.cpp pad template strings (no application/x-dxvideoraw)
GST_START_TEST(CE_scale_rejects_domain_caps) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    GstCaps *domain = gst_caps_from_string("application/x-dxvideoraw");

    for (const char *padname : {"sink", "src"}) {
        GstPadTemplate *t = gst_element_class_get_pad_template(
            GST_ELEMENT_GET_CLASS(e), padname);
        fail_unless(t != nullptr);
        GstCaps *tpl = gst_pad_template_get_caps(t);
        fail_if(gst_caps_can_intersect(tpl, domain),
                "%s template must NOT accept domain caps (dxvideoraw)", padname);
    }
    gst_caps_unref(domain);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_scale_output_dims: width/height property → output caps resolution matches
// Target: gst_dxscale_transform_caps L206-210 (width/height applied)
// MUT: width/height not applied in transform_caps → output resolution mismatch → fail
GST_START_TEST(CE_scale_output_dims) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(
        "videotestsrc num-buffers=5 "
        "! video/x-raw,format=I420,width=640,height=480,framerate=30/1 "
        "! dxscale width=320 height=240 "
        "! fakesink name=fsink", &err);
    fail_unless(err == nullptr && pipe != nullptr);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10*GST_SECOND,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    fail_unless(msg != nullptr);
    fail_unless_equals_int(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);

    GstElement *fsink = gst_bin_get_by_name(GST_BIN(pipe), "fsink");
    GstPad *sinkpad = gst_element_get_static_pad(fsink, "sink");
    GstCaps *caps = gst_pad_get_current_caps(sinkpad);
    fail_unless(caps != nullptr);
    GstStructure *s = gst_caps_get_structure(caps, 0);
    gint w = 0, h2 = 0;
    gst_structure_get_int(s, "width", &w);
    gst_structure_get_int(s, "height", &h2);
    fail_unless_equals_int(w, 320);
    fail_unless_equals_int(h2, 240);

    gst_caps_unref(caps);
    gst_object_unref(sinkpad);
    gst_object_unref(fsink);
    gst_message_unref(msg);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

// CE_scale_output_buffer_size: resize output buffer size = plane_size(new_w, new_h)
// Target: gst_dxscale_transform_size (GST_VIDEO_INFO_SIZE)
// + gst_dxscale_transform L330-343 (kernel transform)
// MUT: remove transform_size → negotiation failure → no output
GST_START_TEST(CE_scale_output_buffer_size) {
    Harness h("dxscale");
    g_object_set(h.element(), "width", 160u, "height", 120u, nullptr);
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = gst_harness_create_buffer(h.h, 320*240*3);
    GST_BUFFER_PTS(b) = 0;
    gst_harness_push(h.h, b);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "resize must produce output");
    gsize expected = 160 * 120 * 3;
    gsize actual = gst_buffer_get_size(out);
    fail_unless(actual == expected,
                "output size=%zu, expected %zu (160x120 RGB)", actual, expected);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_scale_meta_on_resize: DXFrameMeta copied in resize path
// Target: gst_dxscale_transform L345-351 (meta copy code)
// MUT: remove L345-351 → output has no DXFrameMeta → fail
GST_START_TEST(CE_scale_meta_on_resize) {
    Harness h("dxscale");
    g_object_set(h.element(), "width", 160u, "height", 120u, nullptr);
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = gst_harness_create_buffer(h.h, 320*240*3);
    GST_BUFFER_PTS(b) = 0;
    b = dx_create_frame_meta(b);
    DXFrameMeta *fm = dx_get_frame_meta(b);
    fm->_stream_id = 3; fm->_width = 320; fm->_height = 240;

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "resize must produce output");

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr, "DXFrameMeta must be copied in resize path");
    fail_unless_equals_int(ofm->_stream_id, 3);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_scale_pts_on_resize: PTS preserved in resize path
// Target: gst_dxscale_transform L345 (GST_BUFFER_COPY_TIMESTAMPS)
// MUT: remove L345 → PTS not copied → fail
GST_START_TEST(CE_scale_pts_on_resize) {
    Harness h("dxscale");
    g_object_set(h.element(), "width", 160u, "height", 120u, nullptr);
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = gst_harness_create_buffer(h.h, 320*240*3);
    GST_BUFFER_PTS(b) = 7777777;
    gst_harness_push(h.h, b);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr);
    fail_unless(GST_BUFFER_PTS(out) == 7777777,
                "PTS must be preserved in resize path, got %" G_GUINT64_FORMAT,
                GST_BUFFER_PTS(out));
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_scale_same_size_skips_meta: same-size path skips meta copy
// (known implementation gap pin -- gst_copy_video_frame does not copy meta)
// Target: gst_dxscale_transform L327-328 (same-size early return)
// This TC documents the implementation gap. Update this TC if fixed.
GST_START_TEST(CE_scale_same_size_skips_meta) {
    Harness h("dxscale");
    // width=0,height=0 (default) → same dimensions as input
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = gst_harness_create_buffer(h.h, 320*240*3);
    GST_BUFFER_PTS(b) = 0;
    b = dx_create_frame_meta(b);
    DXFrameMeta *fm = dx_get_frame_meta(b);
    fm->_stream_id = 5;

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "same-size must produce output");

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    // known gap: same-size path returns from gst_copy_video_frame BEFORE meta copy
    fail_unless(ofm == nullptr,
                "same-size path skips meta copy (known gap). "
                "If this fails, the gap has been fixed — update this test.");
    gst_buffer_unref(out);
}
GST_END_TEST;

static Suite *dxscale_suite(void) {
    Suite *s = suite_create("dxscale");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_scale_rejects_domain_caps);
    tcase_add_test(tc, CE_scale_output_dims);
    tcase_add_test(tc, CE_scale_output_buffer_size);
    tcase_add_test(tc, CE_scale_meta_on_resize);
    tcase_add_test(tc, CE_scale_pts_on_resize);
    tcase_add_test(tc, CE_scale_same_size_skips_meta);
    return s;
}

GST_CHECK_MAIN(dxscale);
