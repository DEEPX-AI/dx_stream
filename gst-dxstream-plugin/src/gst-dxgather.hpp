#ifndef GST_DXGATHER_H
#define GST_DXGATHER_H

#include <condition_variable>
#include <gst/gst.h>
#include <map>
#include <mutex>

G_BEGIN_DECLS

#define GST_TYPE_DXGATHER (gst_dxgather_get_type())
G_DECLARE_FINAL_TYPE(GstDxGather, gst_dxgather, GST, DXGATHER, GstElement)

struct _GstDxGather {
    GstElement parent_instance;

    std::map<gint, GstPad *> _sinkpads;
    std::map<gint, GstBuffer *> _buffers;
    GstPad *_srcpad;

    std::mutex _mutex;
    std::condition_variable _cv;

    GThread *_thread;
    gboolean _running;
};

G_END_DECLS

#endif /* GST_DXGATHER_H */
