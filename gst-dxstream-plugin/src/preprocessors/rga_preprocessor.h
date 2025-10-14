#pragma once

#include "preprocessor.h"

#ifdef HAVE_LIBRGA
#include "rga/RgaUtils.h"
#include "rga/im2d.hpp"

class RgaPreprocessor : public Preprocessor {
public:
    RgaPreprocessor(GstDxPreprocess *elem);
    ~RgaPreprocessor();

    bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, void *output, cv::Rect *roi) override;

private:
    bool calculate_nv12_strides_short(int w, int h, int wa, int ha, int *ws, int *hs);
};

#endif // HAVE_LIBRGA