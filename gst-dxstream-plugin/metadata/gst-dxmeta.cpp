#include "gst-dxmeta.hpp"

DXFrameMeta *dx_create_frame_meta(GstBuffer *buffer) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_add_meta(buffer, DX_FRAME_META_INFO, nullptr);
    return frame_meta;
}

DXObjectMeta *dx_create_object_meta(GstBuffer *buffer) {
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    DXObjectMeta *object_meta = (DXObjectMeta *)gst_buffer_add_meta(
        buffer, DX_OBJECT_META_INFO, nullptr);
    return object_meta;
}

GstDxMsgMeta *dx_create_msg_meta(GstBuffer *buffer) {
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    GstDxMsgMeta *msg_meta =
        (GstDxMsgMeta *)gst_buffer_add_meta(buffer, GST_DXMSG_META_INFO, nullptr);
    return msg_meta;
}

DXFrameMeta *dx_get_frame_meta(GstBuffer *buffer) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    return frame_meta;
}

DXObjectMeta *dx_get_object_meta(GstBuffer *buffer) {
    DXObjectMeta *object_meta =
        (DXObjectMeta *)gst_buffer_get_meta(buffer, DX_OBJECT_META_API_TYPE);
    return object_meta;
}

GstDxMsgMeta *dx_get_msg_meta(GstBuffer *buffer) {
    GstDxMsgMeta *msg_meta =
        (GstDxMsgMeta *)gst_buffer_get_meta(buffer, GST_DXMSG_META_API_TYPE);
    return msg_meta;
}

void dx_add_object_meta_to_frame_meta(DXObjectMeta *object_meta,
                                      DXFrameMeta *frame_meta) {
    frame_meta->_object_meta_list =
        g_list_append(frame_meta->_object_meta_list, object_meta);
}

void dx_add_payload_to_buffer(GstBuffer *buffer, DxMsgPayload *payload) {
    GstDxMsgMeta *msg_meta = dx_create_msg_meta(buffer);
    DxMsgPayload *msgPayload = g_new0(DxMsgPayload, 1);
    msgPayload->_data = g_memdup(payload->_data, payload->_size);
    msgPayload->_size = payload->_size;

    msg_meta->_payload = (gpointer)msgPayload;
}
