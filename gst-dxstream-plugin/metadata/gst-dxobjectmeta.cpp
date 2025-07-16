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
    std::string uuid_str = g_uuid_string_random();

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, reinterpret_cast<const Bytef *>(uuid_str.c_str()),
                uuid_str.length());

    return static_cast<gint>(crc & G_MAXINT);
}

GType dx_object_meta_api_get_type(void) {
    static GType type;

    static const gchar *tags[] = {"dx_object_meta", nullptr};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("DXObjectMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }

    return type;
}

const GstMetaInfo *dx_object_meta_get_info(void) {
    static const GstMetaInfo *meta_info = nullptr;

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
    dx_meta->_label_name = nullptr;
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
    new (&dx_meta->_seg_cls_map) dxs::SegClsMap();
    // dx_meta->_seg_cls_map.data.clear();
    // dx_meta->_seg_cls_map.width = 0;
    // dx_meta->_seg_cls_map.height = 0;

    new (&dx_meta->_input_tensors) std::map<int, dxs::DXTensors>();
    new (&dx_meta->_output_tensors) std::map<int, dxs::DXTensors>();

    return TRUE;
}

static void dx_object_meta_free(GstMeta *meta, GstBuffer *buffer) {
    DXObjectMeta *dx_meta = (DXObjectMeta *)meta;

    if (dx_meta->_label_name) {
        g_string_free(dx_meta->_label_name, TRUE);
        dx_meta->_label_name = nullptr;
    }
    
    using point_f_vec = std::vector<dxs::Point_f>;
    using float_vec = std::vector<float>;
    
    dx_meta->_keypoints.~float_vec();
    dx_meta->_body_feature.~float_vec();
    dx_meta->_face_landmarks.~point_f_vec();
    dx_meta->_face_feature.~float_vec();

    for (auto &tensors : dx_meta->_input_tensors) {
        if (tensors.second._data) {
            free(tensors.second._data);
            tensors.second._data = nullptr;
        }
        tensors.second._tensors.clear();
    }
    for (auto &tensors : dx_meta->_output_tensors) {
        if (tensors.second._data) {
            free(tensors.second._data);
            tensors.second._data = nullptr;
        }
        tensors.second._tensors.clear();
    }

    dx_meta->_seg_cls_map.~SegClsMap();
}

void copy_tensor(DXObjectMeta *src_meta, DXObjectMeta *dst_meta) {
    // copy tensor
    for (auto &input_tensors : src_meta->_input_tensors) {
        dxs::DXTensors new_tensors;

        new_tensors._mem_size = input_tensors.second._mem_size;
        new_tensors._data = malloc(new_tensors._mem_size);
        memcpy(new_tensors._data, input_tensors.second._data,
               new_tensors._mem_size);
        new_tensors._tensors = input_tensors.second._tensors;

        dst_meta->_input_tensors[input_tensors.first] = new_tensors;
    }

    for (auto &output_tensors : src_meta->_output_tensors) {
        dxs::DXTensors new_tensors;

        new_tensors._mem_size = output_tensors.second._mem_size;
        new_tensors._data = malloc(new_tensors._mem_size);
        memcpy(new_tensors._data, output_tensors.second._data,
               new_tensors._mem_size);
        new_tensors._tensors = output_tensors.second._tensors;

        dst_meta->_output_tensors[output_tensors.first] = new_tensors;
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

    size_t object_length = (size_t)g_list_length(frame_meta->_object_meta_list);
    for (size_t i = 0; i < object_length; i++) {
        DXObjectMeta *object_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        if (object_meta->_meta_id == src_object_meta->_meta_id) {
            return FALSE;
        }
    }

    DXObjectMeta *dest_object_meta =
        (DXObjectMeta *)gst_buffer_add_meta(dest, DX_OBJECT_META_INFO, nullptr);

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

    if (src_object_meta->_seg_cls_map.data.size() > 0) {
        dest_object_meta->_seg_cls_map.data =
            src_object_meta->_seg_cls_map.data;
        dest_object_meta->_seg_cls_map.width =
            src_object_meta->_seg_cls_map.width;
        dest_object_meta->_seg_cls_map.height =
            src_object_meta->_seg_cls_map.height;
    }

    copy_tensor(src_object_meta, dest_object_meta);

    dx_add_object_meta_to_frame_meta(dest_object_meta, frame_meta);
    return TRUE;
}
