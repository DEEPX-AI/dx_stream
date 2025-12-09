#include <librdkafka/rdkafka.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <json-glib/json-glib.h>

static volatile sig_atomic_t run = 1;

static void sigterm(int sig) { run = 0; }

static void print_json_all(JsonNode *root, size_t len) {
    char *formatted = json_to_string(root, true);
    printf("Received payload %zd bytes, (All): %s\n", len, formatted);
    g_free(formatted);
}

static void print_seq_id(JsonObject *object, size_t len) {
    if (!json_object_has_member(object, "seqId"))
        return;

    JsonNode *seqId_node = json_object_get_member(object, "seqId");
    if (!JSON_NODE_HOLDS_VALUE(seqId_node))
        return;

    int seqId = json_node_get_int(seqId_node);
    printf("Received payload %zd bytes, seqId: %d\n", len, seqId);
}

static void parse_message(rd_kafka_message_t *msg, int bPrintAll) {
    char *payload = (char *)msg->payload;
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_data(parser, payload, -1, &error)) {
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

    JsonObject *object = json_node_get_object(root);
    if (bPrintAll) {
        print_json_all(root, msg->len);
    } else {
        print_seq_id(object, msg->len);
    }

    g_object_unref(parser);
}

static void print_usage() {
    fprintf(stderr, "Usage: kafka_consume_example -n <hostname> -p <port> -t <topic>\n");
    fprintf(stderr, "  -n <hostname>  Kafka broker hostname (e.g., localhost)\n");
    fprintf(stderr, "  -p <port>      Kafka broker port (default: 9092)\n");
    fprintf(stderr, "  -t <topic>     Topic name to subscribe to\n");
}

static bool parse_args(int argc, char *argv[], char **hostname, int *port, char **topic) {
    int opt;
    bool ret = true;

    *hostname = nullptr;
    *port = 9092; // default port
    *topic = nullptr;

    while ((opt = getopt(argc, argv, "n:p:t:")) != -1) {
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
        default:
            print_usage();
            ret = false;
        }
    }

    if (*hostname == nullptr || *topic == nullptr) {
        print_usage();
        ret = false;
    }
    return ret;
}

int main(int argc, char **argv) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_partition_list_t *topic_list;
    char errstr[512];
    char *hostname, *topic;
    int port;
    char broker[256];

    if (!parse_args(argc, argv, &hostname, &port, &topic)) {
        return 1;
    }

    // Construct broker address
    snprintf(broker, sizeof(broker), "%s:%d", hostname, port);

    /* Create configuration objects */
    conf = rd_kafka_conf_new();

    /* Set configuration properties */
    rd_kafka_conf_set(conf, "bootstrap.servers", broker, errstr,
                      sizeof(errstr));
    rd_kafka_conf_set(conf, "group.id", "my-group", errstr, sizeof(errstr));

    /* Create consumer instance */
    rk = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
    if (!rk) {
        fprintf(stderr, "Failed to create consumer: %s\n", errstr);
        rd_kafka_conf_destroy(conf);
        return 1;
    }

    /* Create topic partition list and add topic */
    topic_list = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(topic_list, topic,
                                      RD_KAFKA_PARTITION_UA);

    /* Subscribe to topic */
    if (rd_kafka_subscribe(rk, topic_list) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        fprintf(stderr, "Failed to subscribe to topic: %s\n",
                rd_kafka_err2str(rd_kafka_last_error()));
        rd_kafka_topic_partition_list_destroy(topic_list);
        rd_kafka_destroy(rk);
        rd_kafka_conf_destroy(conf);
        return 1;
    }

    /* Set up signal handler for clean shutdown */
    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    /* Consume messages */
    while (run) {
        rd_kafka_message_t *msg;

        msg = rd_kafka_consumer_poll(rk, 1000);
        if (msg) {
            if (msg->err == RD_KAFKA_RESP_ERR_NO_ERROR) {
                // printf("Received message (%zd bytes): %.*s\n", msg->len,
                // (int)msg->len, (char *)msg->payload);
                parse_message(msg, 0);
            } else {
                fprintf(stderr, "Failed to consume message: %s\n",
                        rd_kafka_err2str(msg->err));
            }
            rd_kafka_message_destroy(msg);
        }
    }

    /* Clean up */
    rd_kafka_topic_partition_list_destroy(topic_list);
    rd_kafka_unsubscribe(rk);
    rd_kafka_destroy(rk);
    rd_kafka_conf_destroy(conf);

    return 0;
}
