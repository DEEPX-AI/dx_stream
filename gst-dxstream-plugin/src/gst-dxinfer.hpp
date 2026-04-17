#ifndef GST_DXINFER_H
#define GST_DXINFER_H

#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
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

struct GstDxInferPushContext {
    GThread *push_thread;
    gboolean push_running;
    std::queue<std::pair<int, GstBuffer *>> push_queue;
    std::mutex push_lock;
    std::condition_variable cv;
};

struct GstDxInferEosContext {
    std::mutex eos_lock;
    bool global_eos;
    std::set<int> stream_eos_arrived;
    std::map<int, int> stream_pending_buffers;
};

struct GstDxInferTimingContext {
    gint64 avg_latency;
    GQueue *recent_latencies;
    GstClockTime prev_ts;
    GstClockTimeDiff throttling_delay;
    GstClockTimeDiff throttling_accum;
    GstClockTime qos_timestamp;
    GstClockTimeDiff qos_timediff;
};

struct _GstDxInfer {
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
    size_t _num_devices;
    size_t _output_tensor_size;
    int _last_req_id;

    GstDxInferPushContext _push_ctx;
    GstDxInferEosContext _eos_ctx;
    GstDxInferTimingContext _timing_ctx;
};

using GstDxInfer = struct _GstDxInfer;

G_END_DECLS

#endif // GST_DXINFER_H
