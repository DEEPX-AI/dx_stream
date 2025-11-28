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
constexpr int MAX_MQTT_FIELD_LEN = 1024;
typedef struct {
    struct mosquitto *_mosq;

    char _username[MAX_MQTT_FIELD_LEN];
    char _password[MAX_MQTT_FIELD_LEN];
    char _clientid[MAX_MQTT_FIELD_LEN];

    bool _tls_enable;
    bool _tls_insecure;
    char _tls_cafile[MAX_MQTT_FIELD_LEN];
    char _tls_capath[MAX_MQTT_FIELD_LEN];
    char _tls_certfile[MAX_MQTT_FIELD_LEN];
    char _tls_keyfile[MAX_MQTT_FIELD_LEN];

    bool _connected;
    int _connection_timeout;

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
            } else if (key == "tls_insecure") {
                pClient->_tls_insecure = (value == "1");
            } else if (key == "connection_timeout") {
                pClient->_connection_timeout = std::stoi(value);
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

static void cleanup_mqtt(MqttClientInfo_t *pClient, bool destroy_client,
                         bool lib_cleanup) {
    if (destroy_client && pClient->_mosq) {
        mosquitto_destroy(pClient->_mosq);
    }
    if (lib_cleanup) {
        mosquitto_lib_cleanup();
    }
    g_free(pClient);
}

// Connection callback functions
static void on_connect(struct mosquitto *mosq, void *userdata, int result) {
    MqttClientInfo_t *pClient = (MqttClientInfo_t *)userdata;
    if (result == 0) {
        pClient->_connected = true;
        GST_INFO("MQTT connection established");
    } else {
        pClient->_connected = false;
        GST_ERROR("MQTT connection failed: %s", mosquitto_strerror(result));
    }
}

static void on_disconnect(struct mosquitto *mosq, void *userdata, int result) {
    MqttClientInfo_t *pClient = (MqttClientInfo_t *)userdata;
    pClient->_connected = false;
    GST_WARNING("MQTT disconnected: %s", mosquitto_strerror(result));
}

DxMsg_Bal_Handle_t dxmsg_bal_connect_mqtt(char *conn_info, char *cfg_path) {
    MqttClientInfo_t *pClient = g_new0(MqttClientInfo_t, 1);
    std::string host;
    int port = 1883;
    int rc;
    int max_wait_ms = 5000; // 5 seconds default timeout
    int wait_step_ms = 100;
    int waited_ms = 0;

    GST_DEBUG_CATEGORY_INIT(broker, "broker", 0,
                            "broker category for dxmsgbroker element");
    GST_TRACE("|JCP|\n");

    // Initialize connection state
    pClient->_connected = false;
    pClient->_tls_insecure = false;
    pClient->_connection_timeout = 5000; // Default 5 seconds

    if (!dxmsg_bal_is_valid_connInfo_mqtt(conn_info, host, &port)) {
        GST_ERROR("Error, Invalid connection info: %s\n", conn_info);
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    if (cfg_path != nullptr &&
        dxmsg_bal_read_config_mqtt((DxMsg_Bal_Handle_t)pClient, cfg_path) !=
            DXMSG_BAL_OK) {
        GST_ERROR("Error, Failed to read config file: %s\n", cfg_path);
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, mosquitto_lib_init() failed: %s\n",
                  mosquitto_strerror(rc));
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    pClient->_mosq = (pClient->_clientid[0] != '\0')
                         ? mosquitto_new(pClient->_clientid, true, nullptr)
                         : mosquitto_new(nullptr, true, nullptr);

    if (pClient->_mosq == nullptr) {
        GST_ERROR("Error, mosquitto_new() failed\n");
        cleanup_mqtt(pClient, false, true);
        return nullptr;
    }

    if (pClient->_tls_enable) {
        char *cafile = (pClient->_tls_cafile[0] != '\0') ? pClient->_tls_cafile : nullptr;
        char *capath = (pClient->_tls_capath[0] != '\0') ? pClient->_tls_capath : nullptr;
        char *certfile = (pClient->_tls_certfile[0] != '\0') ? pClient->_tls_certfile : nullptr;
        char *keyfile = (pClient->_tls_keyfile[0] != '\0') ? pClient->_tls_keyfile : nullptr;

        GST_DEBUG("TLS settings - CA file: %s, CA path: %s, Cert file: %s, Key file: %s", 
                cafile ? cafile : "(null)",
                capath ? capath : "(null)", 
                certfile ? certfile : "(null)",
                keyfile ? keyfile : "(null)");

        rc = mosquitto_tls_set(pClient->_mosq, cafile, capath, certfile,
                               keyfile, nullptr);
        if (rc != MOSQ_ERR_SUCCESS) {
            GST_ERROR("Error, Failed to set TLS: %s\n", mosquitto_strerror(rc));
            cleanup_mqtt(pClient, true, true);
            return nullptr;
        }

        // Set TLS insecure mode for self-signed certificates
        if (pClient->_tls_insecure) {
            rc = mosquitto_tls_insecure_set(pClient->_mosq, true);
            if (rc != MOSQ_ERR_SUCCESS) {
                GST_ERROR("Error, Failed to set TLS insecure mode: %s\n", mosquitto_strerror(rc));
                cleanup_mqtt(pClient, true, true);
                return nullptr;
            }
            GST_DEBUG("TLS insecure mode enabled for self-signed certificates");
        }

        GST_DEBUG("Set TLS: %s, %s, %s, %s\n", cafile ? cafile : "(null)",
                  capath ? capath : "(null)", certfile ? certfile : "(null)",
                  keyfile ? keyfile : "(null)");
    }

    if (pClient->_username[0] != 0 && pClient->_password[0] != 0) {
        rc = mosquitto_username_pw_set(pClient->_mosq, pClient->_username,
                                       pClient->_password);
        if (rc != MOSQ_ERR_SUCCESS) {
            GST_ERROR("Error, Failed to set username and password: %s\n",
                      mosquitto_strerror(rc));
            cleanup_mqtt(pClient, true, true);
            return nullptr;
        }
        GST_DEBUG("Set username and password: %s\n", pClient->_username);
    }

    // Set connection callbacks
    mosquitto_connect_callback_set(pClient->_mosq, on_connect);
    mosquitto_disconnect_callback_set(pClient->_mosq, on_disconnect);
    mosquitto_user_data_set(pClient->_mosq, pClient);

    rc = mosquitto_connect_async(pClient->_mosq, host.c_str(), port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to connect to MQTT broker: %s\n",
                  mosquitto_strerror(rc));
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    rc = mosquitto_loop_start(pClient->_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to start Mosquitto loop: %s\n",
                  mosquitto_strerror(rc));
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    // Wait for connection to be established
    max_wait_ms = pClient->_connection_timeout;
    while (!pClient->_connected && waited_ms < max_wait_ms) {
        g_usleep(wait_step_ms * 1000); // Convert to microseconds
        waited_ms += wait_step_ms;
    }

    if (!pClient->_connected) {
        GST_ERROR("Error, Connection timeout after %d ms\n", max_wait_ms);
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    GST_INFO("MQTT connection established successfully after %d ms", waited_ms);

    return (DxMsg_Bal_Handle_t)pClient;
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

    // Check connection status
    if (!pClient->_connected) {
        GST_ERROR("Error, Failed to publish message: %s\n", "The client is not currently connected");
        return DXMSG_BAL_ERR_BROKER;
    }

    rc = mosquitto_publish(pClient->_mosq, nullptr, topic, payload_len, payload,
                           0, false);
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