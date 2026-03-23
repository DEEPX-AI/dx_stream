#ifndef __GST_DXMSGBROKER_H__
#define __GST_DXMSGBROKER_H__

#include <gst/base/gstbasesink.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXMSGBROKER (gst_dxmsgbroker_get_type())
G_DECLARE_FINAL_TYPE(GstDxMsgBroker, gst_dxmsgbroker, GST, DXMSGBROKER,
                     GstBaseSink)

/**
 * BAL: Broker Abstraction Layer
 */

/* BAL Error */
enum class DxMsg_Bal_Error {
    DXMSG_BAL_OK,
    DXMSG_BAL_ERR_INVALID,
    DXMSG_BAL_ERR_BROKER,
    DXMSG_BAL_ERR_UNKNOWN
};
using DxMsg_Bal_Error_t = DxMsg_Bal_Error;

/* BAL Handle */
struct DxMsg_Bal_Handle;
using DxMsg_Bal_Handle_t = DxMsg_Bal_Handle*;

/* BAL method */
using DxMsg_Bal_ConnectFptr_t = DxMsg_Bal_Handle_t (*)(char *conn_info,
                                                        char *cfg_path);
using DxMsg_Bal_SendFptr_t = DxMsg_Bal_Error_t (*)(DxMsg_Bal_Handle_t handle,
                                                    const char *topic,
                                                    const void *payload,
                                                    int payload_len);
using DxMsg_Bal_DisconnectFptr_t = DxMsg_Bal_Error_t (*)(
    DxMsg_Bal_Handle_t handle);

struct _GstDxMsgBroker {
    GstBaseSink parent;

    DxMsg_Bal_Handle_t _handle;

    gchar *_conn_info;
    gchar *_config;
    gchar *_topic;
    gchar *_broker_name; /*mqtt, kafka*/

    DxMsg_Bal_ConnectFptr_t _connect_function;
    DxMsg_Bal_SendFptr_t _send_function;
    DxMsg_Bal_DisconnectFptr_t _disconnect_function;

    /*debug purpose*/
    guint64 _msgbroker_count;
};

G_END_DECLS

#endif /* __GST_DXMSGBROKER_H__ */