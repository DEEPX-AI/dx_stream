#ifndef GST_DXGENOBJ_H
#define GST_DXGENOBJ_H

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXGENOBJ (gst_dxgenobj_get_type())
G_DECLARE_FINAL_TYPE(GstDxGenObj, gst_dxgenobj, GST, DXGENOBJ, GstBaseTransform)

struct _GstDxGenObj {
    GstBaseTransform _parent_instance;

    gboolean _box;
    gboolean _face_box;
    gboolean _label;
    gboolean _confidence;
    gboolean _track_id;
};

G_END_DECLS

#endif // GST_DXGENOBJ_H
