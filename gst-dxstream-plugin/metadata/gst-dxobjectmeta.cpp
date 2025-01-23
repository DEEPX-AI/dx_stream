#include "gst-dxobjectmeta.hpp"
#include "gst-dxmeta.hpp"
#include <zlib.h>

static gboolean dx_object_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer);
static void dx_object_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean dx_object_meta_transform(GstBuffer *dest, GstMeta *meta,
                                         GstBuffer *buffer, GQuark type,
                                         gpointer data);

static gint generate_meta_id_uuid() {
    gchar *uuid = g_uuid_string_random();

    guint32 crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (const unsigned char *)uuid, strlen(uuid));

    g_free(uuid);
    return (gint)(crc & G_MAXINT);
}

GType dx_object_meta_api_get_type(void) {
    static GType type;

    static const gchar *tags[] = {"dx_object_meta", NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("DXObjectMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }

    return type;
}

const GstMetaInfo *dx_object_meta_get_info(void) {
    static const GstMetaInfo *meta_info = NULL;

    if (g_once_init_enter(&meta_info)) {
        const GstMetaInfo *mi = gst_meta_register(
            DX_OBJECT_META_API_TYPE, "DXObjectMeta", sizeof(DXObjectMeta),
            (GstMetaInitFunction)dx_object_meta_init,
            (GstMetaFreeFunction)dx_object_meta_free,
            (GstMetaTransformFunction)dx_object_meta_transform);
        g_once_init_leave(&meta_info, mi);
    }
    return meta_info;
}

static gboolean dx_object_meta_init(GstMeta *meta, gpointer params,
                                    GstBuffer *buffer) {
    DXObjectMeta *dx_meta = (DXObjectMeta *)meta;

    dx_meta->_meta_id = generate_meta_id_uuid();

    // body
    dx_meta->_track_id = -1;
    dx_meta->_label = -1;
    dx_meta->_label_name = NULL;
    dx_meta->_confidence = -1.0;
    new (&dx_meta->_keypoints) std::vector<float>();
    dx_meta->_keypoints.clear();
    new (&dx_meta->_body_feature) std::vector<float>();
    dx_meta->_body_feature.clear();
    dx_meta->_box[0] = 0;
    dx_meta->_box[1] = 0;
    dx_meta->_box[2] = 0;
    dx_meta->_box[3] = 0;

    // face
    dx_meta->_face_confidence = -1.0;
    new (&dx_meta->_face_landmarks) std::vector<dxs::Point_f>();
    dx_meta->_face_landmarks.clear();
    new (&dx_meta->_face_feature) std::vector<float>();
    dx_meta->_face_feature.clear();
    dx_meta->_face_box[0] = 0;
    dx_meta->_face_box[1] = 0;
    dx_meta->_face_box[2] = 0;
    dx_meta->_face_box[3] = 0;

    // segmentation
    dx_meta->_seg_cls_map = dxs::SegClsMap();

    return TRUE;
}

static void dx_object_meta_free(GstMeta *meta, GstBuffer *buffer) {
    DXObjectMeta *dx_meta = (DXObjectMeta *)meta;

    if (dx_meta->_label_name) {
        g_string_free(dx_meta->_label_name, TRUE);
        dx_meta->_label_name = NULL;
    }
    dx_meta->_keypoints.clear();
    dx_meta->_body_feature.clear();

    dx_meta->_face_landmarks.clear();
    dx_meta->_face_feature.clear();
    if (dx_meta->_seg_cls_map.data) {
        free(dx_meta->_seg_cls_map.data);
        dx_meta->_seg_cls_map.data = NULL;
    }
}

static gboolean dx_object_meta_transform(GstBuffer *dest, GstMeta *meta,
                                         GstBuffer *buffer, GQuark type,
                                         gpointer data) {
    DXObjectMeta *src_object_meta = (DXObjectMeta *)meta;
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(dest, DX_FRAME_META_API_TYPE);

    if (!frame_meta) {
        return FALSE;
    }

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);
    for (int i = 0; i < object_length; i++) {
        DXObjectMeta *object_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        if (object_meta->_meta_id == src_object_meta->_meta_id) {
            return FALSE;
        }
    }

    DXObjectMeta *dest_object_meta =
        (DXObjectMeta *)gst_buffer_add_meta(dest, DX_OBJECT_META_INFO, NULL);

    dest_object_meta->_meta_id = src_object_meta->_meta_id;

    dest_object_meta->_track_id = src_object_meta->_track_id;
    dest_object_meta->_label = src_object_meta->_label;
    if (src_object_meta->_label_name) {
        dest_object_meta->_label_name =
            g_string_new_len(src_object_meta->_label_name->str,
                             src_object_meta->_label_name->len);
    } else {
        dest_object_meta->_label_name = nullptr;
    }
    dest_object_meta->_confidence = src_object_meta->_confidence;
    dest_object_meta->_box[0] = src_object_meta->_box[0];
    dest_object_meta->_box[1] = src_object_meta->_box[1];
    dest_object_meta->_box[2] = src_object_meta->_box[2];
    dest_object_meta->_box[3] = src_object_meta->_box[3];
    if (!src_object_meta->_keypoints.empty()) {
        dest_object_meta->_keypoints = src_object_meta->_keypoints;
    }
    if (!src_object_meta->_body_feature.empty()) {
        dest_object_meta->_body_feature = src_object_meta->_body_feature;
    }

    dest_object_meta->_face_box[0] = src_object_meta->_face_box[0];
    dest_object_meta->_face_box[1] = src_object_meta->_face_box[1];
    dest_object_meta->_face_box[2] = src_object_meta->_face_box[2];
    dest_object_meta->_face_box[3] = src_object_meta->_face_box[3];
    dest_object_meta->_face_confidence = src_object_meta->_face_confidence;
    for (const auto &point : src_object_meta->_face_landmarks) {
        dest_object_meta->_face_landmarks.push_back(point);
    }
    if (!src_object_meta->_face_feature.empty()) {
        dest_object_meta->_face_feature = src_object_meta->_face_feature;
    }

    if (src_object_meta->_seg_cls_map.data != nullptr) {
        size_t seg_map_size = src_object_meta->_seg_cls_map.width *
                              src_object_meta->_seg_cls_map.height;

        dest_object_meta->_seg_cls_map.data =
            (unsigned char *)malloc(seg_map_size);
        memcpy(dest_object_meta->_seg_cls_map.data,
               src_object_meta->_seg_cls_map.data, seg_map_size);

        dest_object_meta->_seg_cls_map.width =
            src_object_meta->_seg_cls_map.width;
        dest_object_meta->_seg_cls_map.height =
            src_object_meta->_seg_cls_map.height;
    } else {
        dest_object_meta->_seg_cls_map.data = nullptr;
        dest_object_meta->_seg_cls_map.width = 0;
        dest_object_meta->_seg_cls_map.height = 0;
    }

    dx_add_object_meta_to_frame_meta(dest_object_meta, frame_meta);
    return TRUE;
}
