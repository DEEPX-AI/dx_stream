// P2.2 — dxconvert contract tests (rewritten)
// Core: dxconvert is a kernel-based color format converter (RGB<->BGR<->I420<->NV12).
// Same format → passthrough, different format → kernel transform + meta copy.
// All CE_conv TCs target specific lines in the source.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CC1_pad_templates) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
    GstPadTemplate *sink_t = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(e), "sink");
    fail_unless(sink_t != nullptr);
    GstCaps *sink_caps = gst_pad_template_get_caps(sink_t);
    fail_if(gst_caps_is_any(sink_caps), "dxconvert must not have ANY caps");
    GstCaps *rgb = gst_caps_from_string("video/x-raw,format=RGB");
    fail_unless(gst_caps_can_intersect(sink_caps, rgb));
    gst_caps_unref(rgb);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_conv_rejects_domain_caps: dxconvert is normal-mode only — domain caps must NOT intersect
// sink/src templates so caps negotiation auto-blocks domain placement.
// Target: gst-dxconvert.cpp pad template strings (no application/x-dxvideoraw)
// MUT: changing template to include dxvideoraw would make this TC fail.
GST_START_TEST(CE_conv_rejects_domain_caps) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
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

// CE_conv_rgb_to_bgr_pixels: RGB→BGR conversion pixel channel swap verification
// Target: gst_dxconvert_transform L279 (kernel_pool->transform call)
// MUT: remove L279 → output pixels same as input (R/B not swapped) → fail
GST_START_TEST(CE_conv_rgb_to_bgr_pixels) {
    Harness h("dxconvert");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=RGB,width=4,height=4,framerate=1/1");
    gst_harness_set_sink_caps_str(h.h,
        "video/x-raw,format=BGR,width=4,height=4,framerate=1/1");

    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    for (gsize i = 0; i < sz; i += 3) {
        map.data[i]   = 0xFF; // R
        map.data[i+1] = 0x00; // G
        map.data[i+2] = 0x00; // B
    }
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 0;
    GST_BUFFER_DURATION(b) = GST_SECOND;

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "conversion must produce output");

    GstMapInfo omap;
    gst_buffer_map(out, &omap, GST_MAP_READ);
    // BGR format: red = B=0x00, G=0x00, R=0xFF
    fail_unless_equals_int(omap.data[0], 0x00);  // B (was R=0xFF in RGB)
    fail_unless_equals_int(omap.data[1], 0x00);  // G
    fail_unless_equals_int(omap.data[2], 0xFF);  // R (was B=0x00 in RGB)
    gst_buffer_unmap(out, &omap);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_conv_passthrough: same format → passthrough (returns same buffer)
// Target: gst_dxconvert_set_caps L201-206 (passthrough=TRUE set)
// MUT: remove L205 (set_passthrough) → different buffer allocated → fail
GST_START_TEST(CE_conv_passthrough) {
    Harness h("dxconvert");
    gst_harness_set_sink_caps_str(h.h,
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1");

    gsize sz = 320 * 240 * 3 / 2;
    GstBuffer *b = gst_harness_create_buffer(h.h, sz);
    GST_BUFFER_PTS(b) = 0;
    gst_buffer_ref(b);
    gst_harness_push(h.h, b);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "passthrough must produce output");
    fail_unless(out == b,
                "same format (I420→I420) must return same buffer (passthrough)");
    gst_buffer_unref(b);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_conv_meta_on_conversion: DXFrameMeta copied in format conversion path
// Target: gst_dxconvert_transform L289-295 (meta copy code)
// MUT: remove L289-295 → output has no DXFrameMeta → fail
GST_START_TEST(CE_conv_meta_on_conversion) {
    Harness h("dxconvert");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=RGB,width=4,height=4,framerate=1/1");
    gst_harness_set_sink_caps_str(h.h,
        "video/x-raw,format=BGR,width=4,height=4,framerate=1/1");

    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 12345;

    b = dx_create_frame_meta(b);
    DXFrameMeta *fm = dx_get_frame_meta(b);
    fm->_stream_id = 7;
    fm->_width = 4;
    fm->_height = 4;

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "conversion must produce output");
    fail_unless(out != b, "conversion path must allocate new buffer");

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr,
                "DXFrameMeta must be copied in conversion path");
    fail_unless_equals_int(ofm->_stream_id, 7);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_conv_output_size: I420→RGB conversion output buffer size differs
// Target: gst_dxconvert_transform_size L233-249 (GST_VIDEO_INFO_SIZE)
// MUT: remove transform_size → caps negotiation failure → no output
GST_START_TEST(CE_conv_output_size) {
    Harness h("dxconvert");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=I420,width=320,height=240,framerate=30/1");
    gst_harness_set_sink_caps_str(h.h,
        "video/x-raw,format=RGB,width=320,height=240,framerate=30/1");

    gsize i420_sz = 320 * 240 * 3 / 2;
    gsize rgb_sz  = 320 * 240 * 3;
    GstBuffer *b = gst_harness_create_buffer(h.h, i420_sz);
    GST_BUFFER_PTS(b) = 0;
    gst_harness_push(h.h, b);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "I420→RGB must produce output");
    gsize out_sz = gst_buffer_get_size(out);
    fail_unless(out_sz == rgb_sz,
                "output size=%zu, expected RGB size=%zu", out_sz, rgb_sz);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_conv_pts_on_conversion: PTS preserved in conversion path
// Target: gst_dxconvert_transform L289 (GST_BUFFER_COPY_TIMESTAMPS)
// MUT: remove L289 → PTS 0 (uninitialized) → fail
GST_START_TEST(CE_conv_pts_on_conversion) {
    Harness h("dxconvert");
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=RGB,width=4,height=4,framerate=1/1");
    gst_harness_set_sink_caps_str(h.h,
        "video/x-raw,format=BGR,width=4,height=4,framerate=1/1");

    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 9876543;

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr);
    fail_unless(GST_BUFFER_PTS(out) == 9876543,
                "PTS must be preserved in conversion path, got %" G_GUINT64_FORMAT,
                GST_BUFFER_PTS(out));
    gst_buffer_unref(out);
}
GST_END_TEST;

static Suite *dxconvert_suite(void) {
    Suite *s = suite_create("dxconvert");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CC1_pad_templates);
    tcase_add_test(tc, CE_conv_rejects_domain_caps);
    tcase_add_test(tc, CE_conv_rgb_to_bgr_pixels);
    tcase_add_test(tc, CE_conv_passthrough);
    tcase_add_test(tc, CE_conv_meta_on_conversion);
    tcase_add_test(tc, CE_conv_output_size);
    tcase_add_test(tc, CE_conv_pts_on_conversion);
    return s;
}

GST_CHECK_MAIN(dxconvert);
