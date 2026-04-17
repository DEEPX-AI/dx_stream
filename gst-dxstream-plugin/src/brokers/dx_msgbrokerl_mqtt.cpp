#include <gst/gst.h>

#include "gst-dxmsgbroker.hpp"

#include "dx_msgbrokerl_mqtt.hpp"

#include <fstream>
#include <mosquitto.h>
#include <sstream>
#include <string>
#include <tuple>

/* debug */
GST_DEBUG_CATEGORY_STATIC(broker);
#define GST_CAT_DEFAULT broker

struct MqttClientInfo {
    struct mosquitto *_mosq;

    std::string _username;
    std::string _password;
    std::string _clientid;

    bool _tls_enable;
    bool _tls_insecure;
    std::string _tls_cafile;
    std::string _tls_capath;
    std::string _tls_certfile;
    std::string _tls_keyfile;

    bool _connected;
    int _connection_timeout;

};
using MqttClientInfo_t = MqttClientInfo;

static DxMsg_Bal_Error_t dxmsg_bal_read_config_mqtt(DxMsg_Bal_Handle_t handle,
                                                    const char *cfg_file) {
    auto *pClient = (MqttClientInfo_t *)handle;

    g_return_val_if_fail(handle != nullptr, DxMsg_Bal_Error::DXMSG_BAL_ERR_INVALID);
    g_return_val_if_fail(cfg_file != nullptr, DxMsg_Bal_Error::DXMSG_BAL_ERR_INVALID);

    if (!g_file_test(cfg_file, G_FILE_TEST_EXISTS)) {
        return DxMsg_Bal_Error::DXMSG_BAL_ERR_UNKNOWN;
    }

    std::ifstream file(cfg_file);
    if (!file.is_open()) {
        return DxMsg_Bal_Error::DXMSG_BAL_ERR_UNKNOWN;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        std::string value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            key.erase(key.find_last_not_of(" \n\r\t") + 1);
            value.erase(0, value.find_first_not_of(" \n\r\t"));

            if (key == "username") {
                pClient->_username = value;
            } else if (key == "password") {
                pClient->_password = value;
            } else if (key == "client-id") {
                pClient->_clientid = value;
            } else if (key == "tls_enable") {
                pClient->_tls_enable = (value == "1");
            } else if (key == "tls_insecure") {
                pClient->_tls_insecure = (value == "1");
            } else if (key == "connection_timeout") {
                pClient->_connection_timeout = std::stoi(value);
            } else if (key == "tls_cafile") {
                pClient->_tls_cafile = value;
            } else if (key == "tls_capath") {
                pClient->_tls_capath = value;
            } else if (key == "tls_certfile") {
                pClient->_tls_certfile = value;
            } else if (key == "tls_keyfile") {
                pClient->_tls_keyfile = value;
            }
        }
    }

    file.close();
    return DxMsg_Bal_Error::DXMSG_BAL_OK;
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
    } catch (const std::invalid_argument &) {
        return false;
    } catch (const std::out_of_range &) {
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
    std::ignore = mosq;
    auto *pClient = (MqttClientInfo_t *)userdata;
    if (result == 0) {
        pClient->_connected = true;
        GST_INFO("MQTT connection established");
    } else {
        pClient->_connected = false;
        GST_ERROR("MQTT connection failed: %s", mosquitto_strerror(result));
    }
}

static void on_disconnect(struct mosquitto *mosq, void *userdata, int result) {
    std::ignore = mosq;
    auto *pClient = (MqttClientInfo_t *)userdata;
    pClient->_connected = false;
    GST_WARNING("MQTT disconnected: %s", mosquitto_strerror(result));
}

// Helper function to setup TLS settings
static bool setup_tls_settings(MqttClientInfo_t *pClient) {
    const char *cafile = (!pClient->_tls_cafile.empty()) ? pClient->_tls_cafile.c_str() : nullptr;
    const char *capath = (!pClient->_tls_capath.empty()) ? pClient->_tls_capath.c_str() : nullptr;
    const char *certfile = (!pClient->_tls_certfile.empty()) ? pClient->_tls_certfile.c_str() : nullptr;
    const char *keyfile = (!pClient->_tls_keyfile.empty()) ? pClient->_tls_keyfile.c_str() : nullptr;

    GST_DEBUG("TLS settings - CA file: %s, CA path: %s, Cert file: %s, Key file: %s", 
            cafile ? cafile : "(null)",
            capath ? capath : "(null)", 
            certfile ? certfile : "(null)",
            keyfile ? keyfile : "(null)");

    int rc = mosquitto_tls_set(pClient->_mosq, cafile, capath, certfile, keyfile, nullptr);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to set TLS: %s\n", mosquitto_strerror(rc));
        return false;
    }

    if (pClient->_tls_insecure) {
        rc = mosquitto_tls_insecure_set(pClient->_mosq, true);
        if (rc != MOSQ_ERR_SUCCESS) {
            GST_ERROR("Error, Failed to set TLS insecure mode: %s\n", mosquitto_strerror(rc));
            return false;
        }
        GST_DEBUG("TLS insecure mode enabled for self-signed certificates");
    }

    GST_DEBUG("Set TLS: %s, %s, %s, %s\n", cafile ? cafile : "(null)",
              capath ? capath : "(null)", certfile ? certfile : "(null)",
              keyfile ? keyfile : "(null)");
    return true;
}

// Helper function to setup authentication
static bool setup_authentication(MqttClientInfo_t *pClient) {
    if (pClient->_username.empty() || pClient->_password.empty()) {
        return true;
    }

    int rc = mosquitto_username_pw_set(pClient->_mosq, pClient->_username.c_str(),
                                   pClient->_password.c_str());
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to set username and password: %s\n",
                  mosquitto_strerror(rc));
        return false;
    }
    GST_DEBUG("Set username and password: %s\n", pClient->_username.c_str());
    return true;
}

// Helper function to initialize mosquitto client
static bool initialize_mosquitto_client(MqttClientInfo_t *pClient) {
    int rc = mosquitto_lib_init();
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, mosquitto_lib_init() failed: %s\n", mosquitto_strerror(rc));
        return false;
    }

    pClient->_mosq = (!pClient->_clientid.empty())
                         ? mosquitto_new(pClient->_clientid.c_str(), true, nullptr)
                         : mosquitto_new(nullptr, true, nullptr);

    if (pClient->_mosq == nullptr) {
        GST_ERROR("Error, mosquitto_new() failed\n");
        mosquitto_lib_cleanup();
        return false;
    }

    return true;
}

// Helper function to wait for connection
static bool wait_for_connection(const MqttClientInfo_t *pClient) {
    int max_wait_ms = pClient->_connection_timeout;
    int wait_step_ms = 100;
    int waited_ms = 0;

    while (!pClient->_connected && waited_ms < max_wait_ms) {
        g_usleep(wait_step_ms * 1000);
        waited_ms += wait_step_ms;
    }

    if (!pClient->_connected) {
        GST_ERROR("Error, Connection timeout after %d ms\n", max_wait_ms);
        return false;
    }

    GST_INFO("MQTT connection established successfully after %d ms", waited_ms);
    return true;
}

DxMsg_Bal_Handle_t dxmsg_bal_connect_mqtt(char *conn_info, char *cfg_path) {
    auto *pClient = g_new0(MqttClientInfo_t, 1);
    std::string host;
    int port = 1883;

    GST_DEBUG_CATEGORY_INIT(broker, "broker", 0,
                            "broker category for dxmsgbroker element");
    GST_TRACE("|JCP|\n");

    // Initialize connection state
    pClient->_connected = false;
    pClient->_tls_insecure = false;
    pClient->_connection_timeout = 5000;

    if (!dxmsg_bal_is_valid_connInfo_mqtt(conn_info, host, &port)) {
        GST_ERROR("Error, Invalid connection info: %s\n", conn_info);
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    if (cfg_path != nullptr &&
        dxmsg_bal_read_config_mqtt((DxMsg_Bal_Handle_t)pClient, cfg_path) !=
            DxMsg_Bal_Error::DXMSG_BAL_OK) {
        GST_ERROR("Error, Failed to read config file: %s\n", cfg_path);
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    if (!initialize_mosquitto_client(pClient)) {
        cleanup_mqtt(pClient, false, false);
        return nullptr;
    }

    if (pClient->_tls_enable && !setup_tls_settings(pClient)) {
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    if (!setup_authentication(pClient)) {
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    mosquitto_connect_callback_set(pClient->_mosq, on_connect);
    mosquitto_disconnect_callback_set(pClient->_mosq, on_disconnect);
    mosquitto_user_data_set(pClient->_mosq, pClient);

    int rc = mosquitto_connect_async(pClient->_mosq, host.c_str(), port, 60);
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

    if (!wait_for_connection(pClient)) {
        cleanup_mqtt(pClient, true, true);
        return nullptr;
    }

    return (DxMsg_Bal_Handle_t)pClient;
}

DxMsg_Bal_Error_t dxmsg_bal_send_mqtt(DxMsg_Bal_Handle_t handle, const char *topic,
                                      const void *payload, int payload_len) {
    auto *pClient = (MqttClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DxMsg_Bal_Error::DXMSG_BAL_OK;
    int rc;

    GST_TRACE("|JCP|\n");

    if (pClient == nullptr || topic == nullptr || payload == nullptr ||
        payload_len <= 0) {
        GST_ERROR("Error, Failed to publish message: %s\n", "Invalid argument");
        return DxMsg_Bal_Error::DXMSG_BAL_ERR_INVALID;
    }

    // Check connection status
    if (!pClient->_connected) {
        GST_ERROR("Error, Failed to publish message: %s\n", "The client is not currently connected");
        return DxMsg_Bal_Error::DXMSG_BAL_ERR_BROKER;
    }

    rc = mosquitto_publish(pClient->_mosq, nullptr, topic, payload_len, payload,
                           0, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to publish message: %s\n",
                  mosquitto_strerror(rc));
        balError = DxMsg_Bal_Error::DXMSG_BAL_ERR_BROKER;
    } else {
        GST_INFO("Publish message (%d bytes)\n", payload_len);
    }

    return balError;
}

DxMsg_Bal_Error_t dxmsg_bal_disconnect_mqtt(DxMsg_Bal_Handle_t handle) {
    auto *pClient = (MqttClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DxMsg_Bal_Error::DXMSG_BAL_OK;
    int rc;

    GST_TRACE("|JCP|\n");
    if (pClient == nullptr) {
        GST_ERROR("Error, Failed to disconnect: %s\n", "Invalid argument");
        return DxMsg_Bal_Error::DXMSG_BAL_ERR_INVALID;
    }

    rc = mosquitto_disconnect(pClient->_mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to disconnect: %s\n", mosquitto_strerror(rc));
        balError = DxMsg_Bal_Error::DXMSG_BAL_ERR_BROKER;
    }

    rc = mosquitto_loop(pClient->_mosq, 0, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to loop Mosquitto: %s\n",
                  mosquitto_strerror(rc));
        balError = DxMsg_Bal_Error::DXMSG_BAL_ERR_BROKER;
    }
    rc = mosquitto_loop_stop(pClient->_mosq, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        GST_ERROR("Error, Failed to stop Mosquitto loop: %s\n",
                  mosquitto_strerror(rc));
        balError = DxMsg_Bal_Error::DXMSG_BAL_ERR_BROKER;
    }

    mosquitto_destroy(pClient->_mosq);
    mosquitto_lib_cleanup();
    g_free(pClient);
    return balError;
}