#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"
#include <zlib.h>

static gint generate_meta_id_uuid() {
    std::string uuid_str = g_uuid_string_random();

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const Bytef *>(uuid_str.c_str()),
                uuid_str.length());

    return static_cast<gint>(crc & G_MAXINT);
}

DXObjectMeta* dx_acquire_obj_meta_from_pool(void) {
    DXObjectMeta *obj_meta = g_new0(DXObjectMeta, 1);

    obj_meta->_meta_id = generate_meta_id_uuid();

    // body
    obj_meta->_track_id = -1;
    obj_meta->_label = -1;
    obj_meta->_label_name = nullptr;
    obj_meta->_confidence = -1.0;
    new (&obj_meta->_keypoints) std::vector<float>();
    obj_meta->_keypoints.clear();
    new (&obj_meta->_body_feature) std::vector<float>();
    obj_meta->_body_feature.clear();
    obj_meta->_box[0] = 0;
    obj_meta->_box[1] = 0;
    obj_meta->_box[2] = 0;
    obj_meta->_box[3] = 0;

    // face
    obj_meta->_face_confidence = -1.0;
    new (&obj_meta->_face_landmarks) std::vector<dxs::Point_f>();
    obj_meta->_face_landmarks.clear();
    new (&obj_meta->_face_feature) std::vector<float>();
    obj_meta->_face_feature.clear();
    obj_meta->_face_box[0] = 0;
    obj_meta->_face_box[1] = 0;
    obj_meta->_face_box[2] = 0;
    obj_meta->_face_box[3] = 0;

    // segmentation
    new (&obj_meta->_seg_cls_map) dxs::SegClsMap();

    // user meta
    obj_meta->_obj_user_meta_list = nullptr;
    obj_meta->_num_obj_user_meta = 0;

    new (&obj_meta->_input_tensors) std::map<int, dxs::DXTensors>();
    new (&obj_meta->_output_tensors) std::map<int, dxs::DXTensors>();

    return obj_meta;
}

void dx_release_obj_meta(DXObjectMeta *obj_meta) {
    if (!obj_meta) return;

    if (obj_meta->_label_name) {
        g_string_free(obj_meta->_label_name, TRUE);
        obj_meta->_label_name = nullptr;
    }
    
    using point_f_vec = std::vector<dxs::Point_f>;
    using float_vec = std::vector<float>;
    
    obj_meta->_keypoints.~float_vec();
    obj_meta->_body_feature.~float_vec();
    obj_meta->_face_landmarks.~point_f_vec();
    obj_meta->_face_feature.~float_vec();

    for (auto &tensors : obj_meta->_input_tensors) {
        if (tensors.second._data) {
            free(tensors.second._data);
            tensors.second._data = nullptr;
        }
        tensors.second._tensors.clear();
    }
    for (auto &tensors : obj_meta->_output_tensors) {
        if (tensors.second._data) {
            free(tensors.second._data);
            tensors.second._data = nullptr;
        }
        tensors.second._tensors.clear();
    }

    // user meta
    for (GList *l = obj_meta->_obj_user_meta_list; l != nullptr; l = l->next) {
        DXUserMeta *user_meta = (DXUserMeta *)l->data;
        dx_release_user_meta(user_meta);
    }
    g_list_free(obj_meta->_obj_user_meta_list);
    obj_meta->_obj_user_meta_list = nullptr;

    obj_meta->_seg_cls_map.~SegClsMap();
    
    g_free(obj_meta);
}

void dx_copy_obj_meta(DXObjectMeta *src_meta, DXObjectMeta *dst_meta) {
    if (!src_meta || !dst_meta) return;

    dst_meta->_meta_id = src_meta->_meta_id;
    dst_meta->_track_id = src_meta->_track_id;
    dst_meta->_label = src_meta->_label;
    
    if (src_meta->_label_name) {
        dst_meta->_label_name = g_string_new_len(src_meta->_label_name->str,
                                                src_meta->_label_name->len);
    } else {
        dst_meta->_label_name = nullptr;
    }
    
    dst_meta->_confidence = src_meta->_confidence;
    dst_meta->_box[0] = src_meta->_box[0];
    dst_meta->_box[1] = src_meta->_box[1];
    dst_meta->_box[2] = src_meta->_box[2];
    dst_meta->_box[3] = src_meta->_box[3];
    
    if (!src_meta->_keypoints.empty()) {
        dst_meta->_keypoints = src_meta->_keypoints;
    }
    if (!src_meta->_body_feature.empty()) {
        dst_meta->_body_feature = src_meta->_body_feature;
    }

    dst_meta->_face_box[0] = src_meta->_face_box[0];
    dst_meta->_face_box[1] = src_meta->_face_box[1];
    dst_meta->_face_box[2] = src_meta->_face_box[2];
    dst_meta->_face_box[3] = src_meta->_face_box[3];
    dst_meta->_face_confidence = src_meta->_face_confidence;
    
    for (const auto &point : src_meta->_face_landmarks) {
        dst_meta->_face_landmarks.push_back(point);
    }
    if (!src_meta->_face_feature.empty()) {
        dst_meta->_face_feature = src_meta->_face_feature;
    }

    if (src_meta->_seg_cls_map.data.size() > 0) {
        dst_meta->_seg_cls_map.data = src_meta->_seg_cls_map.data;
        dst_meta->_seg_cls_map.width = src_meta->_seg_cls_map.width;
        dst_meta->_seg_cls_map.height = src_meta->_seg_cls_map.height;
    }

    // Copy tensors
    for (auto &input_tensors : src_meta->_input_tensors) {
        dxs::DXTensors new_tensors;
        new_tensors._mem_size = input_tensors.second._mem_size;
        new_tensors._data = malloc(new_tensors._mem_size);
        memcpy(new_tensors._data, input_tensors.second._data, new_tensors._mem_size);
        new_tensors._tensors = input_tensors.second._tensors;
        dst_meta->_input_tensors[input_tensors.first] = new_tensors;
    }

    for (auto &output_tensors : src_meta->_output_tensors) {
        dxs::DXTensors new_tensors;
        new_tensors._mem_size = output_tensors.second._mem_size;
        new_tensors._data = malloc(new_tensors._mem_size);
        memcpy(new_tensors._data, output_tensors.second._data, new_tensors._mem_size);
        new_tensors._tensors = output_tensors.second._tensors;
        dst_meta->_output_tensors[output_tensors.first] = new_tensors;
    }

    dst_meta->_obj_user_meta_list = nullptr;
    dst_meta->_num_obj_user_meta = 0;
    
    for (GList *l = src_meta->_obj_user_meta_list; l != nullptr; l = l->next) {
        DXUserMeta *src_user_meta = (DXUserMeta *)l->data;
        DXUserMeta *dst_user_meta = dx_acquire_user_meta_from_pool();
        
        if (!src_user_meta->copy_func || !src_user_meta->release_func) {
            g_warning("UserMeta missing required copy_func or release_func - skipping copy");
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
        
        dx_add_user_meta_to_obj(dst_meta, dst_user_meta);
    }
}
