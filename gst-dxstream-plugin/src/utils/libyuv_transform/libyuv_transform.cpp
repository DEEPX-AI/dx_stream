#include "libyuv_transform.hpp"
#include "./../../metadata/gst-dxframemeta.hpp"
#include "./../../metadata/gst-dxobjectmeta.hpp"

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

// ============================================================================
// Helper functions
// ============================================================================

static inline void handle_unsupported_format(const char *func_name) {
    GST_ERROR("%s: Not supported color format\n", func_name);
}

static inline bool is_rgb(const gchar *fmt) { 
    return g_strcmp0(fmt, "RGB") == 0; 
}

static inline bool is_bgr(const gchar *fmt) { 
    return g_strcmp0(fmt, "BGR") == 0; 
}

static inline void handle_conversion_error(int result, const char *tag) {
    if (result != 0) {
        GST_ERROR("%s: Failed to convert color\n", tag);
    }
}

static void copy_rgb_with_stride(const uint8_t *src, int stride, uint8_t *dst, 
                                   int width, int height) {
    for (int i = 0; i < height; ++i) {
        memcpy(dst + i * width * 3, src + i * stride, width * 3);
    }
}

static int convert_rgb_from_buffer(const uint8_t *src, int stride, uint8_t *dst,
                                     int width, int height, const gchar *dst_format) {
    if (is_rgb(dst_format)) {
        if (stride == width * 3) {
            memcpy(dst, src, width * height * 3);
        } else {
            copy_rgb_with_stride(src, stride, dst, width, height);
        }
        return 0;
    } else if (is_bgr(dst_format)) {
        return libyuv::RAWToRGB24(src, stride, dst, width * 3, width, height);
    }
    return -1;
}

static int convert_i420_from_buffer(const uint8_t *src_y, int strideY,
                                      const uint8_t *src_u, int strideU,
                                      const uint8_t *src_v, int strideV,
                                      uint8_t *dst, int width, int height,
                                      const gchar *dst_format) {
    if (is_rgb(dst_format)) {
        return libyuv::I420ToRAW(src_y, strideY, src_u, strideU, src_v, 
                                 strideV, dst, width * 3, width, height);
    } else if (is_bgr(dst_format)) {
        return libyuv::I420ToRGB24(src_y, strideY, src_u, strideU, src_v,
                                   strideV, dst, width * 3, width, height);
    }
    return -1;
}

static int convert_nv12_from_buffer(const uint8_t *src_y, int strideY,
                                      const uint8_t *src_uv, int strideUV,
                                      uint8_t *dst, int width, int height,
                                      const gchar *dst_format) {
    if (is_rgb(dst_format)) {
        return libyuv::NV12ToRAW(src_y, strideY, src_uv, strideUV, dst,
                                 width * 3, width, height);
    } else if (is_bgr(dst_format)) {
        return libyuv::NV12ToRGB24(src_y, strideY, src_uv, strideUV, dst,
                                   width * 3, width, height);
    }
    return -1;
}

// ============================================================================
// Internal crop implementations
// ============================================================================

static int I420Crop(const uint8_t *src_y, int src_stride_y, const uint8_t *src_u,
                    int src_stride_u, const uint8_t *src_v, int src_stride_v,
                    int src_width, int src_height, uint8_t *dst, int crop_width,
                    int crop_height, int crop_x, int crop_y) {
    if (crop_x < 0 || crop_y < 0 || crop_x + crop_width > src_width ||
        crop_y + crop_height > src_height) {
        return -1;
    }

    uint8_t *dst_y = dst;
    uint8_t *dst_u = dst + crop_width * crop_height;
    uint8_t *dst_v = dst_u + (crop_width / 2) * (crop_height / 2);

    const uint8_t *cropped_y = src_y + crop_y * src_stride_y + crop_x;
    const uint8_t *cropped_u = src_u + (crop_y / 2) * src_stride_u + (crop_x / 2);
    const uint8_t *cropped_v = src_v + (crop_y / 2) * src_stride_v + (crop_x / 2);

    return libyuv::I420Copy(cropped_y, src_stride_y, cropped_u, src_stride_u,
                            cropped_v, src_stride_v, dst_y, crop_width, dst_u,
                            crop_width / 2, dst_v, crop_width / 2, crop_width,
                            crop_height);
}

static int NV12Crop(const uint8_t *src_y, int src_stride_y, const uint8_t *src_uv,
                    int src_stride_uv, int src_width, int src_height, uint8_t *dst,
                    int crop_width, int crop_height, int crop_x, int crop_y) {
    if (crop_x < 0 || crop_y < 0 || crop_x + crop_width > src_width ||
        crop_y + crop_height > src_height) {
        return -1;
    }

    uint8_t *dst_y = dst;
    uint8_t *dst_uv = dst + crop_width * crop_height;

    const uint8_t *cropped_y = src_y + crop_y * src_stride_y + crop_x;
    for (int i = 0; i < crop_height; ++i) {
        memcpy(dst_y + i * crop_width, cropped_y + i * src_stride_y, crop_width);
    }

    const uint8_t *cropped_uv = src_uv + (crop_y / 2) * src_stride_uv + (crop_x / 2) * 2;
    for (int i = 0; i < crop_height / 2; ++i) {
        memcpy(dst_uv + i * crop_width, cropped_uv + i * src_stride_uv, crop_width);
    }
    return 0;
}

static int RGBCrop(const uint8_t *src, int src_stride, int src_width, int src_height,
                   uint8_t *dst, int crop_width, int crop_height, int crop_x, int crop_y) {
    if (crop_x < 0 || crop_y < 0 || crop_x + crop_width > src_width ||
        crop_y + crop_height > src_height) {
        return -1;
    }

    const uint8_t *cropped_src = src + crop_y * src_stride + crop_x * 3;
    for (int i = 0; i < crop_height; ++i) {
        memcpy(dst + i * crop_width * 3, cropped_src + i * src_stride, crop_width * 3);
    }
    return 0;
}

// ============================================================================
// RAII-based public API - Crop
// ============================================================================

void Crop(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
          int src_width, int src_height, int crop_x, int crop_y, 
          int crop_width, int crop_height, const gchar *format) {
    
    if (crop_width <= 0 || crop_height <= 0) {
        GST_ERROR("Crop: Invalid crop dimensions\n");
        return;
    }

    size_t required_size = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        required_size = crop_height * crop_width * 3;
    } else if (g_strcmp0(format, "I420") == 0 || g_strcmp0(format, "NV12") == 0) {
        required_size = crop_width * crop_height * 3 / 2;
    } else {
        GST_ERROR("Crop: Not supported color format\n");
        return;
    }
    
    dst.resize(required_size);

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_ERROR("Crop: Failed to map GstBuffer\n");
        return;
    }

    int result = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        const uint8_t *src = map.data;
        gint stride = src_width * 3;
        if (input_info) {
            src = map.data + input_info->offset[0];
            stride = input_info->stride[0];
        }
        result = RGBCrop(src, stride, src_width, src_height, dst.data(), 
                        crop_width, crop_height, crop_x, crop_y);
    } else if (g_strcmp0(format, "I420") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);
        gint strideY = src_width;
        gint strideU = src_width / 2;
        gint strideV = src_width / 2;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_u = map.data + input_info->offset[1];
            src_v = map.data + input_info->offset[2];
            strideY = input_info->stride[0];
            strideU = input_info->stride[1];
            strideV = input_info->stride[2];
        }
        result = I420Crop(src_y, strideY, src_u, strideU, src_v, strideV, 
                         src_width, src_height, dst.data(), crop_width, 
                         crop_height, crop_x, crop_y);
    } else if (g_strcmp0(format, "NV12") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + src_width * src_height;
        gint strideY = src_width;
        gint strideUV = src_width / 2;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_uv = map.data + input_info->offset[1];
            strideY = input_info->stride[0];
            strideUV = input_info->stride[1];
        }
        result = NV12Crop(src_y, strideY, src_uv, strideUV, src_width, 
                         src_height, dst.data(), crop_width, crop_height, 
                         crop_x, crop_y);
    }

    if (result != 0) {
        GST_ERROR("Crop: Failed to crop frame\n");
    }
    gst_buffer_unmap(buf, &map);
}

// ============================================================================
// RAII-based public API - Resize
// ============================================================================

void Resize(std::vector<uint8_t>& src, std::vector<uint8_t>& dst, 
            int src_width, int src_height, int dst_width, int dst_height, 
            const gchar *format) {
    
    if (dst_width <= 0 || dst_height <= 0) {
        GST_ERROR("Resize: Invalid dimensions\n");
        return;
    }

    size_t required_size = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        required_size = dst_height * dst_width * 3;
    } else if (g_strcmp0(format, "I420") == 0 || g_strcmp0(format, "NV12") == 0) {
        required_size = dst_width * dst_height * 3 / 2;
    } else {
        GST_ERROR("Resize: Not supported color format\n");
        return;
    }
    
    dst.resize(required_size);

    int result = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        cv::Mat mat_src(src_height, src_width, CV_8UC3, src.data());
        cv::Mat mat_dst(dst_height, dst_width, CV_8UC3, dst.data());
        cv::resize(mat_src, mat_dst, cv::Size(dst_width, dst_height), 0, 0, cv::INTER_LINEAR);
    } else if (g_strcmp0(format, "I420") == 0) {
        uint8_t *dst_y = dst.data();
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);
        const uint8_t *src_y = src.data();
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);

        result = libyuv::I420Scale(
            src_y, src_width, src_u, src_width / 2, src_v, src_width / 2,
            src_width, src_height, dst_y, dst_width, dst_u, dst_width / 2,
            dst_v, dst_width / 2, dst_width, dst_height, libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "NV12") == 0) {
        uint8_t *dst_y = dst.data();
        uint8_t *dst_uv = dst_y + dst_width * dst_height;
        const uint8_t *src_y = src.data();
        const uint8_t *src_uv = src_y + src_width * src_height;

        result = libyuv::NV12Scale(
            src_y, src_width, src_uv, src_width, src_width, src_height, 
            dst_y, dst_width, dst_uv, dst_width, dst_width, dst_height, 
            libyuv::kFilterLinear);
    }

    if (result != 0) {
        GST_ERROR("Resize: Failed to resize frame\n");
    }
}

void Resize(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
            int src_width, int src_height, int dst_width, int dst_height, 
            const gchar *format) {
    
    if (dst_width <= 0 || dst_height <= 0) {
        GST_ERROR("Resize: Invalid dimensions\n");
        return;
    }

    size_t required_size = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        required_size = dst_height * dst_width * 3;
    } else if (g_strcmp0(format, "I420") == 0 || g_strcmp0(format, "NV12") == 0) {
        required_size = dst_width * dst_height * 3 / 2;
    } else {
        GST_ERROR("Resize: Not supported color format\n");
        return;
    }
    
    dst.resize(required_size);

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_ERROR("Resize: Failed to map GstBuffer\n");
        return;
    }

    int result = 0;
    if (g_strcmp0(format, "RGB") == 0) {
        uint8_t *src = map.data;
        if (input_info) {
            src = map.data + input_info->offset[0];
        }
        cv::Mat mat_src(src_height, src_width, CV_8UC3, src);
        cv::Mat mat_dst(dst_height, dst_width, CV_8UC3, dst.data());
        cv::resize(mat_src, mat_dst, cv::Size(dst_width, dst_height), 0, 0, cv::INTER_LINEAR);
    } else if (g_strcmp0(format, "I420") == 0) {
        uint8_t *dst_y = dst.data();
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);
        gint strideY = src_width;
        gint strideU = src_width / 2;
        gint strideV = src_width / 2;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_u = map.data + input_info->offset[1];
            src_v = map.data + input_info->offset[2];
            strideY = input_info->stride[0];
            strideU = input_info->stride[1];
            strideV = input_info->stride[2];
        }
        result = libyuv::I420Scale(
            src_y, strideY, src_u, strideU, src_v, strideV, src_width,
            src_height, dst_y, dst_width, dst_u, dst_width / 2, dst_v,
            dst_width / 2, dst_width, dst_height, libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "NV12") == 0) {
        uint8_t *dst_y = dst.data();
        uint8_t *dst_uv = dst_y + dst_width * dst_height;
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + src_width * src_height;
        gint strideY = src_width;
        gint strideUV = src_width;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_uv = map.data + input_info->offset[1];
            strideY = input_info->stride[0];
            strideUV = input_info->stride[1];
        }
        result = libyuv::NV12Scale(
            src_y, strideY, src_uv, strideUV, src_width, src_height, 
            dst_y, dst_width, dst_uv, dst_width, dst_width, dst_height, 
            libyuv::kFilterLinear);
    }

    if (result != 0) {
        GST_ERROR("Resize: Failed to resize frame\n");
    }
    gst_buffer_unmap(buf, &map);
}

// ============================================================================
// RAII-based public API - CvtColor
// ============================================================================

void CvtColor(std::vector<uint8_t>& src, std::vector<uint8_t>& dst, 
              int width, int height, const gchar *src_format, const gchar *dst_format) {
    
    if (width <= 0 || height <= 0) {
        GST_ERROR("CvtColor: Invalid dimensions\n");
        return;
    }

    dst.resize(width * height * 3);

    int result = 0;
    if (g_strcmp0(src_format, "RGB") == 0) {
        if (is_rgb(dst_format)) {
            memcpy(dst.data(), src.data(), width * height * 3);
        } else if (is_bgr(dst_format)) {
            result = libyuv::RAWToRGB24(src.data(), width * 3, dst.data(), 
                                       width * 3, width, height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else if (g_strcmp0(src_format, "I420") == 0) {
        const uint8_t *src_y = src.data();
        const uint8_t *src_u = src_y + width * height;
        const uint8_t *src_v = src_u + (width / 2) * (height / 2);

        if (is_rgb(dst_format)) {
            result = libyuv::I420ToRAW(src_y, width, src_u, width / 2, src_v,
                                      width / 2, dst.data(), width * 3, width, height);
        } else if (is_bgr(dst_format)) {
            result = libyuv::I420ToRGB24(src_y, width, src_u, width / 2, src_v,
                                        width / 2, dst.data(), width * 3, width, height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        const uint8_t *src_y = src.data();
        const uint8_t *src_uv = src_y + width * height;

        if (is_rgb(dst_format)) {
            result = libyuv::NV12ToRAW(src_y, width, src_uv, width, dst.data(),
                                      width * 3, width, height);
        } else if (is_bgr(dst_format)) {
            result = libyuv::NV12ToRGB24(src_y, width, src_uv, width, dst.data(),
                                        width * 3, width, height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else {
        handle_unsupported_format("CvtColor");
        return;
    }

    handle_conversion_error(result, "CvtColor");
}

void CvtColor(GstBuffer *buf, const GstVideoInfo *input_info, std::vector<uint8_t>& dst, 
              int width, int height, const gchar *src_format, const gchar *dst_format) {
    
    if (width <= 0 || height <= 0) {
        GST_ERROR("CvtColor: Invalid dimensions\n");
        return;
    }

    dst.resize(width * height * 3);

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_ERROR("CvtColor: Failed to map GstBuffer\n");
        return;
    }

    int result = 0;
    if (g_strcmp0(src_format, "RGB") == 0) {
        const uint8_t *src = map.data;
        int stride = width * 3;
        if (input_info) {
            src = map.data + input_info->offset[0];
            stride = input_info->stride[0];
        }
        result = convert_rgb_from_buffer(src, stride, dst.data(), width, height, dst_format);
        if (result == -1) {
            handle_unsupported_format("CvtColor");
            gst_buffer_unmap(buf, &map);
            return;
        }
    } else if (g_strcmp0(src_format, "I420") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + width * height;
        const uint8_t *src_v = src_u + (width / 2) * (height / 2);
        gint strideY = width;
        gint strideU = width / 2;
        gint strideV = width / 2;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_u = map.data + input_info->offset[1];
            src_v = map.data + input_info->offset[2];
            strideY = input_info->stride[0];
            strideU = input_info->stride[1];
            strideV = input_info->stride[2];
        }
        result = convert_i420_from_buffer(src_y, strideY, src_u, strideU, src_v,
                                           strideV, dst.data(), width, height, dst_format);
        if (result == -1) {
            handle_unsupported_format("CvtColor");
            gst_buffer_unmap(buf, &map);
            return;
        }
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + width * height;
        gint strideY = width;
        gint strideUV = width;
        if (input_info) {
            src_y = map.data + input_info->offset[0];
            src_uv = map.data + input_info->offset[1];
            strideY = input_info->stride[0];
            strideUV = input_info->stride[1];
        }
        result = convert_nv12_from_buffer(src_y, strideY, src_uv, strideUV,
                                           dst.data(), width, height, dst_format);
        if (result == -1) {
            handle_unsupported_format("CvtColor");
            gst_buffer_unmap(buf, &map);
            return;
        }
    } else {
        handle_unsupported_format("CvtColor");
        gst_buffer_unmap(buf, &map);
        return;
    }

    handle_conversion_error(result, "CvtColor");
    gst_buffer_unmap(buf, &map);
}