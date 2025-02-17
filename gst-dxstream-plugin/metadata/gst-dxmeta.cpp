#include "gst-dxmeta.hpp"

DXFrameMeta *dx_create_frame_meta(GstBuffer *buffer) {
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_add_meta(buffer, DX_FRAME_META_INFO, NULL);
    return frame_meta;
}

DXObjectMeta *dx_create_object_meta(GstBuffer *buffer) {
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    DXObjectMeta *object_meta =
        (DXObjectMeta *)gst_buffer_add_meta(buffer, DX_OBJECT_META_INFO, NULL);
    return object_meta;
}

void dx_add_object_meta_to_frame_meta(DXObjectMeta *object_meta,
                                      DXFrameMeta *frame_meta) {
    frame_meta->_object_meta_list =
        g_list_append(frame_meta->_object_meta_list, object_meta);
}
