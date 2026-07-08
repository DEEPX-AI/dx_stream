#ifndef GST_DXRATE_H
#define GST_DXRATE_H

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXRATE (gst_dxrate_get_type())
G_DECLARE_FINAL_TYPE(GstDxRate, gst_dxrate, GST, DXRATE, GstBaseTransform)

struct _GstDxRate {
    GstBaseTransform _parent_instance;

    /** Per-stream rate state map (opaque to header — placement-new in init,
     *  delete in finalize). Keyed by DXFrameMeta._stream_id (or 0 in
     *  NORMAL_MODE). Each entry holds prevbuf/segment/out_frame_count/
     *  base_ts/prev_ts/next_ts/last_ts. */
    gpointer _streams;

    /** Element-global monotonic output offset (BUFFER_OFFSET). */
    guint64 _out;

    guint _framerate; /**< framerate numerator (To) */

    /** Properties */
    gboolean _throttle;
};

G_END_DECLS

#endif // GST_DXRATE_H
