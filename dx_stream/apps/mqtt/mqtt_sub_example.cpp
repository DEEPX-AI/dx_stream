#include <cstdio>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <tuple>
#include <vector>

#include "../common/dx_getopt.h"
#ifdef _WIN32
#define getopt  dx_getopt
#define optarg  dx_optarg
#define optind  dx_optind
#endif

#include <json-glib/json-glib.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

struct AppConfig {
    std::string topic;
    bool print_all = false;
    bool display = false;
};

static void display_frame(const char *base64_str) {
    gsize out_len = 0;
    guchar *decoded = g_base64_decode(base64_str, &out_len);
    if (!decoded || out_len == 0) {
        fprintf(stderr, "Failed to decode base64 frameData\n");
        g_free(decoded);
        return;
    }

    std::vector<uchar> buf(decoded, decoded + out_len);
    g_free(decoded);

    cv::Mat img = cv::imdecode(buf, cv::IMREAD_COLOR);
    if (img.empty()) {
        fprintf(stderr, "Failed to decode JPEG from frameData\n");
        return;
    }

    cv::imshow("frameData", img);
    cv::waitKey(1);
}

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    const auto *config = static_cast<const AppConfig *>(obj);

    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        mosquitto_disconnect(mosq);
        return;
    }

    int rc = mosquitto_subscribe(mosq, nullptr, config->topic.c_str(), 0);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
    }
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count,
                  const int *granted_qos) {
    std::ignore = obj;
    std::ignore = mid;

    for (int i = 0; i < qos_count; i++) {
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if (granted_qos[i] > 2) {
            fprintf(stderr, "Error: Subscription %d rejected.\n", i);
            mosquitto_disconnect(mosq);
            return;
        }
    }
}

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
    std::ignore = mosq;
    const auto *config = static_cast<const AppConfig *>(obj);
    const auto *payload = static_cast<const char *>(msg->payload);

    // We need frameData before print_all modifies it, so parse twice or handle carefully
    // Simple approach: extract frameData first, then print
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_data(parser, payload, msg->payloadlen, &error)) {
        fprintf(stderr, "Unable to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        fprintf(stderr, "Received payload is not a JSON object.\n");
        g_object_unref(parser);
        return;
    }

    JsonObject *obj_json = json_node_get_object(root);

    gint64 seq_id = json_object_has_member(obj_json, "seqId")
                        ? json_object_get_int_member(obj_json, "seqId") : -1;
    gint64 num_objects = 0;
    if (json_object_has_member(obj_json, "objects")) {
        num_objects = json_array_get_length(
            json_object_get_array_member(obj_json, "objects"));
    }

    const char *frame_base64 = nullptr;
    std::string frame_base64_copy;
    bool has_frame = false;
    if (json_object_has_member(obj_json, "frameData")) {
        frame_base64 = json_object_get_string_member(obj_json, "frameData");
        has_frame = (frame_base64 && strlen(frame_base64) > 0);
        if (has_frame) {
            frame_base64_copy = frame_base64;
        }
    }

    printf("Received payload %d bytes | seqId: %ld | objects: %ld | frameData: %s\n",
           msg->payloadlen, seq_id, num_objects, has_frame ? "yes" : "no");

    if (config->print_all) {
        if (has_frame) {
            json_object_remove_member(obj_json, "frameData");
            json_object_set_string_member(obj_json, "frameData", "<base64 omitted>");
        }
        char *formatted = json_to_string(root, true);
        printf("%s\n", formatted);
        g_free(formatted);
    }

    if (has_frame && config->display) {
        display_frame(frame_base64_copy.c_str());
    }

    g_object_unref(parser);
}

void print_usage() {
    printf("Usage: mqtt_sub_example -n <hostname> -t <topic> [-p <port>] [-a] [-d]\n");
    printf("  -n <hostname>    MQTT broker hostname\n");
    printf("  -t <topic>       MQTT topic to subscribe to\n");
    printf("  -p <port>        MQTT broker port (default: 1883)\n");
    printf("  -a               Print all JSON fields (frameData shown as <base64 omitted>)\n");
    printf("  -d               Display decoded JPEG frames in a window\n");
}

bool parse_args(int argc, char *argv[], AppConfig *config, char **hostname,
                int *port) {
    int opt;

    *hostname = nullptr;
    *port = 1883;

    while ((opt = getopt(argc, argv, "n:t:p:ad")) != -1) {
        switch (opt) {
        case 'n':
            *hostname = optarg;
            break;
        case 't':
            config->topic = optarg;
            break;
        case 'p':
            *port = atoi(optarg);
            break;
        case 'a':
            config->print_all = true;
            break;
        case 'd':
            config->display = true;
            break;
        default:
            print_usage();
            return false;
        }
    }

    if (*hostname == nullptr || config->topic.empty()) {
        print_usage();
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    struct mosquitto *mosq;
    int rc;
    char *hostname;
    int port;
    AppConfig config;

    if (!parse_args(argc, argv, &config, &hostname, &port)) {
        exit(EXIT_FAILURE);
    }

    mosquitto_lib_init();

    mosq = mosquitto_new(nullptr, true, &config);
    if (mosq == nullptr) {
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_message_callback_set(mosq, on_message);

    rc = mosquitto_connect(mosq, hostname, port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
