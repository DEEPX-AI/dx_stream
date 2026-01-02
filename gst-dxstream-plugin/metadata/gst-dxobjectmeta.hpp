#ifndef DXOBJECTMETA_H
#define DXOBJECTMETA_H

#include "dxcommon.hpp"
#include <glib.h>
#include <map>
#include <string>
#include <vector>

G_BEGIN_DECLS

typedef struct _DXObjectMeta {
    gint _meta_id;

    // body
    gint _track_id;
    gint _label;
    GString *_label_name;
    gfloat _confidence;
    float _box[4];
    std::vector<float> _keypoints;
    std::vector<float> _body_feature;

    // face
    float _face_box[4];
    gfloat _face_confidence;
    std::vector<dxs::Point_f> _face_landmarks;
    std::vector<float> _face_feature;

    // segmentation
    dxs::SegClsMap _seg_cls_map;

    // user meta
    GList *_obj_user_meta_list;
    guint _num_obj_user_meta;

    std::map<int, dxs::DXTensors> _input_tensors;
    std::map<int, dxs::DXTensors> _output_tensors;

} DXObjectMeta;

DXObjectMeta* dx_acquire_obj_meta_from_pool(void);
void dx_release_obj_meta(DXObjectMeta *obj_meta);
void dx_copy_obj_meta(DXObjectMeta *src_meta, DXObjectMeta *dst_meta);

G_END_DECLS

#endif /* DXOBJECTMETA_H */