#ifndef DXOBJECTMETA_H
#define DXOBJECTMETA_H

#include "dxcommon.hpp"
#include <array>
#include <glib.h>
#include <map>
#include <string>
#include <vector>

G_BEGIN_DECLS

struct _DXUserMeta;
using DXUserMeta = struct _DXUserMeta;

struct _DXObjectMeta {
    int _meta_id;

    // body
    int _track_id;
    int _label;
    std::string _label_name;
    float _confidence;
    std::array<float, 4> _box;
    std::vector<float> _keypoints;
    std::vector<float> _body_feature;

    // oriented bounding box [cx, cy, w, h, angle]
    std::vector<float> _obb;

    // face
    std::array<float, 4> _face_box;
    float _face_confidence;
    std::vector<float> _face_landmarks;
    std::vector<float> _face_feature;

    // segmentation
    std::vector<unsigned char> _seg_data;
    int _seg_width = 0;
    int _seg_height = 0;

    // user meta
    std::vector<DXUserMeta*> _obj_user_meta_list;

    // RAII-managed tensors (shallow copy through shared_ptr)
    std::map<int, dxs::DXTensors> _input_tensors;   // preproc_id -> input tensors
    std::map<int, dxs::DXTensors> _output_tensors;   // infer_id -> output tensors

};

using DXObjectMeta = _DXObjectMeta;

DXObjectMeta* dx_acquire_obj_meta_from_pool(void);
void dx_release_obj_meta(DXObjectMeta *obj_meta);
void dx_copy_obj_meta(DXObjectMeta *src_meta, DXObjectMeta *dst_meta);

G_END_DECLS

#endif /* DXOBJECTMETA_H */