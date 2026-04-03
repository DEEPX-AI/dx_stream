#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"
#include <zlib.h>

GST_DEBUG_CATEGORY_STATIC(dxobjectmeta_cat);
#define GST_CAT_DEFAULT dxobjectmeta_cat

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

static gint generate_meta_id_uuid() {
    gchar* uuid_cstr = g_uuid_string_random();
    std::string uuid_str(uuid_cstr);
    g_free(uuid_cstr);  // GLib allocated memory must be freed

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, static_cast<const Bytef *>(static_cast<const void *>(uuid_str.c_str())),
                static_cast<uInt>(uuid_str.length()));

    return static_cast<gint>(crc & G_MAXINT);
}

DXObjectMeta* dx_acquire_obj_meta_from_pool(void) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Initializing DXObjectMeta from pool");
    auto *obj_meta = g_new0(DXObjectMeta, 1);
    if (!obj_meta) {
        GST_CAT_ERROR_SAFE(dxobjectmeta_cat, "Failed to allocate memory for DXObjectMeta");
        return nullptr;
    }

    obj_meta->_meta_id = generate_meta_id_uuid();

    // body
    obj_meta->_track_id = -1;
    obj_meta->_label = -1;
    new (&obj_meta->_label_name) std::string();
    obj_meta->_confidence = -1.0;
    new (&obj_meta->_keypoints) std::vector<float>();
    new (&obj_meta->_body_feature) std::vector<float>();
    new (&obj_meta->_obb) std::vector<float>();
    obj_meta->_box[0] = 0;
    obj_meta->_box[1] = 0;
    obj_meta->_box[2] = 0;
    obj_meta->_box[3] = 0;

    // face
    obj_meta->_face_confidence = -1.0;
    new (&obj_meta->_face_landmarks) std::vector<float>();
    new (&obj_meta->_face_feature) std::vector<float>();
    obj_meta->_face_box[0] = 0;
    obj_meta->_face_box[1] = 0;
    obj_meta->_face_box[2] = 0;
    obj_meta->_face_box[3] = 0;

    // segmentation
    new (&obj_meta->_seg_data) std::vector<unsigned char>();
    obj_meta->_seg_width = 0;
    obj_meta->_seg_height = 0;

    // user meta
    new (&obj_meta->_obj_user_meta_list) std::vector<DXUserMeta*>();

    // tensors
    new (&obj_meta->_input_tensors) std::map<int, dxs::DXTensors>();
    new (&obj_meta->_output_tensors) std::map<int, dxs::DXTensors>();

    return obj_meta;
}

void dx_release_obj_meta(DXObjectMeta *obj_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Releasing DXObjectMeta");
    if (!obj_meta) return;

    // Release user metadata
    for (auto *user_meta : obj_meta->_obj_user_meta_list) {
        dx_release_user_meta(user_meta);
    }
    obj_meta->_obj_user_meta_list.clear();

    // RAII: shared_ptr automatically releases memory when ref count reaches 0
    obj_meta->_input_tensors.clear();
    obj_meta->_output_tensors.clear();
    
    // NOSONAR - Explicit destructor calls required for placement new objects
    // Objects were constructed with placement new in dx_acquire_obj_meta_from_pool,
    // so destructors must be called explicitly before GLib frees the memory
    using float_vec = std::vector<float>;
    
    obj_meta->_label_name.~basic_string();
    obj_meta->_keypoints.~float_vec();
    obj_meta->_body_feature.~float_vec();
    obj_meta->_obb.~float_vec();
    obj_meta->_face_landmarks.~float_vec();
    obj_meta->_face_feature.~float_vec();
    using uchar_vec = std::vector<unsigned char>;
    obj_meta->_seg_data.~uchar_vec();
    obj_meta->_obj_user_meta_list.~vector();
    obj_meta->_input_tensors.~map();
    obj_meta->_output_tensors.~map();
    
    g_free(obj_meta);
}

void dx_copy_obj_meta(DXObjectMeta *src_meta, DXObjectMeta *dst_meta) {
    GST_CAT_DEBUG_SAFE(dxobjectmeta_cat, "Copying DXObjectMeta");
    if (!src_meta || !dst_meta) return;

    dst_meta->_meta_id = src_meta->_meta_id;
    dst_meta->_track_id = src_meta->_track_id;
    dst_meta->_label = src_meta->_label;
    
    // std::string copy
    dst_meta->_label_name = src_meta->_label_name;
    
    dst_meta->_confidence = src_meta->_confidence;
    dst_meta->_box[0] = src_meta->_box[0];
    dst_meta->_box[1] = src_meta->_box[1];
    dst_meta->_box[2] = src_meta->_box[2];
    dst_meta->_box[3] = src_meta->_box[3];
    
    dst_meta->_keypoints = src_meta->_keypoints;
    dst_meta->_body_feature = src_meta->_body_feature;
    dst_meta->_obb = src_meta->_obb;

    dst_meta->_face_box[0] = src_meta->_face_box[0];
    dst_meta->_face_box[1] = src_meta->_face_box[1];
    dst_meta->_face_box[2] = src_meta->_face_box[2];
    dst_meta->_face_box[3] = src_meta->_face_box[3];
    dst_meta->_face_confidence = src_meta->_face_confidence;
    
    dst_meta->_face_landmarks = src_meta->_face_landmarks;
    dst_meta->_face_feature = src_meta->_face_feature;

    if (!src_meta->_seg_data.empty()) {
        dst_meta->_seg_data = src_meta->_seg_data;
        dst_meta->_seg_width = src_meta->_seg_width;
        dst_meta->_seg_height = src_meta->_seg_height;
    }

    // Deep copy user metadata
    dst_meta->_obj_user_meta_list.clear();
    for (auto *src_user_meta : src_meta->_obj_user_meta_list) {
        auto *dst_user_meta = dx_acquire_user_meta_from_pool();
        
        if (!src_user_meta->copy_func || !src_user_meta->release_func) {
            GST_CAT_WARNING_SAFE(dxobjectmeta_cat, "UserMeta missing required copy_func or release_func - skipping copy");
            dx_release_user_meta(dst_user_meta);
            continue;
        }
        
        if (src_user_meta->user_meta_data) {
            dst_user_meta->user_meta_data = src_user_meta->copy_func(src_user_meta->user_meta_data);
        } else {
            dst_user_meta->user_meta_data = nullptr;
        }
        
        dst_user_meta->user_meta_size = src_user_meta->user_meta_size;
        dst_user_meta->user_meta_type = src_user_meta->user_meta_type;
        dst_user_meta->release_func = src_user_meta->release_func;
        dst_user_meta->copy_func = src_user_meta->copy_func;
        
        dst_meta->_obj_user_meta_list.push_back(dst_user_meta);
    }

    // Shallow copy tensors (shared ownership via shared_ptr)
    dst_meta->_input_tensors = src_meta->_input_tensors;
    dst_meta->_output_tensors = src_meta->_output_tensors;
}
