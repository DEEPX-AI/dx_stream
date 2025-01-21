#include <gst/gst.h>

#include "gst-dxmsgbroker.hpp"

#include "dx_msgbrokerl_mqtt.hpp"

#include <fstream>
#include <mosquitto.h>
#include <sstream>
#include <string>

/* debug */
GST_DEBUG_CATEGORY_STATIC(broker);
#define GST_CAT_DEFAULT broker

/* Local */
#define MAX_MQTT_FIELD_LEN 1024
typedef struct {
    struct mosquitto *_mosq;

    char _username[MAX_MQTT_FIELD_LEN];
    char _password[MAX_MQTT_FIELD_LEN];
    char _clientid[MAX_MQTT_FIELD_LEN];

    bool _tls_enable;
    char _tls_cafile[MAX_MQTT_FIELD_LEN];
    char _tls_capath[MAX_MQTT_FIELD_LEN];
    char _tls_certfile[MAX_MQTT_FIELD_LEN];
    char _tls_keyfile[MAX_MQTT_FIELD_LEN];

} MqttClientInfo_t;
// typedef MqttClientInfo_t *MqttClientHandle_t;

static DxMsg_Bal_Error_t dxmsg_bal_read_config_mqtt(DxMsg_Bal_Handle_t handle,
                                                    const char *cfg_file) {
    MqttClientInfo_t *pClient = (MqttClientInfo_t *)handle;

    g_return_val_if_fail(handle != nullptr, DXMSG_BAL_ERR_INVALID);
    g_return_val_if_fail(cfg_file != nullptr, DXMSG_BAL_ERR_INVALID);

    if (!g_file_test(cfg_file, G_FILE_TEST_EXISTS)) {
        return DXMSG_BAL_ERR_UNKNOWN;
    }

    std::ifstream file(cfg_file);
    if (!file.is_open()) {
        return DXMSG_BAL_ERR_UNKNOWN;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            key.erase(key.find_last_not_of(" \n\r\t") + 1);
            value.erase(0, value.find_first_not_of(" \n\r\t"));

            if (key == "username") {
                g_strlcpy(pClient->_username, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "password") {
                g_strlcpy(pClient->_password, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "client-id") {
                g_strlcpy(pClient->_clientid, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "tls_enable") {
                pClient->_tls_enable = (value == "1");
            } else if (key == "tls_cafile") {
                g_strlcpy(pClient->_tls_cafile, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "tls_capath") {
                g_strlcpy(pClient->_tls_capath, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "tls_certfile") {
                g_strlcpy(pClient->_tls_certfile, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            } else if (key == "tls_keyfile") {
                g_strlcpy(pClient->_tls_keyfile, value.c_str(),
                          MAX_MQTT_FIELD_LEN - 1);
            }
        }
    }

    file.close();
    return DXMSG_BAL_OK;
}

static bool dxmsg_bal_is_valid_connInfo_mqtt(const char *conn_info,
                                             std::string &hostname,
                                             int *portnum) {

    g_return_val_if_fail(conn_info != nullptr, false);

    std::string conn_info_str(conn_info);
    size_t delimiter_pos = conn_info_str.find(':');
    if (delimiter_pos == std::string::npos) {
        return false;
    }

    hostname = conn_info_str.substr(0, delimiter_pos);
    std::string port_str = conn_info_str.substr(delimiter_pos + 1);

    try {
        int port = std::stoi(port_str);
        if (port <= 0) {
            return false;
        }
        *portnum = port;
    } catch (const std::invalid_argument &e) {
        return false;
    } catch (const std::out_of_range &e) {
        return false;
    }

    return true;
}

DxMsg_Bal_Handle_t dxmsg_bal_connect_mqtt(char *conn_info, char *cfg_path) {
    MqttClientInfo_t *pClient = g_new0(MqttClientInfo_t, 1);
    std::string host = "";
    int port;
    int rc;

    GST_DEBUG_CATEGORY_INIT(broker, "broker", 0,
                            "broker category for dxmsgbroker element");
    GST_TRACE("|JCP|\n");

    /* conn_info => host, port */
    if (!dxmsg_bal_is_valid_connInfo_mqtt(conn_info, host, &port)) {
        GST_ERROR("Error, Invalid connection info: %s\n", conn_info);
        g_free(pClient);
        return nullptr;
    }

    /* read config file */
    if (cfg_path != nullptr) {
        if (dxmsg_bal_read_config_mqtt((DxMsg_Bal_Handle_t)pClient, cfg_path) !=
            DXMSG_BAL_OK) {
            GST_ERROR("Error, Failed to read config file: %s\n", cfg_path);
            g_free(pClient);
            return nullptr;
        }
    }

    /* mosquitto_lib_init */
    rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, mosquitto_lib_init() failed: %s\n",
                  mosquitto_strerror(rc));
        g_free(pClient);
        return nullptr;
    }

    /* mosquitto_new */
    if (pClient->_clientid[0] != '\0') {
        pClient->_mosq = mosquitto_new(pClient->_clientid, true, NULL);
    } else {
        pClient->_mosq = mosquitto_new(NULL, true, NULL);
    }
    if (pClient->_mosq == NULL) {
        GST_ERROR("Error, mosquitto_new() failed: %s\n",
                  mosquitto_strerror(rc));
        mosquitto_lib_cleanup();
        g_free(pClient);
        return nullptr;
    }

    /* mosquitto_tls_set */
    if (pClient->_tls_enable) {
        char *cafile = pClient->_tls_cafile[0] ? pClient->_tls_cafile : NULL;
        char *capath = pClient->_tls_capath[0] ? pClient->_tls_capath : NULL;
        char *certfile =
            pClient->_tls_certfile[0] ? pClient->_tls_certfile : NULL;
        char *keyfile = pClient->_tls_keyfile[0] ? pClient->_tls_keyfile : NULL;
        rc = mosquitto_tls_set(pClient->_mosq, cafile, capath, certfile,
                               keyfile, NULL);
        if (rc != MOSQ_ERR_SUCCESS) {
            GST_ERROR("Error, Failed to set TLS: %s\n", mosquitto_strerror(rc));
            goto Error;
        } else {
            GST_DEBUG("Set TLS: %s, %s, %s, %s\n", cafile, capath, certfile,
                      keyfile);
        }
    }

    /* mosquitto_username_pw_set */
    if (pClient->_username[0] != 0 && pClient->_password[0] != 0) {
        rc = mosquitto_username_pw_set(pClient->_mosq, pClient->_username,
                                       pClient->_password);
        if (rc != MOSQ_ERR_SUCCESS) {
            GST_ERROR("Error, Failed to set username and password: %s\n",
                      mosquitto_strerror(rc));
            goto Error;
        } else {
            GST_DEBUG("Set username and password: %s\n", pClient->_username);
        }
    }

    /* mosquitto_connect_async */
    rc = mosquitto_connect_async(pClient->_mosq, host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to connect to MQTT broker: %s\n",
                  mosquitto_strerror(rc));
        goto Error;
    }

    rc = mosquitto_loop_start(pClient->_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to start Mosquitto loop: %s\n",
                  mosquitto_strerror(rc));
        goto Error;
    }

    return (DxMsg_Bal_Handle_t)pClient;

Error:
    mosquitto_destroy(pClient->_mosq);
    mosquitto_lib_cleanup();
    g_free(pClient);
    return nullptr;
}

DxMsg_Bal_Error_t dxmsg_bal_send_mqtt(DxMsg_Bal_Handle_t handle, char *topic,
                                      const void *payload, int payload_len) {
    MqttClientInfo_t *pClient = (MqttClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DXMSG_BAL_OK;
    int rc;

    GST_TRACE("|JCP|\n");

    if (pClient == nullptr || topic == nullptr || payload == nullptr ||
        payload_len <= 0) {
        GST_ERROR("Error, Failed to publish message: %s\n", "Invalid argument");
        return DXMSG_BAL_ERR_INVALID;
    }

    rc = mosquitto_publish(pClient->_mosq, NULL, topic, payload_len, payload, 0,
                           false);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to publish message: %s\n",
                  mosquitto_strerror(rc));
        balError = DXMSG_BAL_ERR_BROKER;
    } else {
        GST_INFO("Publish message (%d bytes)\n", payload_len);
    }

    return balError;
}

DxMsg_Bal_Error_t dxmsg_bal_disconnect_mqtt(DxMsg_Bal_Handle_t handle) {
    MqttClientInfo_t *pClient = (MqttClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DXMSG_BAL_OK;
    int rc;

    GST_TRACE("|JCP|\n");
    if (pClient == nullptr) {
        GST_ERROR("Error, Failed to disconnect: %s\n", "Invalid argument");
        return DXMSG_BAL_ERR_INVALID;
    }

    rc = mosquitto_disconnect(pClient->_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to disconnect: %s\n", mosquitto_strerror(rc));
        balError = DXMSG_BAL_ERR_BROKER;
    }

    rc = mosquitto_loop(pClient->_mosq, 0, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to loop Mosquitto: %s\n",
                  mosquitto_strerror(rc));
        balError = DXMSG_BAL_ERR_BROKER;
    }
    rc = mosquitto_loop_stop(pClient->_mosq, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to stop Mosquitto loop: %s\n",
                  mosquitto_strerror(rc));
        balError = DXMSG_BAL_ERR_BROKER;
    }

    mosquitto_destroy(pClient->_mosq);
    mosquitto_lib_cleanup();
    g_free(pClient);
    return balError;
}