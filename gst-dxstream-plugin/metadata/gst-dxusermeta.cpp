#include "gst-dxusermeta.hpp"
#include "gst-dxframemeta.hpp" 
#include "gst-dxobjectmeta.hpp"
#include <string.h>

DXUserMeta* dx_acquire_user_meta_from_pool(void) {
    DXUserMeta *user_meta = g_new0(DXUserMeta, 1);
    
    user_meta->user_meta_data = NULL;
    user_meta->user_meta_size = 0;
    user_meta->user_meta_type = DX_USER_META_FRAME;  // Default to frame type
    
    // Set to NULL to force user to provide proper functions
    user_meta->release_func = NULL;
    user_meta->copy_func = NULL;
    
    return user_meta;
}

void dx_release_user_meta(DXUserMeta *user_meta) {
    if (!user_meta) return;
    
    if (user_meta->user_meta_data) {
        if (user_meta->release_func) {
            user_meta->release_func(user_meta->user_meta_data);
        } else {
            g_warning("No release_func set - potential memory leak!");
        }
    }
    
    g_free(user_meta);
}

gboolean dx_user_meta_set_data(DXUserMeta *user_meta,
                              gpointer data,
                              gsize size,
                              guint meta_type,
                              GDestroyNotify release_func,
                              GBoxedCopyFunc copy_func) {
    if (!user_meta) return FALSE;
    
    if (!release_func || !copy_func) {
        g_warning("Both release_func and copy_func are required for user metadata");
        return FALSE;
    }
    
    if (user_meta->user_meta_data && user_meta->release_func) {
        user_meta->release_func(user_meta->user_meta_data);
    }
    
    user_meta->user_meta_data = data;
    user_meta->user_meta_size = size;
    user_meta->user_meta_type = meta_type;
    user_meta->release_func = release_func;
    user_meta->copy_func = copy_func;
    
    return TRUE;
}

gboolean dx_add_user_meta_to_frame(DXFrameMeta *frame_meta, DXUserMeta *user_meta) {
    if (!frame_meta || !user_meta) {
        return FALSE;
    }
    
    if (!user_meta->release_func || !user_meta->copy_func) {
        g_warning("DXUserMeta must have both release_func and copy_func set before adding to frame");
        return FALSE;
    }
    
    frame_meta->_frame_user_meta_list = g_list_append(frame_meta->_frame_user_meta_list, user_meta);
    frame_meta->_num_frame_user_meta++;
    
    return TRUE;
}

gboolean dx_add_user_meta_to_obj(DXObjectMeta *obj_meta, DXUserMeta *user_meta) {
    if (!obj_meta || !user_meta) {
        return FALSE;
    }
    
    if (!user_meta->release_func || !user_meta->copy_func) {
        g_warning("DXUserMeta must have both release_func and copy_func set before adding to object");
        return FALSE;
    }
    
    obj_meta->_obj_user_meta_list = g_list_append(obj_meta->_obj_user_meta_list, user_meta);
    obj_meta->_num_obj_user_meta++;
    
    return TRUE;
}

GList* dx_get_frame_user_metas(DXFrameMeta *frame_meta) {
    if (!frame_meta) return NULL;
    
    return g_list_copy(frame_meta->_frame_user_meta_list);
}

GList* dx_get_object_user_metas(DXObjectMeta *obj_meta) {
    if (!obj_meta) return NULL;
    
    return g_list_copy(obj_meta->_obj_user_meta_list);
}