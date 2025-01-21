#ifndef GST_DXRATE_H
#define GST_DXRATE_H

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXRATE (gst_dxrate_get_type())
G_DECLARE_FINAL_TYPE(GstDxRate, gst_dxrate, GST, DXRATE, GstBaseTransform)

struct _GstDxRate {
    GstBaseTransform _parent_instance;

    GstBuffer *_prevbuf;      /**< previous buffer */
    GstSegment _segment;      /**< current segment */
    guint64 _out_frame_count; /**< number of frames output */

    guint _framerate; /**< framerate numerator (To) */

    /** Timestamp */
    guint64 _base_ts; /**< used in next_ts calculation */
    guint64 _prev_ts; /**< Previous buffer timestamp */
    guint64 _next_ts; /**< Timestamp of next buffer to output */
    guint64 _last_ts; /**< Timestamp of last input buffer */

    /** Properties */
    guint64 _out;
    gboolean _throttle; /**< throttle property */
};

G_END_DECLS

#endif // GST_DXRATE_H