#include "libyuv_preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils/libyuv_transform/libyuv_transform.hpp"
#include <algorithm>
#include <iostream>

LibyuvPreprocessor::LibyuvPreprocessor(GstDxPreprocess *elem) : Preprocessor(elem) {
}

LibyuvPreprocessor::~LibyuvPreprocessor() {
}

void LibyuvPreprocessor::check_temp_buffers(DXFrameMeta *frame_meta) {
    auto iter = element->_crop_frame.find(frame_meta->_stream_id);
    if (iter == element->_crop_frame.end()) {
        element->_crop_frame[frame_meta->_stream_id] = nullptr;
    }

    iter = element->_convert_frame.find(frame_meta->_stream_id);
    if (iter == element->_convert_frame.end()) {
        element->_convert_frame[frame_meta->_stream_id] = nullptr;
    }

    iter = element->_resized_frame.find(frame_meta->_stream_id);
    if (iter == element->_resized_frame.end()) {
        element->_resized_frame[frame_meta->_stream_id] = nullptr;
    }
}

bool LibyuvPreprocessor::preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, void *output, cv::Rect *roi) {
    int width = frame_meta->_width;
    int height = frame_meta->_height;

    bool cropped = false;
    if (roi->width != 0 && roi->height != 0) {
        if (roi->x < 0 || roi->y < 0 || 
            roi->x + roi->width > frame_meta->_width ||
            roi->y + roi->height > frame_meta->_height) {
            GST_ERROR_OBJECT(element, "Invalid ROI: (%d,%d,%d,%d) for frame (%dx%d)", 
                              roi->x, roi->y, roi->width, roi->height,
                              frame_meta->_width, frame_meta->_height);
            return false;
        }
        
        Crop(buf, &element->_input_info[frame_meta->_stream_id],
             &element->_crop_frame[frame_meta->_stream_id], frame_meta->_width,
             frame_meta->_height, roi->x, roi->y, roi->width, roi->height,
             frame_meta->_format);
        width = roi->width;
        height = roi->height;
        cropped = true;
    }

    int newWidth = element->_resize_width;
    int newHeight = element->_resize_height;
    if (element->_keep_ratio) {
        float ratioDest = (float)element->_resize_width / element->_resize_height;
        float ratioSrc = (float)width / height;
        if (ratioSrc < ratioDest) {
            newHeight = element->_resize_height;
            newWidth = newHeight * ratioSrc;
        } else {
            newWidth = element->_resize_width;
            newHeight = newWidth / ratioSrc;
        }
    }

    bool resized = false;
    if (newWidth != width || newHeight != height) {
        if (cropped) {
            Resize(element->_crop_frame[frame_meta->_stream_id],
                   &element->_resized_frame[frame_meta->_stream_id], width,
                   height, newWidth, newHeight, frame_meta->_format);
        } else {
            Resize(buf, &element->_input_info[frame_meta->_stream_id],
                   &element->_resized_frame[frame_meta->_stream_id], width, height,
                   newWidth, newHeight, frame_meta->_format);
        }
        resized = true;
    }

    bool converted = false;
    if (g_strcmp0(frame_meta->_format, element->_color_format) != 0) {
        if (resized) {
            CvtColor(element->_resized_frame[frame_meta->_stream_id],
                    &element->_convert_frame[frame_meta->_stream_id], newWidth,
                    newHeight, frame_meta->_format, element->_color_format);
        } else if (cropped) {
            CvtColor(element->_crop_frame[frame_meta->_stream_id],
                    &element->_convert_frame[frame_meta->_stream_id], width,
                    height, frame_meta->_format, element->_color_format);
        } else {
            CvtColor(buf, &element->_input_info[frame_meta->_stream_id],
                    &element->_convert_frame[frame_meta->_stream_id], width, height, 
                    frame_meta->_format, element->_color_format);
        }
        converted = true;
    }

    if (element->_keep_ratio) {
        int total_pad_w = element->_resize_width - newWidth;
        int total_pad_h = element->_resize_height - newHeight;
        uint16_t left = total_pad_w / 2;
        uint16_t right = total_pad_w - left;
        uint16_t top = total_pad_h / 2;
        uint16_t bottom = total_pad_h - top;

        cv::Mat temp;
        GstMapInfo map;
        if (converted) {
            temp = cv::Mat(newHeight, newWidth, CV_8UC3,
                           element->_convert_frame[frame_meta->_stream_id]);
        } else if (resized) {
            temp = cv::Mat(newHeight, newWidth, CV_8UC3,
                           element->_resized_frame[frame_meta->_stream_id]);
        } else if (cropped) {
            temp = cv::Mat(height, width, CV_8UC3,
                           element->_crop_frame[frame_meta->_stream_id]);
        } else {
            if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
                g_error("Failed to map GstBuffer\n");
                return false;
            }
            temp = cv::Mat(height, width, CV_8UC3, map.data);
        }
        
        cv::Mat resizedFrame(element->_resize_height, element->_resize_width, CV_8UC3, output);
        
        if (top + bottom + left + right != 0) {
            cv::copyMakeBorder(
                temp, resizedFrame, top, bottom, left, right, cv::BORDER_CONSTANT,
                cv::Scalar(element->_pad_value, element->_pad_value, element->_pad_value));
        } else {
            temp.copyTo(resizedFrame);
        }
        
        temp.release();
        if (!cropped && !resized && !converted) {
            gst_buffer_unmap(buf, &map);
        }
    } else {
        if (converted) {
            memcpy(output, element->_convert_frame[frame_meta->_stream_id],
                    newWidth * newHeight * 3);
        } else if (resized) {
            memcpy(output, element->_resized_frame[frame_meta->_stream_id],
                    newWidth * newHeight * 3);
        } else if (cropped) {
            memcpy(output, element->_crop_frame[frame_meta->_stream_id],
                    width * height * 3);
        } else {
            GstMapInfo map;
            if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
                g_error("Failed to map GstBuffer\n");
                return false;
            }
            memcpy(output, map.data, width * height * 3);
            gst_buffer_unmap(buf, &map);
        }
    }
    return true;
}
