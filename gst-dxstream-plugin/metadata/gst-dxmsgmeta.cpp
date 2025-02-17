#include "gst-dxmsgmeta.hpp"

GType gst_dxmsg_meta_api_get_type(void) {
    static GType type;
    static const gchar *tags[] = {"gst_dxmsg_meta", NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("GstDxMsgMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

static gboolean gst_dxmsg_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer) {
    GstDxMsgMeta *dxmsg_meta = (GstDxMsgMeta *)meta;
    dxmsg_meta->_payload = 0;
    return TRUE;
}

static void gst_dxmsg_meta_free(GstMeta *meta, GstBuffer *buffer) {
    GstDxMsgMeta *dxmsg_meta = (GstDxMsgMeta *)meta;
    DxMsgPayload *payload = (DxMsgPayload *)dxmsg_meta->_payload;

    if (payload) {
        // GST_INFO("|JCP-M| gst_dxmsg_meta_free: payload=%p,
        // payload->_data=%p\n",
        //          payload, payload->_data);

        g_free(payload->_data);
        g_free(payload);
        payload = NULL;
    }
}

const GstMetaInfo *gst_dxmsg_meta_get_info(void) {
    static const GstMetaInfo *info = NULL;

    if (g_once_init_enter(&info)) {
        const GstMetaInfo *meta_info = gst_meta_register(
            GST_DXMSG_META_API_TYPE, "GstDxMsgMeta", sizeof(GstDxMsgMeta),
            (GstMetaInitFunction)gst_dxmsg_meta_init,
            (GstMetaFreeFunction)gst_dxmsg_meta_free,
            (GstMetaTransformFunction)NULL);
        g_once_init_leave(&info, meta_info);
    }
    return info;
}

void gst_buffer_add_dxmsg_meta(GstBuffer *buffer, DxMsgPayload *payload) {
    GstDxMsgMeta *dxmsg_meta =
        (GstDxMsgMeta *)gst_buffer_add_meta(buffer, GST_DXMSG_META_INFO, NULL);

#if 0 /** create payload in msg converter layer */
    DxMsgPayload *msgPayload = g_new0(DxMsgPayload, 1);
    msgPayload->_data = g_memdup(payload->_data, payload->_size);
    msgPayload->_size = payload->_size;

    dxmsg_meta->_payload = (gpointer)msgPayload;
#else
    dxmsg_meta->_payload = (gpointer)payload;
#endif
    // GST_INFO(
    //     "|JCP-M| gst_buffer_add_dxmsg_meta: payload=%p, payload->_data=%p\n",
    //     dxmsg_meta->_payload, ((DxMsgPayload *)dxmsg_meta->_payload)->_data);
    // return dxmsg_meta;
}
