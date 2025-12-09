#ifndef GST_DXINFER_H
#define GST_DXINFER_H

#include "gst-dxmeta.hpp"
#include <condition_variable>
#include <dxrt/dxrt_api.h>
#include <gst/gst.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

G_BEGIN_DECLS

#define GST_TYPE_DXINFER (gst_dxinfer_get_type())
G_DECLARE_FINAL_TYPE(GstDxInfer, gst_dxinfer, GST, DXINFER, GstElement)

const int MAX_PUSH_QUEUE_SIZE = 5;

typedef struct _GstDxInfer {
    GstElement _parent_instance;
    GstPad *_srcpad;

    guint _preproc_id;
    guint _infer_id;

    gboolean _secondary_mode;
    gboolean _use_ort;
    gchar *_model_path;
    gchar *_config_path;

    std::shared_ptr<dxrt::InferenceEngine> _ie;
    std::shared_ptr<dxrt::InferenceOption> _infer_option;
    int _last_req_id;

    int _output_tensor_size;

    GThread *_push_thread;
    gboolean _push_running;
    std::queue<std::pair<int, GstBuffer *>> _push_queue;
    std::mutex _push_lock;
    std::condition_variable _cv;

    std::mutex _eos_lock;
    bool _global_eos;
    std::set<int> _stream_eos_arrived;
    std::map<int, int> _stream_pending_buffers;

    gint64 _avg_latency;
    GQueue *_recent_latencies;

    GstClockTime _prev_ts;
    GstClockTimeDiff _throttling_delay;
    GstClockTimeDiff _throttling_accum;

    GstClockTime _qos_timestamp;
    GstClockTimeDiff _qos_timediff;
} GstDxInfer;

G_END_DECLS

#endif // GST_DXINFER_H
