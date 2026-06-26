#include "gst-dxmsgmeta.hpp"
#include <tuple>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN(dxmeta_cat);

#define GST_CAT_DEBUG_SAFE(cat, ...) \
    G_STMT_START { \
        if (cat) { \
            gst_debug_log(cat, GST_LEVEL_DEBUG, __FILE__, GST_FUNCTION, __LINE__, NULL, __VA_ARGS__); \
        } \
    } G_STMT_END

#define GST_CAT_ERROR_SAFE(cat, ...) \
    G_STMT_START { \
        if (cat) { \
            gst_debug_log(cat, GST_LEVEL_ERROR, __FILE__, GST_FUNCTION, __LINE__, NULL, __VA_ARGS__); \
        } \
    } G_STMT_END

#define GST_CAT_WARNING_SAFE(cat, ...) \
    G_STMT_START { \
        if (cat) { \
            gst_debug_log(cat, GST_LEVEL_WARNING, __FILE__, GST_FUNCTION, __LINE__, NULL, __VA_ARGS__); \
        } \
    } G_STMT_END

static gboolean gst_dxmsg_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer);
static void gst_dxmsg_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_dxmsg_meta_transform(GstBuffer *dest, GstMeta *meta,
                                       GstBuffer *buffer, GQuark type,
                                       gpointer data);

GType gst_dxmsg_meta_api_get_type(void) {
    static GType type;

    if (g_once_init_enter(&type)) {
        static const gchar *tags[] = {"gst_dxmsg_meta", nullptr}; // NOSONAR - GStreamer API requires C-style array (const gchar**)
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
    std::ignore = params;
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Initializing GstDxMsgMeta");
    auto *dxmsg_meta = (GstDxMsgMeta *)meta;
    dxmsg_meta->_payload = nullptr;
    return TRUE;
}

static void gst_dxmsg_meta_free(GstMeta *meta, GstBuffer *buffer) {
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Freeing GstDxMsgMeta");
    auto *dxmsg_meta = (GstDxMsgMeta *)meta;
    auto *payload = (DxMsgPayload *)dxmsg_meta->_payload;

    if (payload) {
        GST_CAT_DEBUG_SAFE(dxmeta_cat, "Freeing payload data (size=%u)", payload->_size);
        g_free(payload->_data);
        g_free(payload);
        payload = nullptr;
    }
}

// NOSONAR - GStreamer GstMetaTransformFunction signature requires non-const GstBuffer* parameters
static gboolean gst_dxmsg_meta_transform(GstBuffer *dest, GstMeta *meta,
                                         GstBuffer *buffer, GQuark type,
                                         gpointer data) {
    std::ignore = type;
    std::ignore = data;
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Transforming GstDxMsgMeta");
    const auto *src_msg_meta = (const GstDxMsgMeta *)meta;
    const auto *exist_msg_meta = dx_get_msg_meta(dest);
    if (exist_msg_meta) {
        return FALSE;
    }
    dest = dx_create_msg_meta(dest);
    auto *dst_msg_meta = dx_get_msg_meta(dest);
    
    const auto *src_payload = (const DxMsgPayload *)src_msg_meta->_payload;
    if (src_payload) {
        auto *dst_payload = g_new0(DxMsgPayload, 1);
        dst_payload->_data = g_memdup(src_payload->_data, src_payload->_size);
        dst_payload->_size = src_payload->_size;
        dst_msg_meta->_payload = (gpointer)dst_payload;
    } else {
        dst_msg_meta->_payload = nullptr;
    }
    return TRUE;
}

GstBuffer *dx_create_msg_meta(GstBuffer *buffer) {
    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Creating GstDxMsgMeta");
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    gst_buffer_add_meta(buffer, GST_DXMSG_META_INFO, nullptr);
    return buffer;
}

GstDxMsgMeta *dx_get_msg_meta(GstBuffer *buffer) {
    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Getting GstDxMsgMeta");
    auto *msg_meta =
        (GstDxMsgMeta *)gst_buffer_get_meta(buffer, GST_DXMSG_META_API_TYPE);
    return msg_meta;
}

void dx_add_payload_to_buffer(GstBuffer *buffer, const DxMsgPayload *payload) {
    std::ignore = buffer;
    std::ignore = payload;
    
    GST_CAT_DEBUG_SAFE(dxmeta_cat, "Adding payload to buffer (size=%u)", payload->_size);
    buffer = dx_create_msg_meta(buffer);
    auto *msg_meta = dx_get_msg_meta(buffer);

    auto *msgPayload = g_new0(DxMsgPayload, 1);
    msgPayload->_data = g_memdup(payload->_data, payload->_size);
    msgPayload->_size = payload->_size;

    msg_meta->_payload = (gpointer)msgPayload;
}

