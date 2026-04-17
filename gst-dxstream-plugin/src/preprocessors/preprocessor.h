#pragma once

#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include "../transforms/video_transform_kernel.hpp"
#include <memory>

// Forward declarations to avoid circular includes
struct _GstDxPreprocess;
using GstDxPreprocess = struct _GstDxPreprocess;

struct _DXFrameMeta;
using DXFrameMeta = struct _DXFrameMeta;

struct _DXObjectMeta;
using DXObjectMeta = struct _DXObjectMeta;

class Preprocessor {
public:
    explicit Preprocessor(GstDxPreprocess *elem,
                          std::unique_ptr<dxt::IVideoTransformKernel> kernel);
    virtual ~Preprocessor() = default;

    bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta,
                    uint8_t *output, cv::Rect *roi);

    bool primary_process(GstBuffer* buf);
    bool secondary_process(GstBuffer* buf);

    GstBuffer* check_frame_meta(GstBuffer* buf);

    bool check_primary_interval(GstBuffer* buf);

protected:
    bool process_object(GstBuffer* buf, DXFrameMeta *frame_meta, DXObjectMeta *object_meta, const int &preprocess_id);
    void cleanup_temp_buffers(int stream_id);
    bool check_object(const DXFrameMeta *frame_meta, DXObjectMeta *object_meta);
    bool check_object_roi(const float *box, const int *roi) const;
    void transpose_hwc_to_chw(uint8_t* output, const uint8_t* input, guint channels, guint height, guint width) const;
    GstDxPreprocess* get_element() const { return element; }

private:
    GstDxPreprocess *element;
    std::unique_ptr<dxt::IVideoTransformKernel> kernel_;
};
