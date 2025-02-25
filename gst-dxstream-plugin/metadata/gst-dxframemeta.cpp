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

    dx_meta->_roi[0] = -1;
    dx_meta->_roi[1] = -1;
    dx_meta->_roi[2] = -1;
    dx_meta->_roi[3] = -1;

    new (&dx_meta->_input_memory_pool) std::map<int, MemoryPool *>();
    new (&dx_meta->_output_memory_pool) std::map<int, MemoryPool *>();
    new (&dx_meta->_input_tensor) std::map<int, dxs::DXTensor>();
    new (&dx_meta->_output_tensor) std::map<int, std::vector<dxs::DXTensor>>();

    return TRUE;
}

static void dx_frame_meta_free(GstMeta *meta, GstBuffer *buffer) {
    DXFrameMeta *dx_meta = (DXFrameMeta *)meta;

    g_list_free(dx_meta->_object_meta_list);
    dx_meta->_object_meta_list = NULL;

    dx_meta->_buf = NULL;

    for (auto &tmp : dx_meta->_input_memory_pool) {
        MemoryPool *pool = (MemoryPool *)tmp.second;
        int preproc_id = tmp.first;
        auto tensor = dx_meta->_input_tensor.find(preproc_id);
        if (tensor != dx_meta->_input_tensor.end()) {
            pool->deallocate(tensor->second._data);
        }
    }
    dx_meta->_input_memory_pool.clear();
    dx_meta->_input_tensor.clear();

    for (auto &tmp : dx_meta->_output_memory_pool) {
        MemoryPool *pool = (MemoryPool *)tmp.second;
        int preproc_id = tmp.first;
        auto tensor = dx_meta->_output_tensor.find(preproc_id);
        if (tensor != dx_meta->_output_tensor.end()) {
            pool->deallocate(tensor->second[0]._data);
        }
    }
    dx_meta->_output_memory_pool.clear();
    dx_meta->_output_tensor.clear();
}

void copy_tensor(DXFrameMeta *src_meta, DXFrameMeta *dst_meta) {
    // clear pool & tensor
    dst_meta->_input_memory_pool.clear();
    for (auto &pool : src_meta->_input_memory_pool) {
        dst_meta->_input_memory_pool[pool.first] = pool.second;
    }
    dst_meta->_input_tensor.clear();

    dst_meta->_output_memory_pool.clear();
    for (auto &pool : src_meta->_output_memory_pool) {
        dst_meta->_output_memory_pool[pool.first] = pool.second;
    }
    dst_meta->_output_tensor.clear();

    // copy tensor
    for (auto &input_tensor : src_meta->_input_tensor) {
        dxs::DXTensor new_tensor;

        new_tensor._name = input_tensor.second._name;
        new_tensor._shape = input_tensor.second._shape;
        new_tensor._type = input_tensor.second._type;
        new_tensor._data =
            dst_meta->_input_memory_pool[input_tensor.first]->allocate();
        new_tensor._phyAddr = input_tensor.second._phyAddr;
        new_tensor._elemSize = input_tensor.second._elemSize;

        memcpy(
            new_tensor._data, input_tensor.second._data,
            dst_meta->_input_memory_pool[input_tensor.first]->get_block_size());

        dst_meta->_input_tensor[input_tensor.first] = new_tensor;
    }

    for (auto &temp : src_meta->_output_tensor) {
        void *data = dst_meta->_output_memory_pool[temp.first]->allocate();
        memcpy(data, src_meta->_output_tensor[temp.first][0]._data,
               dst_meta->_output_memory_pool[temp.first]->get_block_size());
        dst_meta->_output_tensor[temp.first] = std::vector<dxs::DXTensor>();
        for (auto &tensor : temp.second) {
            dxs::DXTensor new_tensor;
            new_tensor._name = tensor._name;
            new_tensor._shape = tensor._shape;
            new_tensor._type = tensor._type;
            new_tensor._data = static_cast<void *>(
                static_cast<uint8_t *>(data) + tensor._phyAddr);
            new_tensor._phyAddr = tensor._phyAddr;
            new_tensor._elemSize = tensor._elemSize;
            dst_meta->_output_tensor[temp.first].push_back(new_tensor);
        }
    }
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
        (DXFrameMeta *)gst_buffer_add_meta(dest, DX_FRAME_META_INFO, NULL);
    dx_frame_meta_copy(buffer, src_frame_meta, dest, dst_frame_meta);
    dst_frame_meta->_buf = dest;
    return TRUE;
}