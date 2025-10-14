#ifndef DX_META_H
#define DX_META_H

#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include "gst-dxmsgmeta.hpp"

#include <gst/gst.h>

DXFrameMeta *dx_create_frame_meta(GstBuffer *buffer);

DXObjectMeta *dx_create_object_meta(GstBuffer *buffer);

GstDxMsgMeta *dx_create_msg_meta(GstBuffer *buffer);

DXFrameMeta *dx_get_frame_meta(GstBuffer *buffer);

DXObjectMeta *dx_get_object_meta(GstBuffer *buffer);

GstDxMsgMeta *dx_get_msg_meta(GstBuffer *buffer);

void dx_add_object_meta_to_frame_meta(DXObjectMeta *object_meta,
                                      DXFrameMeta *frame_meta);

void dx_add_payload_to_buffer(GstBuffer *buffer, DxMsgPayload *payload);

#endif /* DX_META_H */