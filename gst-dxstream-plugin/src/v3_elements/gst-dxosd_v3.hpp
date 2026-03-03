#ifndef GST_DXOSD_V3_H
#define GST_DXOSD_V3_H

#include <cstdint>
#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_DXOSD_V3 (gst_dxosd_v3_get_type())
G_DECLARE_FINAL_TYPE(GstDxOsdV3, gst_dxosd_v3, GST, DXOSD_V3, GstBaseTransform)

struct _GstDxOsdV3 {
    GstBaseTransform parent_instance;
    
    // Video dimensions
    gint width;
    gint height;
    
    // FPS tracking
    gboolean enable_fps;
    guint64 total_frames;
    GstClockTime fps_update_time;
    gdouble measured_fps;
};

G_END_DECLS

#endif // GST_DXOSD_V3_H
