#include "libyuv_transform_kernel.hpp"

#include <libyuv.h>
#include <opencv2/opencv.hpp>
#include <gst/gst.h>

#include <algorithm>
#include <cstring>

#define GST_CAT_DEFAULT dxt_libyuv_debug
GST_DEBUG_CATEGORY_STATIC(dxt_libyuv_debug);

namespace dxt {

// ---------------------------------------------------------------------------
// capabilities
// ---------------------------------------------------------------------------

BackendCaps LibyuvTransformKernel::capabilities() const {
    return BackendCaps{
        .name             = "libyuv",
        .hw_accelerated   = false,
        .supports_dma_buf = false,
        .max_width        = 16384,
        .max_height       = 16384,
        .src_formats      = { VideoFormat::I420, VideoFormat::NV12,
                              VideoFormat::RGB, VideoFormat::BGR },
        .dst_formats      = { VideoFormat::I420, VideoFormat::NV12,
                              VideoFormat::RGB, VideoFormat::BGR },
    };
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool LibyuvTransformKernel::init(const FrameDesc& dst_template,
                                  const TransformOps& ops) {
    static gsize debug_once = 0;
    if (g_once_init_enter(&debug_once)) {
        GST_DEBUG_CATEGORY_INIT(dxt_libyuv_debug, "dxt_libyuv", 0,
                                "DXT libyuv transform kernel");
        g_once_init_leave(&debug_once, 1);
    }

    if (!TransformKernelBase::init(dst_template, ops)) {
        return false;
    }

    GST_DEBUG("LibyuvTransformKernel: init OK  dst=%dx%d  fmt=%d  keep_ratio=%d",
              dst_template_.width, dst_template_.height,
              static_cast<int>(dst_template_.format),
              ops_.keep_aspect_ratio);
    return true;
}

// ---------------------------------------------------------------------------
// fill_padding
// ---------------------------------------------------------------------------

void LibyuvTransformKernel::fill_padding(uint8_t* dst, int width, int height,
                                          int stride, VideoFormat fmt) const {
    int bpp = bytes_per_pixel(fmt);
    if (bpp == 3) {
        // RGB/BGR — fill with pad color
        for (int row = 0; row < height; ++row) {
            uint8_t* line = dst + row * stride;
            for (int col = 0; col < width; ++col) {
                line[col * 3 + 0] = ops_.padding.pad_r;
                line[col * 3 + 1] = ops_.padding.pad_g;
                line[col * 3 + 2] = ops_.padding.pad_b;
            }
        }
    } else {
        // YUV planar — fill Y with pad_r (luma), UV with 128 (neutral chroma)
        // This is a simplification; proper YUV grey would be (pad_r, 128, 128)
        memset(dst, ops_.padding.pad_r, width * height);
        int uv_size = (fmt == VideoFormat::I420)
                          ? (width / 2) * (height / 2) * 2
                          : (width) * (height / 2);
        memset(dst + width * height, 128, uv_size);
    }
}

// ---------------------------------------------------------------------------
// scale helpers
// ---------------------------------------------------------------------------

bool LibyuvTransformKernel::scale_i420(
    const uint8_t* src_y, int src_stride_y,
    const uint8_t* src_u, int src_stride_u,
    const uint8_t* src_v, int src_stride_v,
    int src_w, int src_h,
    uint8_t* dst_y, int dst_stride_y,
    uint8_t* dst_u, int dst_stride_u,
    uint8_t* dst_v, int dst_stride_v,
    int dst_w, int dst_h) const
{
    int ret = libyuv::I420Scale(
        src_y, src_stride_y, src_u, src_stride_u, src_v, src_stride_v,
        src_w, src_h,
        dst_y, dst_stride_y, dst_u, dst_stride_u, dst_v, dst_stride_v,
        dst_w, dst_h, libyuv::kFilterLinear);
    return ret == 0;
}

bool LibyuvTransformKernel::scale_nv12(
    const uint8_t* src_y, int src_stride_y,
    const uint8_t* src_uv, int src_stride_uv,
    int src_w, int src_h,
    uint8_t* dst_y, int dst_stride_y,
    uint8_t* dst_uv, int dst_stride_uv,
    int dst_w, int dst_h) const
{
    int ret = libyuv::NV12Scale(
        src_y, src_stride_y, src_uv, src_stride_uv, src_w, src_h,
        dst_y, dst_stride_y, dst_uv, dst_stride_uv, dst_w, dst_h,
        libyuv::kFilterLinear);
    return ret == 0;
}

bool LibyuvTransformKernel::scale_rgb(
    const uint8_t* src, int src_stride,
    int src_w, int src_h,
    uint8_t* dst, int dst_stride,
    int dst_w, int dst_h) const
{
    // libyuv has no direct RGB24 scaler; use OpenCV
    cv::Mat mat_src(src_h, src_w, CV_8UC3, const_cast<uint8_t*>(src), src_stride);
    cv::Mat mat_dst(dst_h, dst_w, CV_8UC3, dst, dst_stride);
    cv::resize(mat_src, mat_dst, cv::Size(dst_w, dst_h), 0, 0, cv::INTER_LINEAR);
    return true;
}

// ---------------------------------------------------------------------------
// convert_color — full 4×4 color conversion matrix
//
// dst_0 / dst_stride_0 = plane 0 (Y for planar, packed data for RGB/BGR)
// dst_1 / dst_stride_1 = plane 1 (U for I420, UV for NV12, nullptr for packed)
// dst_2 / dst_stride_2 = plane 2 (V for I420, nullptr otherwise)
// ---------------------------------------------------------------------------

bool LibyuvTransformKernel::convert_color(
    VideoFormat src_fmt,
    int src_w, int src_h,
    int src_stride_y, int src_stride_u, int src_stride_v,
    const uint8_t* src_y, const uint8_t* src_u, const uint8_t* src_v,
    const uint8_t* src_uv,
    uint8_t* dst_0, int dst_stride_0,
    uint8_t* dst_1, int dst_stride_1,
    uint8_t* dst_2, int dst_stride_2,
    VideoFormat dst_fmt) const
{
    int ret = -1;

    // ---- Source: I420 ----
    if (src_fmt == VideoFormat::I420) {
        if (dst_fmt == VideoFormat::RGB) {
            ret = libyuv::I420ToRAW(src_y, src_stride_y,
                                    src_u, src_stride_u,
                                    src_v, src_stride_v,
                                    dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::BGR) {
            ret = libyuv::I420ToRGB24(src_y, src_stride_y,
                                      src_u, src_stride_u,
                                      src_v, src_stride_v,
                                      dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::NV12) {
            ret = libyuv::I420ToNV12(src_y, src_stride_y,
                                     src_u, src_stride_u,
                                     src_v, src_stride_v,
                                     dst_0, dst_stride_0,
                                     dst_1, dst_stride_1,
                                     src_w, src_h);
        }
    }
    // ---- Source: NV12 ----
    else if (src_fmt == VideoFormat::NV12) {
        if (dst_fmt == VideoFormat::RGB) {
            ret = libyuv::NV12ToRAW(src_y, src_stride_y,
                                    src_uv, src_stride_u,
                                    dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::BGR) {
            ret = libyuv::NV12ToRGB24(src_y, src_stride_y,
                                      src_uv, src_stride_u,
                                      dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::I420) {
            ret = libyuv::NV12ToI420(src_y, src_stride_y,
                                     src_uv, src_stride_u,
                                     dst_0, dst_stride_0,
                                     dst_1, dst_stride_1,
                                     dst_2, dst_stride_2,
                                     src_w, src_h);
        }
    }
    // ---- Source: RGB (libyuv RAW) ----
    else if (src_fmt == VideoFormat::RGB) {
        if (dst_fmt == VideoFormat::RGB) {
            for (int r = 0; r < src_h; ++r)
                memcpy(dst_0 + r * dst_stride_0,
                       src_y + r * src_stride_y,
                       src_w * 3);
            ret = 0;
        } else if (dst_fmt == VideoFormat::BGR) {
            ret = libyuv::RAWToRGB24(src_y, src_stride_y,
                                     dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::I420) {
            ret = libyuv::RAWToI420(src_y, src_stride_y,
                                    dst_0, dst_stride_0,
                                    dst_1, dst_stride_1,
                                    dst_2, dst_stride_2,
                                    src_w, src_h);
        } else if (dst_fmt == VideoFormat::NV12) {
            // Two-step: RGB → I420 (scratch) → NV12
            int y_stride = src_w;
            int u_stride = src_w / 2;
            int v_stride = src_w / 2;
            std::vector<uint8_t> tmp(src_w * src_h * 3 / 2);
            uint8_t* t_y = tmp.data();
            uint8_t* t_u = t_y + src_w * src_h;
            uint8_t* t_v = t_u + (src_w / 2) * (src_h / 2);
            ret = libyuv::RAWToI420(src_y, src_stride_y,
                                    t_y, y_stride, t_u, u_stride, t_v, v_stride,
                                    src_w, src_h);
            if (ret == 0) {
                ret = libyuv::I420ToNV12(t_y, y_stride, t_u, u_stride, t_v, v_stride,
                                         dst_0, dst_stride_0,
                                         dst_1, dst_stride_1,
                                         src_w, src_h);
            }
        }
    }
    // ---- Source: BGR (libyuv RGB24) ----
    else if (src_fmt == VideoFormat::BGR) {
        if (dst_fmt == VideoFormat::BGR) {
            for (int r = 0; r < src_h; ++r)
                memcpy(dst_0 + r * dst_stride_0,
                       src_y + r * src_stride_y,
                       src_w * 3);
            ret = 0;
        } else if (dst_fmt == VideoFormat::RGB) {
            // BGR→RGB is same as RAWToRGB24 (swaps R and B)
            ret = libyuv::RAWToRGB24(src_y, src_stride_y,
                                     dst_0, dst_stride_0, src_w, src_h);
        } else if (dst_fmt == VideoFormat::I420) {
            ret = libyuv::RGB24ToI420(src_y, src_stride_y,
                                      dst_0, dst_stride_0,
                                      dst_1, dst_stride_1,
                                      dst_2, dst_stride_2,
                                      src_w, src_h);
        } else if (dst_fmt == VideoFormat::NV12) {
            // Two-step: BGR → I420 (scratch) → NV12
            int y_stride = src_w;
            int u_stride = src_w / 2;
            int v_stride = src_w / 2;
            std::vector<uint8_t> tmp(src_w * src_h * 3 / 2);
            uint8_t* t_y = tmp.data();
            uint8_t* t_u = t_y + src_w * src_h;
            uint8_t* t_v = t_u + (src_w / 2) * (src_h / 2);
            ret = libyuv::RGB24ToI420(src_y, src_stride_y,
                                      t_y, y_stride, t_u, u_stride, t_v, v_stride,
                                      src_w, src_h);
            if (ret == 0) {
                ret = libyuv::I420ToNV12(t_y, y_stride, t_u, u_stride, t_v, v_stride,
                                         dst_0, dst_stride_0,
                                         dst_1, dst_stride_1,
                                         src_w, src_h);
            }
        }
    }

    if (ret != 0) {
        GST_ERROR("LibyuvTransformKernel: color conversion failed (src=%d dst=%d)",
                  static_cast<int>(src_fmt), static_cast<int>(dst_fmt));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// transform
// ---------------------------------------------------------------------------

TransformResult LibyuvTransformKernel::transform(const FrameDesc&  src,
                                                  FrameDesc&        dst,
                                                  int               slot_id,
                                                  const DynamicOps* dynamic) {
    TransformResult result;
    result.success = false;

    if (!initialized_) {
        GST_ERROR("LibyuvTransformKernel: transform called before init()");
        return result;
    }

    // ------------------------------------------------------------------
    // 1. Crop — pointer arithmetic (zero copy)
    // ------------------------------------------------------------------
    CropRect crop = effective_crop(src, dynamic);
    int crop_w = crop.enabled ? crop.w : src.width;
    int crop_h = crop.enabled ? crop.h : src.height;
    int cx = crop.enabled ? crop.x : 0;
    int cy = crop.enabled ? crop.y : 0;

    // Pointers into the crop region (no buffer allocation)
    const uint8_t* cr_y  = nullptr;
    const uint8_t* cr_u  = nullptr;
    const uint8_t* cr_v  = nullptr;
    const uint8_t* cr_uv = nullptr;
    int cr_stride_y = 0, cr_stride_u = 0, cr_stride_v = 0;

    switch (src.format) {
        case VideoFormat::I420: {
            cr_y = src.planes[0].data + cy * src.planes[0].stride + cx;
            cr_u = src.planes[1].data + (cy / 2) * src.planes[1].stride + (cx / 2);
            cr_v = src.planes[2].data + (cy / 2) * src.planes[2].stride + (cx / 2);
            cr_stride_y = src.planes[0].stride;
            cr_stride_u = src.planes[1].stride;
            cr_stride_v = src.planes[2].stride;
            break;
        }
        case VideoFormat::NV12: {
            cr_y  = src.planes[0].data + cy * src.planes[0].stride + cx;
            cr_uv = src.planes[1].data + (cy / 2) * src.planes[1].stride + (cx / 2) * 2;
            cr_stride_y = src.planes[0].stride;
            cr_stride_u = src.planes[1].stride;  // UV stride for NV12
            break;
        }
        case VideoFormat::RGB:
        case VideoFormat::BGR: {
            cr_y = src.planes[0].data + cy * src.planes[0].stride + cx * 3;
            cr_stride_y = src.planes[0].stride;
            break;
        }
    }

    // ------------------------------------------------------------------
    // 2. Compute output placement (letterbox or full)
    // ------------------------------------------------------------------
    int dst_x, dst_y, content_w, content_h;
    compute_dst_rect(crop_w, crop_h, dst_x, dst_y, content_w, content_h);

    const int out_w = dst_template_.width;
    const int out_h = dst_template_.height;
    VideoFormat src_fmt = src.format;
    VideoFormat dst_fmt = dst_template_.format;

    bool needs_scale   = (crop_w != content_w || crop_h != content_h);
    bool needs_convert = (src_fmt != dst_fmt);
    bool needs_pad     = (ops_.keep_aspect_ratio && ops_.padding.enabled &&
                          (dst_x > 0 || dst_y > 0));

    // ------------------------------------------------------------------
    // 3. Fill padding if letterbox is active
    // ------------------------------------------------------------------
    if (needs_pad) {
        fill_padding(dst.planes[0].data, out_w, out_h,
                     dst.planes[0].stride, dst_fmt);
    }

    // Compute output pointer and stride for content region
    int bpp = bytes_per_pixel(dst_fmt);
    int dst_content_stride = dst.planes[0].stride;
    uint8_t* dst_content_ptr = dst.planes[0].data;
    uint8_t* dst_uv_ptr   = dst.num_planes > 1 ? dst.planes[1].data : nullptr;
    uint8_t* dst_v_ptr    = dst.num_planes > 2 ? dst.planes[2].data : nullptr;
    int      dst_uv_stride = dst.num_planes > 1 ? dst.planes[1].stride : 0;
    int      dst_v_stride  = dst.num_planes > 2 ? dst.planes[2].stride : 0;

    if (bpp > 0) {
        dst_content_ptr += dst_y * dst_content_stride + dst_x * bpp;
    } else if (dst_x > 0 || dst_y > 0) {
        // YUV 포맷: 각 plane에 letterbox 오프셋 적용
        dst_content_ptr += dst_y * dst_content_stride + dst_x;
        int uv_y = dst_y / 2;
        int uv_x = (dst_fmt == VideoFormat::NV12) ? dst_x : dst_x / 2;
        if (dst_uv_ptr) dst_uv_ptr += uv_y * dst_uv_stride + uv_x;
        if (dst_v_ptr)  dst_v_ptr  += uv_y * dst_v_stride  + uv_x;
    }

    // ------------------------------------------------------------------
    // 4. Dispatch based on what's needed
    // ------------------------------------------------------------------

    // Case A: Same format, scale only (or no-op)
    if (!needs_convert) {
        if (!needs_scale) {
            // Direct copy to output
            if (bpp > 0) {
                // Packed format — row copy
                for (int r = 0; r < content_h; ++r)
                    memcpy(dst_content_ptr + r * dst_content_stride,
                           cr_y + r * cr_stride_y,
                           content_w * bpp);
            } else if (dst_fmt == VideoFormat::I420) {
                scale_i420(cr_y, cr_stride_y, cr_u, cr_stride_u, cr_v, cr_stride_v,
                           crop_w, crop_h,
                           dst_content_ptr, dst_content_stride,
                           dst_uv_ptr, dst_uv_stride,
                           dst_v_ptr, dst_v_stride,
                           content_w, content_h);
            } else if (dst_fmt == VideoFormat::NV12) {
                scale_nv12(cr_y, cr_stride_y, cr_uv, cr_stride_u,
                           crop_w, crop_h,
                           dst_content_ptr, dst_content_stride,
                           dst_uv_ptr, dst_uv_stride,
                           content_w, content_h);
            }
        } else {
            // Scale in same format
            if (bpp > 0) {
                scale_rgb(cr_y, cr_stride_y, crop_w, crop_h,
                          dst_content_ptr, dst_content_stride,
                          content_w, content_h);
            } else if (dst_fmt == VideoFormat::I420) {
                scale_i420(cr_y, cr_stride_y, cr_u, cr_stride_u, cr_v, cr_stride_v,
                           crop_w, crop_h,
                           dst_content_ptr, dst_content_stride,
                           dst_uv_ptr, dst_uv_stride,
                           dst_v_ptr, dst_v_stride,
                           content_w, content_h);
            } else if (dst_fmt == VideoFormat::NV12) {
                scale_nv12(cr_y, cr_stride_y, cr_uv, cr_stride_u,
                           crop_w, crop_h,
                           dst_content_ptr, dst_content_stride,
                           dst_uv_ptr, dst_uv_stride,
                           content_w, content_h);
            }
        }
        result.success = true;
    }
    // Case B: Convert only (no scale)
    else if (!needs_scale) {
        result.success = convert_color(
            src_fmt, crop_w, crop_h,
            cr_stride_y, cr_stride_u, cr_stride_v,
            cr_y, cr_u, cr_v, cr_uv,
            dst_content_ptr, dst_content_stride,
            dst_uv_ptr, dst_uv_stride,
            dst_v_ptr, dst_v_stride,
            dst_fmt);
    }
    // Case C: Scale + Convert (needs 1 scratch buffer)
    else {
        // Scale in source format domain first (smaller intermediate).
        // libyuv uses ceil-division for chroma plane heights: (h+1)/2.
        // The scratch buffer must account for this to avoid overflow
        // when content_h is odd.
        size_t scratch_size = 0;
        const int half_w = (content_w + 1) / 2;
        const int half_h = (content_h + 1) / 2;
        if (src_fmt == VideoFormat::I420) {
            scratch_size = (size_t)content_w * content_h
                         + 2 * (size_t)half_w * half_h;
        } else if (src_fmt == VideoFormat::NV12) {
            scratch_size = (size_t)content_w * content_h
                         + (size_t)content_w * half_h;
        } else {
            scratch_size = content_w * content_h * 3;
        }

        auto& sbuf = scratch_[slot_id];
        if (sbuf.size() < scratch_size)
            sbuf.resize(scratch_size);

        bool scale_ok = false;
        if (src_fmt == VideoFormat::I420) {
            uint8_t* sc_y = sbuf.data();
            uint8_t* sc_u = sc_y + content_w * content_h;
            uint8_t* sc_v = sc_u + (size_t)half_w * half_h;
            scale_ok = scale_i420(cr_y, cr_stride_y, cr_u, cr_stride_u,
                                  cr_v, cr_stride_v,
                                  crop_w, crop_h,
                                  sc_y, content_w,
                                  sc_u, half_w,
                                  sc_v, half_w,
                                  content_w, content_h);
        } else if (src_fmt == VideoFormat::NV12) {
            uint8_t* sc_y  = sbuf.data();
            uint8_t* sc_uv = sc_y + content_w * content_h;
            scale_ok = scale_nv12(cr_y, cr_stride_y, cr_uv, cr_stride_u,
                                  crop_w, crop_h,
                                  sc_y, content_w,
                                  sc_uv, content_w,
                                  content_w, content_h);
        } else {
            scale_ok = scale_rgb(cr_y, cr_stride_y, crop_w, crop_h,
                                 sbuf.data(), content_w * 3,
                                 content_w, content_h);
        }

        if (!scale_ok) {
            GST_ERROR("LibyuvTransformKernel: scale step failed");
            return result;
        }

        // Now convert scaled data → output
        const uint8_t* s_y = sbuf.data();
        const uint8_t* s_u = nullptr;
        const uint8_t* s_v = nullptr;
        const uint8_t* s_uv = nullptr;
        int s_stride_y = 0, s_stride_u = 0, s_stride_v = 0;

        if (src_fmt == VideoFormat::I420) {
            s_stride_y = content_w;
            s_stride_u = half_w;
            s_stride_v = half_w;
            s_u = s_y + content_w * content_h;
            s_v = s_u + (size_t)half_w * half_h;
        } else if (src_fmt == VideoFormat::NV12) {
            s_stride_y = content_w;
            s_stride_u = content_w;
            s_uv = s_y + content_w * content_h;
        } else {
            s_stride_y = content_w * 3;
        }

        result.success = convert_color(
            src_fmt, content_w, content_h,
            s_stride_y, s_stride_u, s_stride_v,
            s_y, s_u, s_v, s_uv,
            dst_content_ptr, dst_content_stride,
            dst_uv_ptr, dst_uv_stride,
            dst_v_ptr, dst_v_stride,
            dst_fmt);
    }

    // ------------------------------------------------------------------
    // 5. Build result
    // ------------------------------------------------------------------
    if (result.success && ops_.keep_aspect_ratio) {
        result.content_rect = { dst_x, dst_y, content_w, content_h, true };
    }
    return result;
}

}  // namespace dxt
