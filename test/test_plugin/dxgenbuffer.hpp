#ifndef GST_DXGENBUFFER_H
#define GST_DXGENBUFFER_H

#include <gst/base/gstpushsrc.h>
#include <gst/gst.h>
#include <string>

G_BEGIN_DECLS

#define GST_TYPE_DXGENBUFFER (gst_dxgenbuffer_get_type())
G_DECLARE_FINAL_TYPE(GstDxGenBuffer, gst_dxgenbuffer, GST, DXGENBUFFER,
                     GstPushSrc)

struct _GstDxGenBuffer {
    GstPushSrc parent;
    gchar *image_path;
    guint framerate_n;
    guint framerate_d;
    GstClockTime frame_duration;
};

G_END_DECLS

#endif // GST_DXGENBUFFER_H
