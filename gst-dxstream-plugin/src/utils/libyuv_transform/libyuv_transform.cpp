#include "libyuv_transform.hpp"

#define ALIGN_UP(value, align) (((value) + (align) - 1) & ~((align) - 1))

int I420Crop(const uint8_t *src_y, int src_stride_y, const uint8_t *src_u,
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
    const uint8_t *cropped_u =
        src_u + (crop_y / 2) * src_stride_u + (crop_x / 2);
    const uint8_t *cropped_v =
        src_v + (crop_y / 2) * src_stride_v + (crop_x / 2);

    return libyuv::I420Copy(cropped_y, src_stride_y, cropped_u, src_stride_u,
                            cropped_v, src_stride_v, dst_y, crop_width, dst_u,
                            crop_width / 2, dst_v, crop_width / 2, crop_width,
                            crop_height);
}

int NV12Crop(const uint8_t *src_y, int src_stride_y, const uint8_t *src_uv,
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
        memcpy(dst_y + i * crop_width, cropped_y + i * src_stride_y,
               crop_width);
    }

    const uint8_t *cropped_uv =
        src_uv + (crop_y / 2) * src_stride_uv + (crop_x / 2) * 2;
    for (int i = 0; i < crop_height / 2; ++i) {
        memcpy(dst_uv + i * crop_width, cropped_uv + i * src_stride_uv,
               crop_width);
    }
    return 0;
}

int RGBCrop(const uint8_t *src, int src_stride, int src_width, int src_height,
            uint8_t *dst, int crop_width, int crop_height, int crop_x,
            int crop_y) {
    if (crop_x < 0 || crop_y < 0 || crop_x + crop_width > src_width ||
        crop_y + crop_height > src_height) {
        return -1;
    }

    const uint8_t *cropped_src = src + crop_y * src_stride + crop_x * 3;

    for (int i = 0; i < crop_height; ++i) {
        memcpy(dst + i * crop_width * 3, cropped_src + i * src_stride,
               crop_width * 3);
    }

    return 0;
}

void Crop(GstBuffer *buf, GstVideoInfo *input_info, uint8_t **dst, int src_width, int src_height,
          int crop_x, int crop_y, int crop_width, int crop_height,
          const gchar *format) {
    int result = 0;

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_error("Crop: Failed to map GstBuffer\n");
    }
    if (crop_width <= 0 || crop_height <= 0) {
        g_error("Crop: Invalid crop dimensions\n");
    }

    if (g_strcmp0(format, "RGB") == 0) {
        if (!*dst) {
            *dst = (uint8_t *)malloc(crop_height * crop_width * 3);
        }
        const uint8_t *src = map.data;
        gint stride = src_width * 3;
        if (input_info) {
            src = map.data + input_info->offset[0];
            stride = input_info->stride[0];
        }
        result = RGBCrop(src, stride, src_width, src_height, *dst, crop_width,
                         crop_height, crop_x, crop_y);
    } else if (g_strcmp0(format, "I420") == 0) {
        if (!*dst) {
            *dst = (uint8_t *)malloc(crop_width * crop_height * 3 / 2);
        }
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
        result =
            I420Crop(src_y, strideY, src_u, strideU, src_v, strideV, src_width,
                     src_height, *dst, crop_width, crop_height, crop_x, crop_y);

    } else if (g_strcmp0(format, "NV12") == 0) {
        if (!*dst) {
            *dst = (uint8_t *)malloc(crop_width * crop_height * 3 / 2);
        }
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

        result =
            NV12Crop(src_y, strideY, src_uv, strideUV, src_width, src_height,
                     *dst, crop_width, crop_height, crop_x, crop_y);
    } else {
        g_error("Crop: Not supported color format\n");
    }

    if (result != 0) {
        g_error("Crop: Failed to crop frame\n");
    }
    gst_buffer_unmap(buf, &map);
}

void Resize(uint8_t *src, uint8_t **dst, int src_width, int src_height,
            int dst_width, int dst_height, const gchar *format) {
    if (dst_width <= 0 || dst_height <= 0) {
        g_warning("Resize: Invalid crop dimensions\n");
        return;
    }

    int result = 0;

    if (g_strcmp0(format, "RGB") == 0) {
        if (!*dst) {
            *dst = (uint8_t *)malloc(dst_width * dst_height * 3);
        }

        cv::Mat mat_src(src_height, src_width, CV_8UC3, src);
        cv::Mat mat_dst;
        cv::resize(mat_src, mat_dst, cv::Size(dst_width, dst_height), 0, 0,
                   cv::INTER_LINEAR);
        memcpy(*dst, mat_dst.data, dst_width * dst_height * 3);

    } else if (g_strcmp0(format, "I420") == 0) {
        int aligned_width = ALIGN_UP(dst_width, 2);
        int aligned_height = ALIGN_UP(dst_height, 2);
        if (!*dst) {
            *dst = (uint8_t *)malloc(aligned_width * aligned_height * 3 / 2);
        }

        uint8_t *dst_y = *dst;
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);
        const uint8_t *src_y = src;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);

        result = libyuv::I420Scale(
            src_y, src_width, src_u, src_width / 2, src_v, src_width / 2,
            src_width, src_height, dst_y, dst_width, dst_u, dst_width / 2,
            dst_v, dst_width / 2, dst_width, dst_height, libyuv::kFilterLinear);

    } else if (g_strcmp0(format, "NV12") == 0) {
        int aligned_width = ALIGN_UP(dst_width, 2);
        int aligned_height = ALIGN_UP(dst_height, 2);
        if (!*dst) {
            *dst = (uint8_t *)malloc(aligned_width * aligned_height * 3 / 2);
        }

        uint8_t *dst_y = *dst;
        uint8_t *dst_uv = dst_y + dst_width * dst_height;
        const uint8_t *src_y = src;
        const uint8_t *src_uv = src + src_width * src_height;

        result =
            libyuv::NV12Scale(src_y, src_width, src_uv, src_width, src_width,
                              src_height, dst_y, dst_width, dst_uv, dst_width,
                              dst_width, dst_height, libyuv::kFilterLinear);
    } else {
        g_warning("Resize: Not supported color format\n");
        return;
    }

    if (result != 0) {
        g_warning("Resize: Failed to resize frame\n");
        return;
    }
}

void Resize(GstBuffer *buf, GstVideoInfo *input_info, uint8_t **dst, int src_width, int src_height,
            int dst_width, int dst_height, const gchar *format) {
    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_error("Resize(buffer): Failed to map GstBuffer\n");
    }
    int result = 0;
    if (dst_width <= 0 || dst_height <= 0) {
        g_error("Resize(buffer): Invalid crop dimensions\n");
    }
    if (g_strcmp0(format, "RGB") == 0) {
        if (!*dst) {
            *dst = (uint8_t *)malloc(dst_width * dst_height * 3);
        }
        uint8_t *src = map.data;
        if (input_info) {
            src = map.data + input_info->offset[0];
        }
        cv::Mat mat_src = cv::Mat(src_height, src_width, CV_8UC3, src);
        cv::Mat mat_dst = cv::Mat(dst_height, dst_width, CV_8UC3, *dst);

        cv::resize(mat_src, mat_dst, cv::Size(dst_width, dst_height), 0, 0,
                   cv::INTER_LINEAR);

    } else if (g_strcmp0(format, "I420") == 0) {
        int aligned_width = ALIGN_UP(dst_width, 2);
        int aligned_height = ALIGN_UP(dst_height, 2);
        if (!*dst) {
            *dst = (uint8_t *)malloc(aligned_width * aligned_height * 3 / 2);
        }
        uint8_t *dst_y = *dst;
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
        int aligned_width = ALIGN_UP(dst_width, 2);
        int aligned_height = ALIGN_UP(dst_height, 2);
        if (!*dst) {
            *dst = (uint8_t *)malloc(aligned_width * aligned_height * 3 / 2);
        }
        uint8_t *dst_y = *dst;
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

        result =
            libyuv::NV12Scale(src_y, strideY, src_uv, strideUV, src_width,
                              src_height, dst_y, dst_width, dst_uv, dst_width,
                              dst_width, dst_height, libyuv::kFilterLinear);
    } else {
        g_error("Resize(buffer): Not supported color format\n");
    }
    if (result != 0) {
        g_error("Resize(buffer): Failed to crop frame\n");
    }
    gst_buffer_unmap(buf, &map);
}

static inline void allocate_dst_buffer(uint8_t **dst, int size) {
    if (!*dst) {
        *dst = (uint8_t *)malloc(size);
        if (!*dst) {
            g_error("Memory allocation failed\n");
        }
    }
}

static inline void handle_unsupported_format(const char *func_name) {
    g_error("%s: Not supported color format\n", func_name);
}

static bool is_rgb(const gchar *fmt) { return g_strcmp0(fmt, "RGB") == 0; }

static bool is_bgr(const gchar *fmt) { return g_strcmp0(fmt, "BGR") == 0; }

static void handle_conversion_error(int result, const char *tag) {
    if (result != 0) {
        g_error("%s: Failed to convert color\n", tag);
    }
}

static void convert_from_rgb(const uint8_t *src, int width, int height,
                             int stride, const gchar *dst_format, uint8_t **dst,
                             const char *tag) {
    int size = width * height * 3;
    allocate_dst_buffer(dst, size);

    if (is_rgb(dst_format)) {
        memcpy(*dst, src, size);
    } else if (is_bgr(dst_format)) {
        int result =
            libyuv::RAWToRGB24(src, stride, *dst, stride, width, height);
        handle_conversion_error(result, tag);
    } else {
        handle_unsupported_format(tag);
    }
}

static void convert_from_i420(const uint8_t *y, const uint8_t *u,
                              const uint8_t *v, int strideY, int strideU,
                              int strideV, int width, int height,
                              const gchar *dst_format, uint8_t **dst,
                              const char *tag) {
    int size = width * height * 3;
    allocate_dst_buffer(dst, size);

    if (is_rgb(dst_format)) {
        int result = libyuv::I420ToRAW(y, strideY, u, strideU, v, strideV, *dst,
                                       width * 3, width, height);
        handle_conversion_error(result, tag);
    } else if (is_bgr(dst_format)) {
        int result = libyuv::I420ToRGB24(y, strideY, u, strideU, v, strideV,
                                         *dst, width * 3, width, height);
        handle_conversion_error(result, tag);
    } else {
        handle_unsupported_format(tag);
    }
}

static void convert_from_nv12(const uint8_t *y, const uint8_t *uv, int strideY,
                              int strideUV, int width, int height,
                              const gchar *dst_format, uint8_t **dst,
                              const char *tag) {
    int size = width * height * 3;
    allocate_dst_buffer(dst, size);

    if (is_rgb(dst_format)) {
        int result = libyuv::NV12ToRAW(y, strideY, uv, strideUV, *dst,
                                       width * 3, width, height);
        handle_conversion_error(result, tag);
    } else if (is_bgr(dst_format)) {
        int result = libyuv::NV12ToRGB24(y, strideY, uv, strideUV, *dst,
                                         width * 3, width, height);
        handle_conversion_error(result, tag);
    } else {
        handle_unsupported_format(tag);
    }
}

static bool map_buffer(GstBuffer *buf, GstMapInfo *map, const char *tag) {
    if (!gst_buffer_map(buf, map, GST_MAP_READ)) {
        g_error("%s: Failed to map GstBuffer\n", tag);
        return false;
    }
    return true;
}

static void handle_rgb(const uint8_t *data, GstVideoInfo *input_info, int width,
                       int height, const gchar *dst_format, uint8_t **dst,
                       const char *tag) {
    const uint8_t *src = data;
    int stride = width * 3;
    if (input_info) {
        src = data + input_info->offset[0];
        stride = input_info->stride[0];
    }
    convert_from_rgb(src, width, height, stride, dst_format, dst, tag);
}

static void handle_i420(const uint8_t *data, GstVideoInfo *input_info, int width,
                        int height, const gchar *dst_format, uint8_t **dst,
                        const char *tag) {
    const uint8_t *y = data + (input_info ? input_info->offset[0] : 0);
    const uint8_t *u = data + (input_info ? input_info->offset[1] : width * height);
    const uint8_t *v =
        data +
        (input_info ? input_info->offset[2] : width * height + (width / 2) * (height / 2));
    int strideY = input_info ? input_info->stride[0] : width;
    int strideU = input_info ? input_info->stride[1] : width / 2;
    int strideV = input_info ? input_info->stride[2] : width / 2;

    convert_from_i420(y, u, v, strideY, strideU, strideV, width, height,
                      dst_format, dst, tag);
}

static void handle_nv12(const uint8_t *data, GstVideoInfo *input_info, int width,
                        int height, const gchar *dst_format, uint8_t **dst,
                        const char *tag) {
    const uint8_t *y = data + (input_info ? input_info->offset[0] : 0);
    const uint8_t *uv = data + (input_info ? input_info->offset[1] : width * height);
    int strideY = input_info ? input_info->stride[0] : width;
    int strideUV = input_info ? input_info->stride[1] : width;

    convert_from_nv12(y, uv, strideY, strideUV, width, height, dst_format, dst,
                      tag);
}

void CvtColor(GstBuffer *buf, GstVideoInfo *input_info, uint8_t **dst, int width, int height,
              const gchar *src_format, const gchar *dst_format) {
    const char *tag = "CvtColor(buffer)";
    if (width <= 0 || height <= 0) {
        g_error("%s: Invalid dimensions\n", tag);
        return;
    }

    GstMapInfo map;
    if (!map_buffer(buf, &map, tag))
        return;

    if (g_strcmp0(src_format, "RGB") == 0) {
        handle_rgb(map.data, input_info, width, height, dst_format, dst, tag);
    } else if (g_strcmp0(src_format, "I420") == 0) {
        handle_i420(map.data, input_info, width, height, dst_format, dst, tag);
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        handle_nv12(map.data, input_info, width, height, dst_format, dst, tag);
    } else {
        handle_unsupported_format(tag);
    }

    gst_buffer_unmap(buf, &map);
}

void CvtColor(uint8_t *src, uint8_t **dst, int width, int height,
              const gchar *src_format, const gchar *dst_format) {

    if (width <= 0 || height <= 0) {
        g_error("CvtColor: Invalid dimensions\n");
        return;
    }

    int ret = 0;

    auto alloc_and_process = [&](int size) { allocate_dst_buffer(dst, size); };

    if (g_strcmp0(src_format, "RGB") == 0) {
        alloc_and_process(width * height * 3);

        if (g_strcmp0(dst_format, "RGB") == 0) {
            memcpy(*dst, src, width * height * 3);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            ret = libyuv::RAWToRGB24(src, width * 3, *dst, width * 3, width,
                                     height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else if (g_strcmp0(src_format, "I420") == 0) {
        alloc_and_process(width * height * 3);

        const uint8_t *src_y = src;
        const uint8_t *src_u = src + width * height;
        const uint8_t *src_v = src_u + (width / 2) * (height / 2);

        if (g_strcmp0(dst_format, "RGB") == 0) {
            ret = libyuv::I420ToRAW(src_y, width, src_u, width / 2, src_v,
                                    width / 2, *dst, width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            ret =
                libyuv::I420ToRGB24(src_y, width, src_u, width / 2, src_v,
                                    width / 2, *dst, width * 3, width, height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        alloc_and_process(width * height * 3);

        const uint8_t *src_y = src;
        const uint8_t *src_uv = src + width * height;

        if (g_strcmp0(dst_format, "RGB") == 0) {
            ret = libyuv::NV12ToRAW(src_y, width, src_uv, width, *dst,
                                    width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            ret = libyuv::NV12ToRGB24(src_y, width, src_uv, width, *dst,
                                      width * 3, width, height);
        } else {
            handle_unsupported_format("CvtColor");
            return;
        }
    } else {
        handle_unsupported_format("CvtColor");
        return;
    }

    if (ret != 0) {
        g_error("CvtColor: Failed to convert color\n");
    }
}

void RGB24toI420(DXFrameMeta *frame_meta, GstVideoInfo *input_info, uint8_t *surface) {
    GstMapInfo map;
    if (!gst_buffer_is_writable(frame_meta->_buf)) {
        frame_meta->_buf = gst_buffer_make_writable(frame_meta->_buf);
    }
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_WRITE)) {
        g_error("RGB24toI420: Failed to map GstBuffer");
    }
    uint8_t *dstY;
    uint8_t *dstU;
    uint8_t *dstV;
    gint strideY;
    gint strideU;
    gint strideV;
    if (input_info) {
        dstY = map.data + input_info->offset[0];
        dstU = map.data + input_info->offset[1];
        dstV = map.data + input_info->offset[2];

        strideY = input_info->stride[0];
        strideU = input_info->stride[1];
        strideV = input_info->stride[2];
    } else {
        dstY = map.data;
        dstU = map.data + frame_meta->_width * frame_meta->_height;
        dstV = dstU + (frame_meta->_width / 2) * (frame_meta->_height / 2);

        strideY = frame_meta->_width;
        strideU = frame_meta->_width / 2;
        strideV = frame_meta->_width / 2;
    }
    int result = libyuv::RAWToI420(surface, frame_meta->_width * 3, dstY,
                                   strideY, dstU, strideU, dstV, strideV,
                                   frame_meta->_width, frame_meta->_height);
    if (result != 0) {
        g_error("Failed to convert RGB to I420");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void RGB24toNV12(DXFrameMeta *frame_meta, GstVideoInfo *input_info, uint8_t *surface) {
    GstMapInfo map;
    if (!gst_buffer_is_writable(frame_meta->_buf)) {
        frame_meta->_buf = gst_buffer_make_writable(frame_meta->_buf);
    }
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_WRITE)) {
        g_error("RGB24 to NV12: Failed to map GstBuffer");
    }
    uint8_t *dstY;
    uint8_t *dstUV;
    gint strideY;
    gint strideUV;
    if (input_info) {
        dstY = map.data + input_info->offset[0];
        dstUV = map.data + input_info->offset[1];

        strideY = input_info->stride[0];
        strideUV = input_info->stride[1];
    } else {
        dstY = map.data;
        dstUV = map.data + frame_meta->_width * frame_meta->_height;

        strideY = frame_meta->_width;
        strideUV = frame_meta->_width;
    }
    cv::Mat yuv_frame;
    cv::Mat surface_mat =
        cv::Mat(frame_meta->_height, frame_meta->_width, CV_8UC3, surface);
    cv::cvtColor(surface_mat, yuv_frame, cv::COLOR_RGB2YUV_I420);

    unsigned char *y_plane = yuv_frame.data;
    unsigned char *u_plane = y_plane + frame_meta->_width * frame_meta->_height;
    unsigned char *v_plane =
        u_plane + (frame_meta->_width / 2) * (frame_meta->_height / 2);

    int result = libyuv::I420ToNV12(
        y_plane, frame_meta->_width, u_plane, frame_meta->_width / 2, v_plane,
        frame_meta->_width / 2, dstY, strideY, dstUV, strideUV,
        frame_meta->_width, frame_meta->_height);
    if (result != 0) {
        g_error("Failed to convert I420 to NV12");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void SurfaceToOrigin(DXFrameMeta *frame_meta, GstVideoInfo *input_info, uint8_t *surface) {
    if (g_strcmp0(frame_meta->_format, "RGB") == 0) {
        GstMapInfo map_info;
        if (!gst_buffer_map(frame_meta->_buf, &map_info, GST_MAP_READ)) {
            g_error("SurfaceToOrigin: Failed to map GstBuffer");
        }
        memcpy(map_info.data, surface,
               3 * frame_meta->_height * frame_meta->_width);
        gst_buffer_unmap(frame_meta->_buf, &map_info);
    } else if (g_strcmp0(frame_meta->_format, "I420") == 0) {
        RGB24toI420(frame_meta, input_info, surface);
    } else if (g_strcmp0(frame_meta->_format, "NV12") == 0) {
        RGB24toNV12(frame_meta, input_info, surface);
    } else {
        g_error("SurfaceToOrigin: Not support color format \n");
    }
}
