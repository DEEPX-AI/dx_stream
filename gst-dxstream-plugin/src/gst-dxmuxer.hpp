#ifndef GST_DXMUXER_H
#define GST_DXMUXER_H

#include <condition_variable>
#include <gst/gst.h>
#include <map>
#include <mutex>
#include <vector>

G_BEGIN_DECLS

#define GST_TYPE_DXMUXER (gst_dxmuxer_get_type())
G_DECLARE_FINAL_TYPE(GstDxMuxer, gst_dxmuxer, GST, DXMUXER, GstElement)

struct _GstDxMuxer {
    GstElement parent_instance;

    std::map<gint, GstPad *> _sinkpads;
    GstPad *_srcpad;

    gboolean _live_source;

    GThread *_thread;
    gboolean _running;

    GstClockTime *_pts;

    std::map<int, bool> _eos_list;

    std::map<int, std::mutex> _mutexes;
    std::condition_variable _cv;

    std::map<int, GstSegment> _segments;
    std::map<int, GstBuffer *> _buffers;
    std::map<int, GstCaps *> _caps;

    GstClockTime _qos_timestamp;
    GstClockTimeDiff _qos_timediff;
    GstClockTimeDiff _throttling_delay;
};

G_END_DECLS

#endif /* GST_DXMUXER_H */