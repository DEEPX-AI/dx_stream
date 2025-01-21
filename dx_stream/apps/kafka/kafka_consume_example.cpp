#include <librdkafka/rdkafka.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include <json-glib/json-glib.h>

static volatile sig_atomic_t run = 1;

static void sigterm(int sig) { run = 0; }

static void parse_message(rd_kafka_message_t *msg, int bPrintAll) {
    char *payload = (char *)msg->payload;

    // printf("Received message (%zd bytes), par=%d, off=%ld, key(%zd)\n", msg->len, msg->partition, msg->offset,
    //        msg->key_len);

    JsonParser *parser;
    JsonNode *root;
    JsonObject *object;
    GError *error = NULL;

    parser = json_parser_new();
    if (!json_parser_load_from_data(parser, payload, -1, &error)) {
        fprintf(stderr, "Unable to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    root = json_parser_get_root(parser);
    if (JSON_NODE_HOLDS_OBJECT(root)) {
        object = json_node_get_object(root);
        /* print whole json */
        if (bPrintAll) {
            char *formatted = json_to_string(root, true);
            printf("Received payload %zd bytes, (All): %s\n", msg->len, formatted);
            g_free(formatted);
        } else {
            /* print only seqId */
            if (json_object_has_member(object, "seqId")) {
                JsonNode *seqId_node = json_object_get_member(object, "seqId");
                if (JSON_NODE_HOLDS_VALUE(seqId_node)) {
                    int seqId = json_node_get_int(seqId_node);
                    printf("Received payload %zd bytes, seqId: %d\n", msg->len, seqId);
                }
            }
        }
    } else {
        fprintf(stderr, "Received payload is not a JSON object.\n");
    }

    g_object_unref(parser);
}

int main(int argc, char **argv) {
    rd_kafka_t *rk;
    rd_kafka_conf_t *conf;
    rd_kafka_topic_partition_list_t *topic_list;
    char errstr[512];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <broker> <topic>\n", argv[0]);
        return 1;
    }

    /* Create configuration objects */
    conf = rd_kafka_conf_new();

    /* Set configuration properties */
    rd_kafka_conf_set(conf, "bootstrap.servers", argv[1], errstr, sizeof(errstr));
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
    rd_kafka_topic_partition_list_add(topic_list, argv[2], RD_KAFKA_PARTITION_UA);

    /* Subscribe to topic */
    if (rd_kafka_subscribe(rk, topic_list) != RD_KAFKA_RESP_ERR_NO_ERROR) {
        fprintf(stderr, "Failed to subscribe to topic: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
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
                // printf("Received message (%zd bytes): %.*s\n", msg->len, (int)msg->len, (char *)msg->payload);
                parse_message(msg, 0);
            } else {
                fprintf(stderr, "Failed to consume message: %s\n", rd_kafka_err2str(msg->err));
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
