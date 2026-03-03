#ifndef GST_DXVIDEOCONVERT_H
#define GST_DXVIDEOCONVERT_H

#include "dxcommon.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_DXVIDEOCONVERT (gst_dxvideoconvert_get_type())
G_DECLARE_FINAL_TYPE(GstDxVideoConvert, gst_dxvideoconvert, GST, DXVIDEOCONVERT,
                     GstBaseTransform)

struct _GstDxVideoConvert {
    GstBaseTransform _parent_instance;
    
    GstVideoInfo _input_info;
    GstVideoInfo _output_info;

    uint8_t* _input_buffer {nullptr};
    uint8_t* _output_buffer {nullptr};

    gboolean _negotiated;
};

G_END_DECLS

#endif // GST_DXVIDEOCONVERT_H
