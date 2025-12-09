#ifndef __GST_DXMSGCONV_H__
#define __GST_DXMSGCONV_H__

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include "gst-dxmsgmeta.hpp"

G_BEGIN_DECLS

#define GST_TYPE_DXMSGCONV (gst_dxmsgconv_get_type())

G_DECLARE_FINAL_TYPE(GstDxMsgConv, gst_dxmsgconv, GST, DXMSGCONV,
                     GstBaseTransform)

typedef DxMsgContext *(*DXMsg_CreateContextFptr)();
typedef void (*DXMsg_DeleteContextFptr)(DxMsgContext *context);
typedef DxMsgPayload *(*DXMsg_ConvertPayloadFptr)(DxMsgContext *context,
                                                  GstDxMsgMetaInfo *meta_info);
struct _GstDxMsgConv {
    GstBaseTransform _parent_instance;

    guint64 _seq_id;
    guint _message_interval;
    GstVideoInfo _input_info;
    gchar *_config_file_path;
    gchar *_library_file_path;
    void *_library_handle;

    DxMsgContext *_context;

    DXMsg_CreateContextFptr _create_context_function;
    DXMsg_DeleteContextFptr _delete_context_function;
    DXMsg_ConvertPayloadFptr _convert_payload_function;
};

G_END_DECLS

#endif /* __GST_DXMSGCONV_H__ */