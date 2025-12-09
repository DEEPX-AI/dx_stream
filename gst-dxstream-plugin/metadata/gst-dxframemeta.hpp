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

typedef struct _DXFrameMeta DXFrameMeta;

struct _DXFrameMeta {
    GstMeta _meta;
    
    gint _stream_id;
    gint _width;
    gint _height;
    const gchar *_format;
    const gchar *_name;
    gfloat _frame_rate;

    int _roi[4];

    GList *_object_meta_list;

    std::map<int, dxs::DXTensors> _input_tensors;
    std::map<int, dxs::DXTensors> _output_tensors;
};

GType dx_frame_meta_api_get_type(void);
const GstMetaInfo *dx_frame_meta_get_info(void);
void dx_frame_meta_copy(GstBuffer *src_buffer, DXFrameMeta *src_frame_meta,
                        GstBuffer *dst_buffer, DXFrameMeta *dst_frame_meta);

G_END_DECLS

#endif /* DXFRAMEMETA_H */
