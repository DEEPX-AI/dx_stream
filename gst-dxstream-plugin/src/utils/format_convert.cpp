#include "format_convert.hpp"

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

uint8_t *Crop(GstBuffer *buf, int src_width, int src_height, int crop_x,
              int crop_y, int crop_width, int crop_height,
              const gchar *format) {
    uint8_t *dst = nullptr;
    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer\n");
        return dst;
    }

    GstVideoMeta *meta = gst_buffer_get_video_meta(buf);
    int result = 0;

    if (crop_width <= 0 || crop_height <= 0) {
        g_printerr("Invalid crop dimensions\n");
        gst_buffer_unmap(buf, &map);
        return dst;
    }

    if (g_strcmp0(format, "RGB") == 0) {
        dst = (uint8_t *)malloc(crop_height * crop_width * 3);
        const uint8_t *src = map.data;
        gint stride = src_width * 3;
        if (meta) {
            src = map.data + meta->offset[0];
            stride = meta->stride[0];
        }
        result = RGBCrop(src, stride, src_width, src_height, dst, crop_width,
                         crop_height, crop_x, crop_y);
    } else if (g_strcmp0(format, "I420") == 0) {
        dst = (uint8_t *)malloc(crop_width * crop_height * 3 / 2);
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);
        gint strideY = src_width;
        gint strideU = src_width / 2;
        gint strideV = src_width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_u = map.data + meta->offset[1];
            src_v = map.data + meta->offset[2];
            strideY = meta->stride[0];
            strideU = meta->stride[1];
            strideV = meta->stride[2];
        }
        result =
            I420Crop(src_y, strideY, src_u, strideU, src_v, strideV, src_width,
                     src_height, dst, crop_width, crop_height, crop_x, crop_y);

    } else if (g_strcmp0(format, "NV12") == 0) {
        dst = (uint8_t *)malloc(crop_width * crop_height * 3 / 2);
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + src_width * src_height;
        gint strideY = src_width;
        gint strideUV = src_width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_uv = map.data + meta->offset[1];
            strideY = meta->stride[0];
            strideUV = meta->stride[1];
        }

        result =
            NV12Crop(src_y, strideY, src_uv, strideUV, src_width, src_height,
                     dst, crop_width, crop_height, crop_x, crop_y);
    } else {
        g_printerr("Not supported color format\n");
    }

    if (result != 0) {
        g_printerr("Failed to crop frame\n");
    }
    gst_buffer_unmap(buf, &map);
    return dst;
}

uint8_t *Resize(uint8_t *src, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format) {
    uint8_t *dst = nullptr;
    int result = 0;
    if (dst_width <= 0 || dst_height <= 0) {
        g_printerr("Invalid crop dimensions\n");
        return dst;
    }
    if (g_strcmp0(format, "RGB") == 0) {
        g_printerr("Invalid crop dimensions\n");
        dst = (uint8_t *)malloc(dst_width * dst_height * 3);
        uint8_t *dst_r = dst;
        uint8_t *dst_g = dst_r + dst_width * dst_height;
        uint8_t *dst_b = dst_g + dst_width * dst_height;

        const uint8_t *src_r = src;
        const uint8_t *src_g = src + src_width * src_height;
        const uint8_t *src_b = src_g + src_width * src_height;

        gint stride = src_width * 3;
        result = libyuv::ScalePlane(src_r, stride, src_width, src_height, dst_r,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
        result = libyuv::ScalePlane(src_g, stride, src_width, src_height, dst_g,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
        result = libyuv::ScalePlane(src_b, stride, src_width, src_height, dst_b,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "I420") == 0) {
        dst = (uint8_t *)malloc(dst_width * dst_height * 3 / 2);
        uint8_t *dst_y = dst;
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);
        const uint8_t *src_y = src;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);
        gint strideY = src_width;
        gint strideU = src_width / 2;
        gint strideV = src_width / 2;
        result = libyuv::I420Scale(
            src_y, strideY, src_u, strideU, src_v, strideV, src_width,
            src_height, dst_y, dst_width, dst_u, dst_width / 2, dst_v,
            dst_width / 2, dst_width, dst_height, libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "NV12") == 0) {
        dst = (uint8_t *)malloc(dst_width * dst_height * 3 / 2);
        uint8_t *dst_y = dst;
        uint8_t *dst_uv = dst_y + dst_width * dst_height;
        const uint8_t *src_y = src;
        const uint8_t *src_uv = src + src_width * src_height;
        gint strideY = src_width;
        gint strideUV = src_width / 2;

        result =
            libyuv::NV12Scale(src_y, strideY, src_uv, strideUV, src_width,
                              src_height, dst_y, dst_width, dst_uv, dst_width,
                              dst_width, dst_height, libyuv::kFilterLinear);
    } else {
        g_printerr("Not supported color format\n");
    }
    if (result != 0) {
        free(dst);
        g_printerr("Failed to crop frame\n");
        return nullptr;
    }
    return dst;
}

uint8_t *Resize(GstBuffer *buf, int src_width, int src_height, int dst_width,
                int dst_height, const gchar *format) {
    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer\n");
        return nullptr;
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(buf);
    int result = 0;
    if (dst_width <= 0 || dst_height <= 0) {
        g_printerr("Invalid crop dimensions\n");
        gst_buffer_unmap(buf, &map);
        return nullptr;
    }
    uint8_t *dst = nullptr;
    if (g_strcmp0(format, "RGB") == 0) {
        g_printerr("Invalid crop dimensions\n");
        dst = (uint8_t *)malloc(dst_width * dst_height * 3);
        uint8_t *dst_r = dst;
        uint8_t *dst_g = dst_r + dst_width * dst_height;
        uint8_t *dst_b = dst_g + dst_width * dst_height;

        const uint8_t *src_r = map.data;
        const uint8_t *src_g = map.data + src_width * src_height;
        const uint8_t *src_b = src_g + src_width * src_height;

        gint stride = src_width * 3;

        if (meta) {
            src_r = map.data + meta->offset[0];
            src_g = map.data + meta->offset[1];
            src_b = map.data + meta->offset[2];

            stride = meta->stride[0];
        }
        result = libyuv::ScalePlane(src_r, stride, src_width, src_height, dst_r,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
        result = libyuv::ScalePlane(src_g, stride, src_width, src_height, dst_g,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
        result = libyuv::ScalePlane(src_b, stride, src_width, src_height, dst_b,
                                    dst_width, dst_width, dst_height,
                                    libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "I420") == 0) {
        dst = (uint8_t *)malloc(dst_width * dst_height * 3 / 2);
        uint8_t *dst_y = dst;
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + src_width * src_height;
        const uint8_t *src_v = src_u + (src_width / 2) * (src_height / 2);
        gint strideY = src_width;
        gint strideU = src_width / 2;
        gint strideV = src_width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_u = map.data + meta->offset[1];
            src_v = map.data + meta->offset[2];
            strideY = meta->stride[0];
            strideU = meta->stride[1];
            strideV = meta->stride[2];
        }
        result = libyuv::I420Scale(
            src_y, strideY, src_u, strideU, src_v, strideV, src_width,
            src_height, dst_y, dst_width, dst_u, dst_width / 2, dst_v,
            dst_width / 2, dst_width, dst_height, libyuv::kFilterLinear);
    } else if (g_strcmp0(format, "NV12") == 0) {
        dst = (uint8_t *)malloc(dst_width * dst_height * 3 / 2);
        uint8_t *dst_y = dst;
        uint8_t *dst_uv = dst_y + dst_width * dst_height;
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + src_width * src_height;
        gint strideY = src_width;
        gint strideUV = src_width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_uv = map.data + meta->offset[1];
            strideY = meta->stride[0];
            strideUV = meta->stride[1];
        }

        result =
            libyuv::NV12Scale(src_y, strideY, src_uv, strideUV, src_width,
                              src_height, dst_y, dst_width, dst_uv, dst_width,
                              dst_width, dst_height, libyuv::kFilterLinear);
    } else {
        g_printerr("Not supported color format\n");
    }
    if (result != 0) {
        free(dst);
        gst_buffer_unmap(buf, &map);
        g_printerr("Failed to crop frame\n");
        return nullptr;
    }
    gst_buffer_unmap(buf, &map);
    return dst;
}

uint8_t *CvtColor(GstBuffer *buf, int width, int height,
                  const gchar *src_format, const gchar *dst_format) {

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer\n");
        return nullptr;
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(buf);
    int result = 0;
    if (width <= 0 || height <= 0) {
        g_printerr("Invalid crop dimensions\n");
        gst_buffer_unmap(buf, &map);
        return nullptr;
    }
    uint8_t *dst = nullptr;

    if (g_strcmp0(src_format, "RGB") == 0) {
        const uint8_t *src = map.data;
        gint stride = width * 3;
        if (meta) {
            src = map.data + meta->offset[0];
            stride = meta->stride[0];
        }
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            memcpy(dst, src, width * height * 3);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result =
                libyuv::RAWToRGB24(src, stride, dst, stride, width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else if (g_strcmp0(src_format, "I420") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_u = src_y + width * height;
        const uint8_t *src_v = src_u + (width / 2) * (height / 2);
        gint strideY = width;
        gint strideU = width / 2;
        gint strideV = width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_u = map.data + meta->offset[1];
            src_v = map.data + meta->offset[2];
            strideY = meta->stride[0];
            strideU = meta->stride[1];
            strideV = meta->stride[2];
        }
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result =
                libyuv::I420ToRAW(src_y, strideY, src_u, strideU, src_v,
                                  strideV, dst, width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result =
                libyuv::I420ToRGB24(src_y, strideY, src_u, strideU, src_v,
                                    strideV, dst, width * 3, width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        const uint8_t *src_y = map.data;
        const uint8_t *src_uv = map.data + width * height;
        gint strideY = width;
        gint strideUV = width / 2;
        if (meta) {
            src_y = map.data + meta->offset[0];
            src_uv = map.data + meta->offset[1];
            strideY = meta->stride[0];
            strideUV = meta->stride[1];
        }
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result = libyuv::NV12ToRAW(src_y, strideY, src_uv, strideUV,
                                           dst, width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result = libyuv::NV12ToRGB24(src_y, strideY, src_uv, strideUV,
                                             dst, width * 3, width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else {
        g_printerr("Not support color format \n");
    }
    if (result != 0) {
        free(dst);
        gst_buffer_unmap(buf, &map);
        g_printerr("Failed to crop frame\n");
        return nullptr;
    }
    gst_buffer_unmap(buf, &map);
    return dst;
}

uint8_t *CvtColor(uint8_t *src, int width, int height, const gchar *src_format,
                  const gchar *dst_format) {

    uint8_t *dst = nullptr;
    if (g_strcmp0(src_format, "RGB") == 0) {
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            memcpy(dst, src, width * height * 3);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result = libyuv::RAWToRGB24(src, width * 3, dst, width * 3,
                                            width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else if (g_strcmp0(src_format, "I420") == 0) {
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result = libyuv::I420ToRAW(
                src, width, src + width * height, width / 2,
                (src + width * height) + (width / 2) * (height / 2), width / 2,
                dst, width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result = libyuv::I420ToRGB24(
                src, width, src + width * height, width / 2,
                (src + width * height) + (width / 2) * (height / 2), width / 2,
                dst, width * 3, width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else if (g_strcmp0(src_format, "NV12") == 0) {
        if (g_strcmp0(dst_format, "RGB") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result =
                libyuv::NV12ToRAW(src, width, src + width * height, width, dst,
                                  width * 3, width, height);
        } else if (g_strcmp0(dst_format, "BGR") == 0) {
            dst = (uint8_t *)malloc(width * height * 3);
            int result =
                libyuv::NV12ToRGB24(src, width, src + width * height, width,
                                    dst, width * 3, width, height);
        } else {
            g_printerr("Not support color format \n");
        }
    } else {
        g_printerr("Not support color format \n");
    }
    return dst;
}

void RGB24toI420(DXFrameMeta *frame_meta, uint8_t *surface) {
    GstMapInfo map;
    if (!gst_buffer_is_writable(frame_meta->_buf)) {
        frame_meta->_buf = gst_buffer_make_writable(frame_meta->_buf);
    }
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_WRITE)) {
        g_printerr("Failed to map GstBuffer");
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    uint8_t *dstY;
    uint8_t *dstU;
    uint8_t *dstV;
    gint strideY;
    gint strideU;
    gint strideV;
    if (meta) {
        dstY = map.data + meta->offset[0];
        dstU = map.data + meta->offset[1];
        dstV = map.data + meta->offset[2];

        strideY = meta->stride[0];
        strideU = meta->stride[1];
        strideV = meta->stride[2];
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
        g_printerr("Failed to convert RGB to I420");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void RGB24toNV12(DXFrameMeta *frame_meta, uint8_t *surface) {
    GstMapInfo map;
    if (!gst_buffer_is_writable(frame_meta->_buf)) {
        frame_meta->_buf = gst_buffer_make_writable(frame_meta->_buf);
    }
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_WRITE)) {
        g_printerr("Failed to map GstBuffer");
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    uint8_t *dstY;
    uint8_t *dstUV;
    gint strideY;
    gint strideUV;
    if (meta) {
        dstY = map.data + meta->offset[0];
        dstUV = map.data + meta->offset[1];

        strideY = meta->stride[0];
        strideUV = meta->stride[1];
    } else {
        dstY = map.data;
        dstUV = map.data + frame_meta->_width * frame_meta->_height;

        strideY = frame_meta->_width;
        strideUV = frame_meta->_width / 2;
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
        g_printerr("Failed to convert I420 to NV12");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void SurfaceToOrigin(DXFrameMeta *frame_meta, uint8_t *surface) {
    if (g_strcmp0(frame_meta->_format, "RGB") == 0) {
        GstMapInfo map_info;
        if (!gst_buffer_map(frame_meta->_buf, &map_info, GST_MAP_READ)) {
            g_printerr("Failed to map GstBuffer");
        }
        memcpy(map_info.data, surface,
               3 * frame_meta->_height * frame_meta->_width);
        gst_buffer_unmap(frame_meta->_buf, &map_info);
    } else if (g_strcmp0(frame_meta->_format, "I420") == 0) {
        RGB24toI420(frame_meta, surface);
    } else if (g_strcmp0(frame_meta->_format, "NV12") == 0) {
        RGB24toNV12(frame_meta, surface);
    } else {
        g_printerr("Not support color format \n");
    }
}
