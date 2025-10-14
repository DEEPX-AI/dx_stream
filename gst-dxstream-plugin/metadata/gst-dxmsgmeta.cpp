#include "gst-dxmsgmeta.hpp"

static gboolean gst_dxmsg_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer);
static void gst_dxmsg_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_dxmsg_meta_transform(GstBuffer *dest, GstMeta *meta,
                                       GstBuffer *buffer, GQuark type,
                                       gpointer data);

GType gst_dxmsg_meta_api_get_type(void) {
    static GType type;
    static const gchar *tags[] = {"gst_dxmsg_meta", nullptr};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("GstDxMsgMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

const GstMetaInfo *gst_dxmsg_meta_get_info(void) {
    static const GstMetaInfo *info = nullptr;

    if (g_once_init_enter(&info)) {
        const GstMetaInfo *meta_info = gst_meta_register(
            GST_DXMSG_META_API_TYPE, "GstDxMsgMeta", sizeof(GstDxMsgMeta),
            (GstMetaInitFunction)gst_dxmsg_meta_init,
            (GstMetaFreeFunction)gst_dxmsg_meta_free,
            (GstMetaTransformFunction)gst_dxmsg_meta_transform);
        g_once_init_leave(&info, meta_info);
    }
    return info;
}

static gboolean gst_dxmsg_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer) {
    GstDxMsgMeta *dxmsg_meta = (GstDxMsgMeta *)meta;
    dxmsg_meta->_payload = nullptr;
    return TRUE;
}

static void gst_dxmsg_meta_free(GstMeta *meta, GstBuffer *buffer) {
    GstDxMsgMeta *dxmsg_meta = (GstDxMsgMeta *)meta;
    DxMsgPayload *payload = (DxMsgPayload *)dxmsg_meta->_payload;

    if (payload) {
        g_free(payload->_data);
        g_free(payload);
        payload = nullptr;
    }
}

static gboolean gst_dxmsg_meta_transform(GstBuffer *dest, GstMeta *meta,
                                       GstBuffer *buffer, GQuark type,
                                       gpointer data) {
    GstDxMsgMeta *src_msg_meta = (GstDxMsgMeta *)meta;
    GstDxMsgMeta *exist_msg_meta = (GstDxMsgMeta *)gst_buffer_get_meta(dest, GST_DXMSG_META_API_TYPE);
    if (exist_msg_meta) {
        return FALSE;
    }
    GstDxMsgMeta *dst_msg_meta = (GstDxMsgMeta *)gst_buffer_add_meta(dest, GST_DXMSG_META_INFO, nullptr);
    
    DxMsgPayload *src_payload = (DxMsgPayload *)src_msg_meta->_payload;
    if (src_payload) {
        DxMsgPayload *dst_payload = g_new0(DxMsgPayload, 1);
        dst_payload->_data = g_memdup(src_payload->_data, src_payload->_size);
        dst_payload->_size = src_payload->_size;
        dst_msg_meta->_payload = (gpointer)dst_payload;
    } else {
        dst_msg_meta->_payload = nullptr;
    }
    return TRUE;
}
