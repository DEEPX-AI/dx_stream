#include "rga_preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "../metadata/gst-dxframemeta.hpp"
#include "../metadata/gst-dxobjectmeta.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <rga/rga.h>
#include <rga/im2d.h>
#include <algorithm>
#include <gst/allocators/gstdmabuf.h>

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

bool RgaPreprocessor::preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, uint8_t *output, cv::Rect *roi) {
    if (get_element()->_preprocess.width % 16 != 0 || get_element()->_preprocess.height % 2 != 0) {
        GST_ERROR_OBJECT(get_element(), "ERROR : output W stride must be 16 (H stride 2) aligned ! \n");
        return false;
    }

    if (!output) {
        GST_ERROR_OBJECT(get_element(), "ERROR : output memory is nullptr! \n");
        return false;
    }

    if (g_strcmp0(frame_meta->_format.c_str(), "NV12") != 0) {
        GST_ERROR_OBJECT(get_element(), "ERROR : not supported format (use NV12)! \n");
        return false;
    }

    // Calculate actual stride from buffer size (GstVideoInfo stride can be incorrect)
    GstMemory *mem = gst_buffer_peek_memory(buf, 0);
    gsize mem_size = gst_memory_get_sizes(mem, NULL, NULL);
    
    // NV12: Y plane (stride×hstride) + UV plane (stride×hstride/2)
    // mem_size = stride × hstride × 1.5
    int hstride = ((frame_meta->_height + 15) / 16) * 16;  // 16-aligned height
    int actual_stride = mem_size / (hstride * 3 / 2);

    rga_buffer_t src_img;
    GstMapInfo map;
    bool is_dmabuf = false;
    bool buffer_mapped = false;

    if (gst_is_dmabuf_memory(mem)) {
        gint fd = gst_dmabuf_memory_get_fd(mem);
        if (fd >= 0) {
            // DMA-Buffer path - zero-copy hardware acceleration
            src_img = wrapbuffer_fd(
                fd, frame_meta->_width, frame_meta->_height, 
                RK_FORMAT_YCbCr_420_SP,
                actual_stride, hstride);
            is_dmabuf = true;
            GST_DEBUG_OBJECT(get_element(), "Using DMA-Buffer zero-copy path (fd=%d)", fd);
        } else {
            GST_WARNING_OBJECT(get_element(), "Failed to get DMA-Buffer fd, falling back to virtual memory");
        }
    }

    if (!is_dmabuf) {
        // Fallback - map to virtual memory
        if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
            GST_ERROR_OBJECT(get_element(), "ERROR : Failed to map GstBuffer (dxpreprocess) \n");
            return false;
        }
        buffer_mapped = true;
        src_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(map.data), frame_meta->_width,
            frame_meta->_height, RK_FORMAT_YCbCr_420_SP,
            actual_stride, hstride);
        GST_DEBUG_OBJECT(get_element(), "Using virtual memory path");
    }
    
    rga_buffer_t dst_img;
    if (g_strcmp0(get_element()->_preprocess.color_format, "RGB") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), get_element()->_preprocess.width,
            get_element()->_preprocess.height, RK_FORMAT_RGB_888);
    } else if (g_strcmp0(get_element()->_preprocess.color_format, "BGR") == 0) {
        dst_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(output), get_element()->_preprocess.width,
            get_element()->_preprocess.height, RK_FORMAT_BGR_888);
    } else {
        GST_WARNING_OBJECT(get_element(), "Invalid color mode: %s. Use RGB or BGR.", get_element()->_preprocess.color_format);
        if (buffer_mapped) gst_buffer_unmap(buf, &map);
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
        if (roi->x < 0 || roi->y < 0 || 
            roi->x + roi->width > frame_meta->_width ||
            roi->y + roi->height > frame_meta->_height) {
            GST_WARNING_OBJECT(get_element(), "Invalid ROI: (%d,%d,%d,%d) for frame (%dx%d)", 
                     roi->x, roi->y, roi->width, roi->height,
                     frame_meta->_width, frame_meta->_height);
            if (buffer_mapped) gst_buffer_unmap(buf, &map);
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

    if (get_element()->_preprocess.keep_ratio) {
        float ratioDest = (float)get_element()->_preprocess.width / get_element()->_preprocess.height;
        float ratioSrc = (float)width / height;
        int newWidth, newHeight;
        if (ratioSrc < ratioDest) {
            newHeight = get_element()->_preprocess.height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = get_element()->_preprocess.width;
            newHeight = newWidth / ratioSrc;
        }

        int total_pad_w = get_element()->_preprocess.width - newWidth;
        int total_pad_h = get_element()->_preprocess.height - newHeight;
        uint16_t left = total_pad_w / 2;
        uint16_t top = total_pad_h / 2;

        dst_rect.x = left;
        dst_rect.y = top;
        dst_rect.width = newWidth;
        dst_rect.height = newHeight;
    } else {
        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = get_element()->_preprocess.width;
        dst_rect.height = get_element()->_preprocess.height;
    }

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret) {
        GST_ERROR_OBJECT(get_element(), "check error: %d - %s", ret, imStrError((IM_STATUS)ret));
        if (buffer_mapped) gst_buffer_unmap(buf, &map);
        return false;
    }

    if ((float)dst_rect.width / src_rect.width <= 0.125 ||
        (float)dst_rect.width / src_rect.width >= 8 ||
        (float)dst_rect.height / src_rect.height <= 0.125 ||
        (float)dst_rect.height / src_rect.height >= 8) {
        GST_WARNING_OBJECT(get_element(), "DX Preprocess : scale check error, scale limit[1/8 ~ 8] \n");
        if (buffer_mapped) gst_buffer_unmap(buf, &map);
        return false;
    }

    if (src_rect.width < 68 || src_rect.height < 2 || src_rect.width > 8176 ||
        src_rect.height > 8176) {
        GST_WARNING_OBJECT(get_element(), "DX Preprocess : resolution check error, input range[68x2 ~ "
                  "8176x8176] \n");
        if (buffer_mapped) gst_buffer_unmap(buf, &map);
        return false;
    }

    if (dst_rect.width < 68 || dst_rect.height < 2 || dst_rect.width > 8128 ||
        dst_rect.height > 8128) {
        GST_WARNING_OBJECT(get_element(), "DX Preprocess : resolution check error, output range[68x2 ~ "
                  "8128x8128] \n");
        if (buffer_mapped) gst_buffer_unmap(buf, &map);
        return false;
    }

    ret = improcess(src_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);

    if (buffer_mapped) gst_buffer_unmap(buf, &map);
    
    if (ret != IM_STATUS_SUCCESS) {
        GST_ERROR_OBJECT(get_element(), "RGA resize (imresize) failed: %d - %s", ret, imStrError((IM_STATUS)ret));
        return false;
    }
    return true;
}
