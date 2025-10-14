#ifndef GST_DXINPUTSELECTOR_H
#define GST_DXINPUTSELECTOR_H

#include <condition_variable>
#include <gst/gst.h>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <utility> // For std::pair
#include <vector>

G_BEGIN_DECLS

#define GST_TYPE_DXINPUTSELECTOR (gst_dxinputselector_get_type())
G_DECLARE_FINAL_TYPE(GstDxInputSelector, gst_dxinputselector, GST,
                     DXINPUTSELECTOR, GstElement)

struct _GstDxInputSelector {
    GstElement parent_instance;
    std::map<int, GstPad *> _sinkpads;
    GstPad *_srcpad;

    std::mutex _event_mutex;
    std::set<int> _stream_eos_arrived;
    std::set<int> _stream_eos_sent;

    GThread *_thread;
    bool _running;
    std::mutex _buffer_lock;
    std::condition_variable _aquire_cv;
    std::condition_variable _push_cv;
    std::map<int, std::queue<GstBuffer *>> _buffer_queue;

    // A min-heap to store pairs of (PTS, stream_id).
    // This allows finding the stream with the smallest PTS in O(log N) time.
    std::priority_queue<std::pair<GstClockTime, int>,
                        std::vector<std::pair<GstClockTime, int>>,
                        std::greater<std::pair<GstClockTime, int>>>
        _pts_heap;

    guint _max_queue_size;
};

G_END_DECLS

#endif // GST_DXINPUTSELECTOR_H
