#pragma once

#include "preprocessor.h"
#include <map>

class LibyuvPreprocessor : public Preprocessor {
public:
    explicit LibyuvPreprocessor(GstDxPreprocess *elem);
    ~LibyuvPreprocessor() override;

    bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, uint8_t *output, cv::Rect *roi) override;

private:
    struct Libyuv_Params {
        int width;
        int height;
        int newWidth;
        int newHeight;
        bool cropped;
        bool resized;
        bool converted;
    };
    
    Libyuv_Params params;
    
    void check_temp_buffers(const DXFrameMeta *frame_meta) const;

    void crop(GstBuffer* buf, const DXFrameMeta *frame_meta, const cv::Rect *roi, Libyuv_Params& crop_params) const;
    void resize(GstBuffer* buf, const DXFrameMeta *frame_meta, Libyuv_Params& resize_params) const;
    void color_convert(GstBuffer* buf, const DXFrameMeta *frame_meta, Libyuv_Params& convert_params) const;
};