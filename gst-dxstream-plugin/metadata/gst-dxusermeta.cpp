#include "gst-dxusermeta.hpp"
#include "gst-dxframemeta.hpp" 
#include "gst-dxobjectmeta.hpp"
#include <string.h>

GST_DEBUG_CATEGORY_EXTERN(dxobjectmeta_cat);

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

DXUserMeta* dx_acquire_user_meta_from_pool(void) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Acquiring DXUserMeta from pool");
    auto *user_meta = g_new0(DXUserMeta, 1);
    
    user_meta->user_meta_data = nullptr;
    user_meta->user_meta_size = 0;
    user_meta->user_meta_type = DXUserMetaType::DX_USER_META_FRAME;
    
    // Set to nullptr to force user to provide proper functions
    user_meta->release_func = nullptr;
    user_meta->copy_func = nullptr;
    
    return user_meta;
}

void dx_release_user_meta(DXUserMeta *user_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Releasing DXUserMeta");
    if (!user_meta) return;
    
    if (user_meta->user_meta_data) {
        if (user_meta->release_func) {
            user_meta->release_func(user_meta->user_meta_data);
        } else {
            GST_CAT_WARNING_SAFE(dxobjectmeta_cat, "No release_func set for user metadata - potential memory leak!");
        }
    }
    
    g_free(user_meta);
}

// NOSONAR - GLib API requires C function pointers (GDestroyNotify, GBoxedCopyFunc) for compatibility
gboolean dx_user_meta_set_data(DXUserMeta *user_meta,
                              void* data,
                              size_t size,
                              DXUserMetaType meta_type, // NOSONAR
                              GDestroyNotify release_func, // NOSONAR
                              GBoxedCopyFunc copy_func) { // NOSONAR
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Setting data for DXUserMeta of type %d", static_cast<int>(meta_type));
    if (!user_meta) return FALSE;
    
    if (!release_func || !copy_func) {
        GST_CAT_WARNING_SAFE(dxobjectmeta_cat, "Both release_func and copy_func are required for user metadata");
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
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Adding DXUserMeta to frame");
    if (!frame_meta || !user_meta) {
        return FALSE;
    }
    
    if (!user_meta->release_func || !user_meta->copy_func) {
        GST_CAT_WARNING_SAFE(dxobjectmeta_cat, "DXUserMeta must have both release_func and copy_func set before adding to frame");
        return FALSE;
    }
    
    frame_meta->_frame_user_meta_list.push_back(user_meta);
    
    return TRUE;
}

gboolean dx_add_user_meta_to_obj(DXObjectMeta *obj_meta, DXUserMeta *user_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Adding DXUserMeta to object");
    if (!obj_meta || !user_meta) {
        return FALSE;
    }
    
    if (!user_meta->release_func || !user_meta->copy_func) {
        GST_CAT_WARNING_SAFE(dxobjectmeta_cat, "DXUserMeta must have both release_func and copy_func set before adding to object");
        return FALSE;
    }
    
    obj_meta->_obj_user_meta_list.push_back(user_meta);
    
    return TRUE;
}

std::vector<DXUserMeta*>* dx_get_frame_user_metas(DXFrameMeta *frame_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Getting user metas from frame");
    if (!frame_meta) return nullptr;
    
    return &frame_meta->_frame_user_meta_list;
}

std::vector<DXUserMeta*>* dx_get_object_user_metas(DXObjectMeta *obj_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Getting user metas from object");
    if (!obj_meta) return nullptr;
    
    return &obj_meta->_obj_user_meta_list;
}