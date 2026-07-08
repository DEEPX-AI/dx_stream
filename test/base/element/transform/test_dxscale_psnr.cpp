// Phase 4 — dxscale scaling quality verification
// A7: scaling quality through libyuv kernel
// Tests pixel-level correctness for various scale operations

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"

#include <cstring>
#include <cmath>

using namespace dxtest;

static double compute_psnr_plane(GstVideoFrame *a, GstVideoFrame *b, int plane) {
    int w = GST_VIDEO_FRAME_COMP_WIDTH(a, plane);
    int h = GST_VIDEO_FRAME_COMP_HEIGHT(a, plane);
    if (w * h == 0) return 0.0;

    guint8 *da = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(a, plane);
    guint8 *db = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(b, plane);
    int sa = GST_VIDEO_FRAME_PLANE_STRIDE(a, plane);
    int sb = GST_VIDEO_FRAME_PLANE_STRIDE(b, plane);

    double mse = 0.0;
    for (int y = 0; y < h; y++) {
        guint8 *ra = da + y * sa;
        guint8 *rb = db + y * sb;
        for (int x = 0; x < w; x++) {
            double d = (double)ra[x] - (double)rb[x];
            mse += d * d;
        }
    }
    mse /= (w * h);
    if (mse == 0.0) return 100.0;
    return 10.0 * log10(255.0 * 255.0 / mse);
}

static void dump_scale_diag(const char *label, GstVideoFrame *f) {
    fprintf(stderr, "  [DIAG] %s: format=%s %dx%d stride=%d buf_size=%" G_GSIZE_FORMAT "\n",
            label,
            gst_video_format_to_string(GST_VIDEO_FRAME_FORMAT(f)),
            GST_VIDEO_FRAME_WIDTH(f), GST_VIDEO_FRAME_HEIGHT(f),
            GST_VIDEO_FRAME_PLANE_STRIDE(f, 0),
            gst_buffer_get_size(f->buffer));
    guint8 *d = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(f, 0);
    fprintf(stderr, "    Y row0: [%d %d %d %d %d %d %d %d]\n",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
}

static GstBuffer *make_gradient_i420(int w, int h) {
    char caps_str[128];
    snprintf(caps_str, sizeof(caps_str),
             "video/x-raw,format=I420,width=%d,height=%d,framerate=30/1", w, h);
    GstCaps *caps = gst_caps_from_string(caps_str);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    gsize sz = GST_VIDEO_INFO_SIZE(&info);
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);

    GstVideoFrame frame;
    fail_unless(gst_video_frame_map(&frame, &info, buf, GST_MAP_WRITE));

    guint8 *yd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
    int ys = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
    for (int y = 0; y < h; y++) {
        guint8 *row = yd + y * ys;
        for (int x = 0; x < w; x++)
            row[x] = (guint8)(x * 255 / (w > 1 ? w - 1 : 1));
    }
    for (int p = 1; p <= 2; p++) {
        guint8 *pd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, p);
        int ps = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, p);
        int ph = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, p);
        int pw = GST_VIDEO_FRAME_COMP_WIDTH(&frame, p);
        for (int y = 0; y < ph; y++)
            memset(pd + y * ps, 128, pw);
    }

    gst_video_frame_unmap(&frame);
    GST_BUFFER_PTS(buf) = 0;
    GST_BUFFER_DURATION(buf) = GST_SECOND / 30;
    return buf;
}

static GstBuffer *make_gradient_rgb(int w, int h) {
    char caps_str[128];
    snprintf(caps_str, sizeof(caps_str),
             "video/x-raw,format=RGB,width=%d,height=%d,framerate=30/1", w, h);
    GstCaps *caps = gst_caps_from_string(caps_str);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    gsize sz = GST_VIDEO_INFO_SIZE(&info);
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);

    GstVideoFrame frame;
    fail_unless(gst_video_frame_map(&frame, &info, buf, GST_MAP_WRITE));

    guint8 *data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
    int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
    for (int y = 0; y < h; y++) {
        guint8 *row = data + y * stride;
        for (int x = 0; x < w; x++) {
            row[x * 3 + 0] = (guint8)(x * 255 / (w > 1 ? w - 1 : 1));
            row[x * 3 + 1] = (guint8)(y * 255 / (h > 1 ? h - 1 : 1));
            row[x * 3 + 2] = 128;
        }
    }

    gst_video_frame_unmap(&frame);
    GST_BUFFER_PTS(buf) = 0;
    GST_BUFFER_DURATION(buf) = GST_SECOND / 30;
    return buf;
}

// CE_scale_same_size_exact: same input/output size → exact copy
// Target: gst_dxscale_transform L327-328 (same size passthrough copy)
// MUT: remove passthrough check → kernel used → possible quality loss
GST_START_TEST(CE_scale_same_size_exact) {
    GstHarness *h = gst_harness_new("dxscale");
    g_object_set(h->element, "width", 64u, "height", 64u, nullptr);
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    GstBuffer *in = make_gradient_i420(64, 64);
    GstBuffer *in_copy = gst_buffer_copy_deep(in);
    gst_harness_push(h, in);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr);

    const char *i420_caps = "video/x-raw,format=I420,width=64,height=64,framerate=30/1";
    GstCaps *caps = gst_caps_from_string(i420_caps);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    GstVideoFrame orig_frame, out_frame;
    fail_unless(gst_video_frame_map(&orig_frame, &info, in_copy, GST_MAP_READ));
    fail_unless(gst_video_frame_map(&out_frame, &info, out, GST_MAP_READ));

    double psnr = compute_psnr_plane(&orig_frame, &out_frame, 0);

    if (psnr < 99.0) {
        fprintf(stderr, "[DIAG] same-size scale PSNR=%.1fdB FAILED\n", psnr);
        dump_scale_diag("original", &orig_frame);
        dump_scale_diag("output", &out_frame);
    }

    gst_video_frame_unmap(&out_frame);
    gst_video_frame_unmap(&orig_frame);
    gst_buffer_unref(in_copy);
    gst_buffer_unref(out);

    fail_unless(psnr >= 99.0,
                "same-size scale must produce exact copy (Y-PSNR=%.1fdB)", psnr);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_scale_downscale_output_size: 64x64 → 32x32 produces correct buffer size
// Target: gst_dxscale_transform + kernel scaling
GST_START_TEST(CE_scale_downscale_output_size) {
    GstHarness *h = gst_harness_new("dxscale");
    g_object_set(h->element, "width", 32u, "height", 32u, nullptr);
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    GstBuffer *in = make_gradient_i420(64, 64);
    gst_harness_push(h, in);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr);

    gsize expected_size = 32 * 32 + 2 * (16 * 16);
    GstMapInfo map;
    gst_buffer_map(out, &map, GST_MAP_READ);
    fail_unless(map.size >= expected_size,
                "output size %" G_GSIZE_FORMAT " must be >= %" G_GSIZE_FORMAT,
                map.size, expected_size);

    int non_zero = 0;
    for (gsize i = 0; i < map.size; i++)
        if (map.data[i] != 0) non_zero++;
    fail_unless(non_zero > (int)(map.size / 2),
                "scaled output must not be mostly zeros (%d/%d non-zero)",
                non_zero, (int)map.size);

    gst_buffer_unmap(out, &map);
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_scale_roundtrip_psnr: 64→32→64 round-trip PSNR ≥ 15dB
// Target: libyuv scaling kernel quality
// MUT: broken kernel → garbage output → low PSNR
GST_START_TEST(CE_scale_roundtrip_psnr) {
    GstHarness *h_down = gst_harness_new("dxscale");
    g_object_set(h_down->element, "width", 32u, "height", 32u, nullptr);
    gst_harness_set_src_caps_str(h_down,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    GstHarness *h_up = gst_harness_new("dxscale");
    g_object_set(h_up->element, "width", 64u, "height", 64u, nullptr);
    gst_harness_set_src_caps_str(h_up,
        "video/x-raw,format=I420,width=32,height=32,framerate=30/1");

    GstBuffer *original = make_gradient_i420(64, 64);
    GstBuffer *orig_copy = gst_buffer_copy_deep(original);

    gst_harness_push(h_down, original);
    GstBuffer *small = gst_harness_pull(h_down);
    fail_unless(small != nullptr, "downscale must produce output");

    gst_harness_push(h_up, small);
    GstBuffer *roundtrip = gst_harness_pull(h_up);
    fail_unless(roundtrip != nullptr, "upscale must produce output");

    const char *i420_caps = "video/x-raw,format=I420,width=64,height=64,framerate=30/1";
    GstCaps *caps = gst_caps_from_string(i420_caps);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    GstVideoFrame orig_frame, rt_frame;
    fail_unless(gst_video_frame_map(&orig_frame, &info, orig_copy, GST_MAP_READ));
    fail_unless(gst_video_frame_map(&rt_frame, &info, roundtrip, GST_MAP_READ));

    double psnr = compute_psnr_plane(&orig_frame, &rt_frame, 0);

    if (psnr < 15.0) {
        fprintf(stderr, "[DIAG] 64→32→64 roundtrip PSNR=%.1fdB FAILED\n", psnr);
        dump_scale_diag("original", &orig_frame);
        dump_scale_diag("roundtrip", &rt_frame);
    }

    gst_video_frame_unmap(&rt_frame);
    gst_video_frame_unmap(&orig_frame);
    gst_buffer_unref(orig_copy);
    gst_buffer_unref(roundtrip);

    fail_unless(psnr >= 15.0,
                "64→32→64 round-trip Y-plane PSNR=%.1fdB must be >= 15dB", psnr);

    gst_harness_teardown(h_down);
    gst_harness_teardown(h_up);
}
GST_END_TEST;

// CE_scale_rgb_downscale: RGB format scaling produces valid output
// Target: libyuv RGB scaling support
GST_START_TEST(CE_scale_rgb_downscale) {
    GstHarness *h = gst_harness_new("dxscale");
    g_object_set(h->element, "width", 32u, "height", 32u, nullptr);
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");

    GstBuffer *in = make_gradient_rgb(64, 64);
    gst_harness_push(h, in);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr);

    const char *out_caps = "video/x-raw,format=RGB,width=32,height=32,framerate=30/1";
    GstCaps *caps = gst_caps_from_string(out_caps);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    GstVideoFrame frame;
    fail_unless(gst_video_frame_map(&frame, &info, out, GST_MAP_READ));

    guint8 *data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
    int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
    guint8 center = data[16 * stride + 16 * 3];
    fail_unless(center > 50 && center < 200,
                "center pixel R=%u must be in gradient range", center);

    gst_video_frame_unmap(&frame);
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *dxscale_psnr_suite(void) {
    Suite *s = suite_create("dxscale_psnr");
    TCase *tc = tcase_create("scaling_quality");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_scale_same_size_exact);
    tcase_add_test(tc, CE_scale_downscale_output_size);
    tcase_add_test(tc, CE_scale_roundtrip_psnr);
    tcase_add_test(tc, CE_scale_rgb_downscale);
    return s;
}

GST_CHECK_MAIN(dxscale_psnr);
