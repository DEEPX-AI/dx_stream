#pragma once

#include "preprocessor.h"
#include <map>

class LibyuvPreprocessor : public Preprocessor {
public:
    LibyuvPreprocessor(GstDxPreprocess *elem);
    ~LibyuvPreprocessor();

    bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, void *output, cv::Rect *roi) override;

private:
    void check_temp_buffers(DXFrameMeta *frame_meta);
};