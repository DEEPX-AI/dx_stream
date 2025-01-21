#ifndef GST_DXOSD_H
#define GST_DXOSD_H

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXOSD (gst_dxosd_get_type())
G_DECLARE_FINAL_TYPE(GstDxOsd, gst_dxosd, GST, DXOSD, GstBaseTransform)

struct _GstDxOsd {
    GstBaseTransform _parent_instance;
};

G_END_DECLS

#endif // GST_DXOSD_H
