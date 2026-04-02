// ---------------------------------------------------------------------------
// test_libyuv_kernel.cpp — Direct C++ unit tests for the libyuv transform
//                          kernel's 4×4 color conversion matrix.
//
// Tests every non-identity conversion pair via the public factory API:
//   VideoTransformFactory::create_backend("libyuv", dst_template, ops)
//
// No GStreamer pipeline involved — just raw buffer → kernel → raw buffer.
// ---------------------------------------------------------------------------
#include <gst/check/gstcheck.h>
#include <gst/gst.h>

// Internal headers (not installed — relative path from test/elements/)
#include "transforms/video_transform_factory.hpp"
#include "transforms/video_transform_kernel.hpp"

#include <cstdlib>
#include <cstring>
#include <vector>

using namespace dxt;

// ---------------------------------------------------------------------------
// Test dimensions — 64×48 is the smallest reasonable size (even, YUV-safe)
// ---------------------------------------------------------------------------
static constexpr int W = 64;
static constexpr int H = 48;

// ---------------------------------------------------------------------------
// Buffer size calculators
// ---------------------------------------------------------------------------
static size_t buf_size(VideoFormat fmt, int w, int h) {
    switch (fmt) {
        case VideoFormat::I420: return w * h * 3 / 2;
        case VideoFormat::NV12: return w * h * 3 / 2;
        case VideoFormat::RGB:
        case VideoFormat::BGR:  return w * h * 3;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Fill a source buffer with a deterministic pattern
// ---------------------------------------------------------------------------
static void fill_src_buffer(std::vector<uint8_t>& buf, VideoFormat fmt,
                            int w, int h) {
    buf.resize(buf_size(fmt, w, h));

    switch (fmt) {
    case VideoFormat::I420: {
        // Y plane: gradient 16..235
        uint8_t *y = buf.data();
        for (int i = 0; i < w * h; ++i)
            y[i] = static_cast<uint8_t>(16 + (i % 220));
        // U plane: mid-grey
        uint8_t *u = y + w * h;
        std::memset(u, 128, (w / 2) * (h / 2));
        // V plane: mid-grey
        uint8_t *v = u + (w / 2) * (h / 2);
        std::memset(v, 128, (w / 2) * (h / 2));
        break;
    }
    case VideoFormat::NV12: {
        uint8_t *y = buf.data();
        for (int i = 0; i < w * h; ++i)
            y[i] = static_cast<uint8_t>(16 + (i % 220));
        uint8_t *uv = y + w * h;
        for (int i = 0; i < w * (h / 2); i += 2) {
            uv[i]     = 128;  // U
            uv[i + 1] = 128;  // V
        }
        break;
    }
    case VideoFormat::RGB:
    case VideoFormat::BGR: {
        for (size_t i = 0; i < buf.size(); i += 3) {
            buf[i]     = static_cast<uint8_t>((i * 7) % 256);
            buf[i + 1] = static_cast<uint8_t>((i * 13 + 50) % 256);
            buf[i + 2] = static_cast<uint8_t>((i * 3 + 100) % 256);
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Build a FrameDesc for a given raw buffer
// ---------------------------------------------------------------------------
static FrameDesc make_desc(uint8_t *data, VideoFormat fmt, int w, int h) {
    FrameDesc d;
    d.width       = w;
    d.height      = h;
    d.format      = fmt;
    d.memory_type = MemoryType::CPU_VIRTUAL;
    d.dma_fd      = -1;

    switch (fmt) {
    case VideoFormat::I420:
        d.num_planes      = 3;
        d.planes[0].data   = data;
        d.planes[0].stride = w;
        d.planes[0].height = h;
        d.planes[0].offset = 0;
        d.planes[1].data   = data + w * h;
        d.planes[1].stride = w / 2;
        d.planes[1].height = h / 2;
        d.planes[1].offset = w * h;
        d.planes[2].data   = data + w * h + (w / 2) * (h / 2);
        d.planes[2].stride = w / 2;
        d.planes[2].height = h / 2;
        d.planes[2].offset = w * h + (w / 2) * (h / 2);
        break;
    case VideoFormat::NV12:
        d.num_planes      = 2;
        d.planes[0].data   = data;
        d.planes[0].stride = w;
        d.planes[0].height = h;
        d.planes[0].offset = 0;
        d.planes[1].data   = data + w * h;
        d.planes[1].stride = w;
        d.planes[1].height = h / 2;
        d.planes[1].offset = w * h;
        break;
    case VideoFormat::RGB:
    case VideoFormat::BGR:
        d.num_planes      = 1;
        d.planes[0].data   = data;
        d.planes[0].stride = w * 3;
        d.planes[0].height = h;
        d.planes[0].offset = 0;
        break;
    }
    return d;
}

// ---------------------------------------------------------------------------
// Build a dst_template FrameDesc (no data pointer — factory only needs
// dimensions, format, strides)
// ---------------------------------------------------------------------------
static FrameDesc make_dst_template(VideoFormat fmt, int w, int h) {
    FrameDesc d;
    d.width       = w;
    d.height      = h;
    d.format      = fmt;
    d.memory_type = MemoryType::CPU_VIRTUAL;
    d.dma_fd      = -1;
    d.num_planes  = num_planes_for_format(fmt);

    switch (fmt) {
    case VideoFormat::I420:
        d.planes[0].stride = w;
        d.planes[0].height = h;
        d.planes[1].stride = w / 2;
        d.planes[1].height = h / 2;
        d.planes[2].stride = w / 2;
        d.planes[2].height = h / 2;
        break;
    case VideoFormat::NV12:
        d.planes[0].stride = w;
        d.planes[0].height = h;
        d.planes[1].stride = w;
        d.planes[1].height = h / 2;
        break;
    case VideoFormat::RGB:
    case VideoFormat::BGR:
        d.planes[0].stride = w * 3;
        d.planes[0].height = h;
        break;
    }
    return d;
}

// ---------------------------------------------------------------------------
// Core conversion test helper
// ---------------------------------------------------------------------------
static void assert_conversion(VideoFormat src_fmt, VideoFormat dst_fmt) {
    // Prepare source buffer
    std::vector<uint8_t> src_buf;
    fill_src_buffer(src_buf, src_fmt, W, H);

    // Prepare destination buffer (zeroed)
    std::vector<uint8_t> dst_buf(buf_size(dst_fmt, W, H), 0);

    // Create kernel via factory — explicitly request libyuv backend
    FrameDesc dst_template = make_dst_template(dst_fmt, W, H);
    TransformOps ops;  // no crop/scale/padding — pure color conversion

    auto kernel = VideoTransformFactory::create_backend("libyuv",
                                                        dst_template, ops);
    fail_unless(kernel != nullptr,
                "Factory returned nullptr for libyuv backend (%d → %d)",
                static_cast<int>(src_fmt), static_cast<int>(dst_fmt));
    fail_unless_equals_string(kernel->backend_name(), "libyuv");

    // Build frame descriptors
    FrameDesc src_desc = make_desc(src_buf.data(), src_fmt, W, H);
    FrameDesc dst_desc = make_desc(dst_buf.data(), dst_fmt, W, H);

    // Execute transform
    TransformResult result = kernel->transform(src_desc, dst_desc);
    fail_unless(result.success,
                "transform() failed for conversion %d → %d",
                static_cast<int>(src_fmt), static_cast<int>(dst_fmt));

    // Sanity: output buffer should not be all-zero
    bool all_zero = true;
    for (size_t i = 0; i < dst_buf.size(); ++i) {
        if (dst_buf[i] != 0) { all_zero = false; break; }
    }
    fail_unless(!all_zero,
                "Output buffer all-zero for %d → %d",
                static_cast<int>(src_fmt), static_cast<int>(dst_fmt));
}

// ===================================================================
// 1–3: I420 → {NV12, RGB, BGR}
// ===================================================================
GST_START_TEST(test_libyuv_i420_to_nv12) {
    assert_conversion(VideoFormat::I420, VideoFormat::NV12);
}
GST_END_TEST

GST_START_TEST(test_libyuv_i420_to_rgb) {
    assert_conversion(VideoFormat::I420, VideoFormat::RGB);
}
GST_END_TEST

GST_START_TEST(test_libyuv_i420_to_bgr) {
    assert_conversion(VideoFormat::I420, VideoFormat::BGR);
}
GST_END_TEST

// ===================================================================
// 4–6: NV12 → {I420, RGB, BGR}
// ===================================================================
GST_START_TEST(test_libyuv_nv12_to_i420) {
    assert_conversion(VideoFormat::NV12, VideoFormat::I420);
}
GST_END_TEST

GST_START_TEST(test_libyuv_nv12_to_rgb) {
    assert_conversion(VideoFormat::NV12, VideoFormat::RGB);
}
GST_END_TEST

GST_START_TEST(test_libyuv_nv12_to_bgr) {
    assert_conversion(VideoFormat::NV12, VideoFormat::BGR);
}
GST_END_TEST

// ===================================================================
// 7–9: RGB → {I420, NV12, BGR}
// ===================================================================
GST_START_TEST(test_libyuv_rgb_to_i420) {
    assert_conversion(VideoFormat::RGB, VideoFormat::I420);
}
GST_END_TEST

GST_START_TEST(test_libyuv_rgb_to_nv12) {
    assert_conversion(VideoFormat::RGB, VideoFormat::NV12);
}
GST_END_TEST

GST_START_TEST(test_libyuv_rgb_to_bgr) {
    assert_conversion(VideoFormat::RGB, VideoFormat::BGR);
}
GST_END_TEST

// ===================================================================
// 10–12: BGR → {I420, NV12, RGB}
// ===================================================================
GST_START_TEST(test_libyuv_bgr_to_i420) {
    assert_conversion(VideoFormat::BGR, VideoFormat::I420);
}
GST_END_TEST

GST_START_TEST(test_libyuv_bgr_to_nv12) {
    assert_conversion(VideoFormat::BGR, VideoFormat::NV12);
}
GST_END_TEST

GST_START_TEST(test_libyuv_bgr_to_rgb) {
    assert_conversion(VideoFormat::BGR, VideoFormat::RGB);
}
GST_END_TEST

// ===================================================================
// 13: identity — same format should pass through without error
// ===================================================================
GST_START_TEST(test_libyuv_identity_rgb) {
    // Same-format case: factory should still create a kernel that handles it
    // (or the element would use passthrough — but kernel should still work)
    std::vector<uint8_t> src_buf;
    fill_src_buffer(src_buf, VideoFormat::RGB, W, H);

    std::vector<uint8_t> dst_buf(buf_size(VideoFormat::RGB, W, H), 0);

    FrameDesc dst_template = make_dst_template(VideoFormat::RGB, W, H);
    TransformOps ops;
    auto kernel = VideoTransformFactory::create_backend("libyuv",
                                                        dst_template, ops);
    fail_unless(kernel != nullptr, "Factory returned nullptr for identity RGB");

    FrameDesc src_desc = make_desc(src_buf.data(), VideoFormat::RGB, W, H);
    FrameDesc dst_desc = make_desc(dst_buf.data(), VideoFormat::RGB, W, H);

    TransformResult result = kernel->transform(src_desc, dst_desc);
    fail_unless(result.success, "Identity RGB transform failed");

    // Output must match input exactly for same-format
    fail_unless(memcmp(src_buf.data(), dst_buf.data(), src_buf.size()) == 0,
                "Identity RGB: output differs from input");
}
GST_END_TEST

// ===================================================================
// 14: Round-trip: I420 → RGB → I420 (verify no crash, data flows)
// ===================================================================
GST_START_TEST(test_libyuv_roundtrip_i420_rgb) {
    // Step 1: I420 → RGB
    std::vector<uint8_t> i420_src;
    fill_src_buffer(i420_src, VideoFormat::I420, W, H);

    std::vector<uint8_t> rgb_buf(buf_size(VideoFormat::RGB, W, H), 0);

    {
        FrameDesc dt = make_dst_template(VideoFormat::RGB, W, H);
        TransformOps ops;
        auto k = VideoTransformFactory::create_backend("libyuv", dt, ops);
        fail_unless(k != nullptr);

        FrameDesc s = make_desc(i420_src.data(), VideoFormat::I420, W, H);
        FrameDesc d = make_desc(rgb_buf.data(),  VideoFormat::RGB,  W, H);
        auto r = k->transform(s, d);
        fail_unless(r.success, "Round-trip step 1 (I420→RGB) failed");
    }

    // Step 2: RGB → I420
    std::vector<uint8_t> i420_dst(buf_size(VideoFormat::I420, W, H), 0);
    {
        FrameDesc dt = make_dst_template(VideoFormat::I420, W, H);
        TransformOps ops;
        auto k = VideoTransformFactory::create_backend("libyuv", dt, ops);
        fail_unless(k != nullptr);

        FrameDesc s = make_desc(rgb_buf.data(),  VideoFormat::RGB,  W, H);
        FrameDesc d = make_desc(i420_dst.data(), VideoFormat::I420, W, H);
        auto r = k->transform(s, d);
        fail_unless(r.success, "Round-trip step 2 (RGB→I420) failed");
    }

    // Verify output is not all zero (lossy conversion, so not exact match)
    bool all_zero = true;
    for (size_t i = 0; i < i420_dst.size(); ++i) {
        if (i420_dst[i] != 0) { all_zero = false; break; }
    }
    fail_unless(!all_zero, "Round-trip I420→RGB→I420 output is all zero");
}
GST_END_TEST

// ===================================================================
// Test suite
// ===================================================================
Suite *libyuv_kernel_suite(void) {
    Suite *s  = suite_create("LibyuvTransformKernel");
    TCase *tc = tcase_create("ColorConversion");
    tcase_set_timeout(tc, 15.0);

    // I420 → *
    tcase_add_test(tc, test_libyuv_i420_to_nv12);
    tcase_add_test(tc, test_libyuv_i420_to_rgb);
    tcase_add_test(tc, test_libyuv_i420_to_bgr);

    // NV12 → *
    tcase_add_test(tc, test_libyuv_nv12_to_i420);
    tcase_add_test(tc, test_libyuv_nv12_to_rgb);
    tcase_add_test(tc, test_libyuv_nv12_to_bgr);

    // RGB → *
    tcase_add_test(tc, test_libyuv_rgb_to_i420);
    tcase_add_test(tc, test_libyuv_rgb_to_nv12);
    tcase_add_test(tc, test_libyuv_rgb_to_bgr);

    // BGR → *
    tcase_add_test(tc, test_libyuv_bgr_to_i420);
    tcase_add_test(tc, test_libyuv_bgr_to_nv12);
    tcase_add_test(tc, test_libyuv_bgr_to_rgb);

    // Identity + round-trip
    tcase_add_test(tc, test_libyuv_identity_rgb);
    tcase_add_test(tc, test_libyuv_roundtrip_i420_rgb);

    suite_add_tcase(s, tc);
    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    Suite *s    = libyuv_kernel_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int nfailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (nfailed == 0) ? 0 : 1;
}
