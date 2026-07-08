// Phase 4 — dxconvert format conversion quality verification
// A6: color conversion quality through libyuv kernel
// Tests specific pixel-level correctness, not just "buffer produced"

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"

#include <cstring>
#include <cmath>
#include <cstdlib>
#include <vector>

using namespace dxtest;

static double compute_psnr_frames(GstVideoFrame *a, GstVideoFrame *b,
                                  int plane, gboolean all_components) {
    int w, h, pixel_bytes;
    if (all_components && (GST_VIDEO_FRAME_FORMAT(a) == GST_VIDEO_FORMAT_RGB ||
                           GST_VIDEO_FRAME_FORMAT(a) == GST_VIDEO_FORMAT_BGR)) {
        w = GST_VIDEO_FRAME_WIDTH(a) * 3;
        h = GST_VIDEO_FRAME_HEIGHT(a);
        pixel_bytes = w * h;
    } else {
        w = GST_VIDEO_FRAME_COMP_WIDTH(a, plane);
        h = GST_VIDEO_FRAME_COMP_HEIGHT(a, plane);
        pixel_bytes = w * h;
    }
    if (pixel_bytes == 0) return 0.0;

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
    mse /= pixel_bytes;
    if (mse == 0.0) return 100.0;
    return 10.0 * log10(255.0 * 255.0 / mse);
}

static void dump_frame_diag(const char *label, GstVideoFrame *f) {
    int n_planes = GST_VIDEO_FRAME_N_PLANES(f);
    fprintf(stderr, "  [DIAG] %s: format=%s %dx%d planes=%d buf_size=%" G_GSIZE_FORMAT "\n",
            label,
            gst_video_format_to_string(GST_VIDEO_FRAME_FORMAT(f)),
            GST_VIDEO_FRAME_WIDTH(f), GST_VIDEO_FRAME_HEIGHT(f),
            n_planes, gst_buffer_get_size(f->buffer));
    for (int p = 0; p < n_planes; p++) {
        int stride = GST_VIDEO_FRAME_PLANE_STRIDE(f, p);
        int pw = GST_VIDEO_FRAME_COMP_WIDTH(f, p);
        int ph = GST_VIDEO_FRAME_COMP_HEIGHT(f, p);
        guint8 *data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(f, p);
        fprintf(stderr, "    plane[%d]: stride=%d %dx%d first8=[%d %d %d %d %d %d %d %d]\n",
                p, stride, pw, ph,
                data[0], data[1], data[2], data[3],
                data[4], data[5], data[6], data[7]);
    }
}

static void dump_diff_summary(GstVideoFrame *a, GstVideoFrame *b,
                              int plane, gboolean all_components) {
    int w, h;
    if (all_components && (GST_VIDEO_FRAME_FORMAT(a) == GST_VIDEO_FORMAT_RGB ||
                           GST_VIDEO_FRAME_FORMAT(a) == GST_VIDEO_FORMAT_BGR)) {
        w = GST_VIDEO_FRAME_WIDTH(a) * 3;
        h = GST_VIDEO_FRAME_HEIGHT(a);
    } else {
        w = GST_VIDEO_FRAME_COMP_WIDTH(a, plane);
        h = GST_VIDEO_FRAME_COMP_HEIGHT(a, plane);
    }
    guint8 *da = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(a, plane);
    guint8 *db = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(b, plane);
    int sa = GST_VIDEO_FRAME_PLANE_STRIDE(a, plane);
    int sb = GST_VIDEO_FRAME_PLANE_STRIDE(b, plane);

    int max_diff = 0, diff_count = 0;
    long long sum_diff = 0;
    for (int y = 0; y < h; y++) {
        guint8 *ra = da + y * sa;
        guint8 *rb = db + y * sb;
        for (int x = 0; x < w; x++) {
            int d = abs((int)ra[x] - (int)rb[x]);
            if (d > 0) { diff_count++; sum_diff += d; }
            if (d > max_diff) max_diff = d;
        }
    }
    fprintf(stderr, "  [DIAG] diff: %d/%d bytes differ, max_diff=%d, avg_diff=%.1f\n",
            diff_count, w * h, max_diff,
            diff_count > 0 ? (double)sum_diff / diff_count : 0.0);
    // Show first row comparison
    fprintf(stderr, "  [DIAG] row0 orig: [%d %d %d %d %d %d]\n",
            da[0], da[1], da[2], da[3], da[4], da[5]);
    fprintf(stderr, "  [DIAG] row0 rt  : [%d %d %d %d %d %d]\n",
            db[0], db[1], db[2], db[3], db[4], db[5]);
}

static GstBuffer *make_gradient_buffer(const char *caps_str) {
    GstCaps *caps = gst_caps_from_string(caps_str);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    gsize sz = GST_VIDEO_INFO_SIZE(&info);
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);

    GstVideoFrame frame;
    fail_unless(gst_video_frame_map(&frame, &info, buf, GST_MAP_WRITE));

    int w = GST_VIDEO_FRAME_WIDTH(&frame);
    int h = GST_VIDEO_FRAME_HEIGHT(&frame);
    GstVideoFormat fmt = GST_VIDEO_FRAME_FORMAT(&frame);

    if (fmt == GST_VIDEO_FORMAT_RGB || fmt == GST_VIDEO_FORMAT_BGR) {
        guint8 *data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
        int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
        for (int y = 0; y < h; y++) {
            guint8 *row = data + y * stride;
            for (int x = 0; x < w; x++) {
                row[x * 3 + 0] = (guint8)(x * 255 / (w > 1 ? w - 1 : 1));
                row[x * 3 + 1] = (guint8)(y * 255 / (h > 1 ? h - 1 : 1));
                row[x * 3 + 2] = (guint8)((x + y) * 127 / ((w + h > 2) ? (w + h - 2) : 1));
            }
        }
    } else if (fmt == GST_VIDEO_FORMAT_I420) {
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
    } else if (fmt == GST_VIDEO_FORMAT_NV12) {
        guint8 *yd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);
        int ys = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
        for (int y = 0; y < h; y++) {
            guint8 *row = yd + y * ys;
            for (int x = 0; x < w; x++)
                row[x] = (guint8)(x * 255 / (w > 1 ? w - 1 : 1));
        }
        guint8 *uvd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 1);
        int uvs = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 1);
        int uvh = GST_VIDEO_FRAME_COMP_HEIGHT(&frame, 1);
        int uvw = GST_VIDEO_FRAME_COMP_WIDTH(&frame, 1);
        for (int y = 0; y < uvh; y++) {
            guint8 *row = uvd + y * uvs;
            for (int x = 0; x < uvw; x++) {
                row[x * 2 + 0] = 128;
                row[x * 2 + 1] = 128;
            }
        }
    }

    gst_video_frame_unmap(&frame);
    GST_BUFFER_PTS(buf) = 0;
    GST_BUFFER_DURATION(buf) = GST_SECOND / 30;
    return buf;
}

// CE_convert_rgb_to_bgr_swap: RGB→BGR swaps R and B channels
// Target: libyuv_transform_kernel RGB↔BGR conversion
// MUT: swap kernel misconfigured → R,B not swapped → fail
GST_START_TEST(CE_convert_rgb_to_bgr_swap) {
    GstHarness *h = gst_harness_new("dxconvert");
    const char *rgb_caps = "video/x-raw,format=RGB,width=16,height=16,framerate=30/1";
    const char *bgr_caps = "video/x-raw,format=BGR,width=16,height=16,framerate=30/1";
    gst_harness_set_src_caps_str(h, rgb_caps);
    gst_harness_set_sink_caps_str(h, bgr_caps);

    GstBuffer *in = make_gradient_buffer(rgb_caps);
    GstFlowReturn r = gst_harness_push(h, gst_buffer_ref(in));
    fail_unless(r == GST_FLOW_OK, "push failed: %s", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr);

    GstVideoInfo in_info, out_info;
    GstCaps *ic = gst_caps_from_string(rgb_caps);
    GstCaps *oc = gst_caps_from_string(bgr_caps);
    gst_video_info_from_caps(&in_info, ic);
    gst_video_info_from_caps(&out_info, oc);
    gst_caps_unref(ic);
    gst_caps_unref(oc);

    GstVideoFrame in_frame, out_frame;
    fail_unless(gst_video_frame_map(&in_frame, &in_info, in, GST_MAP_READ));
    fail_unless(gst_video_frame_map(&out_frame, &out_info, out, GST_MAP_READ));

    int w = GST_VIDEO_FRAME_WIDTH(&in_frame);
    int h_px = GST_VIDEO_FRAME_HEIGHT(&in_frame);
    int in_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&in_frame, 0);
    int out_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&out_frame, 0);
    guint8 *in_data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&in_frame, 0);
    guint8 *out_data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&out_frame, 0);

    int mismatches = 0;
    for (int y = 0; y < h_px; y++) {
        guint8 *in_row = in_data + y * in_stride;
        guint8 *out_row = out_data + y * out_stride;
        for (int x = 0; x < w; x++) {
            if (in_row[x*3+0] != out_row[x*3+2] ||
                in_row[x*3+1] != out_row[x*3+1] ||
                in_row[x*3+2] != out_row[x*3+0])
                mismatches++;
        }
    }

    gst_video_frame_unmap(&out_frame);
    gst_video_frame_unmap(&in_frame);
    gst_buffer_unref(in);
    gst_buffer_unref(out);

    fail_unless(mismatches == 0,
                "RGB→BGR must swap R,B channels exactly (%d pixel mismatches)",
                mismatches);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_convert_i420_rgb_roundtrip: I420→RGB→I420 round-trip PSNR ≥ 20dB
// Target: libyuv I420↔RGB conversion kernel
// MUT: broken Y/UV plane mapping → garbage → low PSNR
GST_START_TEST(CE_convert_i420_rgb_roundtrip) {
    GstHarness *h1 = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h1,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");
    gst_harness_set_sink_caps_str(h1,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");

    GstHarness *h2 = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h2,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    gst_harness_set_sink_caps_str(h2,
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");

    const char *i420_caps =
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1";
    GstBuffer *original = make_gradient_buffer(i420_caps);
    GstBuffer *in_copy = gst_buffer_copy_deep(original);

    gst_harness_push(h1, original);
    GstBuffer *rgb_buf = gst_harness_pull(h1);
    fail_unless(rgb_buf != nullptr, "I420→RGB must produce output");

    gst_harness_push(h2, rgb_buf);
    GstBuffer *roundtrip = gst_harness_pull(h2);
    fail_unless(roundtrip != nullptr, "RGB→I420 must produce output");

    GstCaps *caps = gst_caps_from_string(i420_caps);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);

    GstVideoFrame orig_frame, rt_frame;
    fail_unless(gst_video_frame_map(&orig_frame, &info, in_copy, GST_MAP_READ));
    fail_unless(gst_video_frame_map(&rt_frame, &info, roundtrip, GST_MAP_READ));

    double psnr = compute_psnr_frames(&orig_frame, &rt_frame, 0, FALSE);

    if (psnr < 20.0) {
        fprintf(stderr, "[DIAG] I420→RGB→I420 PSNR=%.1fdB FAILED (threshold 20dB)\n", psnr);
        dump_frame_diag("original", &orig_frame);
        dump_frame_diag("roundtrip", &rt_frame);
        dump_diff_summary(&orig_frame, &rt_frame, 0, FALSE);
    }

    gst_video_frame_unmap(&rt_frame);
    gst_video_frame_unmap(&orig_frame);
    gst_buffer_unref(in_copy);
    gst_buffer_unref(roundtrip);

    fail_unless(psnr >= 20.0,
                "I420→RGB→I420 round-trip Y-plane PSNR=%.1fdB must be >= 20dB",
                psnr);

    gst_harness_teardown(h1);
    gst_harness_teardown(h2);
}
GST_END_TEST;

// CE_convert_rgb_nv12_roundtrip: RGB→NV12→RGB round-trip PSNR ≥ 20dB
GST_START_TEST(CE_convert_rgb_nv12_roundtrip) {
    GstElementFactory *factory = gst_element_factory_find("dxconvert");
    GstPlugin *plugin = factory ? gst_plugin_feature_get_plugin(GST_PLUGIN_FEATURE(factory)) : nullptr;
    const gchar *origin = plugin ? gst_plugin_get_origin(plugin) : nullptr;
    const gchar *filename = plugin ? gst_plugin_get_filename(plugin) : nullptr;

    GstHarness *h1 = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h1,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    gst_harness_set_sink_caps_str(h1,
        "video/x-raw,format=NV12,width=64,height=64,framerate=30/1");

    GstHarness *h2 = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h2,
        "video/x-raw,format=NV12,width=64,height=64,framerate=30/1");
    gst_harness_set_sink_caps_str(h2,
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");

    const char *rgb_caps =
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1";
    GstBuffer *original = make_gradient_buffer(rgb_caps);
    GstBuffer *in_copy = gst_buffer_copy_deep(original);

    GstCaps *caps = gst_caps_from_string(rgb_caps);
    GstVideoInfo rgb_info;
    gst_video_info_from_caps(&rgb_info, caps);
    gst_caps_unref(caps);

    // === Stage 2: RGB→NV12 ===
    gst_harness_push(h1, original);
    GstBuffer *nv12_buf = gst_harness_pull(h1);
    fail_unless(nv12_buf != nullptr, "RGB→NV12 must produce output");

    // === Stage 3: NV12→RGB round-trip ===
    gst_harness_push(h2, nv12_buf);
    GstBuffer *roundtrip = gst_harness_pull(h2);
    fail_unless(roundtrip != nullptr, "NV12→RGB must produce output");

    GstVideoFrame orig_frame, rt_frame;
    fail_unless(gst_video_frame_map(&orig_frame, &rgb_info, in_copy, GST_MAP_READ));
    fail_unless(gst_video_frame_map(&rt_frame, &rgb_info, roundtrip, GST_MAP_READ));

    // === Stage 4: Compare ===
    double psnr_all = compute_psnr_frames(&orig_frame, &rt_frame, 0, TRUE);

    // Per-channel PSNR
    guint8 *od = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&orig_frame, 0);
    guint8 *rd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&rt_frame, 0);
    int os = GST_VIDEO_FRAME_PLANE_STRIDE(&orig_frame, 0);
    int rs = GST_VIDEO_FRAME_PLANE_STRIDE(&rt_frame, 0);
    int w = GST_VIDEO_FRAME_WIDTH(&orig_frame);
    int h = GST_VIDEO_FRAME_HEIGHT(&orig_frame);

    double mse_r = 0, mse_g = 0, mse_b = 0;
    for (int y = 0; y < h; y++) {
        guint8 *orow = od + y * os;
        guint8 *rrow = rd + y * rs;
        for (int x = 0; x < w; x++) {
            double dr = (double)orow[x*3+0] - rrow[x*3+0];
            double dg = (double)orow[x*3+1] - rrow[x*3+1];
            double db = (double)orow[x*3+2] - rrow[x*3+2];
            mse_r += dr*dr; mse_g += dg*dg; mse_b += db*db;
        }
    }
    int n = w * h;
    mse_r /= n; mse_g /= n; mse_b /= n;
    double psnr_r = mse_r == 0 ? 100.0 : 10.0*log10(65025.0/mse_r);
    double psnr_g = mse_g == 0 ? 100.0 : 10.0*log10(65025.0/mse_g);
    double psnr_b = mse_b == 0 ? 100.0 : 10.0*log10(65025.0/mse_b);

        if (psnr_all < 20.0) {
        fprintf(stderr, "[DIAG] dxconvert plugin: %s (origin: %s)\n",
            filename ? filename : "?", origin ? origin : "?");
        {
            GstVideoFrame f;
            gst_video_frame_map(&f, &rgb_info, in_copy, GST_MAP_READ);
            guint8 *d = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&f, 0);
            int s = GST_VIDEO_FRAME_PLANE_STRIDE(&f, 0);
            fprintf(stderr, "[DIAG] === ORIGINAL RGB ===\n");
            fprintf(stderr, "[DIAG] stride=%d buf_size=%" G_GSIZE_FORMAT "\n",
                s, gst_buffer_get_size(in_copy));
            fprintf(stderr, "[DIAG] row[0]  px[0..3]: [%d,%d,%d] [%d,%d,%d] [%d,%d,%d] [%d,%d,%d]\n",
                d[0],d[1],d[2], d[3],d[4],d[5], d[6],d[7],d[8], d[9],d[10],d[11]);
            guint8 *mid = d + 32 * s;
            fprintf(stderr, "[DIAG] row[32] px[0..3]: [%d,%d,%d] [%d,%d,%d] [%d,%d,%d] [%d,%d,%d]\n",
                mid[0],mid[1],mid[2], mid[3],mid[4],mid[5],
                mid[6],mid[7],mid[8], mid[9],mid[10],mid[11]);
            guint8 *last = d + 63 * s;
            fprintf(stderr, "[DIAG] row[63] px[0..3]: [%d,%d,%d] [%d,%d,%d] [%d,%d,%d] [%d,%d,%d]\n",
                last[0],last[1],last[2], last[3],last[4],last[5],
                last[6],last[7],last[8], last[9],last[10],last[11]);
            gst_video_frame_unmap(&f);
        }
        {
            const char *nv12_caps_str =
            "video/x-raw,format=NV12,width=64,height=64,framerate=30/1";
            GstCaps *nc = gst_caps_from_string(nv12_caps_str);
            GstVideoInfo ni;
            gst_video_info_from_caps(&ni, nc);
            gst_caps_unref(nc);

            GstVideoFrame f;
            gst_video_frame_map(&f, &ni, nv12_buf, GST_MAP_READ);
            guint8 *yd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&f, 0);
            int ys = GST_VIDEO_FRAME_PLANE_STRIDE(&f, 0);
            guint8 *uvd = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&f, 1);
            int uvs = GST_VIDEO_FRAME_PLANE_STRIDE(&f, 1);
            fprintf(stderr, "[DIAG] === NV12 (intermediate) ===\n");
            fprintf(stderr, "[DIAG] Y: stride=%d, UV: stride=%d, buf_size=%" G_GSIZE_FORMAT "\n",
                ys, uvs, gst_buffer_get_size(nv12_buf));
            fprintf(stderr, "[DIAG] Y row[0]:  [%d %d %d %d %d %d %d %d]\n",
                yd[0],yd[1],yd[2],yd[3],yd[4],yd[5],yd[6],yd[7]);
            fprintf(stderr, "[DIAG] Y row[32]: [%d %d %d %d %d %d %d %d]\n",
                yd[32*ys],yd[32*ys+1],yd[32*ys+2],yd[32*ys+3],
                yd[32*ys+4],yd[32*ys+5],yd[32*ys+6],yd[32*ys+7]);
            fprintf(stderr, "[DIAG] UV row[0]: [%d %d %d %d %d %d %d %d]\n",
                uvd[0],uvd[1],uvd[2],uvd[3],uvd[4],uvd[5],uvd[6],uvd[7]);
            gst_video_frame_unmap(&f);
        }
        fprintf(stderr, "[DIAG] === ROUND-TRIP RESULT ===\n");
        fprintf(stderr, "[DIAG] PSNR total=%.1fdB  R=%.1fdB G=%.1fdB B=%.1fdB\n",
            psnr_all, psnr_r, psnr_g, psnr_b);

        {
            guint8 *d = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(&rt_frame, 0);
            int s = GST_VIDEO_FRAME_PLANE_STRIDE(&rt_frame, 0);
            fprintf(stderr, "[DIAG] rt row[0]  px[0..3]: [%d,%d,%d] [%d,%d,%d] [%d,%d,%d] [%d,%d,%d]\n",
                d[0],d[1],d[2], d[3],d[4],d[5], d[6],d[7],d[8], d[9],d[10],d[11]);
            guint8 *mid = d + 32 * s;
            fprintf(stderr, "[DIAG] rt row[32] px[0..3]: [%d,%d,%d] [%d,%d,%d] [%d,%d,%d] [%d,%d,%d]\n",
                mid[0],mid[1],mid[2], mid[3],mid[4],mid[5],
                mid[6],mid[7],mid[8], mid[9],mid[10],mid[11]);
        }

        dump_diff_summary(&orig_frame, &rt_frame, 0, TRUE);
        }

    gst_video_frame_unmap(&rt_frame);
    gst_video_frame_unmap(&orig_frame);
    gst_buffer_unref(in_copy);
    gst_buffer_unref(roundtrip);

    fail_unless(psnr_all >= 20.0,
                "RGB→NV12→RGB round-trip PSNR=%.1fdB must be >= 20dB", psnr_all);

    gst_harness_teardown(h1);
    gst_harness_teardown(h2);
    if (factory) {
        gst_object_unref(factory);
    }
}
GST_END_TEST;

// CE_convert_bgr_to_rgb_values: BGR→RGB specific pixel value verification
// Target: libyuv BGR↔RGB conversion correctness
GST_START_TEST(CE_convert_bgr_to_rgb_values) {
    GstHarness *h = gst_harness_new("dxconvert");
    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=BGR,width=4,height=4,framerate=30/1");
    gst_harness_set_sink_caps_str(h,
        "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");

    gsize sz = 4 * 4 * 3;
    GstBuffer *in = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(in, &map, GST_MAP_WRITE);
    map.data[0] = 10;  map.data[1] = 20;  map.data[2] = 30;
    map.data[3] = 100; map.data[4] = 150; map.data[5] = 200;
    for (gsize i = 6; i < sz; i++) map.data[i] = (guint8)(i % 256);
    gst_buffer_unmap(in, &map);
    GST_BUFFER_PTS(in) = 0;

    GstFlowReturn r = gst_harness_push(h, in);
    fail_unless(r == GST_FLOW_OK);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr);

    GstMapInfo out_map;
    gst_buffer_map(out, &out_map, GST_MAP_READ);

    fail_unless_equals_int(out_map.data[0], 30);
    fail_unless_equals_int(out_map.data[1], 20);
    fail_unless_equals_int(out_map.data[2], 10);
    fail_unless_equals_int(out_map.data[3], 200);
    fail_unless_equals_int(out_map.data[4], 150);
    fail_unless_equals_int(out_map.data[5], 100);

    gst_buffer_unmap(out, &out_map);
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *dxconvert_psnr_suite(void) {
    Suite *s = suite_create("dxconvert_psnr");
    TCase *tc = tcase_create("conversion_quality");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_convert_rgb_to_bgr_swap);
    tcase_add_test(tc, CE_convert_i420_rgb_roundtrip);
    tcase_add_test(tc, CE_convert_rgb_nv12_roundtrip);
    tcase_add_test(tc, CE_convert_bgr_to_rgb_values);
    return s;
}

GST_CHECK_MAIN(dxconvert_psnr);
