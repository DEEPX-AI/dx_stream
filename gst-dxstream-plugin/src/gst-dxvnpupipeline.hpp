#ifndef GST_DXVNPUPIPELINE_H
#define GST_DXVNPUPIPELINE_H

#include <gst/gst.h>
#include <dxvnpu/dxvnpu_api.h>
#include "./../metadata/gst-dxframemeta.hpp"
#include <map>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUPIPELINE (gst_dxvnpupipeline_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuPipeline, gst_dxvnpupipeline, GST,
                     DXVNPUPIPELINE, GstElement)

struct DxVnpuChannelCtx {
    int channel_id{-1};
    GstPad *sinkpad{nullptr};
    GstPad *srcpad{nullptr};

    std::shared_ptr<dxvnpu::MediaPipeline> pipeline;
    dxvnpu::VideoCodec codec{dxvnpu::CODEC_H264};
    int input_width{0};
    int input_height{0};

    // Output thread
    std::unique_ptr<std::thread> output_thread;
    std::atomic<bool> output_running{false};
    std::atomic<GstFlowReturn> last_flow{GST_FLOW_OK};
    std::atomic<bool> flushing{false};

    // Diagnostics
    std::atomic<size_t> put_count{0};
    std::atomic<size_t> get_count{0};
};

struct _GstDxVnpuPipeline {
    GstElement parent_instance;

    // Per-channel state, keyed by pad index
    std::map<int, std::shared_ptr<DxVnpuChannelCtx>> channels;
    std::mutex channels_lock;

    // Properties
    gchar *model_path;
    guint inference_id;
    gboolean keep_ratio;
    gboolean use_ort;
    gint device_id;
    gint max_hdmi_channels;
    gboolean use_vnpu_hdmi;
};

G_END_DECLS

#endif // GST_DXVNPUPIPELINE_H
