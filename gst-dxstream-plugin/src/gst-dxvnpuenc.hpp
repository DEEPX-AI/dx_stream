#ifndef GST_DXVNPUENC_H
#define GST_DXVNPUENC_H

#include <gst/gst.h>
#include <gst/video/gstvideoencoder.h>
#include <dxvnpu/dxvnpu_api.h>
#include <memory>
#include <atomic>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUENC (gst_dxvnpuenc_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuEnc, gst_dxvnpuenc, GST, DXVNPUENC, GstVideoEncoder)

struct _GstDxVnpuEnc {
    GstVideoEncoder parent;

    std::shared_ptr<dxvnpu::VideoEncoder> encoder_module;

    dxvnpu::VideoCodec codec;
    guint bitrate;  // kbps
    GstVideoCodecState *input_state;

    std::atomic<int> hw_pending{0};     // PutFrame++, GetPacket--
};

G_END_DECLS

#endif // GST_DXVNPUENC_H
