#include "gst-dxframemeta.hpp"
#include "gst-dxmeta.hpp"

static gboolean dx_frame_meta_init(GstMeta *meta, gpointer params,
                                   GstBuffer *buffer);
static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean dx_frame_meta_transform(GstBuffer *dest, GstMeta *meta,
                                        GstBuffer *buffer, GQuark type,
                                        gpointer data);

GType dx_frame_meta_api_get_type(void) {
    static GType type;
    static const gchar *tags[] = {"dx_frame_meta", NULL};

    if (g_once_init_enter(&type)) {
        GType _type = gst_meta_api_type_register("DXFrameMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }

    return type;
}

const GstMetaInfo *dx_frame_meta_get_info(void) {
    static const GstMetaInfo *meta_info = NULL;

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
    dx_meta->_buf = NULL;
    dx_meta->_object_meta_list = NULL;
    dx_meta->_stream_id = -1;
    dx_meta->_width = -1;
    dx_meta->_height = -1;

    new (&dx_meta->_input_tensor) std::map<int, dxs::DXNetworkInput>();
    new (&dx_meta->_input_object_tensor)
        std::map<int, std::map<void *, dxs::DXNetworkInput>>();
    dx_meta->_input_tensor.clear();
    dx_meta->_input_object_tensor.clear();

    new (&dx_meta->_input_memory_pool) std::map<int, MemoryPool *>();

    dx_meta->_roi[0] = -1;
    dx_meta->_roi[1] = -1;
    dx_meta->_roi[2] = -1;
    dx_meta->_roi[3] = -1;

    return TRUE;
}

static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer) {
    DXFrameMeta *dx_meta = (DXFrameMeta *)meta;

    g_list_free(dx_meta->_object_meta_list);
    dx_meta->_object_meta_list = NULL;

    dx_meta->_buf = NULL;

    for (auto &tmp : dx_meta->_input_memory_pool) {
        MemoryPool *pool = (MemoryPool *)tmp.second;
        auto tensor_tmp = dx_meta->_input_tensor.find(tmp.first);
        if (tensor_tmp != dx_meta->_input_tensor.end()) {
            pool->deallocate(tensor_tmp->second._data);
            tensor_tmp->second._data = nullptr;
        }
        auto object_tensor_tmp = dx_meta->_input_object_tensor.find(tmp.first);
        if (object_tensor_tmp != dx_meta->_input_object_tensor.end()) {
            std::map<void *, dxs::DXNetworkInput> &inner_map =
                object_tensor_tmp->second;
            for (auto &tensor : inner_map) {
                pool->deallocate(tensor.second._data);
                tensor.second._data = nullptr;
            }
        }
    }
    for (auto &outer_entry : dx_meta->_input_object_tensor) {
        std::map<void *, dxs::DXNetworkInput> &inner_map = outer_entry.second;
        inner_map.clear();
    }
    dx_meta->_input_memory_pool.clear();
    dx_meta->_input_tensor.clear();
    dx_meta->_input_object_tensor.clear();
}

void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta) {
    dst_frame_meta->_object_meta_list = NULL;

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

    dst_frame_meta->_input_memory_pool.clear();
    for (auto &pool : src_frame_meta->_input_memory_pool) {
        dst_frame_meta->_input_memory_pool[pool.first] = pool.second;
    }

    for (auto &outer_entry : dst_frame_meta->_input_object_tensor) {
        std::map<void *, dxs::DXNetworkInput> &inner_map = outer_entry.second;
        inner_map.clear();
    }
    dst_frame_meta->_input_tensor.clear();
    dst_frame_meta->_input_object_tensor.clear();

    for (auto &input_tensor : src_frame_meta->_input_tensor) {
        dst_frame_meta->_input_tensor[input_tensor.first] = dxs::DXNetworkInput(
            dst_frame_meta->_input_memory_pool[input_tensor.first]
                ->get_block_size(),
            dst_frame_meta->_input_memory_pool[input_tensor.first]->allocate());
        memcpy(dst_frame_meta->_input_tensor[input_tensor.first]._data,
               src_frame_meta->_input_tensor[input_tensor.first]._data,
               dst_frame_meta->_input_memory_pool[input_tensor.first]
                   ->get_block_size());
    }
    for (auto &input_object_tensor : src_frame_meta->_input_object_tensor) {
        dst_frame_meta->_input_object_tensor[input_object_tensor.first] =
            std::map<void *, dxs::DXNetworkInput>();
        for (auto &tensor : input_object_tensor.second) {
            dst_frame_meta
                ->_input_object_tensor[input_object_tensor.first]
                                      [tensor.first] = dxs::DXNetworkInput(
                dst_frame_meta->_input_memory_pool[input_object_tensor.first]
                    ->get_block_size(),
                dst_frame_meta->_input_memory_pool[input_object_tensor.first]
                    ->allocate());
            memcpy(dst_frame_meta
                       ->_input_object_tensor[input_object_tensor.first]
                                             [tensor.first]
                       ._data,
                   src_frame_meta
                       ->_input_object_tensor[input_object_tensor.first]
                                             [tensor.first]
                       ._data,
                   dst_frame_meta->_input_memory_pool[input_object_tensor.first]
                       ->get_block_size());
        }
    }
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
        (DXFrameMeta *)gst_buffer_add_meta(dest, DX_FRAME_META_INFO, NULL);
    dx_frame_meta_copy(buffer, src_frame_meta, dest, dst_frame_meta);
    dst_frame_meta->_buf = dest;
    return TRUE;
}