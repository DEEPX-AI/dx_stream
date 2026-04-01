#ifndef GST_DXSCALE_H
#define GST_DXSCALE_H

#include "dxcommon.hpp"
#include "video_transform_kernel.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_DXSCALE (gst_dxscale_get_type())
G_DECLARE_FINAL_TYPE(GstDxScale, gst_dxscale, GST, DXSCALE, GstBaseTransform)

struct _GstDxScale {
    GstBaseTransform _parent_instance;

    GstVideoInfo _input_info;
    GstVideoInfo _output_info;

    dxt::IVideoTransformKernel* _kernel;

    guint _width;
    guint _height;

    gboolean _negotiated;
};

G_END_DECLS

#endif // GST_DXSCALE_H
