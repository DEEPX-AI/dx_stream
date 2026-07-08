#ifndef GST_DXINFER_H
#define GST_DXINFER_H

#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "infer_backend/infer_backend_factory.hpp"
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <gst/gst.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

G_BEGIN_DECLS

#define GST_TYPE_DXINFER (gst_dxinfer_get_type())
G_DECLARE_FINAL_TYPE(GstDxInfer, gst_dxinfer, GST, DXINFER, GstElement)

struct GstDxInferPushEntry {
    bool submitted;
    GstBuffer *buffer;
};

// Lock ordering rule: push_lock may hold eos_lock (via cv predicate), but
// eos_lock must NEVER be held when acquiring push_lock.  Push thread acquires
// them sequentially (never nested), so no deadlock occurs.
struct GstDxInferPushContext {
    GThread *push_thread;
    std::atomic<gboolean> push_running;
    std::queue<GstDxInferPushEntry> push_queue;
    std::mutex push_lock;
    std::condition_variable cv;
};

struct GstDxInferEosContext {
    std::mutex eos_lock;
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
    guint64 throughput_count;
    std::chrono::steady_clock::time_point throughput_start;
};

struct _GstDxInfer {
    GstElement _parent_instance;
    GstPad *_sinkpad;
    GstPad *_srcpad;

    guint _preproc_id;
    guint _infer_id;

    gboolean _secondary_mode;
    gboolean _use_ort;
    gchar *_model_path;
    gchar *_config_path;
    BackendType _backend_type;

    std::unique_ptr<IInferBackend> _backend;
    size_t _output_tensor_size;

    GstDxInferPushContext _push_ctx;
    GstDxInferEosContext _eos_ctx;
    GstDxInferTimingContext _timing_ctx;
};

using GstDxInfer = struct _GstDxInfer;

G_END_DECLS

#endif // GST_DXINFER_H
