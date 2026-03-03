#include "libyuv_preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils/libyuv_transform/libyuv_transform.hpp"
#include <algorithm>
#include <iostream>

LibyuvPreprocessor::LibyuvPreprocessor(GstDxPreprocess *elem) : Preprocessor(elem) {
}

LibyuvPreprocessor::~LibyuvPreprocessor() = default;

void LibyuvPreprocessor::check_temp_buffers(const DXFrameMeta *frame_meta) const {
    // Vectors are automatically initialized as empty when accessed via []
    // No need to explicitly check and initialize to nullptr
    // Just ensure the keys exist in the maps
    if (get_element()->_buffers.crop.find(frame_meta->_stream_id) == get_element()->_buffers.crop.end()) {
        get_element()->_buffers.crop[frame_meta->_stream_id] = std::vector<uint8_t>();
    }

    if (get_element()->_buffers.convert.find(frame_meta->_stream_id) == get_element()->_buffers.convert.end()) {
        get_element()->_buffers.convert[frame_meta->_stream_id] = std::vector<uint8_t>();
    }

    if (get_element()->_buffers.resized.find(frame_meta->_stream_id) == get_element()->_buffers.resized.end()) {
        get_element()->_buffers.resized[frame_meta->_stream_id] = std::vector<uint8_t>();
    }
}

void LibyuvPreprocessor::crop(GstBuffer* buf, const DXFrameMeta *frame_meta, const cv::Rect *roi, Libyuv_Params& crop_params) const {
    if (roi->width != 0 && roi->height != 0) {
        if (roi->x < 0 || roi->y < 0 || 
            roi->x + roi->width > frame_meta->_width ||
            roi->y + roi->height > frame_meta->_height) {
            GST_ERROR_OBJECT(get_element(), "Invalid ROI: (%d,%d,%d,%d) for frame (%dx%d)", 
                              roi->x, roi->y, roi->width, roi->height,
                              frame_meta->_width, frame_meta->_height);
            crop_params.cropped = false;
            return;
        }
        
        Crop(buf, &get_element()->_stream.info[frame_meta->_stream_id],
             get_element()->_buffers.crop[frame_meta->_stream_id], frame_meta->_width,
             frame_meta->_height, roi->x, roi->y, roi->width, roi->height,
             frame_meta->_format.c_str());
        crop_params.width = roi->width;
        crop_params.height = roi->height;
        crop_params.cropped = true;
        return;
    }
    crop_params.cropped = false;
}

void LibyuvPreprocessor::resize(GstBuffer* buf, const DXFrameMeta *frame_meta, Libyuv_Params& resize_params) const {
    if (get_element()->_preprocess.keep_ratio) {
        float ratioDest = (float)get_element()->_preprocess.width / (float)get_element()->_preprocess.height;
        float ratioSrc = (float)resize_params.width / (float)resize_params.height;
        if (ratioSrc < ratioDest) {
            resize_params.newHeight = get_element()->_preprocess.height;
            resize_params.newWidth = static_cast<int>((float)resize_params.newHeight * ratioSrc);
        } else {
            resize_params.newWidth = get_element()->_preprocess.width;
            resize_params.newHeight = static_cast<int>((float)resize_params.newWidth / ratioSrc);
        }
    }

    if (resize_params.newWidth != resize_params.width || resize_params.newHeight != resize_params.height) {
        if (resize_params.cropped) {
            Resize(get_element()->_buffers.crop[frame_meta->_stream_id],
                   get_element()->_buffers.resized[frame_meta->_stream_id], resize_params.width,
                   resize_params.height, resize_params.newWidth, resize_params.newHeight, frame_meta->_format.c_str());
        } else {
            Resize(buf, &get_element()->_stream.info[frame_meta->_stream_id],
                   get_element()->_buffers.resized[frame_meta->_stream_id], resize_params.width, resize_params.height,
                   resize_params.newWidth, resize_params.newHeight, frame_meta->_format.c_str());
        }
        resize_params.resized = true;
        return;
    }
    resize_params.resized = false;
}

void LibyuvPreprocessor::color_convert(GstBuffer* buf, const DXFrameMeta *frame_meta, Libyuv_Params& libyuv_params) const {
    if (g_strcmp0(frame_meta->_format.c_str(), get_element()->_preprocess.color_format) != 0) {
        if (libyuv_params.resized) {
            CvtColor(get_element()->_buffers.resized[frame_meta->_stream_id],
                    get_element()->_buffers.convert[frame_meta->_stream_id], libyuv_params.newWidth,
                    libyuv_params.newHeight, frame_meta->_format.c_str(), get_element()->_preprocess.color_format);
        } else if (libyuv_params.cropped) {
            CvtColor(get_element()->_buffers.crop[frame_meta->_stream_id],
                    get_element()->_buffers.convert[frame_meta->_stream_id], libyuv_params.width,
                    libyuv_params.height, frame_meta->_format.c_str(), get_element()->_preprocess.color_format);
        } else {
            CvtColor(buf, &get_element()->_stream.info[frame_meta->_stream_id],
                    get_element()->_buffers.convert[frame_meta->_stream_id], libyuv_params.width, libyuv_params.height, 
                    frame_meta->_format.c_str(), get_element()->_preprocess.color_format);
        }
        libyuv_params.converted = true;
        return;
    }
    libyuv_params.converted = false;
}

bool LibyuvPreprocessor::preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, uint8_t *output, cv::Rect *roi) {
    params.width = frame_meta->_width;
    params.height = frame_meta->_height;

    crop(buf, frame_meta, roi, params);

    params.newWidth = get_element()->_preprocess.width;
    params.newHeight = get_element()->_preprocess.height;

    resize(buf, frame_meta, params);

    color_convert(buf, frame_meta, params);

    if (get_element()->_preprocess.keep_ratio) {
        int total_pad_w = get_element()->_preprocess.width - params.newWidth;
        int total_pad_h = get_element()->_preprocess.height - params.newHeight;
        
        auto left = total_pad_w / 2;
        auto right = total_pad_w - left;
        auto top = total_pad_h / 2;
        auto bottom = total_pad_h - top;

        cv::Mat temp;
        GstMapInfo map;
        if (params.converted) {
            temp = cv::Mat(params.newHeight, params.newWidth, CV_8UC3,
                           get_element()->_buffers.convert[frame_meta->_stream_id].data());
        } else if (params.resized) {
            temp = cv::Mat(params.newHeight, params.newWidth, CV_8UC3,
                           get_element()->_buffers.resized[frame_meta->_stream_id].data());
        } else if (params.cropped) {
            temp = cv::Mat(params.height, params.width, CV_8UC3,
                           get_element()->_buffers.crop[frame_meta->_stream_id].data());
        } else {
            if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
                g_error("Failed to map GstBuffer\n");
                return false;
            }
            temp = cv::Mat(params.height, params.width, CV_8UC3, map.data);
        }
        
        cv::Mat resizedFrame(get_element()->_preprocess.height, get_element()->_preprocess.width, CV_8UC3, output);
        
        if (top + bottom + left + right != 0) {
            cv::copyMakeBorder(
                temp, resizedFrame, top, bottom, left, right, cv::BORDER_CONSTANT,
                cv::Scalar(get_element()->_preprocess.pad_value, get_element()->_preprocess.pad_value, get_element()->_preprocess.pad_value));
        } else {
            temp.copyTo(resizedFrame);
        }
        
        temp.release();
        if (!params.cropped && !params.resized && !params.converted) {
            gst_buffer_unmap(buf, &map);
        }
    } else {
        if (params.converted) {
            memcpy(output, get_element()->_buffers.convert[frame_meta->_stream_id].data(),
                    params.newWidth * params.newHeight * 3);
        } else if (params.resized) {
            memcpy(output, get_element()->_buffers.resized[frame_meta->_stream_id].data(),
                    params.newWidth * params.newHeight * 3);
        } else if (params.cropped) {
            memcpy(output, get_element()->_buffers.crop[frame_meta->_stream_id].data(),
                    params.height * params.width * 3);
        } else {
            GstMapInfo map;
            if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
                g_error("Failed to map GstBuffer\n");
                return false;
            }
            memcpy(output, map.data, params.height * params.width * 3);
            gst_buffer_unmap(buf, &map);
        }
    }
    return true;
}
