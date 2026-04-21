#ifndef __GST_DXMSGMETA_H__
#define __GST_DXMSGMETA_H__

#include <gst/gst.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_DXMSG_META_API_TYPE (gst_dxmsg_meta_api_get_type())
#define GST_DXMSG_META_INFO (gst_dxmsg_meta_get_info())

struct _DxMsgPayload {
    gpointer _data;
    guint _size;
};

struct _DxMsgContext {
    gpointer _priv_data;
};

struct _GstDxMsgMetaInfo {
    gpointer _frame_meta;
    gpointer _input_info;
    gboolean _include_frame;

    guint64 _seq_id;

    const gchar *_frame_base64;
};

struct _GstDxMsgMeta {
    GstMeta meta;

    gpointer _payload;
};

using DxMsgPayload = struct _DxMsgPayload;
using DxMsgContext = struct _DxMsgContext;
using GstDxMsgMetaInfo = struct _GstDxMsgMetaInfo;
using GstDxMsgMeta = struct _GstDxMsgMeta;

GType gst_dxmsg_meta_api_get_type(void);

const GstMetaInfo *gst_dxmsg_meta_get_info(void);

GstBuffer*dx_create_msg_meta(GstBuffer *buffer);
GstDxMsgMeta *dx_get_msg_meta(GstBuffer *buffer);
void dx_add_payload_to_buffer(GstBuffer *buffer, const DxMsgPayload *payload);

G_END_DECLS

#endif /* __GST_DXMSGMETA_H__ */