#ifndef GST_DXCONVERT_H
#define GST_DXCONVERT_H

#include "dxcommon.hpp"
#include "video_transform_kernel.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_DXCONVERT (gst_dxconvert_get_type())
G_DECLARE_FINAL_TYPE(GstDxConvert, gst_dxconvert, GST, DXCONVERT,
                     GstBaseTransform)

struct _GstDxConvert {
    GstBaseTransform _parent_instance;

    GstVideoInfo _input_info;
    GstVideoInfo _output_info;

    dxt::IVideoTransformKernel* _kernel;

    gboolean _negotiated;
};

G_END_DECLS

#endif // GST_DXCONVERT_H
