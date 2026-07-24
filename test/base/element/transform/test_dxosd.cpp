// P2.4 — dxosd contract tests (rewritten)
// Core: dxosd is an in-place transform. Renders DXFrameMeta bbox onto video.
// Key branches: frame_meta presence (L148), stream_id→video_info lookup (L159),
//               per-format drawing (L181 RGB, L196 NV12, L216 I420).
// In-place, so meta/PTS preservation TCs are meaningless (same buffer).

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB_320 =
    "video/x-raw,format=RGB,width=320,height=240,framerate=30/1";

static GstBuffer *make_rgb_buffer(int w, int h, GstClockTime pts) {
    gsize sz = w * h * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    return b;
}

static int count_nonzero_bytes(GstBuffer *buf) {
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);
    int count = 0;
    for (gsize i = 0; i < map.size; i++) {
        if (map.data[i] != 0) count++;
    }
    gst_buffer_unmap(buf, &map);
    return count;
}

static int count_nonzero_in_row(GstBuffer *buf, int w, int row, int x1, int x2) {
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);
    int count = 0;
    int start = (row * w + x1) * 3;
    int end   = (row * w + x2) * 3;
    for (int i = start; i < end && i < (int)map.size; i++) {
        if (map.data[i] != 0) count++;
    }
    gst_buffer_unmap(buf, &map);
    return count;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxosd", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxosd", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CC1_pad_templates) {
    GstElement *e = gst_element_factory_make("dxosd", nullptr);
    GstPadTemplate *sink_t = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(e), "sink");
    fail_unless(sink_t != nullptr);
    GstCaps *caps = gst_pad_template_get_caps(sink_t);
    GstCaps *rgb = gst_caps_from_string("video/x-raw,format=RGB");
    fail_unless(gst_caps_can_intersect(caps, rgb));
    gst_caps_unref(rgb);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_osd_no_meta_pixels_intact: buffer without DXFrameMeta → all pixels remain 0
// Target: gst_dxosd_transform_ip L148-152 (frame_meta == nullptr → early return)
// MUT: remove L149-151 → null deref on frame_meta->_stream_id → crash
GST_START_TEST(CE_osd_no_meta_pixels_intact) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    gst_harness_push(h.h, b);

    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "no-meta buffer must pass through");
    int nz = count_nonzero_bytes(out);
    fail_unless_equals_int(nz, 0);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_stream_mismatch_no_draw: stream_id mismatch → no video_info → pixels unchanged
// Target: gst_dxosd_transform_ip L159-164 (_stream_info.find() fails → early return)
// MUT: remove L160-163 → invalid iterator used → crash or wrong drawing
GST_START_TEST(CE_osd_stream_mismatch_no_draw) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    DXFrameMeta *fm = make_frame_meta(b, 99, 320, 240);
    DXObjectMeta *o = add_object_to_frame(fm, 1, 0.9f, 10.0f, 10.0f, 60.0f, 60.0f);
    o->_label_name = "person";

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "mismatched stream must pass through");
    int nz = count_nonzero_bytes(out);
    fail_unless_equals_int(nz, 0);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_bbox_draws_at_location: bbox location pixels changed, far pixels unchanged
// Target: gst_dxosd_transform_ip L181-195 (RGB path, draw_object_meta call)
//       + gst_dxosd_sink_event L136 (CAPS → set_stream_info for stream 0)
// MUT: remove L193-195 (for loop + draw_object_meta) → no pixel changes → fail
GST_START_TEST(CE_osd_bbox_draws_at_location) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    int W = 320, H = 240;
    GstBuffer *b = make_rgb_buffer(W, H, 0);
    DXFrameMeta *fm = make_frame_meta(b, 0, W, H);
    DXObjectMeta *o = add_object_to_frame(fm, 1, 0.95f, 100.0f, 80.0f, 220.0f, 160.0f);
    o->_label_name = "car";

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "bbox buffer must produce output");

    int bbox_nz = count_nonzero_in_row(out, W, 80, 100, 220);
    fail_unless(bbox_nz > 0,
                "pixels at bbox top edge (y=80, x=100-220) must be drawn");

    GstMapInfo map;
    gst_buffer_map(out, &map, GST_MAP_READ);
    int corner_nz = 0;
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int idx = (y * W + x) * 3;
            if (map.data[idx] != 0 || map.data[idx+1] != 0 || map.data[idx+2] != 0)
                corner_nz++;
        }
    }
    gst_buffer_unmap(out, &map);
    fail_unless_equals_int(corner_nz, 0);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_wrapped_caps_stream_draw: wrapped CAPS event registers per-stream video_info
// Target: gst_dxosd_sink_event L120-133 (wrapped event parsing + set_stream_info)
// MUT: remove L127-128 → stream 3 video_info not registered → early return at L159 → no draw
GST_START_TEST(CE_osd_wrapped_caps_stream_draw) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    GstCaps *caps3 = gst_caps_from_string(CAPS_RGB_320);
    GstEvent *caps_event = gst_event_new_caps(caps3);
    gst_caps_unref(caps3);
    GstStructure *ws = gst_structure_new("application/x-dx-wrapped-event",
        "stream-id", G_TYPE_INT, 3,
        "event", GST_TYPE_EVENT, caps_event, NULL);
    GstEvent *wrapped = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, ws);
    gst_event_unref(caps_event);
    gst_harness_push_event(h.h, wrapped);

    int W = 320, H = 240;
    GstBuffer *b = make_rgb_buffer(W, H, 0);
    DXFrameMeta *fm = make_frame_meta(b, 3, W, H);
    DXObjectMeta *o = add_object_to_frame(fm, 1, 0.9f, 50.0f, 50.0f, 150.0f, 100.0f);
    o->_label_name = "person";

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "wrapped-caps buffer must produce output");

    int nz = count_nonzero_bytes(out);
    fail_unless(nz > 0,
                "wrapped CAPS for stream 3 must enable drawing (got %d nonzero)", nz);
    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_osd_scale_adjusts_bbox: frame_meta resolution != actual frame → bbox scale adjustment
// Target: gst_dxosd_transform_ip L187-188 (scale_x, scale_y calculation)
// MUT: remove scale calculation (or fix at 1.0) → bbox coords unadjusted → out of bounds → no draw
GST_START_TEST(CE_osd_scale_adjusts_bbox) {
    Harness h("dxosd");
    gst_harness_set_src_caps_str(h.h, CAPS_RGB_320);

    int W = 320, H = 240;
    GstBuffer *b = make_rgb_buffer(W, H, 0);
    // frame_meta: 640x480 (2x actual) → scale_x=2.0, scale_y=2.0
    DXFrameMeta *fm = make_frame_meta(b, 0, 640, 480);
    // bbox in meta coords: (500,400)-(600,460) — outside 320x240 without scaling
    // With scaling (/2): (250,200)-(300,230) — inside actual frame
    DXObjectMeta *o = add_object_to_frame(fm, 1, 0.9f, 500.0f, 400.0f, 600.0f, 460.0f);
    o->_label_name = "truck";

    gst_harness_push(h.h, b);
    GstBuffer *out = gst_harness_try_pull(h.h);
    fail_unless(out != nullptr, "scaled bbox buffer must produce output");

    int scaled_nz = count_nonzero_in_row(out, W, 200, 250, 300);
    fail_unless(scaled_nz > 0,
                "bbox must be drawn at scaled location (y=200, x=250-300), got %d nz",
                scaled_nz);
    gst_buffer_unref(out);
}
GST_END_TEST;

static Suite *dxosd_suite(void) {
    Suite *s = suite_create("dxosd");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CC1_pad_templates);
    tcase_add_test(tc, CE_osd_no_meta_pixels_intact);
    tcase_add_test(tc, CE_osd_stream_mismatch_no_draw);
    tcase_add_test(tc, CE_osd_bbox_draws_at_location);
    tcase_add_test(tc, CE_osd_wrapped_caps_stream_draw);
    tcase_add_test(tc, CE_osd_scale_adjusts_bbox);
    return s;
}

GST_CHECK_MAIN(dxosd);
