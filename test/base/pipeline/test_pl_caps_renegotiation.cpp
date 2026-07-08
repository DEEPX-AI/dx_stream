// Phase 6 — CAPS renegotiation mid-stream
// B2: same element receives different caps mid-stream → dxscale/dxconvert re-negotiate

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"

#include <cstring>

using namespace dxtest;

static GstBuffer *make_buf(int w, int h, GstClockTime pts) {
    gsize sz = w * h * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, map.size);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

// CE_scale_caps_renego: dxscale processes frames, then new caps → new output size
// Target: gst_dxscale_set_caps L264 (kernel_pool reset + rebuild on renegotiation)
// MUT: skip kernel_pool.reset() → stale transform → wrong output size or crash
GST_START_TEST(CE_scale_caps_renego) {
    GstHarness *h = gst_harness_new("dxscale");
    g_object_set(h->element, "width", 32u, "height", 32u, nullptr);

    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");

    GstBuffer *b1 = make_buf(64, 64, 0);
    GstFlowReturn r = gst_harness_push(h, b1);
    fail_unless(r == GST_FLOW_OK, "first push failed: %s", gst_flow_get_name(r));

    GstBuffer *out1 = gst_harness_pull(h);
    fail_unless(out1 != nullptr, "first output expected");

    GstCaps *out_caps1 = gst_pad_get_current_caps(h->sinkpad);
    GstVideoInfo info1;
    gst_video_info_from_caps(&info1, out_caps1);
    fail_unless_equals_int(GST_VIDEO_INFO_WIDTH(&info1), 32);
    fail_unless_equals_int(GST_VIDEO_INFO_HEIGHT(&info1), 32);
    gst_caps_unref(out_caps1);
    gst_buffer_unref(out1);

    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=128,height=128,framerate=30/1");

    GstBuffer *b2 = make_buf(128, 128, GST_SECOND);
    r = gst_harness_push(h, b2);
    fail_unless(r == GST_FLOW_OK, "renegotiated push failed: %s", gst_flow_get_name(r));

    GstBuffer *out2 = gst_harness_pull(h);
    fail_unless(out2 != nullptr, "renegotiated output expected");

    GstCaps *out_caps2 = gst_pad_get_current_caps(h->sinkpad);
    GstVideoInfo info2;
    gst_video_info_from_caps(&info2, out_caps2);
    fail_unless_equals_int(GST_VIDEO_INFO_WIDTH(&info2), 32);
    fail_unless_equals_int(GST_VIDEO_INFO_HEIGHT(&info2), 32);
    gst_caps_unref(out_caps2);
    gst_buffer_unref(out2);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_convert_caps_renego: dxconvert format change mid-stream
// Target: gst_dxconvert set_caps kernel rebuild
GST_START_TEST(CE_convert_caps_renego) {
    GstHarness *h = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=16,height=16,framerate=30/1");
    gst_harness_set_sink_caps_str(h,
        "video/x-raw,format=BGR,width=16,height=16,framerate=30/1");

    GstBuffer *b1 = make_buf(16, 16, 0);
    gst_harness_push(h, b1);
    GstBuffer *out1 = gst_harness_pull(h);
    fail_unless(out1 != nullptr);
    gst_buffer_unref(out1);

    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=I420,width=16,height=16,framerate=30/1");
    gst_harness_set_sink_caps_str(h,
        "video/x-raw,format=RGB,width=16,height=16,framerate=30/1");

    gsize i420_sz = 16 * 16 + 2 * (8 * 8);
    GstBuffer *b2 = gst_buffer_new_allocate(nullptr, i420_sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b2, &map, GST_MAP_WRITE);
    memset(map.data, 128, map.size);
    gst_buffer_unmap(b2, &map);
    GST_BUFFER_PTS(b2) = GST_SECOND;

    GstFlowReturn r = gst_harness_push(h, b2);
    fail_unless(r == GST_FLOW_OK, "renegotiated I420→RGB push failed: %s",
                gst_flow_get_name(r));

    GstBuffer *out2 = gst_harness_pull(h);
    fail_unless(out2 != nullptr, "renegotiated output expected");

    GstMapInfo out_map;
    gst_buffer_map(out2, &out_map, GST_MAP_READ);
    fail_unless(out_map.size == 16 * 16 * 3,
                "output size must be RGB 16x16 (got %zu)", out_map.size);
    gst_buffer_unmap(out2, &out_map);
    gst_buffer_unref(out2);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_rate_caps_renego: dxrate continues to work after caps change
// Target: dxrate framerate property applies across caps renegotiation
GST_START_TEST(CE_rate_caps_renego) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 15u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    GstClockTime dur30 = GST_SECOND / 30;
    for (int i = 0; i < 5; i++) {
        GstBuffer *b = gst_harness_create_buffer(h.h, 64 * 64 * 3 / 2);
        GST_BUFFER_PTS(b) = i * dur30;
        GST_BUFFER_DURATION(b) = dur30;
        gst_harness_push(h.h, b);
    }

    gst_harness_set_src_caps_str(h.h,
        "video/x-raw,format=I420,width=32,height=32,framerate=30/1");

    for (int i = 5; i < 10; i++) {
        GstBuffer *b = gst_harness_create_buffer(h.h, 32 * 32 * 3 / 2);
        GST_BUFFER_PTS(b) = i * dur30;
        GST_BUFFER_DURATION(b) = dur30;
        gst_harness_push(h.h, b);
    }

    int out_count = gst_harness_buffers_received(h.h);
    fail_unless(out_count > 0 && out_count < 10,
                "dxrate at 15fps from 30fps must drop frames (got %d/10)", out_count);
}
GST_END_TEST;

static Suite *caps_renegotiation_suite(void) {
    Suite *s = suite_create("caps_renegotiation");
    TCase *tc = tcase_create("renego");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_scale_caps_renego);
    tcase_add_test(tc, CE_convert_caps_renego);
    tcase_add_test(tc, CE_rate_caps_renego);
    return s;
}

GST_CHECK_MAIN(caps_renegotiation);
