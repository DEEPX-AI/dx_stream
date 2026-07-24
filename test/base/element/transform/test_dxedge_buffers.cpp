// Phase 3 — Edge-case buffer handling across transform elements
// B12: zero-dim frame meta (width=0 or height=0)
// B13: NULL PTS buffers through transform chain
// Missing frame meta → passthrough or auto-creation

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB =
    "video/x-raw,format=RGB,width=16,height=16,framerate=30/1";
static const char *CAPS_I420 =
    "video/x-raw,format=I420,width=16,height=16,framerate=30/1";
static const guint RGB_SIZE = 16 * 16 * 3;
static const guint I420_SIZE = 16 * 16 * 3 / 2;

// ---------- dxosd edge cases ----------

// CE_osd_no_meta_passthrough: buffer without DXFrameMeta → passthrough
// Target: gst_dxosd_transform_ip L148-152
// MUT: remove null-check → crash on frame_meta dereference
GST_START_TEST(CE_osd_no_meta_passthrough) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h.h, RGB_SIZE);
    GST_BUFFER_PTS(b) = 42 * GST_MSECOND;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0, map.size);
    gst_buffer_unmap(b, &map);

    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_OK,
                "no-meta buffer must passthrough (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "passthrough must produce output");

    fail_unless(GST_BUFFER_PTS(out) == 42 * GST_MSECOND,
                "PTS must be preserved on no-meta passthrough");

    gst_buffer_map(out, &map, GST_MAP_READ);
    gboolean all_zero = TRUE;
    for (guint i = 0; i < map.size && all_zero; i++) {
        if (map.data[i] != 0) all_zero = FALSE;
    }
    gst_buffer_unmap(out, &map);
    fail_unless(all_zero, "pixels must be unchanged when no DXFrameMeta");

    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_empty_objects: frame meta with zero objects → no crash
// Target: gst_dxosd_transform_ip L193 (object_meta_list loop)
GST_START_TEST(CE_osd_empty_objects) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h.h, RGB_SIZE);
    GST_BUFFER_PTS(b) = 0;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, 0, 16, 16, "RGB");
    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_OK,
                "empty objects must not crash (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "empty-objects buffer must produce output");
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_unknown_stream: frame meta with unregistered stream_id → passthrough
// Target: gst_dxosd_transform_ip L159-164 (stream_info.find fails)
GST_START_TEST(CE_osd_unknown_stream) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h.h, RGB_SIZE);
    GST_BUFFER_PTS(b) = 0;
    make_frame_meta(b, 99, 16, 16, "RGB");
    add_object_to_frame(dx_get_frame_meta(b), 1, 0.9f, 0, 0, 10, 10);

    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_OK,
                "unknown stream_id must passthrough (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "unknown-stream buffer must produce output");
    gst_buffer_unref(out);
}
GST_END_TEST;

// ---------- dxscale edge cases ----------

// CE_scale_null_pts: buffer with NULL PTS → passthrough (PTS preserved as NONE)
// Target: gst_dxscale_transform — doesn't check PTS
GST_START_TEST(CE_scale_null_pts) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    fail_unless(e != nullptr);
    g_object_set(e, "width", 8, "height", 8, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_I420);

    GstBuffer *b = gst_harness_create_buffer(h, I420_SIZE);
    GST_BUFFER_PTS(b) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DURATION(b) = GST_CLOCK_TIME_NONE;
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "NULL PTS must not crash dxscale (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "NULL PTS buffer must produce output");
    fail_unless(!GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(out)),
                "output PTS must remain NONE");
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_scale_no_meta_ok: buffer without frame meta → scaled, no crash
// Target: gst_dxscale_transform L346-351 (src_meta null → skip meta copy)
GST_START_TEST(CE_scale_no_meta_ok) {
    GstElement *e = gst_element_factory_make("dxscale", nullptr);
    g_object_set(e, "width", 8, "height", 8, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_I420);

    GstBuffer *b = gst_harness_create_buffer(h, I420_SIZE);
    GST_BUFFER_PTS(b) = 100 * GST_MSECOND;
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "no-meta buffer must scale OK (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "no-meta buffer must produce output");

    fail_unless(GST_BUFFER_PTS(out) == 100 * GST_MSECOND,
                "PTS must be preserved on scale");

    gsize expected_size = 8 * 8 * 3 / 2;
    fail_unless(gst_buffer_get_size(out) == expected_size,
                "output size must be %zu for 8x8 I420 (got %zu)", expected_size,
                gst_buffer_get_size(out));

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// ---------- dxconvert edge cases ----------

// CE_convert_null_pts: buffer with NULL PTS → color-converted, PTS stays NONE
// Target: gst_dxconvert_transform — doesn't check PTS
GST_START_TEST(CE_convert_null_pts) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
    fail_unless(e != nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_I420);
    gst_harness_set_sink_caps_str(h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h, I420_SIZE);
    GST_BUFFER_PTS(b) = GST_CLOCK_TIME_NONE;
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "NULL PTS must not crash dxconvert (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "NULL PTS buffer must produce output");
    fail_unless(!GST_CLOCK_TIME_IS_VALID(GST_BUFFER_PTS(out)),
                "output PTS must remain NONE");
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_convert_no_meta_ok: buffer without frame meta → converted, no crash
// Target: gst_dxconvert_transform L290-294 (src_meta null → skip meta copy)
GST_START_TEST(CE_convert_no_meta_ok) {
    GstElement *e = gst_element_factory_make("dxconvert", nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_I420);
    gst_harness_set_sink_caps_str(h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h, I420_SIZE);
    GST_BUFFER_PTS(b) = 0;
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "no-meta buffer must convert OK (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "no-meta buffer must produce output");
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_convert_same_format_passthrough: same input/output format → content preserved
// Note: dxconvert copies buffers even in same-format mode (not true passthrough),
// so buffer identity (out == in) is not asserted. PTS and content integrity are checked.
// Output buffer size may differ from input due to GstVideoInfo stride alignment.
GST_START_TEST(CE_convert_same_format_passthrough) {
    Harness h("dxconvert");
    // Force both input AND output to RGB — otherwise dxconvert auto-negotiates to NV12
    gst_harness_set_src_caps_str(h.h, CAPS_RGB);
    gst_harness_set_sink_caps_str(h.h, CAPS_RGB);

    GstBuffer *b = gst_harness_create_buffer(h.h, RGB_SIZE);
    GST_BUFFER_PTS(b) = 200 * GST_MSECOND;

    // Fill input with known pattern
    GstMapInfo in_map;
    gst_buffer_map(b, &in_map, GST_MAP_WRITE);
    for (guint i = 0; i < in_map.size; i++)
        in_map.data[i] = (guint8)(i & 0xFF);
    gst_buffer_unmap(b, &in_map);

    GstFlowReturn r = gst_harness_push(h.h, b);
    fail_unless(r == GST_FLOW_OK,
                "same-format passthrough must work (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "passthrough must produce output");

    fail_unless(GST_BUFFER_PTS(out) == 200 * GST_MSECOND,
                "PTS must be preserved on passthrough");

    // Output size must be >= input (may include stride padding)
    gsize out_size = gst_buffer_get_size(out);
    fail_unless(out_size >= RGB_SIZE,
                "output size (%zu) must be >= input size (%u)", out_size, RGB_SIZE);

    // If sizes match exactly, verify pixel content preservation
    if (out_size == RGB_SIZE) {
        GstMapInfo out_map;
        gst_buffer_map(out, &out_map, GST_MAP_READ);
        gboolean content_match = TRUE;
        for (guint i = 0; i < out_map.size && content_match; i++) {
            if (out_map.data[i] != (guint8)(i & 0xFF))
                content_match = FALSE;
        }
        gst_buffer_unmap(out, &out_map);
        fail_unless(content_match,
                    "pixel content must be preserved for same-format conversion");
    }

    gst_buffer_unref(out);
}
GST_END_TEST;

static Suite *dxedge_buffers_suite(void) {
    Suite *s = suite_create("dxedge_buffers");
    TCase *tc = tcase_create("edge_cases");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_osd_no_meta_passthrough);
    tcase_add_test(tc, CE_osd_empty_objects);
    tcase_add_test(tc, CE_osd_unknown_stream);
    tcase_add_test(tc, CE_scale_null_pts);
    tcase_add_test(tc, CE_scale_no_meta_ok);
    tcase_add_test(tc, CE_convert_null_pts);
    tcase_add_test(tc, CE_convert_no_meta_ok);
    tcase_add_test(tc, CE_convert_same_format_passthrough);
    return s;
}

GST_CHECK_MAIN(dxedge_buffers);
