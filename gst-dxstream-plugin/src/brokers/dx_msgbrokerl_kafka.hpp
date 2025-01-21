#ifndef __DX_MSGBROKERL_KAFKA_H__
#define __DX_MSGBROKERL_KAFKA_H__

#include "gst-dxmsgbroker.hpp"

DxMsg_Bal_Handle_t dxmsg_bal_connect_kafka(char *conn_info, char *cfg_file);
DxMsg_Bal_Error_t dxmsg_bal_send_kafka(DxMsg_Bal_Handle_t handle, char *topic,
                                       const void *payload, int payload_len);
DxMsg_Bal_Error_t dxmsg_bal_disconnect_kafka(DxMsg_Bal_Handle_t handle);

#endif /* __DX_MSGBROKERL_KAFKA_H__ */