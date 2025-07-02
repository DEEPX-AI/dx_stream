#include "dx_msgconvl_priv.hpp"
#include <glib.h>
#include <stddef.h>
#include <string.h>

#define MAX_EXPECTED_JSON_SIZE ((size_t)(10 * 1024 * 1024))

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
    if (!payload) {
        g_warning("Failed to allocate DxMsgPayload");
        return nullptr;
    }

    gchar *json_data = dxpayload_convert_to_json(context, meta_info);

    if (json_data == nullptr) {
        g_warning("dxpayload_convert_to_json returned null");
        g_free(payload);
        return nullptr;
    }
    size_t json_len = strnlen(json_data, MAX_EXPECTED_JSON_SIZE);
    if (json_len == MAX_EXPECTED_JSON_SIZE &&
        json_data[MAX_EXPECTED_JSON_SIZE - 1] != '\0') {
        g_warning("JSON data is too long (>= %zu bytes) or not null-terminated "
                  "within the checked limit.",
                  MAX_EXPECTED_JSON_SIZE);
        g_free(json_data);
        g_free(payload);
        return nullptr;
    }

    payload->_size = json_len;
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
