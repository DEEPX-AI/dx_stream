#include "rga_preprocessor.h"

#ifdef HAVE_LIBRGA

#include "gst-dxpreprocess.hpp"
#include "gst-dxmeta.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <rga/rga.h>
#include <rga/im2d.h>
#include <algorithm>

RgaPreprocessor::RgaPreprocessor(GstDxPreprocess *elem) : Preprocessor(elem) {
}

RgaPreprocessor::~RgaPreprocessor() {
}

bool RgaPreprocessor::calculate_nv12_strides_short(int w, int h, int wa, int ha, int *ws, int *hs) {
    if (!ws || !hs || w <= 0 || h <= 0 || (h % 2 != 0) || wa < 0 || ha < 0) {
        return false;
    }
    *ws = (wa <= 1) ? w : (((w + wa - 1) / wa) * wa);
    *hs = (ha <= 1) ? h : (((h + ha - 1) / ha) * ha);
    return true;
}

bool RgaPreprocessor::preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, void *output, cv::Rect *roi) {
    if (element->_resize_width % 16 != 0 || element->_resize_height % 2 != 0) {
        g_error("ERROR : output W stride must be 16 (H stride 2) aligned ! \n");
        return true;
    }

    if (!output) {
        g_error("ERROR : output memory is nullptr! \n");
        return false;
    }

    if (g_strcmp0(frame_meta->_format, "NV12") != 0) {
        g_error("ERROR : not supported format (use NV12)! \n");
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        g_error("ERROR : Failed to map GstBuffer (dxpreprocess) \n");
        return false;
    }
    
    int wstride, hstride;
    calculate_nv12_strides_short(frame_meta->_width, frame_meta->_height, 16,
                                 16, &wstride, &hstride);
    rga_buffer_t src_img = wrapbuffer_virtualaddr(
        reinterpret_cast<void *>(map.data), frame_meta->_width,
        frame_meta->_height, RK_FORMAT_YCbCr_420_SP,
        element->_input_info[frame_meta->_stream_id].stride[0], hstride);
    
    rga_buffer_t dst_img;
    if (g_strcmp0(element->_color_format, "RGB") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), element->_resize_width,
            element->_resize_height, RK_FORMAT_RGB_888);
    } else if (g_strcmp0(element->_color_format, "BGR") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), element->_resize_width,
            element->_resize_height, RK_FORMAT_BGR_888);
    } else {
        g_warning("Invalid color mode: %s. Use RGB or BGR.", element->_color_format);
        gst_buffer_unmap(buf, &map);
        return false;
    }

    int width = frame_meta->_width;
    int height = frame_meta->_height;

    im_rect src_rect, dst_rect;
    memset(&src_rect, 0, sizeof(src_rect));
    memset(&dst_rect, 0, sizeof(dst_rect));

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = frame_meta->_width;
    src_rect.height = frame_meta->_height;

    if (roi->width != 0 && roi->height != 0) {
        // ROI 경계 검사
        if (roi->x < 0 || roi->y < 0 || 
            roi->x + roi->width > frame_meta->_width ||
            roi->y + roi->height > frame_meta->_height) {
            g_warning("Invalid ROI: (%d,%d,%d,%d) for frame (%dx%d)", 
                     roi->x, roi->y, roi->width, roi->height,
                     frame_meta->_width, frame_meta->_height);
            gst_buffer_unmap(buf, &map);
            return false;
        }
        
        src_rect.x = std::max(roi->x % 2 == 0 ? roi->x : roi->x + 1, 0);
        src_rect.y = std::max(roi->y % 2 == 0 ? roi->y : roi->y + 1, 0);
        src_rect.width = std::max(roi->width % 2 == 0 ? roi->width : roi->width + 1, 0);
        if (src_rect.width + src_rect.x > frame_meta->_width) {
            src_rect.width = frame_meta->_width - src_rect.x;
        }
        src_rect.height = std::max(roi->height % 2 == 0 ? roi->height : roi->height + 1, 0);
        if (src_rect.height + src_rect.y > frame_meta->_height) {
            src_rect.height = frame_meta->_height - src_rect.y;
        }
        width = src_rect.width;
        height = src_rect.height;
    }

    if (element->_keep_ratio) {
        float ratioDest = (float)element->_resize_width / element->_resize_height;
        float ratioSrc = (float)width / height;
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = element->_resize_height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = element->_resize_width;
            newHeight = newWidth / ratioSrc;
        }

        int total_pad_w = element->_resize_width - newWidth;
        int total_pad_h = element->_resize_height - newHeight;
        uint16_t left = total_pad_w / 2;
        uint16_t top = total_pad_h / 2;

        dst_rect.x = left;
        dst_rect.y = top;
        dst_rect.width = newWidth;
        dst_rect.height = newHeight;
    } else {
        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = element->_resize_width;
        dst_rect.height = element->_resize_height;
    }

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        std::cerr << "check error: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
        gst_buffer_unmap(buf, &map);
        return false;
    }

    if ((float)dst_rect.width / src_rect.width <= 0.125 ||
        (float)dst_rect.width / src_rect.width >= 8 ||
        (float)dst_rect.height / src_rect.height <= 0.125 ||
        (float)dst_rect.height / src_rect.height >= 8) {
        g_warning("DX Preprocess : scale check error, scale limit[1/8 ~ 8] \n");
        gst_buffer_unmap(buf, &map);
        return false;
    }

    if (src_rect.width < 68 || src_rect.height < 2 || src_rect.width > 8176 ||
        src_rect.height > 8176) {
        g_warning("DX Preprocess : resolution check error, input range[68x2 ~ "
                  "8176x8176] \n");
        gst_buffer_unmap(buf, &map);
        return false;
    }

    if (dst_rect.width < 68 || dst_rect.height < 2 || dst_rect.width > 8128 ||
        dst_rect.height > 8128) {
        g_warning("DX Preprocess : resolution check error, output range[68x2 ~ "
                  "8128x8128] \n");
        gst_buffer_unmap(buf, &map);
        return false;
    }

    ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);

    gst_buffer_unmap(buf, &map);
    if (ret != IM_STATUS_SUCCESS) {
        std::cerr << "RGA resize (imresize) failed: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
        return false;
    }
    return true;
}

#endif // HAVE_LIBRGA