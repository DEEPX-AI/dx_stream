// Shared across L0/L1/L2 -- DXFrameMeta / DXObjectMeta / DXUserMeta / DxMsgMeta creation helpers

#pragma once

#include <gst/gst.h>
#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include "gstdxstream/gst-dxusermeta.hpp"
#include "gstdxstream/gst-dxmsgmeta.hpp"

#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace dxtest {

inline DXFrameMeta *make_frame_meta(GstBuffer *buf, int stream_id, int w, int h,
                                     const char *format = "RGB", float fps = 30.0f) {
    dx_create_frame_meta(buf);
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fm->_stream_id = stream_id;
    fm->_width = w;
    fm->_height = h;
    fm->_format = format;
    fm->_frame_rate = fps;
    return fm;
}

inline DXObjectMeta *make_object_meta(int label, float conf,
                                       float bx, float by, float bw, float bh,
                                       int track_id = -1) {
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    o->_label = label;
    o->_confidence = conf;
    o->_box = {bx, by, bw, bh};
    o->_track_id = track_id;
    return o;
}

inline DXObjectMeta *add_object_to_frame(DXFrameMeta *fm, int label, float conf,
                                          float bx, float by, float bw, float bh,
                                          int track_id = -1) {
    DXObjectMeta *o = make_object_meta(label, conf, bx, by, bw, bh, track_id);
    dx_add_obj_meta_to_frame(fm, o);
    return o;
}

// User meta attachment helper based on custom_type
struct SimpleUserPayload {
    int custom_type;
    int magic;
    char tag[32];
};

inline SimpleUserPayload *new_simple_payload(int custom_type, int magic,
                                              const char *tag) {
    auto *p = (SimpleUserPayload *)g_malloc0(sizeof(SimpleUserPayload));
    p->custom_type = custom_type;
    p->magic = magic;
    std::strncpy(p->tag, tag ? tag : "", sizeof(p->tag) - 1);
    return p;
}

// GBoxedCopyFunc signature: gpointer (*)(gpointer)
inline gpointer simple_payload_copy(gpointer src) {
    auto *s = (const SimpleUserPayload *)src;
    auto *d = (SimpleUserPayload *)g_malloc0(sizeof(SimpleUserPayload));
    *d = *s;
    return d;
}

inline DXUserMeta *attach_simple_user_meta(DXFrameMeta *fm, int custom_type,
                                            int magic, const char *tag) {
    DXUserMeta *um = dx_acquire_user_meta_from_pool();
    SimpleUserPayload *p = new_simple_payload(custom_type, magic, tag);
    dx_user_meta_set_data(um, p, sizeof(SimpleUserPayload),
                          DXUserMetaType::DX_USER_META_FRAME,
                          g_free, simple_payload_copy);
    dx_add_user_meta_to_frame(fm, um);
    return um;
}

inline DXUserMeta *attach_simple_user_meta_obj(DXObjectMeta *om, int custom_type,
                                                int magic, const char *tag) {
    DXUserMeta *um = dx_acquire_user_meta_from_pool();
    SimpleUserPayload *p = new_simple_payload(custom_type, magic, tag);
    dx_user_meta_set_data(um, p, sizeof(SimpleUserPayload),
                          DXUserMetaType::DX_USER_META_OBJECT,
                          g_free, simple_payload_copy);
    dx_add_user_meta_to_obj(om, um);
    return um;
}

}  // namespace dxtest
