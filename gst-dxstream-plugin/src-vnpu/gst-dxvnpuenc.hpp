#ifndef GST_DXVNPUENC_H
#define GST_DXVNPUENC_H

#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>
#include <dxvnpu/dxvnpu_c_api.h>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUENC (gst_dxvnpuenc_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuEnc, gst_dxvnpuenc, GST, DXVNPUENC, GstVideoEncoder)

struct _GstDxVnpuEnc {
    GstVideoEncoder parent;

    dxvnpu_pipeline_t encoder_pipeline;

    dxvnpu_codec_t codec;
    guint bitrate;
    gint device_id;
    GstVideoCodecState* input_state;

    gint hw_pending;
    gint flushing;
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
