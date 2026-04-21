#include <librdkafka/rdkafka.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <tuple>
#include <vector>

#include <json-glib/json-glib.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

struct AppConfig {
    bool print_all = false;
    bool display = false;
};

// NOSONAR: cpp:S5421 - This variable must be mutable as it's modified by signal handler
static volatile sig_atomic_t run = 1;  // NOSONAR

static void sigterm(int sig) {
    std::ignore = sig;
    run = 0;
}

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

static void parse_message(const rd_kafka_message_t *msg,
                          const AppConfig *config) {
    const auto *payload = static_cast<const char *>(msg->payload);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_data(parser, payload, msg->len, &error)) {
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

    JsonObject *obj = json_node_get_object(root);

    gint64 seq_id = json_object_has_member(obj, "seqId")
                        ? json_object_get_int_member(obj, "seqId") : -1;
    gint64 num_objects = 0;
    if (json_object_has_member(obj, "objects")) {
        num_objects = json_array_get_length(
            json_object_get_array_member(obj, "objects"));
    }

    const char *frame_base64 = nullptr;
    std::string frame_base64_copy;
    bool has_frame = false;
    if (json_object_has_member(obj, "frameData")) {
        frame_base64 = json_object_get_string_member(obj, "frameData");
        has_frame = (frame_base64 && strlen(frame_base64) > 0);
        if (has_frame) {
            frame_base64_copy = frame_base64;
        }
    }

    printf("Received payload %zd bytes | seqId: %ld | objects: %ld | frameData: %s\n",
           msg->len, seq_id, num_objects, has_frame ? "yes" : "no");

    if (config->print_all) {
        if (has_frame) {
            json_object_remove_member(obj, "frameData");
            json_object_set_string_member(obj, "frameData", "<base64 omitted>");
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

static void print_usage() {
    fprintf(stderr, "Usage: kafka_consume_example -n <hostname> -t <topic> [-p <port>] [-a] [-d]\n");
    fprintf(stderr, "  -n <hostname>    Kafka broker hostname\n");
    fprintf(stderr, "  -t <topic>       Topic name to subscribe to\n");
    fprintf(stderr, "  -p <port>        Kafka broker port (default: 9092)\n");
    fprintf(stderr, "  -a               Print all JSON fields\n");
    fprintf(stderr, "  -d               Display decoded JPEG frames in a window\n");
}

static bool parse_args(int argc, char *argv[], char **hostname, int *port,
                       char **topic, AppConfig *config) {
    int opt;

    *hostname = nullptr;
    *port = 9092;
    *topic = nullptr;

    while ((opt = getopt(argc, argv, "n:p:t:ad")) != -1) {
        switch (opt) {
        case 'n':
            *hostname = optarg;
            break;
        case 'p':
            *port = atoi(optarg);
            break;
        case 't':
            *topic = optarg;
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

    if (*hostname == nullptr || *topic == nullptr) {
        print_usage();
        return false;
    }
    return true;
}

int main(int argc, char **argv) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_partition_list_t *topic_list;
    char errstr[512];
    char *hostname;
    char *topic;
    int port;
    char broker[256];
    AppConfig config;

    if (!parse_args(argc, argv, &hostname, &port, &topic, &config)) {
        return 1;
    }

    snprintf(broker, sizeof(broker), "%s:%d", hostname, port);

    conf = rd_kafka_conf_new();
    rd_kafka_conf_set(conf, "bootstrap.servers", broker, errstr, sizeof(errstr));
    rd_kafka_conf_set(conf, "group.id", "my-group", errstr, sizeof(errstr));

    rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!rk) {
        fprintf(stderr, "Failed to create consumer: %s\n", errstr);
        rd_kafka_conf_destroy(conf);
        return 1;
    }

    topic_list = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topic_list, topic, RD_KAFKA_PARTITION_UA);

    if (rd_kafka_subscribe(rk, topic_list) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        fprintf(stderr, "Failed to subscribe to topic: %s\n",
                rd_kafka_err2str(rd_kafka_last_error()));
        rd_kafka_topic_partition_list_destroy(topic_list);
        rd_kafka_destroy(rk);
        return 1;
    }

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    while (run) {
        rd_kafka_message_t *msg = rd_kafka_consumer_poll(rk, 1000);
        if (msg) {
            if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
                parse_message(msg, &config);
            } else {
                fprintf(stderr, "Failed to consume message: %s\n",
                        rd_kafka_err2str(msg->err));
            }
            rd_kafka_message_destroy(msg);
        }
    }

    rd_kafka_topic_partition_list_destroy(topic_list);
    rd_kafka_unsubscribe(rk);
    rd_kafka_destroy(rk);

    return 0;
}
