#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"


static gboolean dx_frame_meta_init(GstMeta *meta, gpointer params,
                                   GstBuffer *buffer);
static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean dx_frame_meta_transform(GstBuffer *dest, GstMeta *meta,
                                        GstBuffer *buffer, GQuark type,
                                        gpointer data);

GType dx_frame_meta_api_get_type(void) {
    static GType type;
    static const gchar *tags[] = {"dx_frame_meta", nullptr};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("DXFrameMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }

    return type;
}

const GstMetaInfo *dx_frame_meta_get_info(void) {
    static const GstMetaInfo *meta_info = nullptr;

    if (g_once_init_enter(&meta_info)) {
        const GstMetaInfo *mi = gst_meta_register(
            DX_FRAME_META_API_TYPE, "DXFrameMeta", sizeof(DXFrameMeta),
            (GstMetaInitFunction)dx_frame_meta_init,
            (GstMetaFreeFunction)dx_frame_meta_free,
            (GstMetaTransformFunction)dx_frame_meta_transform);
        g_once_init_leave(&meta_info, mi);
    }
    return meta_info;
}

static gboolean dx_frame_meta_init(GstMeta *meta, gpointer params,
                                   GstBuffer *buffer) {
    DXFrameMeta *dx_meta = (DXFrameMeta *)meta;
    
    dx_meta->_object_meta_list = nullptr;
    dx_meta->_stream_id = -1;
    dx_meta->_width = -1;
    dx_meta->_height = -1;

    dx_meta->_roi[0] = -1;
    dx_meta->_roi[1] = -1;
    dx_meta->_roi[2] = -1;
    dx_meta->_roi[3] = -1;

    dx_meta->_frame_user_meta_list = nullptr;
    dx_meta->_num_frame_user_meta = 0;

    new (&dx_meta->_input_tensors) std::map<int, dxs::DXTensors>();
    new (&dx_meta->_output_tensors) std::map<int, dxs::DXTensors>();

    return TRUE;
}

static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer) {
    DXFrameMeta *dx_meta = (DXFrameMeta *)meta;

    for (GList *l = dx_meta->_object_meta_list; l != nullptr; l = l->next) {
        DXObjectMeta *obj_meta = (DXObjectMeta *)l->data;
        dx_release_obj_meta(obj_meta);
    }
    g_list_free(dx_meta->_object_meta_list);
    dx_meta->_object_meta_list = nullptr;

    for (GList *l = dx_meta->_frame_user_meta_list; l != nullptr; l = l->next) {
        DXUserMeta *user_meta = (DXUserMeta *)l->data;
        dx_release_user_meta(user_meta);
    }
    g_list_free(dx_meta->_frame_user_meta_list);
    dx_meta->_frame_user_meta_list = nullptr;

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
}

void copy_tensor(DXFrameMeta *src_meta, DXFrameMeta *dst_meta) {
    // clear tensor
    dst_meta->_input_tensors.clear();
    dst_meta->_output_tensors.clear();

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

void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta) {
    dst_frame_meta->_object_meta_list = nullptr;

    dst_frame_meta->_stream_id = src_frame_meta->_stream_id;
    dst_frame_meta->_width = src_frame_meta->_width;
    dst_frame_meta->_height = src_frame_meta->_height;
    dst_frame_meta->_frame_rate = src_frame_meta->_frame_rate;

    dst_frame_meta->_format = g_strdup(src_frame_meta->_format);
    dst_frame_meta->_name = g_strdup(src_frame_meta->_name);

    dst_frame_meta->_roi[0] = src_frame_meta->_roi[0];
    dst_frame_meta->_roi[1] = src_frame_meta->_roi[1];
    dst_frame_meta->_roi[2] = src_frame_meta->_roi[2];
    dst_frame_meta->_roi[3] = src_frame_meta->_roi[3];

    for (GList *l = src_frame_meta->_object_meta_list; l != nullptr; l = l->next) {
        DXObjectMeta *src_obj_meta = (DXObjectMeta *)l->data;
        DXObjectMeta *dst_obj_meta = dx_acquire_obj_meta_from_pool();
        dx_copy_obj_meta(src_obj_meta, dst_obj_meta);
        dx_add_obj_meta_to_frame(dst_frame_meta, dst_obj_meta);
    }

    dst_frame_meta->_frame_user_meta_list = nullptr;
    dst_frame_meta->_num_frame_user_meta = 0;
    
    for (GList *l = src_frame_meta->_frame_user_meta_list; l != nullptr; l = l->next) {
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
        
        dx_add_user_meta_to_frame(dst_frame_meta, dst_user_meta);
    }

    copy_tensor(src_frame_meta, dst_frame_meta);
}

static gboolean dx_frame_meta_transform(GstBuffer *dest, GstMeta *meta,
                                        GstBuffer *buffer, GQuark type,
                                        gpointer data) {
    DXFrameMeta *src_frame_meta = (DXFrameMeta *)meta;
    DXFrameMeta *exist_frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(dest, DX_FRAME_META_API_TYPE);
    if (exist_frame_meta) {
        return FALSE;
    }
    DXFrameMeta *dst_frame_meta =
        (DXFrameMeta *)gst_buffer_add_meta(dest, DX_FRAME_META_INFO, nullptr);
    dx_frame_meta_copy(buffer, src_frame_meta, dest, dst_frame_meta);
    return TRUE;
}

DXFrameMeta *dx_create_frame_meta(GstBuffer *buffer) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_add_meta(buffer, DX_FRAME_META_INFO, nullptr);
    return frame_meta;
}

DXFrameMeta *dx_get_frame_meta(GstBuffer *buffer) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    return frame_meta;
}

gboolean dx_add_obj_meta_to_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    if (!frame_meta || !obj_meta) return FALSE;
    
    frame_meta->_object_meta_list = g_list_append(frame_meta->_object_meta_list, obj_meta);
    return TRUE;
}

gboolean dx_remove_obj_meta_from_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    if (!frame_meta || !obj_meta) return FALSE;
    
    frame_meta->_object_meta_list = g_list_remove(frame_meta->_object_meta_list, obj_meta);
    dx_release_obj_meta(obj_meta);
    return TRUE;
}
