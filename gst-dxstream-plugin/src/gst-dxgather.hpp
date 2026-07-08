#ifndef GST_DXGATHER_H
#define GST_DXGATHER_H

#include <gst/base/gstaggregator.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXGATHER (gst_dxgather_get_type())
G_DECLARE_FINAL_TYPE(GstDxGather, gst_dxgather, GST, DXGATHER, GstAggregator)

struct _GstDxGather {
    GstAggregator parent_instance;
};

G_END_DECLS

#endif /* GST_DXGATHER_H */
