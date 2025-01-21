#ifndef DX_META_H
#define DX_META_H

#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"

#include <gst/gst.h>

DXFrameMeta *dx_create_frame_meta(GstBuffer *buffer);

DXObjectMeta *dx_create_object_meta(GstBuffer *buffer);

void dx_add_object_meta_to_frame_meta(DXObjectMeta *object_meta,
                                      DXFrameMeta *frame_meta);

#endif /* DX_META_H */