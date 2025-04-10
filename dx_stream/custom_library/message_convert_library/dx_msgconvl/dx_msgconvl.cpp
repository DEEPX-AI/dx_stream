#include "dx_msgconvl_priv.hpp"

extern "C" DxMsgContext *dxmsg_create_context(const gchar *file) {
    DxMsgContext *context = g_new0(DxMsgContext, 1);

    context->_priv_data = (void *)dxcontext_create_contextPriv();
    dxcontext_parse_json_config(file, (DxMsgContextPriv *)context->_priv_data);

    // GST_INFO("|JCP-M| create context=%p, context->_priv_data=%p\n", context,
    //          context->_priv_data);
    return context;
}

extern "C" void dxmsg_delete_context(DxMsgContext *context) {
    g_return_if_fail(context != nullptr);

    // GST_INFO("|JCP-M| delete context=%p, context->_priv_data=%p\n", context,
    //          context->_priv_data);
    dxcontext_delete_contextPriv((DxMsgContextPriv *)context->_priv_data);
    // delete DxMsgContext
    g_free(context);
}

extern "C" DxMsgPayload *dxmsg_convert_payload(DxMsgContext *context,
                                               DxMsgMetaInfo *meta_info) {
    DxMsgPayload *payload = g_new0(DxMsgPayload, 1);
    gchar *json_data = dxpayload_convert_to_json(context, meta_info);

    if (json_data == nullptr) {
        g_free(payload);
        return nullptr;
    }

    payload->_size = strlen(json_data);
    payload->_data = json_data;

    // GST_INFO("|JCP-M| create payload=%p, payload->_data=%p\n", payload,
    //          payload->_data);
    return payload;
}

extern "C" void dxmsg_release_payload(DxMsgContext *context,
                                      DxMsgPayload *payload) {
    // g_return_if_fail(context != nullptr);
    g_return_if_fail(payload != nullptr);

    // GST_INFO("|JCP-M| delete payload=%p, payload->_data=%p\n", payload,
    //          payload->_data);
    g_free(payload->_data);
    g_free(payload);
}
