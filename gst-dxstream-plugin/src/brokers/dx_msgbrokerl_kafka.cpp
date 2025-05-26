#include <gst/gst.h>

#include "gst-dxmsgbroker.hpp"

#include "dx_msgbrokerl_kafka.hpp"

#include <librdkafka/rdkafka.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

/* debug */
GST_DEBUG_CATEGORY_STATIC(broker);
#define GST_CAT_DEFAULT broker

/* Local */
constexpr int MAX_KAFKA_FIELD_LEN = 1024;
typedef struct {
    rd_kafka_t *_rk; /* Kafka producer instance handle */

    /* if any msgbroker global config, then add member */

} KafkaClientInfo_t;
// typedef KafkaClientInfo_t *KafkaClientHandle_t;

static void trim(std::string &str) {
    str.erase(str.begin(),
              std::find_if(str.begin(), str.end(),
                           [](unsigned char ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(),
                           [](unsigned char ch) { return !std::isspace(ch); })
                  .base(),
              str.end());
}

static DxMsg_Bal_Error_t dxmsg_bal_setconf_kafka(rd_kafka_conf_t *conf,
                                                 const char *key,
                                                 const char *val) {
    char errstr[512];

    if (rd_kafka_conf_set(conf, key, val, errstr, sizeof(errstr)) !=
        RD_KAFKA_CONF_OK) {
        GST_ERROR("Error, Failed to set conf %s: %s\n", key, errstr);
        return DXMSG_BAL_ERR_BROKER;
    } else {
        GST_INFO("set conf %s to %s\n", key, val);
        return DXMSG_BAL_OK;
    }
}

static void process_config_line(
    const std::string &line, std::string &currentSection,
    std::map<std::string, std::map<std::string, std::string>> &config) {
    std::string trimmedLine = line;
    trim(trimmedLine);
    if (trimmedLine.empty() || trimmedLine[0] == '#')
        return;

    size_t commentPos = trimmedLine.find('#');
    if (commentPos != std::string::npos) {
        trimmedLine.erase(commentPos);
        trim(trimmedLine);
        if (trimmedLine.empty())
            return;
    }

    if (trimmedLine.front() == '[' && trimmedLine.back() == ']') {
        currentSection = trimmedLine.substr(1, trimmedLine.size() - 2);
        trim(currentSection);
    } else {
        size_t delimiterPos = trimmedLine.find('=');
        if (delimiterPos == std::string::npos)
            return;

        std::string key = trimmedLine.substr(0, delimiterPos);
        std::string value = trimmedLine.substr(delimiterPos + 1);
        trim(key);
        trim(value);

        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        config[currentSection][key] = value;
    }
}

static void log_section(const std::string &sectionName,
                        const std::map<std::string, std::string> &kvs,
                        rd_kafka_conf_t *conf) {
    GST_INFO("[%s]\n", sectionName.c_str());

    for (const auto &kv : kvs) {
        const std::string &key = kv.first;
        const std::string &value = kv.second;
        GST_INFO("%s = %s\n", key.c_str(), value.c_str());

        if (sectionName == "kafka") {
            dxmsg_bal_setconf_kafka(conf, key.c_str(), value.c_str());
        }
    }
}

static DxMsg_Bal_Error_t dxmsg_bal_read_config_kafka(DxMsg_Bal_Handle_t handle,
                                                     const char *cfg_file,
                                                     rd_kafka_conf_t *conf) {
    g_return_val_if_fail(handle != nullptr, DXMSG_BAL_ERR_INVALID);
    g_return_val_if_fail(cfg_file != nullptr, DXMSG_BAL_ERR_INVALID);
    g_return_val_if_fail(conf != nullptr, DXMSG_BAL_ERR_INVALID);

    std::ifstream file(cfg_file);
    if (!file.is_open()) {
        return DXMSG_BAL_ERR_UNKNOWN;
    }

    std::string line, currentSection;
    std::map<std::string, std::map<std::string, std::string>> config;

    while (std::getline(file, line)) {
        process_config_line(line, currentSection, config);
    }

    for (const auto &sectionPair : config) {
        log_section(sectionPair.first, sectionPair.second, conf);
    }

    return DXMSG_BAL_OK;
}

static bool dxmsg_bal_is_valid_connInfo_kafka(const char *conn_info,
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

/**
 * Message delivery report callback.
 *
 * This callback is called exactly once per message, indicating if
 * the message was succesfully delivered
 * (rkmessage->err == RD_KAFKA_RESP_ERR_NO_ERROR) or permanently
 * failed delivery (rkmessage->err != RD_KAFKA_RESP_ERR_NO_ERROR).
 *
 * The callback is triggered from rd_kafka_poll() and executes on
 * the application's thread.
 */
static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t *rkmessage,
                      void *opaque) {
    if (rkmessage->err)
        GST_WARNING("Message delivery failed: %s\n",
                    rd_kafka_err2str(rkmessage->err));
    else
        GST_DEBUG("Message delivered (%zd bytes, "
                  "partition %" PRId32 ")\n",
                  rkmessage->len, rkmessage->partition);
}

static int _kafka_error_cnt = 0;
/**
 * The callback is triggered from rd_kafka_poll()
 */
static void error_cb(rd_kafka_t *rk, int err, const char *reason,
                     void *opaque) {
    rd_kafka_resp_err_t orig_err;
    char errstr[512];

    GST_ERROR("Error(%d): %s: %s\n", err,
              rd_kafka_err2name((rd_kafka_resp_err_t)err), reason);

    if (err == RD_KAFKA_RESP_ERR__FATAL) {
        orig_err = rd_kafka_fatal_error(rk, errstr, sizeof(errstr));
        GST_ERROR("FATAL ERROR: %s: %s\n", rd_kafka_err2name(orig_err), errstr);
    }

    _kafka_error_cnt++;
    /* how to handle in delivery message ??? */
}

DxMsg_Bal_Handle_t dxmsg_bal_connect_kafka(char *conn_info, char *cfg_path) {
    KafkaClientInfo_t *pClient = g_new0(KafkaClientInfo_t, 1);
    rd_kafka_conf_t *conf;

    std::string host = "";
    int port;

    GST_DEBUG_CATEGORY_INIT(broker, "broker", 0,
                            "broker category for dxmsgbroker element");
    GST_TRACE("|JCP|\n");
    // GST_CAT_TRACE(broker, "|kafka|\n");

    /* conn_info => host, port */
    if (!dxmsg_bal_is_valid_connInfo_kafka(conn_info, host, &port)) {
        GST_ERROR("Error, Invalid connection info: %s\n", conn_info);
        g_free(pClient);
        return nullptr;
    }

    /* create Kafka configuration */
    conf = rd_kafka_conf_new();
    if (conf == nullptr) {
        GST_ERROR("Error, Failed to create Kafka configuration\n");
        g_free(pClient);
        return nullptr;
    }

    /* read config file */
    if (cfg_path != nullptr) {
        if (dxmsg_bal_read_config_kafka((DxMsg_Bal_Handle_t)pClient, cfg_path,
                                        conf) != DXMSG_BAL_OK) {
            GST_ERROR("Error, Failed to read config file: %s\n", cfg_path);
            rd_kafka_conf_destroy(conf);
            g_free(pClient);
            return nullptr;
        }
    }

    char broker_list[1024];
    snprintf(broker_list, sizeof(broker_list), "%s:%d", host.c_str(), port);
    if (rd_kafka_conf_set(conf, "bootstrap.servers", broker_list, nullptr, 0) !=
        RD_KAFKA_CONF_OK) {
        GST_ERROR("Error, Failed to set broker list\n");
        rd_kafka_conf_destroy(conf);
        g_free(pClient);
        return nullptr;
    }

    rd_kafka_conf_set_dr_msg_cb(conf, dr_msg_cb);
    rd_kafka_conf_set_error_cb(conf, error_cb);

    /* create Kafka producer */
    pClient->_rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, nullptr, 0);
    if (pClient->_rk == nullptr) {
        GST_ERROR("Error, Failed to create Kafka producer\n");
        rd_kafka_conf_destroy(conf);
        g_free(pClient);
        return nullptr;
    }

    /* Check if the Kafka producer is connected to the broker immediately(???)
     * kafka handle for events callbacks, especially for error_cb
     */
    rd_kafka_poll(pClient->_rk, 1000); /* timeout 1000ms */

    if (_kafka_error_cnt > 0) {
        GST_ERROR("Error, Failed to connect to Kafka broker\n");
        rd_kafka_destroy(pClient->_rk);
        g_free(pClient);
        return nullptr;
    }

    return (DxMsg_Bal_Handle_t)pClient;
}

DxMsg_Bal_Error_t dxmsg_bal_send_kafka(DxMsg_Bal_Handle_t handle, char *topic,
                                       const void *payload, int payload_len) {
    KafkaClientInfo_t *pClient = (KafkaClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DXMSG_BAL_OK;
    rd_kafka_resp_err_t err; /* Error code */

    GST_TRACE("|JCP|\n");
    if (pClient == nullptr || topic == nullptr || payload == nullptr ||
        payload_len <= 0) {
        GST_ERROR("Error, Failed to publish message: %s\n", "Invalid argument");
        return DXMSG_BAL_ERR_INVALID;
    }

    err = (rd_kafka_resp_err_t)rd_kafka_producev(
        pClient->_rk, RD_KAFKA_V_TOPIC(topic),
        RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
        RD_KAFKA_V_VALUE((void *)payload, payload_len),
        RD_KAFKA_V_OPAQUE(nullptr), RD_KAFKA_V_END);
    if (err) {
        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
            GST_WARNING("Queue full, discarding message...\n");
            // rd_kafka_poll(pClient->_rk, 1000);  //for events callbacks
        } else {
            GST_ERROR("Error, Failed to produce message: %s\n",
                      rd_kafka_err2str(err));
        }
    } else {
        GST_INFO("Produced message (%d bytes)\n", payload_len);
    }

    /* A producer application should continually serve
     * the delivery report queue by calling rd_kafka_poll()
     * at frequent intervals.
     * Either put the poll call in your main loop, or in a
     * dedicated thread, or call it after every
     * rd_kafka_produce() call.
     * Just make sure that rd_kafka_poll() is still called
     * during periods where you are not producing any messages
     * to make sure previously produced messages have their
     * delivery report callback served (and any other callbacks
     * you register).
     * */
    rd_kafka_poll(pClient->_rk,
                  0); // non-blocking, FIXME: need to do it continually
    return balError;
}

DxMsg_Bal_Error_t dxmsg_bal_disconnect_kafka(DxMsg_Bal_Handle_t handle) {
    KafkaClientInfo_t *pClient = (KafkaClientInfo_t *)handle;
    DxMsg_Bal_Error_t balError = DXMSG_BAL_OK;

    GST_TRACE("|JCP|\n");
    if (pClient == nullptr) {
        GST_ERROR("Error, Failed to disconnect: %s\n", "Invalid argument");
        return DXMSG_BAL_ERR_INVALID;
    }

    /* Wait for messages to be delivered */
    rd_kafka_flush(pClient->_rk, 10000);

    /* Destroy Kafka producer instance */
    rd_kafka_destroy(pClient->_rk);

    g_free(pClient);
    return balError;
}
