#ifndef __GST_DXMSGCONV_H__
#define __GST_DXMSGCONV_H__

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include "gst-dxmsgmeta.hpp"
#include "transforms/transform_kernel_pool.hpp"

#include <map>
#include <memory>
#include <vector>

G_BEGIN_DECLS

#define GST_TYPE_DXMSGCONV (gst_dxmsgconv_get_type())

G_DECLARE_FINAL_TYPE(GstDxMsgConv, gst_dxmsgconv, GST, DXMSGCONV,
                     GstBaseTransform)

using DXMsg_CreateContextFptr = DxMsgContext *(*)();
using DXMsg_DeleteContextFptr = void (*)(DxMsgContext *context);
using DXMsg_ConvertPayloadFptr = DxMsgPayload *(*)(DxMsgContext *context,
                                                   GstDxMsgMetaInfo *meta_info);
struct _GstDxMsgConv {
    GstBaseTransform _parent_instance;

    std::map<int, guint64> _seq_ids;
    guint _message_interval;
    GstVideoInfo _input_info;
    gchar *_config_file_path;
    gchar *_library_file_path;
    void *_library_handle;
    gboolean _include_frame;
    int _cached_width;
    int _cached_height;
    GstVideoFormat _cached_format;

    DxMsgContext *_context;

    DXMsg_CreateContextFptr _create_context_function;
    DXMsg_DeleteContextFptr _delete_context_function;
    DXMsg_ConvertPayloadFptr _convert_payload_function;

    std::unique_ptr<dxt::TransformKernelPool> _kernel_pool;
    std::vector<uint8_t> _rgb_buf;
    std::vector<unsigned char> _jpeg_buf;
};

G_END_DECLS

#endif /* __GST_DXMSGCONV_H__ */