#ifndef DXFRAMEMETA_H
#define DXFRAMEMETA_H

#include "dxcommon.hpp"
#include <glib.h>
#include <gst/gst.h>
#include <map>
#include <vector>

G_BEGIN_DECLS

#define DX_FRAME_META_API_TYPE (dx_frame_meta_api_get_type())
#define DX_FRAME_META_INFO (dx_frame_meta_get_info())

using DXFrameMeta = struct _DXFrameMeta;
using DXObjectMeta = struct _DXObjectMeta;
using DXUserMeta = struct _DXUserMeta;  // Forward declaration

struct _DXFrameMeta {
    GstMeta _meta;
    
    int _stream_id;
    int _width;
    int _height;
    std::string _format;
    std::string _name;
    float _frame_rate;

    int _roi[4];

    // segmentation
    std::vector<unsigned char> _seg_data;
    int _seg_width = 0;
    int _seg_height = 0;

    // classification result (primary mode)
    int _label;
    std::string _label_name;
    float _label_confidence;

    std::vector<DXObjectMeta*> _object_meta_list;

    std::vector<DXUserMeta*> _frame_user_meta_list;

    // RAII-managed tensors (shallow copy through shared_ptr)
    std::map<int, dxs::DXTensors> _input_tensors;   // preproc_id -> input tensors
    std::map<int, dxs::DXTensors> _output_tensors;   // infer_id -> output tensors
};

DX_API GType dx_frame_meta_api_get_type(void);
DX_API const GstMetaInfo *dx_frame_meta_get_info(void);
DX_API void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta);

DX_API GstBuffer* dx_create_frame_meta(GstBuffer *buffer);
DX_API DXFrameMeta *dx_get_frame_meta(GstBuffer *buffer);
DX_API gboolean dx_add_obj_meta_to_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta);
DX_API gboolean dx_remove_obj_meta_from_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta);

G_END_DECLS

#endif /* DXFRAMEMETA_H */
