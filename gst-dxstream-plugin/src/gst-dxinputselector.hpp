#ifndef GST_DXINPUTSELECTOR_H
#define GST_DXINPUTSELECTOR_H

#include <gst/base/gstaggregator.h>
#include <gst/gst.h>
#include <set>

G_BEGIN_DECLS

#define GST_TYPE_DXINPUTSELECTOR (gst_dxinputselector_get_type())
G_DECLARE_FINAL_TYPE(GstDxInputSelector, gst_dxinputselector, GST,
                     DXINPUTSELECTOR, GstAggregator)

struct _GstDxInputSelector {
    GstAggregator parent_instance;
    std::set<int> _stream_eos_sent;
    guint _max_queue_size;
};

G_END_DECLS

#endif // GST_DXINPUTSELECTOR_H
