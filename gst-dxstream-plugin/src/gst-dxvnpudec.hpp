#ifndef GST_DXVNPUDEC_H
#define GST_DXVNPUDEC_H

#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include <dxvnpu/dxvnpu_api.h>
#include <memory>
#include <mutex>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUDEC (gst_dxvnpudec_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuDec, gst_dxvnpudec, GST, DXVNPUDEC, GstVideoDecoder)

struct _GstDxVnpuDec {
    GstVideoDecoder parent;

    std::shared_ptr<dxvnpu::MediaPipeline> pipeline;

    dxvnpu::VideoCodec codec;
    GstVideoCodecState *input_state;
    gboolean output_configured;

    // Output properties (Processor config)
    GstVideoFormat output_format;   // default NV12
    gint output_width;              // 0 = same as input
    gint output_height;             // 0 = same as input

    // HW pipeline tracking (streaming thread only)
    int hw_pending;
    GstFlowReturn last_flow;
};

G_END_DECLS

#endif // GST_DXVNPUDEC_H
