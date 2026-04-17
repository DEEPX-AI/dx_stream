#ifdef HAVE_LIBRGA

#include "rga_transform_kernel.hpp"
#include "gst_frame_desc.hpp"   // rga_hstride()

#include <cstdio>   // required by im2d.h (uses printf internally)
#include <rga/rga.h>
#include <rga/im2d.h>
#include <gst/gst.h>

#include <algorithm>
#include <cstring>
#include <mutex>

#define GST_CAT_DEFAULT dxt_rga_debug
GST_DEBUG_CATEGORY_STATIC(dxt_rga_debug);

namespace dxt {

// ---------------------------------------------------------------------------
// Format helper — VideoFormat → RK_FORMAT_*
// ---------------------------------------------------------------------------

static RgaSURF_FORMAT to_rga_format(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::NV12: return RK_FORMAT_YCbCr_420_SP;
        case VideoFormat::I420: return RK_FORMAT_YCbCr_420_P;
        case VideoFormat::RGB:  return RK_FORMAT_RGB_888;
        case VideoFormat::BGR:  return RK_FORMAT_BGR_888;
    }
    return RK_FORMAT_RGB_888;  // unreachable
}

static bool is_yuv_format(VideoFormat fmt) {
    return fmt == VideoFormat::NV12 || fmt == VideoFormat::I420;
}

// ---------------------------------------------------------------------------
// capabilities
// ---------------------------------------------------------------------------

BackendCaps RgaTransformKernel::capabilities() const {
    return BackendCaps{
        .name             = "rga",
        .hw_accelerated   = true,
        .supports_dma_buf = true,
        .max_width        = 8176,
        .max_height       = 8176,
        .src_formats      = { VideoFormat::NV12,
                              VideoFormat::RGB,  VideoFormat::BGR },
        .dst_formats      = { VideoFormat::NV12,
                              VideoFormat::RGB,  VideoFormat::BGR },
    };
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool RgaTransformKernel::init(const FrameDesc& dst_template,
                               const TransformOps& ops) {
    static gsize debug_once = 0;
    if (g_once_init_enter(&debug_once)) {
        GST_DEBUG_CATEGORY_INIT(dxt_rga_debug, "dxt_rga", 0, "DXT RGA transform kernel");
        g_once_init_leave(&debug_once, 1);
    }

    // Only formats safe with RGA's single-pointer wrapbuffer model.
    // I420 (3-plane) is excluded: RGA internally computes U/V offsets from
    // wstride×hstride, which may not match GStreamer's plane layout.
    // → I420 falls through to libyuv via the factory.
    switch (dst_template.format) {
        case VideoFormat::NV12:
        case VideoFormat::RGB:
        case VideoFormat::BGR:
            break;
        default:
            GST_INFO("RgaTransformKernel: dst format %s not safe for RGA, rejecting",
                     video_format_to_string(dst_template.format));
            return false;
    }

    if (!TransformKernelBase::init(dst_template, ops)) {
        return false;
    }

    // Prepare internal libyuv fallback (always succeeds for any format pair)
    libyuv_fallback_ = std::make_unique<LibyuvTransformKernel>();
    if (!libyuv_fallback_->init(dst_template, ops)) {
        GST_ERROR("RgaTransformKernel: libyuv fallback init failed (unexpected)");
        return false;
    }

    GST_DEBUG("RgaTransformKernel: init OK  dst=%dx%d  fmt=%s  keep_ratio=%d  "
              "(libyuv fallback ready)",
              dst_template_.width, dst_template_.height,
              video_format_to_string(dst_template_.format),
              ops_.keep_aspect_ratio);
    return true;
}

// ---------------------------------------------------------------------------
// effective_crop  — even-alignment for YUV, passthrough for RGB
// ---------------------------------------------------------------------------

CropRect RgaTransformKernel::effective_crop(const FrameDesc& src,
                                              const DynamicOps* dynamic) const {
    const CropRect* cr = nullptr;

    if (dynamic && dynamic->crop_override && dynamic->crop_override->enabled) {
        cr = dynamic->crop_override;
    } else if (ops_.crop.enabled) {
        cr = &ops_.crop;
    }

    if (!cr) {
        return CropRect{ 0, 0, src.width, src.height, false };
    }

    int x = cr->x, y = cr->y, w = cr->w, h = cr->h;

    // RGA requires even-aligned coordinates for YUV formats
    if (is_yuv_format(src.format)) {
        if (x % 2 != 0) x++;
        if (y % 2 != 0) y++;
        if (w % 2 != 0) w++;
        if (h % 2 != 0) h++;
    }

    // Clamp to frame boundaries
    x = std::max(x, 0);
    y = std::max(y, 0);
    if (x + w > src.width)  w = src.width  - x;
    if (y + h > src.height) h = src.height - y;

    return CropRect{ x, y, w, h, true };
}

// ---------------------------------------------------------------------------
// compute_dst_rect
// ---------------------------------------------------------------------------

void RgaTransformKernel::compute_dst_rect(int src_w, int src_h,
                                           int& dst_x, int& dst_y,
                                           int& dst_w, int& dst_h) const {
    const int out_w = dst_template_.width;
    const int out_h = dst_template_.height;

    if (!ops_.keep_aspect_ratio) {
        dst_x = 0;
        dst_y = 0;
        dst_w = out_w;
        dst_h = out_h;
        return;
    }

    // Letterbox: fit src aspect ratio into output, centred
    float ratio_dst = static_cast<float>(out_w) / out_h;
    float ratio_src = static_cast<float>(src_w) / src_h;

    int new_w, new_h;
    if (ratio_src < ratio_dst) {
        new_h = out_h;
        new_w = static_cast<int>(new_h * ratio_src);
    } else {
        new_w = out_w;
        new_h = static_cast<int>(new_w / ratio_src);
    }

    // Ensure even dimensions for YUV dst
    if (is_yuv_format(dst_template_.format)) {
        new_w &= ~1;
        new_h &= ~1;
    }

    int pad_x = (out_w - new_w) / 2;
    int pad_y = (out_h - new_h) / 2;

    // Ensure even offsets for YUV dst
    if (is_yuv_format(dst_template_.format)) {
        pad_x &= ~1;
        pad_y &= ~1;
    }

    dst_x = pad_x;
    dst_y = pad_y;
    dst_w = new_w;
    dst_h = new_h;
}

// ---------------------------------------------------------------------------
// fill_padding — format-aware letterbox fill
// ---------------------------------------------------------------------------

void RgaTransformKernel::fill_padding(FrameDesc& dst) const {
    if (!ops_.keep_aspect_ratio || !ops_.padding.enabled)
        return;

    const int out_w = dst_template_.width;
    const int out_h = dst_template_.height;
    const VideoFormat fmt = dst_template_.format;

    if (fmt == VideoFormat::RGB || fmt == VideoFormat::BGR) {
        // 3-byte packed: fill with (pad_r, pad_g, pad_b) per pixel
        for (int row = 0; row < out_h; ++row) {
            uint8_t* line = dst.planes[0].data + row * dst.planes[0].stride;
            for (int col = 0; col < out_w; ++col) {
                line[col * 3 + 0] = ops_.padding.pad_r;
                line[col * 3 + 1] = ops_.padding.pad_g;
                line[col * 3 + 2] = ops_.padding.pad_b;
            }
        }
    } else if (fmt == VideoFormat::NV12) {
        // Y plane: fill with Y value  (BT.601: Y ≈ 0.299R + 0.587G + 0.114B)
        // For default gray (114,114,114): Y≈114, U=V=128
        uint8_t y_val = static_cast<uint8_t>(
            0.299f * ops_.padding.pad_r +
            0.587f * ops_.padding.pad_g +
            0.114f * ops_.padding.pad_b);
        if (dst.planes[0].data) {
            std::memset(dst.planes[0].data, y_val,
                        static_cast<size_t>(dst.planes[0].stride) * out_h);
        }
        // UV interleaved plane: fill with (128, 128) = neutral chroma
        if (dst.planes[1].data) {
            std::memset(dst.planes[1].data, 128,
                        static_cast<size_t>(dst.planes[1].stride) * (out_h / 2));
        }
    } else if (fmt == VideoFormat::I420) {
        uint8_t y_val = static_cast<uint8_t>(
            0.299f * ops_.padding.pad_r +
            0.587f * ops_.padding.pad_g +
            0.114f * ops_.padding.pad_b);
        if (dst.planes[0].data) {
            std::memset(dst.planes[0].data, y_val,
                        static_cast<size_t>(dst.planes[0].stride) * out_h);
        }
        if (dst.planes[1].data) {
            std::memset(dst.planes[1].data, 128,
                        static_cast<size_t>(dst.planes[1].stride) * (out_h / 2));
        }
        if (dst.planes[2].data) {
            std::memset(dst.planes[2].data, 128,
                        static_cast<size_t>(dst.planes[2].stride) * (out_h / 2));
        }
    }
}

// ---------------------------------------------------------------------------
// rga_execute — run improcess() on the RGA hardware
// ---------------------------------------------------------------------------

TransformResult RgaTransformKernel::rga_execute(const FrameDesc& src,
                                                 FrameDesc& dst,
                                                 const CropRect& crop,
                                                 int dst_x, int dst_y,
                                                 int dst_w, int dst_h) {
    TransformResult result;
    result.success = false;

    // ------------------------------------------------------------------
    // Build RGA src buffer descriptor
    // ------------------------------------------------------------------
    RgaSURF_FORMAT src_fmt_rga = to_rga_format(src.format);
    int src_wstride = src.planes[0].stride;
    int src_hstride = src.height;  // default: actual height

    // For NV12 src: derive physical hstride from UV plane offset (HW decoder
    // may pad rows beyond the visible height).
    if (src.format == VideoFormat::NV12 && src_wstride > 0 &&
        src.planes[1].offset > src.planes[0].offset) {
        int derived = static_cast<int>(
            (src.planes[1].offset - src.planes[0].offset)
            / static_cast<size_t>(src_wstride));
        if (derived >= src.height) {
            src_hstride = derived;
        }
    }

    // For NV12 Y plane: byte stride == pixel stride
    // For I420 Y plane: byte stride == pixel stride
    // For RGB/BGR: byte stride → pixel stride (3 bytes per pixel).
    // GStreamer may 4-byte-align RGB strides (e.g. 426×3=1278 → 1280);
    // if the byte stride is not a multiple of 3, RGA cannot represent it
    // as an integer pixel stride — fall back to libyuv.
    int src_wstride_px = src_wstride;
    if (src.format == VideoFormat::RGB || src.format == VideoFormat::BGR) {
        if (src_wstride % 3 != 0) {
            GST_WARNING("RgaTransformKernel: src RGB/BGR byte stride %d "
                        "not divisible by 3 → falling back to libyuv",
                        src_wstride);
            return result;
        }
        src_wstride_px = src_wstride / 3;
    }

    rga_buffer_t src_img{};
    if (src.memory_type == MemoryType::DMA_BUF && src.dma_fd >= 0) {
        src_img = wrapbuffer_fd(
            src.dma_fd,
            src.width, src.height,
            src_fmt_rga,
            src_wstride_px, src_hstride);
        GST_DEBUG("RgaTransformKernel: src DMA-buf fd=%d  fmt=%s  wstride=%d  hstride=%d",
                  src.dma_fd, video_format_to_string(src.format),
                  src_wstride_px, src_hstride);
    } else {
        if (src.planes[0].data == nullptr) {
            GST_ERROR("RgaTransformKernel: CPU_VIRTUAL src has null data pointer");
            return result;
        }
        src_img = wrapbuffer_virtualaddr(
            static_cast<void*>(src.planes[0].data),
            src.width, src.height,
            src_fmt_rga,
            src_wstride_px, src_hstride);
        GST_DEBUG("RgaTransformKernel: src virtual  fmt=%s  wstride=%d  hstride=%d",
                  video_format_to_string(src.format),
                  src_wstride_px, src_hstride);
    }

    // ------------------------------------------------------------------
    // Build RGA dst buffer descriptor
    //
    // CRITICAL: dst hstride must be the actual allocated height, NOT
    // rga_hstride().  Using a padded hstride causes RGA to write beyond
    // the buffer → kernel panic.
    // ------------------------------------------------------------------
    RgaSURF_FORMAT dst_fmt_rga = to_rga_format(dst_template_.format);
    int dst_wstride_px = dst.planes[0].stride;
    if (dst_template_.format == VideoFormat::RGB || dst_template_.format == VideoFormat::BGR) {
        if (dst.planes[0].stride % 3 != 0) {
            GST_WARNING("RgaTransformKernel: dst RGB/BGR byte stride %d "
                        "not divisible by 3 → falling back to libyuv",
                        dst.planes[0].stride);
            return result;
        }
        dst_wstride_px = dst.planes[0].stride / 3;
    }

    rga_buffer_t dst_img = wrapbuffer_virtualaddr(
        static_cast<void*>(dst.planes[0].data),
        dst_template_.width, dst_template_.height,
        dst_fmt_rga,
        dst_wstride_px, dst_template_.height);  // hstride = actual height

    GST_DEBUG("RgaTransformKernel: dst fmt=%s  wstride=%d  hstride=%d",
              video_format_to_string(dst_template_.format),
              dst_wstride_px, dst_template_.height);

    // ------------------------------------------------------------------
    // Build im_rect for src and dst
    // ------------------------------------------------------------------
    im_rect src_rect{};
    src_rect.x      = crop.enabled ? crop.x : 0;
    src_rect.y      = crop.enabled ? crop.y : 0;
    src_rect.width  = crop.enabled ? crop.w : src.width;
    src_rect.height = crop.enabled ? crop.h : src.height;

    im_rect dst_rect{};
    dst_rect.x      = dst_x;
    dst_rect.y      = dst_y;
    dst_rect.width  = dst_w;
    dst_rect.height = dst_h;

    // ------------------------------------------------------------------
    // RGA hardware limit check (every frame)
    // Ref: https://github.com/airockchip/librga/blob/main/docs/Rockchip_Developer_Guide_RGA_EN.md
    // ------------------------------------------------------------------
    // Input resolution range: 68x2 ~ 8176x8176
    if (src_rect.width < 68 || src_rect.height < 2 ||
        src_rect.width > 8176 || src_rect.height > 8176) {
        GST_WARNING("RgaTransformKernel: src resolution %dx%d out of range "
                    "[68x2 ~ 8176x8176] → libyuv fallback",
                    src_rect.width, src_rect.height);
        return result;
    }
    // Output resolution range: 68x2 ~ 8128x8128
    if (dst_rect.width < 68 || dst_rect.height < 2 ||
        dst_rect.width > 8128 || dst_rect.height > 8128) {
        GST_WARNING("RgaTransformKernel: dst resolution %dx%d out of range "
                    "[68x2 ~ 8128x8128] → libyuv fallback",
                    dst_rect.width, dst_rect.height);
        return result;
    }
    // Scale ratio limit: 1/8 ~ 8 (inclusive, per RGA3 spec)
    float w_scale = static_cast<float>(dst_rect.width) / src_rect.width;
    float h_scale = static_cast<float>(dst_rect.height) / src_rect.height;
    if (w_scale < 0.125f || w_scale > 8.0f ||
        h_scale < 0.125f || h_scale > 8.0f) {
        GST_WARNING("RgaTransformKernel: scale ratio (%.3f, %.3f) exceeds "
                    "[1/8 ~ 8] → libyuv fallback", w_scale, h_scale);
        return result;
    }

    // ------------------------------------------------------------------
    // One-time RGA3 core pinning (thread-safe via std::call_once)
    // ------------------------------------------------------------------
    static std::once_flag rga_config_flag;
    std::call_once(rga_config_flag, [] {
        // Pin to RGA3 cores only.
        // RGA2-Enhance on RK3588 has 32-bit IOMMU (4GB limit); buffers
        // allocated above 4GB physical address cause kernel panic.
        // RGA3 cores have 40-bit addressing — safe for all memory.
        imconfig(IM_CONFIG_SCHEDULER_CORE,
                 IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    });

    // ------------------------------------------------------------------
    // Per-frame imcheck validation
    // ------------------------------------------------------------------
    int check = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (check != IM_STATUS_NOERROR) {
        GST_WARNING("RgaTransformKernel: imcheck rejected %s(%dx%d)→%s(%dx%d): %s  "
                    "→ libyuv fallback for this frame",
                    video_format_to_string(src.format), src_rect.width, src_rect.height,
                    video_format_to_string(dst_template_.format),
                    dst_rect.width, dst_rect.height,
                    imStrError(static_cast<IM_STATUS>(check)));
        return result;
    }

    // ------------------------------------------------------------------
    // Execute — restrict to RGA3 cores only.
    // RGA2-Enhance has 32-bit IOMMU (max 4GB), which causes kernel panic
    // when accessing buffers allocated above 4GB physical address.
    // RGA3 cores have 40-bit IOMMU, safe for all memory addresses.
    // ------------------------------------------------------------------
    im_opt_t opt{};
    opt.core = IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1;

    int ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {},
                        0, nullptr, &opt, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        GST_WARNING("RgaTransformKernel: improcess failed: %d - %s  "
                    "→ libyuv fallback for this frame",
                    ret, imStrError(static_cast<IM_STATUS>(ret)));
        return result;
    }

    // ------------------------------------------------------------------
    // Build result
    // ------------------------------------------------------------------
    result.success = true;
    if (ops_.keep_aspect_ratio) {
        result.content_rect = { dst_x, dst_y, dst_w, dst_h, true };
    }
    return result;
}

// ---------------------------------------------------------------------------
// transform — main entry point
// ---------------------------------------------------------------------------

TransformResult RgaTransformKernel::transform(const FrameDesc&  src,
                                               FrameDesc&        dst,
                                               int               slot_id,
                                               const DynamicOps* dynamic) {
    TransformResult result;
    result.success = false;

    if (!initialized_) {
        GST_ERROR("RgaTransformKernel: transform called before init()");
        return result;
    }

    // Validate dst pointer
    if (dst.planes[0].data == nullptr) {
        GST_ERROR("RgaTransformKernel: dst data pointer is null");
        return result;
    }

    // ------------------------------------------------------------------
    // Determine effective crop (source region to read)
    // ------------------------------------------------------------------
    CropRect crop = effective_crop(src, dynamic);
    int src_region_w = crop.enabled ? crop.w : src.width;
    int src_region_h = crop.enabled ? crop.h : src.height;

    // ------------------------------------------------------------------
    // Compute dst placement (full-fill or letterbox)
    // ------------------------------------------------------------------
    int dst_x, dst_y, dst_w, dst_h;
    compute_dst_rect(src_region_w, src_region_h, dst_x, dst_y, dst_w, dst_h);

    // ------------------------------------------------------------------
    // Fill padding region if letterbox is active
    // (must be BEFORE rga_execute — padding paints the background,
    //  then RGA writes content on top within dst_rect)
    // ------------------------------------------------------------------
    fill_padding(dst);

    // ------------------------------------------------------------------
    // Attempt RGA execution
    // ------------------------------------------------------------------
    result = rga_execute(src, dst, crop, dst_x, dst_y, dst_w, dst_h);

    // If rga_execute failed (resolution/scale/imcheck/improcess),
    // delegate this frame to libyuv — no frame drop.
    // (libyuv handles its own padding internally, so the double fill is harmless)
    if (!result.success) {
        return libyuv_fallback_->transform(src, dst, slot_id, dynamic);
    }

    return result;
}
}  // namespace dxt

#endif  // HAVE_LIBRGA
