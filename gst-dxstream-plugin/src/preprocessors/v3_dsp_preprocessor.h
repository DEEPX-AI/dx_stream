#pragma once

#include "preprocessor.h"
#include <map>

class V3DspPreprocessor : public Preprocessor {
public:
    explicit V3DspPreprocessor(GstDxPreprocess *elem);
    ~V3DspPreprocessor() override;

    bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, uint8_t *output, cv::Rect *roi) override;

private:
    void check_temp_buffers(DXFrameMeta *frame_meta) const;
    
    // Helper functions for preprocessing pipeline
    bool crop_image(const uint8_t* input, uint8_t* output, DXFrameMeta* frame_meta, 
                   const cv::Rect* roi, int src_width, int src_height) const;
    void calculate_aspect_ratio_size(int src_width, int src_height,
                                     int& target_width, int& target_height) const;
    bool resize_image(const uint8_t* input, uint8_t* output, DXFrameMeta* frame_meta,
                     int src_width, int src_height, int dst_width, int dst_height,
                     bool is_contiguous) const;
    bool convert_color(const uint8_t* input, uint8_t* output, DXFrameMeta* frame_meta,
                      int width, int height, bool is_contiguous) const;
    bool apply_padding(const uint8_t* input, uint8_t* output, int content_width, int content_height) const;

    uint8_t* _buffer_0 {nullptr};
    uint8_t* _buffer_1 {nullptr};
};