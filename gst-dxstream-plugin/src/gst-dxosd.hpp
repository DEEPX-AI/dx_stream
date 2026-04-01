#ifndef GST_DXOSD_H
#define GST_DXOSD_H

#include <cstdint>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_DXOSD (gst_dxosd_get_type())
G_DECLARE_FINAL_TYPE(GstDxOsd, gst_dxosd, GST, DXOSD, GstBaseTransform)

struct _GstDxOsd {
    GstBaseTransform parent;
    // Stateless transform - all processing done in-place
};

G_END_DECLS

#endif // GST_DXOSD_H
