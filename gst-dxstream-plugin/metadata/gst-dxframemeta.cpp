#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"
#include <algorithm>  // for std::find

GST_DEBUG_CATEGORY_EXTERN(dxframemeta_cat);
#define GST_CAT_DEFAULT dxframemeta_cat

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

static gboolean dx_frame_meta_init(GstMeta *meta, gpointer params,
                                   GstBuffer *buffer);
static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean dx_frame_meta_transform(GstBuffer *dest, GstMeta *meta,
                                        GstBuffer *buffer, GQuark type,
                                        gpointer data);

GType dx_frame_meta_api_get_type(void) {
    static GType type;

    if (g_once_init_enter(&type)) {
        static const gchar* tags[] = {"dx_frame_meta", nullptr};  // NOSONAR - GStreamer API requires C-style array (const gchar**)
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
    std::ignore = params;
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Initializing DXFrameMeta");
    auto *dx_meta = (DXFrameMeta *)meta;
    
    dx_meta->_stream_id = -1;
    dx_meta->_width = -1;
    dx_meta->_height = -1;

    dx_meta->_roi[0] = -1;
    dx_meta->_roi[1] = -1;
    dx_meta->_roi[2] = -1;
    dx_meta->_roi[3] = -1;

    dx_meta->_seg_width = 0;
    dx_meta->_seg_height = 0;

    dx_meta->_label = -1;
    dx_meta->_label_confidence = 0.0f;

    // Initialize C++ objects with placement new
    new (&dx_meta->_format) std::string();
    new (&dx_meta->_name) std::string();
    new (&dx_meta->_object_meta_list) std::vector<DXObjectMeta*>();
    new (&dx_meta->_frame_user_meta_list) std::vector<DXUserMeta*>();
    new (&dx_meta->_input_tensors) std::map<int, dxs::DXTensors>();
    new (&dx_meta->_output_tensors) std::map<int, dxs::DXTensors>();
    new (&dx_meta->_seg_data) std::vector<unsigned char>();
    new (&dx_meta->_label_name) std::string();

    return TRUE;
}

static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer) {
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Freeing DXFrameMeta");
    auto *dx_meta = (DXFrameMeta *)meta;

    // Release object metadata
    for (auto *obj_meta : dx_meta->_object_meta_list) {
        dx_release_obj_meta(obj_meta);
    }
    dx_meta->_object_meta_list.clear();

    // Release user metadata
    for (auto *user_meta : dx_meta->_frame_user_meta_list) {
        dx_release_user_meta(user_meta);
    }
    dx_meta->_frame_user_meta_list.clear();

    // RAII: shared_ptr automatically releases memory when ref count reaches 0
    dx_meta->_input_tensors.clear();
    dx_meta->_output_tensors.clear();
    
    // NOSONAR - Explicit destructor calls required for placement new objects
    // Objects were constructed with placement new in dx_frame_meta_init,
    // so destructors must be called explicitly before GStreamer frees the memory
    dx_meta->_format.~basic_string(); // NOSONAR
    dx_meta->_name.~basic_string(); // NOSONAR
    dx_meta->_label_name.~basic_string(); // NOSONAR
    dx_meta->_object_meta_list.~vector(); // NOSONAR
    dx_meta->_frame_user_meta_list.~vector(); // NOSONAR
    dx_meta->_seg_data.~vector(); // NOSONAR
    dx_meta->_input_tensors.~map(); // NOSONAR
    dx_meta->_output_tensors.~map(); // NOSONAR
}

void copy_tensor(DXFrameMeta *src_meta, DXFrameMeta *dst_meta) {
    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Shallow copying DXFrameMeta tensors (shared ownership)");

    // Shallow copy: shared_ptr<void> reference counts are increased
    // - DXTensors._data is shared_ptr<void>, so data is shared
    // Memory is automatically freed when last reference is released
    dst_meta->_input_tensors = src_meta->_input_tensors;
    dst_meta->_output_tensors = src_meta->_output_tensors;
}

void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta) {
    std::ignore = src_buffer;
    std::ignore = dst_buffer;
    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Copying DXFrameMeta");

    dst_frame_meta->_stream_id = src_frame_meta->_stream_id;
    dst_frame_meta->_width = src_frame_meta->_width;
    dst_frame_meta->_height = src_frame_meta->_height;
    dst_frame_meta->_frame_rate = src_frame_meta->_frame_rate;

    dst_frame_meta->_format = src_frame_meta->_format;
    dst_frame_meta->_name = src_frame_meta->_name;

    dst_frame_meta->_label = src_frame_meta->_label;
    dst_frame_meta->_label_name = src_frame_meta->_label_name;
    dst_frame_meta->_label_confidence = src_frame_meta->_label_confidence;

    dst_frame_meta->_roi[0] = src_frame_meta->_roi[0];
    dst_frame_meta->_roi[1] = src_frame_meta->_roi[1];
    dst_frame_meta->_roi[2] = src_frame_meta->_roi[2];
    dst_frame_meta->_roi[3] = src_frame_meta->_roi[3];

    // Copy segmentation data
    if (!src_frame_meta->_seg_data.empty()) {
        dst_frame_meta->_seg_data = src_frame_meta->_seg_data;
        dst_frame_meta->_seg_width = src_frame_meta->_seg_width;
        dst_frame_meta->_seg_height = src_frame_meta->_seg_height;
    }

    // Deep copy object metadata
    dst_frame_meta->_object_meta_list.clear();
    for (auto *src_obj_meta : src_frame_meta->_object_meta_list) {
        auto *dst_obj_meta = dx_acquire_obj_meta_from_pool();
        dx_copy_obj_meta(src_obj_meta, dst_obj_meta);
        dst_frame_meta->_object_meta_list.push_back(dst_obj_meta);
    }

    // Deep copy user metadata
    dst_frame_meta->_frame_user_meta_list.clear();
    for (auto *src_user_meta : src_frame_meta->_frame_user_meta_list) {
        auto *dst_user_meta = dx_acquire_user_meta_from_pool();
        
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
        
        dst_frame_meta->_frame_user_meta_list.push_back(dst_user_meta);
    }

    copy_tensor(src_frame_meta, dst_frame_meta);
}

static gboolean dx_frame_meta_transform(GstBuffer *dest, GstMeta *meta,
                                        GstBuffer *buffer, GQuark type,
                                        gpointer data) {
    std::ignore = type;
    std::ignore = data;
    std::ignore = buffer;

    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Transforming DXFrameMeta");
    auto *src_frame_meta = (DXFrameMeta *)meta;
    const auto *exist_frame_meta = dx_get_frame_meta(dest);
    if (exist_frame_meta) {
        return FALSE;
    }
    dest = dx_create_frame_meta(dest);
    auto *dst_frame_meta = dx_get_frame_meta(dest);
    
    dx_frame_meta_copy(buffer, src_frame_meta, dest, dst_frame_meta);
    return TRUE;
}

GstBuffer* dx_create_frame_meta(GstBuffer *buffer) {
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }
    gst_buffer_add_meta(buffer, DX_FRAME_META_INFO, nullptr);
    return buffer;
}

DXFrameMeta *dx_get_frame_meta(GstBuffer *buffer) {
    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Getting DXFrameMeta");
    auto *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    return frame_meta;
}

gboolean dx_add_obj_meta_to_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Adding DXObjectMeta to DXFrameMeta");
    if (!frame_meta || !obj_meta) return FALSE;
    
    frame_meta->_object_meta_list.push_back(obj_meta);
    return TRUE;
}

gboolean dx_remove_obj_meta_from_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    GST_CAT_DEBUG_SAFE(dxframemeta_cat, "Removing DXObjectMeta from DXFrameMeta");
    if (!frame_meta || !obj_meta) return FALSE;
    
    auto it = std::find(frame_meta->_object_meta_list.begin(), 
                        frame_meta->_object_meta_list.end(), 
                        obj_meta);
    if (it != frame_meta->_object_meta_list.end()) {
        frame_meta->_object_meta_list.erase(it);
        dx_release_obj_meta(obj_meta);
        return TRUE;
    }
    return FALSE;
}
