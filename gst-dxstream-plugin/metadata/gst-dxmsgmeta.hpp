#ifndef __GST_DXMSGMETA_H__
#define __GST_DXMSGMETA_H__

#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _DxMsgPayload {
    gpointer _data;
    guint _size;
} DxMsgPayload;

typedef struct _DxMsgContext {
    gpointer _priv_data; // DxMsgContextPriv
} DxMsgContext;

typedef struct _DxMsgMetaInfo {
    gpointer _frame_meta;

    gboolean _include_frame;

    /** debug purpose */
    guint64 _seq_id;
} DxMsgMetaInfo;
typedef struct _GstDxMsgMeta {
    GstMeta meta;

    gpointer _payload;
} GstDxMsgMeta;

GType gst_dxmsg_meta_api_get_type(void);

const GstMetaInfo *gst_dxmsg_meta_get_info(void);

#define GST_DXMSG_META_API_TYPE (gst_dxmsg_meta_api_get_type())
#define GST_DXMSG_META_INFO (gst_dxmsg_meta_get_info())

GstDxMsgMeta *gst_buffer_add_dxmsg_meta(GstBuffer *buffer,
                                        DxMsgPayload *payload);

G_END_DECLS

#endif /* __GST_DXMSGMETA_H__ */