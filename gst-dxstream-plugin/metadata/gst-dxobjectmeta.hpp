#ifndef DXOBJECTMETA_H
#define DXOBJECTMETA_H

#include "dxcommon.hpp"
#include <glib.h>
#include <gst/gst.h>
#include <string>
#include <vector>

G_BEGIN_DECLS

#define DX_OBJECT_META_API_TYPE (dx_object_meta_api_get_type())
#define DX_OBJECT_META_INFO (dx_object_meta_get_info())

typedef struct _DXObjectMeta {
    GstMeta _meta;
    gint _meta_id;
    gint _stream_id;

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

} DXObjectMeta;

GType dx_object_meta_api_get_type(void);
const GstMetaInfo *dx_object_meta_get_info(void);

G_END_DECLS

#endif /* DXOBJECTMETA_H */