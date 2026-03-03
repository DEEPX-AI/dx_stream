#include "v3_dsp_preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "../metadata/gst-dxframemeta.hpp"
#include "../metadata/gst-dxobjectmeta.hpp"
#include "dxcv/dxcvext.hpp"
#include <algorithm>

V3DspPreprocessor::V3DspPreprocessor(GstDxPreprocess *elem) : Preprocessor(elem) {
    _buffer_0 = dxcvext::allocDspBuffer();
    _buffer_1 = _buffer_0 + 0xC00000; // 12MB offset
}

V3DspPreprocessor::~V3DspPreprocessor() {
    if (_buffer_0) {
        dxcvext::freeDspBuffer(_buffer_0);
        _buffer_0 = nullptr;
        _buffer_1 = nullptr;
    }
}

bool V3DspPreprocessor::preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, uint8_t *output, cv::Rect *roi) {
    // Copy input buffer to temporary buffer
    GstMapInfo in_map = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(buf, &in_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(get_element(), "Failed to map input buffer");
        return false;
    }
    memcpy(_buffer_0, in_map.data, in_map.size);
    gst_buffer_unmap(buf, &in_map);

    // Initialize processing state
    uint8_t* current_buffer = _buffer_0;
    uint8_t* work_buffer = _buffer_1;
    int current_width = frame_meta->_width;
    int current_height = frame_meta->_height;
    bool is_contiguous = false;  // Track if data is in contiguous memory

    // Step 1: Crop if needed
    if (roi->width != 0 && roi->height != 0) {
        if (!crop_image(current_buffer, work_buffer, frame_meta, roi, 
                       current_width, current_height)) {
            return false;
        }
        std::swap(current_buffer, work_buffer);
        current_width = roi->width;
        current_height = roi->height;
        is_contiguous = true;
    }

    // Step 2: Calculate target size with aspect ratio
    int target_width = get_element()->_preprocess.width;
    int target_height = get_element()->_preprocess.height;
    if (get_element()->_preprocess.keep_ratio) {
        calculate_aspect_ratio_size(current_width, current_height, 
                                    target_width, target_height);
    }

    // Step 3: Resize if needed
    if (target_width != current_width || target_height != current_height) {
        if (!resize_image(current_buffer, work_buffer, frame_meta, 
                         current_width, current_height, target_width, target_height,
                         is_contiguous)) {
            return false;
        }
        std::swap(current_buffer, work_buffer);
        current_width = target_width;
        current_height = target_height;
        is_contiguous = true;
    }

    // Step 4: Color convert if needed
    if (g_strcmp0(frame_meta->_format.c_str(), get_element()->_preprocess.color_format) != 0) {
        if (!convert_color(current_buffer, work_buffer, frame_meta, 
                          current_width, current_height, is_contiguous)) {
            return false;
        }
        std::swap(current_buffer, work_buffer);
    }

    // Step 5: Apply padding and copy to output
    if (get_element()->_preprocess.keep_ratio) {
        return apply_padding(current_buffer, output, 
                           current_width, current_height);
    } else {
        memcpy(output, current_buffer, 
               get_element()->_preprocess.width * get_element()->_preprocess.height * get_element()->_preprocess.channel);
        return true;
    }
}

bool V3DspPreprocessor::crop_image(const uint8_t* input, uint8_t* output, 
                                   DXFrameMeta* frame_meta, const cv::Rect* roi,
                                   int src_width, int src_height) const {
    if (roi->x < 0 || roi->y < 0 || 
        roi->x + roi->width > src_width ||
        roi->y + roi->height > src_height) {
        GST_ERROR_OBJECT(get_element(), "Invalid ROI: (%d,%d,%d,%d) for frame (%dx%d)", 
                        roi->x, roi->y, roi->width, roi->height, src_width, src_height);
        return false;
    }

    if (g_strcmp0(frame_meta->_format.c_str(), "I420") == 0) {
        const uint8_t *src_y = input + get_element()->_stream.info[frame_meta->_stream_id].offset[0];
        const uint8_t *src_u = input + get_element()->_stream.info[frame_meta->_stream_id].offset[1];
        const uint8_t *src_v = input + get_element()->_stream.info[frame_meta->_stream_id].offset[2];
        int src_stride_y = get_element()->_stream.info[frame_meta->_stream_id].stride[0];
        int src_stride_u = get_element()->_stream.info[frame_meta->_stream_id].stride[1];
        int src_stride_v = get_element()->_stream.info[frame_meta->_stream_id].stride[2];

        uint8_t *dst_y = output;
        uint8_t *dst_u = dst_y + roi->width * roi->height;
        uint8_t *dst_v = dst_u + (roi->width / 2) * (roi->height / 2);

        dxcvext::I420Crop(const_cast<uint8_t*>(src_y), src_stride_y, const_cast<uint8_t*>(src_u), src_stride_u, const_cast<uint8_t*>(src_v), src_stride_v,
                         dst_y, roi->width, dst_u, roi->width / 2, dst_v, roi->width / 2,
                         src_width, src_height, roi->width, roi->height, roi->x, roi->y);
    } else if (g_strcmp0(frame_meta->_format.c_str(), "RGB") == 0 || 
               g_strcmp0(frame_meta->_format.c_str(), "BGR") == 0) {
        dxcvext::RGBCrop(const_cast<uint8_t*>(input), src_width * 3, output, roi->width * 3,
                        src_width, src_height, roi->width, roi->height, roi->x, roi->y);
    } else {
        GST_ERROR_OBJECT(get_element(), "Unsupported format for cropping: %s", frame_meta->_format.c_str());
        return false;
    }
    return true;
}

void V3DspPreprocessor::calculate_aspect_ratio_size(int src_width, int src_height,
                                                    int& target_width, int& target_height) const {
    float ratio_dest = (float)target_width / target_height;
    float ratio_src = (float)src_width / src_height;
    
    if (ratio_src < ratio_dest) {
        target_width = target_height * ratio_src;
    } else {
        target_height = target_width / ratio_src;
    }
}

bool V3DspPreprocessor::resize_image(const uint8_t* input, uint8_t* output, 
                                     DXFrameMeta* frame_meta,
                                     int src_width, int src_height,
                                     int dst_width, int dst_height,
                                     bool is_contiguous) const {
    if (g_strcmp0(frame_meta->_format.c_str(), "I420") == 0) {
        const uint8_t *src_y;
        const uint8_t *src_u;
        const uint8_t *src_v;
        int src_stride_y;
        int src_stride_u;
        int src_stride_v;

        if (is_contiguous) {
            // Data from crop or previous operation - contiguous memory
            src_y = input;
            src_u = src_y + src_width * src_height;
            src_v = src_u + (src_width / 2) * (src_height / 2);
            src_stride_y = src_width;
            src_stride_u = src_width / 2;
            src_stride_v = src_width / 2;
        } else {
            // Original data - use offset and stride
            src_y = input + get_element()->_stream.info[frame_meta->_stream_id].offset[0];
            src_u = input + get_element()->_stream.info[frame_meta->_stream_id].offset[1];
            src_v = input + get_element()->_stream.info[frame_meta->_stream_id].offset[2];
            src_stride_y = get_element()->_stream.info[frame_meta->_stream_id].stride[0];
            src_stride_u = get_element()->_stream.info[frame_meta->_stream_id].stride[1];
            src_stride_v = get_element()->_stream.info[frame_meta->_stream_id].stride[2];
        }

        uint8_t *dst_y = output;
        uint8_t *dst_u = dst_y + dst_width * dst_height;
        uint8_t *dst_v = dst_u + (dst_width / 2) * (dst_height / 2);

        dxcvext::I420Scale(const_cast<uint8_t*>(src_y), src_stride_y, const_cast<uint8_t*>(src_u), src_stride_u, const_cast<uint8_t*>(src_v), src_stride_v,
                          src_width, src_height,
                          dst_y, dst_width, dst_u, dst_width / 2, dst_v, dst_width / 2,
                          dst_width, dst_height);
    } else if (g_strcmp0(frame_meta->_format.c_str(), "RGB") == 0) {
        dxcvext::RGB24Scale(const_cast<uint8_t*>(input), src_width * 3, src_width, src_height,
                           output, dst_width * 3, dst_width, dst_height);
    } else if (g_strcmp0(frame_meta->_format.c_str(), "BGR") == 0) {
        dxcvext::BGR24Scale(const_cast<uint8_t*>(input), src_width * 3, src_width, src_height,
                           output, dst_width * 3, dst_width, dst_height);
    } else {
        GST_ERROR_OBJECT(get_element(), "Unsupported format for resizing: %s", frame_meta->_format.c_str());
        return false;
    }
    return true;
}

bool V3DspPreprocessor::convert_color(const uint8_t* input, uint8_t* output,
                                      DXFrameMeta* frame_meta,
                                      int width, int height,
                                      bool is_contiguous) const {
    if (g_strcmp0(frame_meta->_format.c_str(), "I420") == 0 && 
        (g_strcmp0(get_element()->_preprocess.color_format, "RGB") == 0 || 
         g_strcmp0(get_element()->_preprocess.color_format, "BGR") == 0)) {
        
        const uint8_t *src_y;
        const uint8_t *src_u;
        const uint8_t *src_v;
        int src_stride_y;
        int src_stride_u;
        int src_stride_v;

        if (is_contiguous) {
            // Data from crop/resize - contiguous memory
            src_y = input;
            src_u = src_y + width * height;
            src_v = src_u + (width / 2) * (height / 2);
            src_stride_y = width;
            src_stride_u = width / 2;
            src_stride_v = width / 2;
        } else {
            // Original data - use offset and stride
            src_y = input + get_element()->_stream.info[frame_meta->_stream_id].offset[0];
            src_u = input + get_element()->_stream.info[frame_meta->_stream_id].offset[1];
            src_v = input + get_element()->_stream.info[frame_meta->_stream_id].offset[2];
            src_stride_y = get_element()->_stream.info[frame_meta->_stream_id].stride[0];
            src_stride_u = get_element()->_stream.info[frame_meta->_stream_id].stride[1];
            src_stride_v = get_element()->_stream.info[frame_meta->_stream_id].stride[2];
        }

        if (g_strcmp0(get_element()->_preprocess.color_format, "RGB") == 0) {
            dxcvext::I420ToRGB24(const_cast<uint8_t*>(src_y), src_stride_y, const_cast<uint8_t*>(src_u), src_stride_u, const_cast<uint8_t*>(src_v), src_stride_v,
                                output, width * 3, width, height);
        } else {
            dxcvext::I420ToBGR24(const_cast<uint8_t*>(src_y), src_stride_y, const_cast<uint8_t*>(src_u), src_stride_u, const_cast<uint8_t*>(src_v), src_stride_v,
                                output, width * 3, width, height);
        }
    } else {
        GST_ERROR_OBJECT(get_element(), "Unsupported format conversion: %s to %s", 
                        frame_meta->_format.c_str(), get_element()->_preprocess.color_format);
        return false;
    }
    return true;
}

bool V3DspPreprocessor::apply_padding(const uint8_t* input, uint8_t* output,
                                      int content_width, int content_height) const {
    int total_pad_w = get_element()->_preprocess.width - content_width;
    int total_pad_h = get_element()->_preprocess.height - content_height;
    
    if (total_pad_w == 0 && total_pad_h == 0) {
        // No padding needed
        memcpy(output, input, content_width * content_height * get_element()->_preprocess.channel);
        return true;
    }

    if (g_strcmp0(get_element()->_preprocess.color_format, "RGB") != 0 && 
        g_strcmp0(get_element()->_preprocess.color_format, "BGR") != 0) {
        GST_ERROR_OBJECT(get_element(), "Padding is only supported for RGB/BGR format");
        return false;
    }

    uint16_t left = total_pad_w / 2;
    uint16_t top = total_pad_h / 2;

    // Fill entire output with padding value
    memset(output, get_element()->_preprocess.pad_value, 
           get_element()->_preprocess.width * get_element()->_preprocess.height * 3);

    // Copy content to center
    for (int row = 0; row < content_height; row++) {
        const uint8_t* src_ptr = input + row * content_width * 3;
        uint8_t* dst_ptr = output + ((row + top) * get_element()->_preprocess.width + left) * 3;
        memcpy(dst_ptr, src_ptr, content_width * 3);
    }

    return true;
}
