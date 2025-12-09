#ifndef __DX_MSGCONVL_PRIV_H__
#define __DX_MSGCONVL_PRIV_H__

#include "dx_stream/gst-dxmeta.hpp"
#include "dx_stream/gst-dxmsgmeta.hpp"

// private property from config file
typedef struct _DxMsgContextPriv {
    guint _customId;
    std::vector<std::string> _object_include_list;

} DxMsgContextPriv;

DxMsgContextPriv *dxcontext_create_contextPriv(void);
void dxcontext_delete_contextPriv(DxMsgContextPriv *contextPriv);

bool dxcontext_parse_json_config(const gchar *file,
                                 DxMsgContextPriv *contextPriv);

gchar *dxpayload_convert_to_json(DxMsgContext *context,
                                 GstDxMsgMetaInfo *meta_info);

#endif /* __DX_MSGCONVL_PRIV_H__ */