#pragma once

#include <opencv2/opencv.hpp>
#include <gst/gst.h>

// Forward declarations to avoid circular includes
struct _GstDxPreprocess;
typedef struct _GstDxPreprocess GstDxPreprocess;

struct _DXFrameMeta;
typedef struct _DXFrameMeta DXFrameMeta;

struct _DXObjectMeta;
typedef struct _DXObjectMeta DXObjectMeta;

class Preprocessor {
public:
    Preprocessor(GstDxPreprocess *elem) : element(elem) {}
    virtual ~Preprocessor() = default;

    virtual bool preprocess(GstBuffer* buf, DXFrameMeta *frame_meta, void *output, cv::Rect *roi) = 0;
    
    bool primary_process(GstBuffer* buf);
    bool secondary_process(GstBuffer* buf);

    void check_frame_meta(GstBuffer* buf);
    
    bool check_primary_interval(GstBuffer* buf);

protected:
    bool process_object(GstBuffer* buf, DXFrameMeta *frame_meta, DXObjectMeta *object_meta, int &preprocess_id);
    void cleanup_temp_buffers(int stream_id);
    bool check_object(DXFrameMeta *frame_meta, DXObjectMeta *object_meta);
    bool check_object_roi(float *box, int *roi);

    GstDxPreprocess *element;
};
