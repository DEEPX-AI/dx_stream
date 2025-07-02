#include <cstdio>
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <json-glib/json-glib.h>

typedef struct _myData_t {
    char topic[32];
} myData_t;

/* Callback called when the client receives a CONNACK message from the broker.
 */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    int rc;
    myData_t *myData = (myData_t *)obj;

    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        mosquitto_disconnect(mosq);
    }

    rc = mosquitto_subscribe(mosq, nullptr, myData->topic, 0);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error subscribing: %s\n", mosquitto_strerror(rc));
        mosquitto_disconnect(mosq);
    }
}

/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count,
                  const int *granted_qos) {
    int i;
    bool have_subscription = false;

    for (i = 0; i < qos_count; i++) {
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if (granted_qos[i] <= 2) {
            have_subscription = true;
        }
    }
    if (have_subscription == false) {
        fprintf(stderr, "Error: All subscriptions rejected.\n");
        mosquitto_disconnect(mosq);
    }
}

/* Callback called when the client receives a message. */
void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
    char *payload = (char *)msg->payload;

    // printf("%s %d %s\n", msg->topic, msg->qos, (char *)msg->payload);
    JsonParser *parser;
    JsonNode *root;
    GError *error = nullptr;

    parser = json_parser_new();
    if (!json_parser_load_from_data(parser, payload, -1, &error)) {
        fprintf(stderr, "Unable to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    root = json_parser_get_root(parser);
    if (JSON_NODE_HOLDS_OBJECT(root)) {
        /* print whole json */
        char *formatted = json_to_string(root, true);
        printf("Received JSON payload: %s\n", formatted);
        g_free(formatted);

        // /* print only seqId */
        // JsonObject *object = json_node_get_object(root);
        // if (0 && json_object_has_member(object, "seqId")) {
        //     JsonNode *seqId_node = json_object_get_member(object, "seqId");
        //     if (JSON_NODE_HOLDS_VALUE(seqId_node)) {
        //         int seqId = json_node_get_int(seqId_node);
        //         printf("seqId: %d\n", seqId);
        //     }
        // }
    } else {
        fprintf(stderr, "Received payload is not a JSON object.\n");
    }

    g_object_unref(parser);
}

void print_usage() {
    printf("Usage: mqtt_sub_example -h <hostname> -t <topic> [-p <port>]\n");
}

bool parse_args(int argc, char *argv[], char **hostname, char **topic,
                int *port) {
    int opt;
    bool ret = true;

    *hostname = nullptr;
    *topic = nullptr;
    *port = 1883; // default port

    while ((opt = getopt(argc, argv, "h:t:p:")) != -1) {
        switch (opt) {
        case 'h':
            *hostname = optarg;
            break;
        case 't':
            *topic = optarg;
            break;
        case 'p':
            *port = atoi(optarg);
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

int main(int argc, char *argv[]) {
    struct mosquitto *mosq;
    int rc;

    char *hostname, *topic;
    int port;
    myData_t data;

    rc = parse_args(argc, argv, &hostname, &topic, &port);
    if (rc == false) {
        exit(EXIT_FAILURE);
    }

    mosquitto_lib_init();

    mosq = mosquitto_new(nullptr, true, nullptr);
    if (mosq == nullptr) {
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }

    snprintf(data.topic, sizeof(data.topic), "%s", topic);
    data.topic[sizeof(data.topic) - 1] = '\0';
    mosquitto_user_data_set(mosq, &data);

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_message_callback_set(mosq, on_message);

    rc = mosquitto_connect(mosq, "localhost", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
        return 1;
    }

    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_lib_cleanup();
    return 0;
}
