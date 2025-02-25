#ifndef GST_DXINFER_H
#define GST_DXINFER_H

#include "gst-dxmeta.hpp"
#include "memory_pool.hpp"
#include <condition_variable>
#include <dxrt/dxrt_api.h>
#include <gst/gst.h>
#include <memory>
#include <mutex>
#include <queue>

G_BEGIN_DECLS

#define GST_TYPE_DXINFER (gst_dxinfer_get_type())
G_DECLARE_FINAL_TYPE(GstDxInfer, gst_dxinfer, GST, DXINFER, GstElement)

const int MAX_QUEUE_SIZE = 3;

typedef struct _GstDxInfer {
    GstElement _parent_instance;
    GstPad *_srcpad;

    guint _preproc_id;
    guint _infer_id;

    gboolean _secondary_mode;
    gchar *_model_path;
    gchar *_config_path;

    std::shared_ptr<dxrt::InferenceEngine> _ie;
    int _last_req_id;

    GThread *_thread;
    gboolean _running;
    std::queue<GstBuffer *> _buffer_queue;
    std::mutex _queue_lock;

    GThread *_push_thread;
    gboolean _push_running;
    std::vector<GstBuffer *> _push_queue;
    std::vector<GstBuffer *> _skip_queue;
    std::mutex _push_lock;

    std::condition_variable _cv;
    std::condition_variable _push_cv;

    MemoryPool _pool;
    guint _pool_size;

    gint64 _avg_latency;
    GQueue *_recent_latencies;

    GstClockTime _prev_ts;
    GstClockTimeDiff _throttling_delay;
    GstClockTimeDiff _throttling_accum;

    GstClockTime _qos_timestamp;
    GstClockTimeDiff _qos_timediff;

    guint _buffer_cnt;

    std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
    guint _frame_count_for_fps;

} GstDxInfer;

G_END_DECLS

#endif // GST_DXINFER_H
