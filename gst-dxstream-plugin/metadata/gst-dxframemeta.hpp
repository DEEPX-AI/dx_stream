#ifndef DXFRAMEMETA_H
#define DXFRAMEMETA_H

#include "dxcommon.hpp"
#include "memory_pool.hpp"
#include <dxrt/dxrt_api.h>
#include <glib.h>
#include <gst/gst.h>
#include <map>
#include <opencv2/opencv.hpp>
#include <vector>

G_BEGIN_DECLS

#define DX_FRAME_META_API_TYPE (dx_frame_meta_api_get_type())
#define DX_FRAME_META_INFO (dx_frame_meta_get_info())

typedef struct _DXFrameMeta DXFrameMeta;

struct _DXFrameMeta {
    GstMeta _meta;

    GstBuffer *_buf;

    gint _stream_id;
    gint _width;
    gint _height;
    const gchar *_format;
    const gchar *_name;
    gfloat _frame_rate;

    int _roi[4];

    GList *_object_meta_list;

    std::map<int, MemoryPool *> _input_memory_pool;
    std::map<int, MemoryPool *> _output_memory_pool;

    std::map<int, dxs::DXTensor> _input_tensor;
    std::map<int, std::vector<dxs::DXTensor>> _output_tensor;

    // std::map<int, dxs::DXNetworkInput> _input_tensor;
    // std::map<int, std::map<void *, dxs::DXNetworkInput>>
    // _input_object_tensor;

    // std::map<int, dxrt::TensorPtrs> _output_tensor;
    // std::map<int, std::map<void *, dxrt::TensorPtrs>> _output_object_tensor;
};

GType dx_frame_meta_api_get_type(void);
const GstMetaInfo *dx_frame_meta_get_info(void);
void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta);

G_END_DECLS

#endif /* DXFRAMEMETA_H */
