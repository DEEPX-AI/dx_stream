#ifdef HAVE_DXVNPU

#include "vnpu_transform_kernel.hpp"
#include <gst/gst.h>
#include <cstring>
#include <algorithm>

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

namespace dxt {

// ---------------------------------------------------------------------------
// Format mapping
// ---------------------------------------------------------------------------

dxvnpu::ColorFormat VnpuTransformKernel::to_dxvnpu_format(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::NV12: return dxvnpu::COLOR_FORMAT_YUV420SP;
        case VideoFormat::RGB:  return dxvnpu::COLOR_FORMAT_RGB888;
        case VideoFormat::BGR:  return dxvnpu::COLOR_FORMAT_BGR888;
        default:                return dxvnpu::COLOR_FORMAT_UNKNOWN;
    }
}

VideoFormat VnpuTransformKernel::from_dxvnpu_format(dxvnpu::ColorFormat fmt) {
    switch (fmt) {
        case dxvnpu::COLOR_FORMAT_YUV420SP: return VideoFormat::NV12;
        case dxvnpu::COLOR_FORMAT_RGB888:   return VideoFormat::RGB;
        case dxvnpu::COLOR_FORMAT_BGR888:   return VideoFormat::BGR;
        default:                            return VideoFormat::NV12;
    }
}

// ---------------------------------------------------------------------------
// capabilities
// ---------------------------------------------------------------------------

BackendCaps VnpuTransformKernel::capabilities() const {
    BackendCaps caps;
    caps.name                       = "vnpu";
    caps.hw_accelerated             = true;
    caps.supports_dma_buf           = false;
    caps.supports_dynamic_input_size = false;  // VNPU VP is fixed after init
    caps.max_width                  = 8192;
    caps.max_height                 = 8192;
    caps.src_formats = { VideoFormat::NV12, VideoFormat::RGB, VideoFormat::BGR };
    caps.dst_formats = { VideoFormat::NV12, VideoFormat::RGB, VideoFormat::BGR };
    return caps;
}

// ---------------------------------------------------------------------------
// init — store output config only; VideoProcessor created lazily
// ---------------------------------------------------------------------------

bool VnpuTransformKernel::init(const FrameDesc& dst_template,
                                const TransformOps& ops) {
    if (dxvnpu::GetDeviceCount() <= 0) {
        GST_DEBUG("VnpuTransformKernel: no VNPU device available, skipping");
        return false;
    }

    // Validate dst format is supported
    auto dxvnpu_dst = to_dxvnpu_format(dst_template.format);
    if (dxvnpu_dst == dxvnpu::COLOR_FORMAT_UNKNOWN) {
        GST_WARNING("VnpuTransformKernel: unsupported dst format");
        return false;
    }

    // VNPU internal VPSS uses RGA which requires aligned dimensions.
    // Non-aligned widths can cause device crash (unrecoverable).
    if (dst_template.width % 2 != 0 || dst_template.height % 2 != 0) {
        GST_DEBUG("VnpuTransformKernel: odd dimensions %dx%d not supported",
                  dst_template.width, dst_template.height);
        return false;
    }

    processor_.reset();
    return TransformKernelBase::init(dst_template, ops);
}

// ---------------------------------------------------------------------------
// transform
// ---------------------------------------------------------------------------

TransformResult VnpuTransformKernel::transform(const FrameDesc&  src,
                                                FrameDesc&        dst,
                                                int               slot_id,
                                                const DynamicOps* dynamic) {
    (void)slot_id;
    TransformResult result;

    // Validate src format
    auto dxvnpu_src_fmt = to_dxvnpu_format(src.format);
    if (dxvnpu_src_fmt == dxvnpu::COLOR_FORMAT_UNKNOWN) {
        GST_ERROR("VnpuTransformKernel: unsupported src format");
        return result;
    }

    // Reject odd src dimensions (VPSS/RGA requires even for YUV)
    if (src.width % 2 != 0 || src.height % 2 != 0) {
        GST_ERROR("VnpuTransformKernel: odd src dimensions %dx%d not supported",
                  src.width, src.height);
        return result;
    }

    // Lazy init: create VideoProcessor on first call
    if (!processor_) {
        input_width_  = src.width;
        input_height_ = src.height;
        input_format_ = src.format;

        dxvnpu::ProcessorConfig cfg;
        cfg.input_width   = src.width;
        cfg.input_height  = src.height;
        cfg.input_format  = dxvnpu_src_fmt;
        cfg.input_fps     = -1;
        cfg.output_width  = dst_template_.width;
        cfg.output_height = dst_template_.height;
        cfg.output_format = to_dxvnpu_format(dst_template_.format);
        cfg.output_fps    = -1;
        cfg.keep_aspect_ratio = ops_.keep_aspect_ratio;

        try {
            processor_ = std::make_unique<dxvnpu::VideoProcessor>(cfg);
        } catch (const std::exception& e) {
            GST_ERROR("VnpuTransformKernel: failed to create VideoProcessor: %s",
                      e.what());
            return result;
        }

        if (!processor_->IsStarted()) {
            GST_ERROR("VnpuTransformKernel: VideoProcessor failed to start");
            processor_.reset();
            return result;
        }

        GST_INFO("VnpuTransformKernel: created VideoProcessor %dx%d → %dx%d",
                 src.width, src.height, dst_template_.width, dst_template_.height);
    }

    // Validate input matches the fixed config
    if (src.width != input_width_ || src.height != input_height_ ||
        src.format != input_format_) {
        GST_ERROR("VnpuTransformKernel: input changed (%dx%d %d) → (%dx%d %d), "
                  "not supported after init",
                  input_width_, input_height_, static_cast<int>(input_format_),
                  src.width, src.height, static_cast<int>(src.format));
        return result;
    }

    // Build input DXFrame — copy from FrameDesc planes respecting strides
    auto input_frame = std::make_shared<dxvnpu::DXFrame>(
        src.width, src.height, to_dxvnpu_format(src.format));
    int in_wstride = input_frame->width_stride;  // initially == width
    int in_hstride = input_frame->height_stride;  // initially == height

    if (src.format == VideoFormat::NV12) {
        size_t alloc_size = static_cast<size_t>(in_wstride) * in_hstride * 3 / 2;
        input_frame->allocateData(alloc_size);
        uint8_t* frame_data = input_frame->getRawData();
        // Y plane
        for (int row = 0; row < src.height; ++row) {
            std::memcpy(frame_data + row * in_wstride,
                        src.planes[0].data + row * src.planes[0].stride,
                        src.width);
        }
        // UV plane
        uint8_t* uv_dst = frame_data + static_cast<size_t>(in_wstride) * in_hstride;
        int uv_h = src.height / 2;
        for (int row = 0; row < uv_h; ++row) {
            std::memcpy(uv_dst + row * in_wstride,
                        src.planes[1].data + row * src.planes[1].stride,
                        src.width);
        }
    } else {
        // RGB/BGR — single plane
        int bpp = bytes_per_pixel(src.format);
        int row_bytes = src.width * bpp;
        size_t alloc_size = static_cast<size_t>(in_wstride) * in_hstride * bpp;
        input_frame->allocateData(alloc_size);
        uint8_t* frame_data = input_frame->getRawData();
        for (int row = 0; row < src.height; ++row) {
            std::memcpy(frame_data + row * in_wstride * bpp,
                        src.planes[0].data + row * src.planes[0].stride,
                        row_bytes);
        }
    }

    // Process
    auto output_frame = processor_->Process(input_frame);
    if (!output_frame) {
        GST_ERROR("VnpuTransformKernel: Process() returned null");
        return result;
    }

    // Copy output to dst FrameDesc using output stride
    const uint8_t* out_data = output_frame->getRawData();
    int out_w = dst_template_.width;
    int out_h = dst_template_.height;
    int out_wstride = output_frame->width_stride;
    int out_hstride = output_frame->height_stride;

    GST_DEBUG("VnpuTransformKernel: output %dx%d stride %dx%d",
              out_w, out_h, out_wstride, out_hstride);

    if (dst.format == VideoFormat::NV12) {
        // Y plane
        for (int row = 0; row < out_h; ++row) {
            std::memcpy(dst.planes[0].data + row * dst.planes[0].stride,
                        out_data + row * out_wstride,
                        out_w);
        }
        // UV plane
        const uint8_t* uv_src = out_data + static_cast<size_t>(out_wstride) * out_hstride;
        int uv_h = out_h / 2;
        for (int row = 0; row < uv_h; ++row) {
            std::memcpy(dst.planes[1].data + row * dst.planes[1].stride,
                        uv_src + row * out_wstride,
                        out_w);
        }
    } else {
        // RGB/BGR
        int bpp = bytes_per_pixel(dst.format);
        int row_bytes = out_w * bpp;
        int out_row_stride = out_wstride * bpp;
        for (int row = 0; row < out_h; ++row) {
            std::memcpy(dst.planes[0].data + row * dst.planes[0].stride,
                        out_data + row * out_row_stride,
                        row_bytes);
        }
    }

    // Content rect for letterbox (VNPU handles this internally when
    // keep_aspect_ratio is set, but we report the full output rect here;
    // exact sub-rect would require VNPU API extension)
    result.success = true;
    if (ops_.keep_aspect_ratio) {
        compute_dst_rect(src.width, src.height,
                         result.content_rect.x, result.content_rect.y,
                         result.content_rect.w, result.content_rect.h);
        result.content_rect.valid = true;
    }

    return result;
}

}  // namespace dxt

#endif  // HAVE_DXVNPU
