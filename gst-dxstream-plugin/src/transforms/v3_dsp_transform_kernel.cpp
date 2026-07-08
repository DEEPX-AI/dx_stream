#ifdef DEEPX_V3

#include "v3_dsp_transform_kernel.hpp"

#include "dxcv/dxcvext.hpp"
#include <gst/gst.h>

#include <algorithm>
#include <cstring>

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

namespace dxt {

// DSP buffer layout: 24 MB total, split into two 12 MB halves
static constexpr size_t DSP_BUFFER_HALF_OFFSET = 0xC00000;  // 12 MB

// ---------------------------------------------------------------------------
// ctor / dtor
// ---------------------------------------------------------------------------

V3DspTransformKernel::V3DspTransformKernel() = default;

V3DspTransformKernel::~V3DspTransformKernel() {
    if (dsp_buffer_0_) {
        dxcvext::freeDspBuffer(dsp_buffer_0_);
        dsp_buffer_0_ = nullptr;
        dsp_buffer_1_ = nullptr;
    }
}

// ---------------------------------------------------------------------------
// capabilities
// ---------------------------------------------------------------------------

BackendCaps V3DspTransformKernel::capabilities() const {
    BackendCaps caps;
    caps.name             = "v3dsp";
    caps.hw_accelerated   = true;
    caps.supports_dma_buf = false;
    caps.max_width        = 8192;
    caps.max_height       = 8192;
    caps.src_formats      = { VideoFormat::I420, VideoFormat::RGB, VideoFormat::BGR };
    caps.dst_formats      = { VideoFormat::RGB, VideoFormat::BGR };
    return caps;
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool V3DspTransformKernel::init(const FrameDesc& dst_template,
                                 const TransformOps& ops) {
    // Allocate DSP double buffer
    dsp_buffer_0_ = dxcvext::allocDspBuffer();
    if (!dsp_buffer_0_) {
        GST_ERROR("V3DspTransformKernel: failed to allocate DSP buffer");
        return false;
    }
    dsp_buffer_1_ = dsp_buffer_0_ + DSP_BUFFER_HALF_OFFSET;

    if (!TransformKernelBase::init(dst_template, ops)) {
        return false;
    }

    GST_DEBUG("V3DspTransformKernel: init OK  dst=%dx%d  fmt=%d  keep_ratio=%d",
              dst_template_.width, dst_template_.height,
              static_cast<int>(dst_template_.format),
              ops_.keep_aspect_ratio);
    return true;
}

// ---------------------------------------------------------------------------
// crop helpers
// ---------------------------------------------------------------------------

bool V3DspTransformKernel::crop_i420(
    const uint8_t* src, int src_stride_y, int src_stride_u, int src_stride_v,
    size_t offset_y, size_t offset_u, size_t offset_v,
    uint8_t* dst, int src_w, int src_h,
    int crop_x, int crop_y, int crop_w, int crop_h) const
{
    const uint8_t* src_y = src + offset_y;
    const uint8_t* src_u = src + offset_u;
    const uint8_t* src_v = src + offset_v;

    uint8_t* dst_y = dst;
    uint8_t* dst_u = dst_y + crop_w * crop_h;
    uint8_t* dst_v = dst_u + (crop_w / 2) * (crop_h / 2);

    dxcvext::I420Crop(
        const_cast<uint8_t*>(src_y), src_stride_y,
        const_cast<uint8_t*>(src_u), src_stride_u,
        const_cast<uint8_t*>(src_v), src_stride_v,
        dst_y, crop_w, dst_u, crop_w / 2, dst_v, crop_w / 2,
        src_w, src_h, crop_w, crop_h, crop_x, crop_y);

    return true;
}

bool V3DspTransformKernel::crop_rgb(const uint8_t* src, uint8_t* dst,
                                     int src_w, int src_h,
                                     int crop_x, int crop_y,
                                     int crop_w, int crop_h) const {
    dxcvext::RGBCrop(
        const_cast<uint8_t*>(src), src_w * 3,
        dst, crop_w * 3,
        src_w, src_h, crop_w, crop_h, crop_x, crop_y);
    return true;
}

// ---------------------------------------------------------------------------
// scale helpers
// ---------------------------------------------------------------------------

bool V3DspTransformKernel::scale_i420(const uint8_t* src, uint8_t* dst,
                                       int src_w, int src_h,
                                       int dst_w, int dst_h) const {
    const uint8_t* src_y = src;
    const uint8_t* src_u = src_y + src_w * src_h;
    const uint8_t* src_v = src_u + (src_w / 2) * (src_h / 2);

    uint8_t* dst_y = dst;
    uint8_t* dst_u = dst_y + dst_w * dst_h;
    uint8_t* dst_v = dst_u + (dst_w / 2) * (dst_h / 2);

    dxcvext::I420Scale(
        const_cast<uint8_t*>(src_y), src_w,
        const_cast<uint8_t*>(src_u), src_w / 2,
        const_cast<uint8_t*>(src_v), src_w / 2,
        src_w, src_h,
        dst_y, dst_w, dst_u, dst_w / 2, dst_v, dst_w / 2,
        dst_w, dst_h);
    return true;
}

bool V3DspTransformKernel::scale_rgb(const uint8_t* src, uint8_t* dst,
                                      int src_w, int src_h,
                                      int dst_w, int dst_h,
                                      VideoFormat fmt) const {
    if (fmt == VideoFormat::RGB) {
        dxcvext::RGB24Scale(
            const_cast<uint8_t*>(src), src_w * 3, src_w, src_h,
            dst, dst_w * 3, dst_w, dst_h);
    } else {  // BGR
        dxcvext::BGR24Scale(
            const_cast<uint8_t*>(src), src_w * 3, src_w, src_h,
            dst, dst_w * 3, dst_w, dst_h);
    }
    return true;
}

// ---------------------------------------------------------------------------
// color convert
// ---------------------------------------------------------------------------

bool V3DspTransformKernel::convert_i420_to_packed(
    const uint8_t* src, uint8_t* dst,
    int width, int height, VideoFormat dst_fmt) const
{
    const uint8_t* src_y = src;
    const uint8_t* src_u = src_y + width * height;
    const uint8_t* src_v = src_u + (width / 2) * (height / 2);

    if (dst_fmt == VideoFormat::RGB) {
        dxcvext::I420ToRGB24(
            const_cast<uint8_t*>(src_y), width,
            const_cast<uint8_t*>(src_u), width / 2,
            const_cast<uint8_t*>(src_v), width / 2,
            dst, width * 3, width, height);
    } else {  // BGR
        dxcvext::I420ToBGR24(
            const_cast<uint8_t*>(src_y), width,
            const_cast<uint8_t*>(src_u), width / 2,
            const_cast<uint8_t*>(src_v), width / 2,
            dst, width * 3, width, height);
    }
    return true;
}

// ---------------------------------------------------------------------------
// transform
// ---------------------------------------------------------------------------

TransformResult V3DspTransformKernel::transform(const FrameDesc&  src,
                                                 FrameDesc&        dst,
                                                 int               slot_id,
                                                 const DynamicOps* dynamic) {
    (void)slot_id;  // V3 DSP uses internal double buffer, not per-slot

    TransformResult result;
    result.success = false;

    if (!initialized_) {
        GST_ERROR("V3DspTransformKernel: transform called before init()");
        return result;
    }

    // ------------------------------------------------------------------
    // 0. Copy src data into DSP buffer_0
    //    (DSP operations require data in DSP-allocated memory)
    // ------------------------------------------------------------------
    size_t src_size = 0;
    switch (src.format) {
        case VideoFormat::I420:
            src_size = src.planes[0].stride * src.height +
                       src.planes[1].stride * (src.height / 2) +
                       src.planes[2].stride * (src.height / 2);
            break;
        case VideoFormat::RGB:
        case VideoFormat::BGR:
            src_size = src.planes[0].stride * src.height;
            break;
        case VideoFormat::NV12:
            GST_ERROR("V3DspTransformKernel: NV12 not supported by V3 DSP");
            return result;
    }

    // Copy from mapped GstBuffer (src plane data) into DSP buffer
    // For I420, the src might have non-contiguous planes (GstVideoInfo offsets),
    // so we must respect the plane layout when the source data pointer
    // is the same base with different offsets.
    if (src.format == VideoFormat::I420 && src.num_planes == 3) {
        // Copy respecting original layout (offsets may differ from contiguous)
        memcpy(dsp_buffer_0_ + src.planes[0].offset, src.planes[0].data,
               src.planes[0].stride * src.height);
        memcpy(dsp_buffer_0_ + src.planes[1].offset, src.planes[1].data,
               src.planes[1].stride * (src.height / 2));
        memcpy(dsp_buffer_0_ + src.planes[2].offset, src.planes[2].data,
               src.planes[2].stride * (src.height / 2));
    } else {
        memcpy(dsp_buffer_0_, src.planes[0].data, src_size);
    }

    // ------------------------------------------------------------------
    // 1. Processing state (ping-pong buffers)
    // ------------------------------------------------------------------
    uint8_t* current = dsp_buffer_0_;
    uint8_t* work    = dsp_buffer_1_;
    int cur_w = src.width;
    int cur_h = src.height;
    bool is_contiguous = false;  // After crop, data is packed contiguously
    VideoFormat cur_fmt = src.format;

    // ------------------------------------------------------------------
    // 2. Crop (if needed)
    // ------------------------------------------------------------------
    CropRect crop = effective_crop(src, dynamic);
    if (crop.enabled && (crop.x != 0 || crop.y != 0 ||
                         crop.w != src.width || crop.h != src.height)) {
        if (cur_fmt == VideoFormat::I420) {
            crop_i420(current,
                      src.planes[0].stride, src.planes[1].stride, src.planes[2].stride,
                      src.planes[0].offset, src.planes[1].offset, src.planes[2].offset,
                      work, cur_w, cur_h,
                      crop.x, crop.y, crop.w, crop.h);
        } else {
            crop_rgb(current, work, cur_w, cur_h,
                     crop.x, crop.y, crop.w, crop.h);
        }
        std::swap(current, work);
        cur_w = crop.w;
        cur_h = crop.h;
        is_contiguous = true;
    }

    // ------------------------------------------------------------------
    // 3. Calculate target size (with aspect ratio)
    // ------------------------------------------------------------------
    int dst_x, dst_y, content_w, content_h;
    compute_dst_rect(cur_w, cur_h, dst_x, dst_y, content_w, content_h);

    // ------------------------------------------------------------------
    // 4. Scale (if needed)
    // ------------------------------------------------------------------
    bool needs_scale = (content_w != cur_w || content_h != cur_h);
    if (needs_scale) {
        if (cur_fmt == VideoFormat::I420) {
            if (!is_contiguous) {
                // If not contiguous, we need to handle stride/offset —
                // but after the memcpy to DSP buffer and possible crop,
                // the data should be either original-layout or contiguous.
                // For the original-layout case (no crop happened),
                // scale_i420 expects contiguous packed I420 input.
                // We must pack it first.
                // However, the original V3DspPreprocessor also hit this:
                // it passed offset/stride from GstVideoInfo when is_contiguous==false.
                // We replicate that: use stride info directly.
                // Note: scale_i420 internally uses contiguous strides,
                // so for non-contiguous case we'd need a different path.
                // The original code handled this by passing stride info to dxcvext::I420Scale.
                // Let's replicate the original exact behavior.
                const uint8_t* sy = current + src.planes[0].offset;
                const uint8_t* su = current + src.planes[1].offset;
                const uint8_t* sv = current + src.planes[2].offset;
                uint8_t* dy = work;
                uint8_t* du = dy + content_w * content_h;
                uint8_t* dv = du + (content_w / 2) * (content_h / 2);

                dxcvext::I420Scale(
                    const_cast<uint8_t*>(sy), src.planes[0].stride,
                    const_cast<uint8_t*>(su), src.planes[1].stride,
                    const_cast<uint8_t*>(sv), src.planes[2].stride,
                    cur_w, cur_h,
                    dy, content_w, du, content_w / 2, dv, content_w / 2,
                    content_w, content_h);
            } else {
                scale_i420(current, work, cur_w, cur_h, content_w, content_h);
            }
        } else {
            scale_rgb(current, work, cur_w, cur_h, content_w, content_h, cur_fmt);
        }
        std::swap(current, work);
        cur_w = content_w;
        cur_h = content_h;
        is_contiguous = true;
    }

    // ------------------------------------------------------------------
    // 5. Color convert (if needed)
    // ------------------------------------------------------------------
    VideoFormat dst_fmt = dst_template_.format;
    bool needs_convert = (cur_fmt != dst_fmt);

    if (needs_convert) {
        if (cur_fmt == VideoFormat::I420 &&
            (dst_fmt == VideoFormat::RGB || dst_fmt == VideoFormat::BGR)) {
            convert_i420_to_packed(current, work, cur_w, cur_h, dst_fmt);
        } else {
            GST_ERROR("V3DspTransformKernel: unsupported conversion %d → %d",
                      static_cast<int>(cur_fmt), static_cast<int>(dst_fmt));
            return result;
        }
        std::swap(current, work);
        cur_fmt = dst_fmt;
    }

    // ------------------------------------------------------------------
    // 6. Padding + copy to output
    // ------------------------------------------------------------------
    const int out_w = dst_template_.width;
    const int out_h = dst_template_.height;
    int bpp = bytes_per_pixel(dst_fmt);

    if (ops_.keep_aspect_ratio && (dst_x > 0 || dst_y > 0)) {
        // Fill entire output with padding value
        memset(dst.planes[0].data, ops_.padding.pad_r,
               out_w * out_h * bpp);

        // Copy content to center
        for (int row = 0; row < content_h; ++row) {
            const uint8_t* src_row = current + row * content_w * bpp;
            uint8_t* dst_row = dst.planes[0].data +
                               ((row + dst_y) * out_w + dst_x) * bpp;
            memcpy(dst_row, src_row, content_w * bpp);
        }
    } else {
        // No padding — direct copy
        memcpy(dst.planes[0].data, current, out_w * out_h * bpp);
    }

    // ------------------------------------------------------------------
    // 7. Build result
    // ------------------------------------------------------------------
    result.success = true;
    if (ops_.keep_aspect_ratio) {
        result.content_rect = { dst_x, dst_y, content_w, content_h, true };
    }
    return result;
}

}  // namespace dxt

#endif  // DEEPX_V3
