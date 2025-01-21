#include "format_convert.hpp"

void convert_format_nv12(DXFrameMeta *frame_meta) {
    GstMapInfo map;
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer");
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    int result;
    if (meta) {
        const uint8_t *srcY = map.data + meta->offset[0];
        const uint8_t *srcUV = map.data + meta->offset[1];

        result =
            libyuv::NV12ToRAW(srcY, meta->stride[0], srcUV, meta->stride[1],
                              frame_meta->_rgb_surface.data, meta->width * 3,
                              meta->width, meta->height);
    } else {
        const uint8_t *srcY = map.data;
        const uint8_t *srcUV =
            map.data + frame_meta->_width * frame_meta->_height;
        result = libyuv::NV12ToRAW(
            srcY, frame_meta->_width, srcUV, frame_meta->_width,
            frame_meta->_rgb_surface.data, frame_meta->_width * 3,
            frame_meta->_width, frame_meta->_height);
    }
    if (result != 0) {
        g_printerr("Failed to convert NV12 to RGB");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void convert_format_i420(DXFrameMeta *frame_meta) {
    GstMapInfo map;
    if (!gst_buffer_map(frame_meta->_buf, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer");
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    int result;
    if (meta) {
        const uint8_t *srcY = map.data + meta->offset[0];
        const uint8_t *srcU = map.data + meta->offset[1];
        const uint8_t *srcV = map.data + meta->offset[2];
        result = libyuv::I420ToRAW(srcY, meta->stride[0], srcU, meta->stride[1],
                                   srcV, meta->stride[2],
                                   frame_meta->_rgb_surface.data,
                                   meta->width * 3, meta->width, meta->height);
    } else {
        const uint8_t *srcY = map.data;
        const uint8_t *srcU =
            map.data + frame_meta->_width * frame_meta->_height;
        const uint8_t *srcV =
            srcU + (frame_meta->_width / 2) * (frame_meta->_height / 2);

        result = libyuv::I420ToRAW(
            srcY, frame_meta->_width, srcU, frame_meta->_width / 2, srcV,
            frame_meta->_width / 2, frame_meta->_rgb_surface.data,
            frame_meta->_width * 3, frame_meta->_width, frame_meta->_height);
    }
    if (result != 0) {
        g_printerr("Failed to convert I420 to RGB");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void set_surface(DXFrameMeta *frame_meta) {
    if (g_strcmp0(frame_meta->_format, "RGB") == 0) {
        GstMapInfo map_info;
        if (!gst_buffer_map(frame_meta->_buf, &map_info, GST_MAP_READ)) {
            g_printerr("Failed to map GstBuffer");
        }
        memcpy(frame_meta->_rgb_surface.data, map_info.data,
               3 * frame_meta->_height * frame_meta->_width);
        gst_buffer_unmap(frame_meta->_buf, &map_info);
    } else if (g_strcmp0(frame_meta->_format, "I420") == 0) {
        convert_format_i420(frame_meta);
    } else if (g_strcmp0(frame_meta->_format, "NV12") == 0) {
        convert_format_nv12(frame_meta);
    } else {
        g_printerr("Not support color format \n");
    }
}

cv::Mat convert_color(DXFrameMeta *frame_meta, gchar *format) {
    cv::Mat originFrame(frame_meta->_height, frame_meta->_width, CV_8UC3);

    if (frame_meta->_rgb_surface.data == nullptr) {
        g_printerr("RGB Surface empty ! \n");
    }
    if (g_strcmp0(format, "RGB") == 0) {
        memcpy(originFrame.data, frame_meta->_rgb_surface.data,
               3 * frame_meta->_height * frame_meta->_width);
    } else if (g_strcmp0(format, "BGR") == 0) {
        cv::cvtColor(frame_meta->_rgb_surface, originFrame, cv::COLOR_RGB2BGR);
    } else if (g_strcmp0(format, "GRAY") == 0) {
        cv::cvtColor(frame_meta->_rgb_surface, originFrame, cv::COLOR_RGB2GRAY);
    } else {
        g_printerr("Not support color format \n");
    }

    return originFrame;
}

void RGB24toI420(DXFrameMeta *frame_meta) {
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
    int result = libyuv::RAWToI420(
        frame_meta->_rgb_surface.data, frame_meta->_width * 3, dstY, strideY,
        dstU, strideU, dstV, strideV, frame_meta->_width, frame_meta->_height);
    if (result != 0) {
        g_printerr("Failed to convert RGB to I420");
    }
    gst_buffer_unmap(frame_meta->_buf, &map);
}

void RGB24toNV12(DXFrameMeta *frame_meta) {
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
    cv::cvtColor(frame_meta->_rgb_surface, yuv_frame, cv::COLOR_RGB2YUV_I420);

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

void SurfaceToOrigin(DXFrameMeta *frame_meta) {
    if (g_strcmp0(frame_meta->_format, "RGB") == 0) {
        GstMapInfo map_info;
        if (!gst_buffer_map(frame_meta->_buf, &map_info, GST_MAP_READ)) {
            g_printerr("Failed to map GstBuffer");
        }
        memcpy(map_info.data, frame_meta->_rgb_surface.data,
               3 * frame_meta->_height * frame_meta->_width);
        gst_buffer_unmap(frame_meta->_buf, &map_info);
    } else if (g_strcmp0(frame_meta->_format, "I420") == 0) {
        RGB24toI420(frame_meta);
    } else if (g_strcmp0(frame_meta->_format, "NV12") == 0) {
        RGB24toNV12(frame_meta);
    } else {
        g_printerr("Not support color format \n");
    }
}

void I420ToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height) {
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer");
        return;
    }

    GstVideoMeta *meta = gst_buffer_get_video_meta(buffer);
    if (meta) {

        const uint8_t *srcY = map.data + meta->offset[0];
        const uint8_t *srcU = map.data + meta->offset[1];
        const uint8_t *srcV = map.data + meta->offset[2];
        int result = libyuv::I420ToABGR(srcY, meta->stride[0], srcU,
                                        meta->stride[1], srcV, meta->stride[2],
                                        dst, width * 4, width, height);

        if (result != 0) {
            g_printerr("Failed to convert I420 to RGBA");
        }
    } else {
        const uint8_t *srcY = map.data;
        const uint8_t *srcU = map.data + width * height;
        const uint8_t *srcV = srcU + (width / 2) * (height / 2);

        int result =
            libyuv::I420ToARGB(srcY, width, srcU, width / 2, srcV, width / 2,
                               dst, width * 4, width, height);

        if (result != 0) {
            g_printerr("Failed to convert I420 to RGBA");
        }
    }
    gst_buffer_unmap(buffer, &map);
}

void RGBToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height) {
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer");
        return;
    }

    const uint8_t *src = map.data;

    int result =
        libyuv::RAWToARGB(src, width * 3, dst, width * 4, width, height);

    if (result != 0) {
        g_printerr("Failed to convert RGB to RGBA");
    }

    gst_buffer_unmap(buffer, &map);
}

void NV12ToRGBA(GstBuffer *buffer, uint8_t *dst, int width, int height) {
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        g_printerr("Failed to map GstBuffer");
        return;
    }
    GstVideoMeta *meta = gst_buffer_get_video_meta(buffer);
    if (meta) {
        const uint8_t *srcY = map.data + meta->offset[0];
        const uint8_t *srcUV = map.data + meta->offset[1];

        int result =
            libyuv::NV12ToARGB(srcY, meta->stride[0], srcUV, meta->stride[1],
                               dst, width * 4, width, height);

        if (result != 0) {
            g_printerr("Failed to convert NV12 to RGBA");
        }
    } else {
        const uint8_t *srcY = map.data;
        const uint8_t *srcUV = map.data + width * height;
        int result = libyuv::NV12ToABGR(srcY, width, srcUV, width, dst,
                                        width * 4, width, height);
        if (result != 0) {
            g_printerr("Failed to convert NV12 to RGBA");
        }
    }
    gst_buffer_unmap(buffer, &map);
}
