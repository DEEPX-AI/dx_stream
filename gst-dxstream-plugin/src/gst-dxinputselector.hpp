#ifndef GST_DXINPUTSELECTOR_H
#define GST_DXINPUTSELECTOR_H

#include <condition_variable>
#include <gst/gst.h>
#include <map>
#include <mutex>
#include <queue>
#include <vector>
#include <set>

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
    bool _global_eos;

    GThread *_thread;
    bool _running;
    std::mutex _buffer_lock;
    std::condition_variable _aquire_cv;
    std::condition_variable _push_cv;
    std::map<int, GstBuffer *> _buffer_queue;
};

G_END_DECLS

#endif // GST_DXINPUTSELECTOR_H