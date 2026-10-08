// Caps: sink=H.264/H.265 byte-stream AU / src=host-memory NV12
// L1: automatic GstVideoDecoder handling
// L2: none
// LATENCY: no fixed element latency
#ifndef GST_DXVNPUDEC_H
#define GST_DXVNPUDEC_H

#include <gst/gst.h>
#include <gst/video/gstvideodecoder.h>
#include <dxvnpu/dxvnpu_c_api.h>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUDEC (gst_dxvnpudec_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuDec, gst_dxvnpudec, GST, DXVNPUDEC, GstVideoDecoder)

struct _GstDxVnpuDec {
    GstVideoDecoder parent;

    dxvnpu_pipeline_t pipeline;
    dxvnpu_codec_t codec;
    gint device_id;
    gint coded_width;
    gint coded_height;
    gboolean output_configured;
    gint flushing;
    gboolean eos_sent;
    GMutex sdk_lock;
    GMutex sdk_io_lock;
    GCond sdk_idle;
    guint sdk_calls;
    GMutex output_lock;
    GCond output_ready;
    GstFlowReturn output_flow;
    gboolean output_eos;
};

G_END_DECLS

#endif
