#ifndef GST_DXOUTPUTSELECTOR_H
#define GST_DXOUTPUTSELECTOR_H

#include <gst/gst.h>
#include <map>
#include <mutex>
#include <set>

G_BEGIN_DECLS

#define GST_TYPE_DXOUTPUTSELECTOR (gst_dxoutputselector_get_type())
G_DECLARE_FINAL_TYPE(GstDxOutputSelector, gst_dxoutputselector, GST,
                     DXOUTPUTSELECTOR, GstElement)

struct _GstDxOutputSelector {
    GstElement parent_instance;

    std::map<int, GstPad *> _srcpads;
    std::map<int, GstCaps *> _cached_caps_for_stream;
    GstPad *_sinkpad;

    std::set<int> _eos_stream_id;

    int _last_stream_id;
};

G_END_DECLS

#endif // GST_DXOUTPUTSELECTOR_H