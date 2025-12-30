#ifndef DXUSERMETA_H
#define DXUSERMETA_H

#include <glib.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _DXUserMeta DXUserMeta;

struct _DXUserMeta {
    gpointer user_meta_data;
    gsize user_meta_size;
    guint user_meta_type;
    
    GDestroyNotify release_func;
    GBoxedCopyFunc copy_func;
};

typedef enum {
    DX_USER_META_FRAME = 0x1000,   // Frame-level user metadata
    DX_USER_META_OBJECT = 0x2000,  // Object-level user metadata
} DXUserMetaType;

DXUserMeta* dx_acquire_user_meta_from_pool(void);
void dx_release_user_meta(DXUserMeta *user_meta);

gboolean dx_user_meta_set_data(DXUserMeta *user_meta,
                              gpointer data,
                              gsize size,
                              guint meta_type,
                              GDestroyNotify release_func,
                              GBoxedCopyFunc copy_func);

gboolean dx_add_user_meta_to_frame(struct _DXFrameMeta *frame_meta, DXUserMeta *user_meta);
gboolean dx_add_user_meta_to_obj(struct _DXObjectMeta *obj_meta, DXUserMeta *user_meta);

GList* dx_get_frame_user_metas(struct _DXFrameMeta *frame_meta);
GList* dx_get_object_user_metas(struct _DXObjectMeta *obj_meta);

G_END_DECLS

#endif /* DXUSERMETA_H */