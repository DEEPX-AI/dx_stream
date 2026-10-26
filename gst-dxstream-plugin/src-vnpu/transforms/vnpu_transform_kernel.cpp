#include "vnpu_transform_kernel.hpp"
#include <gst/gst.h>
#include <algorithm>
#include <cstring>
#include <vector>

#define GST_CAT_DEFAULT vnpu_transform_kernel_cat
GST_DEBUG_CATEGORY_STATIC(vnpu_transform_kernel_cat);

constexpr int kVnpuInputTimeoutMs = 100;
constexpr gint64 kVnpuOutputTimeoutUs = 3 * G_USEC_PER_SEC;

static void ensure_vnpu_transform_debug_category() {
    static gsize initialized = 0;
    if (g_once_init_enter(&initialized)) {
        GST_DEBUG_CATEGORY_INIT(vnpu_transform_kernel_cat, "vnpu-transform-kernel", 0,
                                "DXVNPU transform kernel");
        g_once_init_leave(&initialized, 1);
    }
}

namespace dxt {

VnpuTransformKernel::~VnpuTransformKernel() {
    if (processor_) {
        dxvnpu_pipeline_destroy(&processor_);
    }
}

dxvnpu_color_format_t VnpuTransformKernel::to_dxvnpu_format(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::NV12: return DXVNPU_COLOR_YUV420SP;
        case VideoFormat::RGB:  return DXVNPU_COLOR_RGB888;
        case VideoFormat::BGR:  return DXVNPU_COLOR_BGR888;
        default:                return DXVNPU_COLOR_UNKNOWN;
    }
}

VideoFormat VnpuTransformKernel::from_dxvnpu_format(dxvnpu_color_format_t fmt) {
    switch (fmt) {
        case DXVNPU_COLOR_YUV420SP: return VideoFormat::NV12;
        case DXVNPU_COLOR_RGB888:   return VideoFormat::RGB;
        case DXVNPU_COLOR_BGR888:   return VideoFormat::BGR;
        default:                    return VideoFormat::NV12;
    }
}

BackendCaps VnpuTransformKernel::capabilities() const {
    BackendCaps caps;
    caps.name = "vnpu";
    caps.hw_accelerated = true;
    caps.supports_dma_buf = false;
    caps.supports_dynamic_input_size = false;
    caps.max_width = 8192;
    caps.max_height = 8192;
    caps.src_formats = { VideoFormat::NV12, VideoFormat::RGB, VideoFormat::BGR };
    caps.dst_formats = { VideoFormat::NV12, VideoFormat::RGB, VideoFormat::BGR };
    return caps;
}

bool VnpuTransformKernel::init(const FrameDesc& dst_template,
                               const TransformOps& ops) {
    ensure_vnpu_transform_debug_category();
    uint64_t device_count = 0;
    if (dxvnpu_get_device_count(&device_count) != DXVNPU_OK || device_count == 0) {
        GST_DEBUG("VnpuTransformKernel: no VNPU device available, skipping");
        return false;
    }

    auto dxvnpu_dst = to_dxvnpu_format(dst_template.format);
    if (dxvnpu_dst == DXVNPU_COLOR_UNKNOWN) {
        GST_WARNING("VnpuTransformKernel: unsupported dst format");
        return false;
    }

    if (dst_template.width % 2 != 0 || dst_template.height % 2 != 0) {
        GST_DEBUG("VnpuTransformKernel: odd dimensions %dx%d not supported",
                  dst_template.width, dst_template.height);
        return false;
    }

    if (processor_) {
        dxvnpu_pipeline_destroy(&processor_);
    }
    return TransformKernelBase::init(dst_template, ops);
}

TransformResult VnpuTransformKernel::transform(const FrameDesc& src,
                                               FrameDesc& dst,
                                               int slot_id,
                                               const DynamicOps* dynamic) {
    (void)slot_id;
    TransformResult result;

    auto dxvnpu_src_fmt = to_dxvnpu_format(src.format);
    if (dxvnpu_src_fmt == DXVNPU_COLOR_UNKNOWN) {
        GST_ERROR("VnpuTransformKernel: unsupported src format");
        return result;
    }

    if (src.width % 2 != 0 || src.height % 2 != 0) {
        GST_ERROR("VnpuTransformKernel: odd src dimensions %dx%d not supported",
                  src.width, src.height);
        return result;
    }

    const CropRect crop = effective_crop(src, dynamic);
    const bool has_dynamic_crop = dynamic && dynamic->crop_override &&
                                  dynamic->crop_override->enabled;
    const int source_width = crop.enabled ? crop.w : src.width;
    const int source_height = crop.enabled ? crop.h : src.height;
    if (ops_.keep_aspect_ratio) {
        compute_dst_rect(source_width, source_height,
                         result.content_rect.x, result.content_rect.y,
                         result.content_rect.w, result.content_rect.h);
        result.content_rect.valid = true;
    } else {
        result.content_rect = {0, 0, dst_template_.width, dst_template_.height, true};
    }

    const int content_w = result.content_rect.w;
    const int content_h = result.content_rect.h;

    if (source_width < 68 || source_height < 64 ||
        source_width > 8176 || source_height > 8176 ||
        content_w < 68 || content_h < 64 ||
        content_w > 8128 || content_h > 8128 ||
        content_w > source_width * 8 || source_width > content_w * 8 ||
        content_h > source_height * 8 || source_height > content_h * 8) {
        GST_DEBUG("VnpuTransformKernel: resize %dx%d -> %dx%d is outside VNPU limits",
                  source_width, source_height, content_w, content_h);
        return result;
    }

    if (!processor_) {
        input_width_ = src.width;
        input_height_ = src.height;
        input_format_ = src.format;
        dynamic_crop_ = has_dynamic_crop;

        dxvnpu_config_t cfg = nullptr;
        dxvnpu_status_t status = dxvnpu_processor_config_create(&cfg);
        if (status == DXVNPU_OK) {
            status = dxvnpu_processor_config_set_input_format(
                cfg, src.width, src.height, dxvnpu_src_fmt);
        }
        if (status == DXVNPU_OK) {
            status = dxvnpu_processor_config_set_input_framerate(cfg, -1);
        }
        if (status == DXVNPU_OK) {
            status = dxvnpu_processor_config_set_output_format(
                cfg, dynamic_crop_ ? dst_template_.width : content_w,
                dynamic_crop_ ? dst_template_.height : content_h,
                to_dxvnpu_format(dst_template_.format));
        }
        if (status == DXVNPU_OK) {
            status = dxvnpu_processor_config_set_output_framerate(cfg, -1);
        }
        if (status == DXVNPU_OK) {
            status = dxvnpu_processor_config_set_keep_aspect_ratio(
                cfg, dynamic_crop_ && ops_.keep_aspect_ratio ? 1 : 0);
        }
        if (status == DXVNPU_OK) {
            status = dxvnpu_pipeline_create(&processor_, cfg,
                                            DXVNPU_PIPELINE_FLAG_DIRECT_OUTPUT, -1);
        }
        if (cfg) {
            dxvnpu_config_destroy(&cfg);
        }
        if (status != DXVNPU_OK) {
            GST_ERROR("VnpuTransformKernel: failed to create processor: %s",
                      dxvnpu_status_string(status));
            return result;
        }

        GST_INFO("VnpuTransformKernel: created processor %dx%d -> %dx%d",
                 src.width, src.height, content_w, content_h);
    }

    if (src.width != input_width_ || src.height != input_height_ ||
        src.format != input_format_) {
        GST_ERROR("VnpuTransformKernel: input changed (%dx%d %d) -> (%dx%d %d), not supported after init",
                  input_width_, input_height_, static_cast<int>(input_format_),
                  src.width, src.height, static_cast<int>(src.format));
        return result;
    }
    if (dynamic_crop_ != has_dynamic_crop) {
        GST_ERROR("VnpuTransformKernel: dynamic crop mode changed after init");
        return result;
    }

    std::vector<uint8_t> input_data;
    if (src.format == VideoFormat::NV12) {
        int stride = src.width;
        int slice_height = src.height;
        size_t size = static_cast<size_t>(stride) * slice_height * 3 / 2;
        input_data.resize(size);
        uint8_t* frame_data = input_data.data();
        for (int row = 0; row < src.height; ++row) {
            std::memcpy(frame_data + row * stride,
                        src.planes[0].data + row * src.planes[0].stride,
                        src.width);
        }
        uint8_t* uv_dst = frame_data + static_cast<size_t>(stride) * slice_height;
        int uv_h = src.height / 2;
        for (int row = 0; row < uv_h; ++row) {
            std::memcpy(uv_dst + row * stride,
                        src.planes[1].data + row * src.planes[1].stride,
                        src.width);
        }
    } else {
        int bpp = bytes_per_pixel(src.format);
        int stride = src.width * bpp;
        size_t size = static_cast<size_t>(stride) * src.height;
        input_data.resize(size);
        uint8_t* frame_data = input_data.data();
        for (int row = 0; row < src.height; ++row) {
            std::memcpy(frame_data + row * stride,
                        src.planes[0].data + row * src.planes[0].stride,
                        stride);
        }
    }
    const uint32_t input_stride = static_cast<uint32_t>(src.width);
    dxvnpu_frame_info_t input_info = {
        static_cast<uint32_t>(src.width),
        static_cast<uint32_t>(src.height),
        input_stride,
        static_cast<uint32_t>(src.height),
        dxvnpu_src_fmt
    };
    dxvnpu_buffer_t input = nullptr;
    dxvnpu_status_t status = dxvnpu_pipeline_acquire_input_buffer(
        processor_, &input, kVnpuInputTimeoutMs);
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_data(input, input_data.data(), input_data.size());
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_frame_info(input, &input_info);
    }
    if (status == DXVNPU_OK && crop.enabled) {
        const dxvnpu_crop_t sdk_crop = {
            1, 0,
            static_cast<float>(crop.x), static_cast<float>(crop.y),
            static_cast<float>(crop.w), static_cast<float>(crop.h),
        };
        status = dxvnpu_buffer_set_crop(input, &sdk_crop);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_pipeline_put_buffer(processor_, input,
                                            kVnpuInputTimeoutMs);
    }
    if (input) {
        dxvnpu_buffer_release(&input);
    }
    if (status != DXVNPU_OK) {
        GST_ERROR("VnpuTransformKernel: put failed: %s", dxvnpu_status_string(status));
        return result;
    }
    dxvnpu_buffer_t out = nullptr;
    const gint64 output_deadline = g_get_monotonic_time() + kVnpuOutputTimeoutUs;
    do {
        status = dxvnpu_pipeline_get_buffer(processor_, &out, 0);
        if (status == DXVNPU_TIMEOUT)
            g_usleep(1000);
    } while (status == DXVNPU_TIMEOUT &&
             g_get_monotonic_time() < output_deadline);
    if (status != DXVNPU_OK) {
        GST_ERROR("VnpuTransformKernel: get failed: %s", dxvnpu_status_string(status));
        dxvnpu_pipeline_destroy(&processor_);
        return result;
    }
    const void* data = nullptr;
    size_t data_size = 0;
    dxvnpu_frame_info_t output_info = {};
    if (dxvnpu_buffer_view(out, &data, &data_size) != DXVNPU_OK ||
        dxvnpu_buffer_get_frame_info(out, &output_info) != DXVNPU_OK) {
        GST_ERROR("VnpuTransformKernel: failed to read output buffer");
        dxvnpu_buffer_release(&out);
        return result;
    }

    const uint8_t* out_data = static_cast<const uint8_t*>(data);
    int out_w = dynamic_crop_ ? dst_template_.width : content_w;
    int out_h = dynamic_crop_ ? dst_template_.height : content_h;
    const int output_bpp = dst.format == VideoFormat::NV12 ? 1 : bytes_per_pixel(dst.format);
    int out_stride = output_info.stride > 0
                         ? static_cast<int>(output_info.stride) * output_bpp
                         : out_w * output_bpp;
    int out_slice_height = output_info.slice_height > 0
                               ? static_cast<int>(output_info.slice_height) : out_h;
    if (ops_.padding.enabled) {
        if (dst.format == VideoFormat::NV12) {
            for (int row = 0; row < dst_template_.height; ++row)
                std::memset(dst.planes[0].data + row * dst.planes[0].stride,
                            ops_.padding.pad_r, dst_template_.width);
            for (int row = 0; row < dst_template_.height / 2; ++row)
                std::memset(dst.planes[1].data + row * dst.planes[1].stride,
                            128, dst_template_.width);
        } else {
            const uint8_t first = dst.format == VideoFormat::BGR
                                      ? ops_.padding.pad_b : ops_.padding.pad_r;
            const uint8_t third = dst.format == VideoFormat::BGR
                                      ? ops_.padding.pad_r : ops_.padding.pad_b;
            for (int row = 0; row < dst_template_.height; ++row)
                for (int col = 0; col < dst_template_.width; ++col) {
                    uint8_t* pixel = dst.planes[0].data + row * dst.planes[0].stride + col * 3;
                    pixel[0] = first; pixel[1] = ops_.padding.pad_g; pixel[2] = third;
                }
        }
    }

    const int dst_x = result.content_rect.x;
    const int dst_y = result.content_rect.y;
    if (dst.format == VideoFormat::NV12) {
        for (int row = 0; row < out_h; ++row) {
            if (dynamic_crop_ && (row < dst_y || row >= dst_y + content_h))
                continue;
            const int copy_x = dynamic_crop_ ? dst_x : 0;
            const int copy_w = dynamic_crop_ ? content_w : out_w;
            std::memcpy(dst.planes[0].data + (row + (dynamic_crop_ ? 0 : dst_y)) *
                        dst.planes[0].stride + (dynamic_crop_ ? copy_x : dst_x),
                        out_data + row * out_stride + copy_x, copy_w);
        }
        const uint8_t* uv_src = out_data + static_cast<size_t>(out_stride) * out_slice_height;
        int uv_h = out_h / 2;
        for (int row = 0; row < uv_h; ++row) {
            if (dynamic_crop_ && (row < dst_y / 2 || row >= (dst_y + content_h) / 2))
                continue;
            const int copy_x = dynamic_crop_ ? dst_x : 0;
            const int copy_w = dynamic_crop_ ? content_w : out_w;
            std::memcpy(dst.planes[1].data + (row + (dynamic_crop_ ? 0 : dst_y / 2)) *
                        dst.planes[1].stride + (dynamic_crop_ ? copy_x : dst_x),
                        uv_src + row * out_stride + copy_x, copy_w);
        }
    } else {
        int bpp = bytes_per_pixel(dst.format);
        int row_bytes = (dynamic_crop_ ? content_w : out_w) * bpp;
        for (int row = 0; row < out_h; ++row) {
            if (dynamic_crop_ && (row < dst_y || row >= dst_y + content_h))
                continue;
            std::memcpy(dst.planes[0].data + (row + (dynamic_crop_ ? 0 : dst_y)) *
                        dst.planes[0].stride + (dynamic_crop_ ? dst_x : 0) * bpp,
                        out_data + row * out_stride + (dynamic_crop_ ? dst_x : 0) * bpp,
                        row_bytes);
        }
    }

    dxvnpu_buffer_release(&out);
    result.success = true;
    return result;
}

}
