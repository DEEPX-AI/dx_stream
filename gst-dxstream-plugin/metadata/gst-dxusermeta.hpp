#ifndef DXUSERMETA_H
#define DXUSERMETA_H

#include <glib.h>
#include <gst/gst.h>
#include <vector>

G_BEGIN_DECLS

enum class DXUserMetaType {
    DX_USER_META_FRAME = 0x1000,   // Frame-level user metadata
    DX_USER_META_OBJECT = 0x2000,  // Object-level user metadata
};

struct _DXUserMeta {
    void* user_meta_data;
    size_t user_meta_size;
    DXUserMetaType user_meta_type;
    
    GDestroyNotify release_func;
    GBoxedCopyFunc copy_func;
};

using DXUserMeta = struct _DXUserMeta;

DXUserMeta* dx_acquire_user_meta_from_pool(void);
void dx_release_user_meta(DXUserMeta *user_meta);

gboolean dx_user_meta_set_data(DXUserMeta *user_meta,
                              void* data,
                              size_t size,
                              DXUserMetaType meta_type,
                              GDestroyNotify release_func,
                              GBoxedCopyFunc copy_func);

gboolean dx_add_user_meta_to_frame(struct _DXFrameMeta *frame_meta, DXUserMeta *user_meta);
gboolean dx_add_user_meta_to_obj(struct _DXObjectMeta *obj_meta, DXUserMeta *user_meta);

std::vector<DXUserMeta*>* dx_get_frame_user_metas(struct _DXFrameMeta *frame_meta);
std::vector<DXUserMeta*>* dx_get_object_user_metas(struct _DXObjectMeta *obj_meta);

G_END_DECLS

#endif /* DXUSERMETA_H */