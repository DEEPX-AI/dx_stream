#ifdef HAVE_LIBRGA

#include "rga_transform_kernel.hpp"
#include "gst_frame_desc.hpp"   // rga_hstride()

#include <cstdio>   // required by im2d.h (uses printf internally)
#include <rga/rga.h>
#include <rga/im2d.h>
#include <gst/gst.h>

#include <algorithm>
#include <cstring>

#define GST_CAT_DEFAULT dxt_rga_debug
GST_DEBUG_CATEGORY_STATIC(dxt_rga_debug);

namespace dxt {

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
        .src_formats      = { VideoFormat::NV12 },
        .dst_formats      = { VideoFormat::RGB, VideoFormat::BGR },
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

    // RGA NV12 → RGB/BGR: dst width must be 16-aligned, height 2-aligned
    if (dst_template.width % 16 != 0 || dst_template.height % 2 != 0) {
        GST_ERROR("RgaTransformKernel: dst dimensions (%dx%d) must be "
                  "width%%16==0 and height%%2==0",
                  dst_template.width, dst_template.height);
        return false;
    }

    // Only NV12 src supported
    // (src format is validated per-frame; we check dst here)
    if (dst_template.format != VideoFormat::RGB &&
        dst_template.format != VideoFormat::BGR) {
        GST_ERROR("RgaTransformKernel: unsupported dst format (use RGB or BGR)");
        return false;
    }

    if (!TransformKernelBase::init(dst_template, ops)) {
        return false;
    }

    GST_DEBUG("RgaTransformKernel: init OK  dst=%dx%d  fmt=%s  keep_ratio=%d",
              dst_template_.width, dst_template_.height,
              video_format_to_string(dst_template_.format),
              ops_.keep_aspect_ratio);
    return true;
}

// ---------------------------------------------------------------------------
// effective_crop
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

    // RGA requires even-aligned coordinates for NV12
    int x = (cr->x % 2 == 0) ? cr->x : cr->x + 1;
    int y = (cr->y % 2 == 0) ? cr->y : cr->y + 1;
    int w = (cr->w % 2 == 0) ? cr->w : cr->w + 1;
    int h = (cr->h % 2 == 0) ? cr->h : cr->h + 1;

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

    int pad_x = (out_w - new_w) / 2;
    int pad_y = (out_h - new_h) / 2;

    dst_x = pad_x;
    dst_y = pad_y;
    dst_w = new_w;
    dst_h = new_h;
}

// ---------------------------------------------------------------------------
// transform
// ---------------------------------------------------------------------------

TransformResult RgaTransformKernel::transform(const FrameDesc&  src,
                                               FrameDesc&        dst,
                                               int               /*slot_id*/,
                                               const DynamicOps* dynamic) {
    TransformResult result;
    result.success = false;

    if (!initialized_) {
        GST_ERROR("RgaTransformKernel: transform called before init()");
        return result;
    }

    // Validate src format
    if (src.format != VideoFormat::NV12) {
        GST_ERROR("RgaTransformKernel: src format must be NV12");
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
    // RGA only writes to dst_rect; pixels outside are not touched.
    // ------------------------------------------------------------------
    if (ops_.keep_aspect_ratio && ops_.padding.enabled) {
        int out_w = dst_template_.width;
        int out_h = dst_template_.height;
        // Fill entire dst with pad color before RGA writes the content
        for (int row = 0; row < out_h; ++row) {
            uint8_t* line = dst.planes[0].data + row * dst.planes[0].stride;
            for (int col = 0; col < out_w; ++col) {
                line[col * 3 + 0] = ops_.padding.pad_r;
                line[col * 3 + 1] = ops_.padding.pad_g;
                line[col * 3 + 2] = ops_.padding.pad_b;
            }
        }
    }

    // ------------------------------------------------------------------
    // Build RGA src buffer descriptor
    // ------------------------------------------------------------------
    int hstride_val    = rga_hstride(src.height);
    int src_wstride    = src.planes[0].stride;  // byte stride = pixel stride for NV12 Y

    rga_buffer_t src_img;
    if (src.memory_type == MemoryType::DMA_BUF && src.dma_fd >= 0) {
        src_img = wrapbuffer_fd(
            src.dma_fd,
            src.width, src.height,
            RK_FORMAT_YCbCr_420_SP,
            src_wstride, hstride_val);
        GST_DEBUG("RgaTransformKernel: using DMA-buf fd=%d  wstride=%d  hstride=%d",
                  src.dma_fd, src_wstride, hstride_val);
    } else {
        if (src.planes[0].data == nullptr) {
            GST_ERROR("RgaTransformKernel: CPU_VIRTUAL src has null data pointer");
            return result;
        }
        src_img = wrapbuffer_virtualaddr(
            static_cast<void*>(src.planes[0].data),
            src.width, src.height,
            RK_FORMAT_YCbCr_420_SP,
            src_wstride, hstride_val);
        GST_DEBUG("RgaTransformKernel: using virtual addr  wstride=%d  hstride=%d",
                  src_wstride, hstride_val);
    }

    // ------------------------------------------------------------------
    // Build RGA dst buffer descriptor
    // ------------------------------------------------------------------
    RgaSURF_FORMAT dst_fmt_rga;
    switch (dst_template_.format) {
        case VideoFormat::RGB: dst_fmt_rga = RK_FORMAT_RGB_888;  break;
        case VideoFormat::BGR: dst_fmt_rga = RK_FORMAT_BGR_888;  break;
        default:
            GST_ERROR("RgaTransformKernel: unsupported dst format in transform()");
            return result;
    }

    rga_buffer_t dst_img = wrapbuffer_virtualaddr(
        static_cast<void*>(dst.planes[0].data),
        dst_template_.width, dst_template_.height,
        dst_fmt_rga);

    // ------------------------------------------------------------------
    // Build im_rect for src and dst
    // ------------------------------------------------------------------
    im_rect src_rect{};
    src_rect.x      = crop.enabled ? crop.x : 0;
    src_rect.y      = crop.enabled ? crop.y : 0;
    src_rect.width  = src_region_w;
    src_rect.height = src_region_h;

    im_rect dst_rect{};
    dst_rect.x      = dst_x;
    dst_rect.y      = dst_y;
    dst_rect.width  = dst_w;
    dst_rect.height = dst_h;

    // ------------------------------------------------------------------
    // Resolution / scale-ratio guards
    // ------------------------------------------------------------------
    if (src_rect.width  < 68 || src_rect.height < 2  ||
        src_rect.width  > 8176 || src_rect.height > 8176) {
        GST_WARNING("RgaTransformKernel: src resolution out of range [68x2 ~ 8176x8176]");
        return result;
    }
    if (dst_rect.width  < 68 || dst_rect.height < 2  ||
        dst_rect.width  > 8128 || dst_rect.height > 8128) {
        GST_WARNING("RgaTransformKernel: dst resolution out of range [68x2 ~ 8128x8128]");
        return result;
    }

    float scale_w = static_cast<float>(dst_rect.width)  / src_rect.width;
    float scale_h = static_cast<float>(dst_rect.height) / src_rect.height;
    if (scale_w < 0.125f || scale_w > 8.0f ||
        scale_h < 0.125f || scale_h > 8.0f) {
        GST_WARNING("RgaTransformKernel: scale ratio out of [1/8, 8] range "
                    "(scale_w=%.3f  scale_h=%.3f)", scale_w, scale_h);
        return result;
    }

    // ------------------------------------------------------------------
    // Scheduler and validity check
    // ------------------------------------------------------------------
    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);

    int check = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (check != IM_STATUS_NOERROR) {
        GST_ERROR("RgaTransformKernel: imcheck failed: %d - %s",
                  check, imStrError(static_cast<IM_STATUS>(check)));
        return result;
    }

    // ------------------------------------------------------------------
    // Execute
    // ------------------------------------------------------------------
    int ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        GST_ERROR("RgaTransformKernel: improcess failed: %d - %s",
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

}  // namespace dxt

#endif  // HAVE_LIBRGA
