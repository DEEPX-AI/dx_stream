// P5.2 — dxmsgbroker contract tests (lifecycle only)
// Core: Inherits GstBaseSink. Selects MQTT/Kafka via broker-name.
// Attempts connect in start(). No external broker, so only verifies up to connect failure.
// Invalid broker-name → ERROR. Valid name but unreachable → ERROR.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

#include <cstring>

using namespace dxtest;

static void assert_pipeline_error_contains(const gchar *launch, const gchar *needle) {
    GError *parse_error = nullptr;
    GstElement *pipe = gst_parse_launch(launch, &parse_error);
    fail_unless(parse_error == nullptr && pipe != nullptr,
                "pipeline parse failed: %s", parse_error ? parse_error->message : "unknown");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PAUSED);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, GST_MESSAGE_ERROR);
    fail_unless(msg != nullptr, "expected pipeline error containing '%s'", needle);

    GError *error = nullptr;
    gchar *debug = nullptr;
    gst_message_parse_error(msg, &error, &debug);
    fail_unless(error != nullptr && g_strrstr(error->message, needle) != nullptr,
                "expected error message containing '%s', got '%s'", needle,
                error ? error->message : "<null>");
    g_clear_error(&error);
    g_free(debug);
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxmsgbroker", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxmsgbroker", nullptr);

    gchar *broker = nullptr;
    gchar *conn = nullptr;
    gchar *cfg = nullptr;
    gchar *topic = nullptr;

    g_object_get(e, "broker-name", &broker,
                 "conn-info", &conn,
                 "config", &cfg,
                 "topic", &topic, nullptr);

    fail_unless(g_strcmp0(broker, "mqtt") == 0,
                "broker-name default must be 'mqtt' (got '%s')", broker);
    fail_unless(conn == nullptr, "conn-info default must be null");
    fail_unless(cfg == nullptr, "config default must be null");
    fail_unless(topic == nullptr, "topic default must be null");

    g_free(broker);
    g_free(conn);
    g_free(cfg);
    g_free(topic);

    g_object_set(e, "broker-name", "kafka",
                 "conn-info", "localhost:9092",
                 "topic", "test_topic", nullptr);

    g_object_get(e, "broker-name", &broker,
                 "conn-info", &conn,
                 "topic", &topic, nullptr);
    fail_unless(g_strcmp0(broker, "kafka") == 0);
    fail_unless(g_strcmp0(conn, "localhost:9092") == 0);
    fail_unless(g_strcmp0(topic, "test_topic") == 0);

    g_free(broker);
    g_free(conn);
    g_free(topic);

    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_broker_invalid_type_error: invalid broker-name → READY failure
// Target: gst_dxmsgbroker_start L207-211
// MUT: remove L207-211 → nullptr function pointers → crash on render
GST_START_TEST(CE_broker_invalid_type_error) {
    GstElement *e = gst_element_factory_make("dxmsgbroker", nullptr);
    g_object_set(e, "broker-name", "invalid_broker", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_broker_connect_fail_error: valid broker name + unreachable → READY failure
// Target: gst_dxmsgbroker_start L216-220
// MUT: remove L216-220 → nullptr handle → crash on render
GST_START_TEST(CE_broker_connect_fail_error) {
    GstElement *e = gst_element_factory_make("dxmsgbroker", nullptr);
    g_object_set(e, "broker-name", "mqtt",
                 "conn-info", "127.0.0.1:1", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_broker_missing_conn_info_error: missing conn-info → SETTINGS error before broker connect
// Target: gst_dxmsgbroker_start required property validation
GST_START_TEST(CE_broker_missing_conn_info_error) {
    assert_pipeline_error_contains(
        "fakesrc num-buffers=0 ! dxmsgbroker broker-name=mqtt topic=test_topic",
        "conn-info");
}
GST_END_TEST;

// CE_broker_missing_topic_error: missing topic → SETTINGS error before broker connect
// Target: gst_dxmsgbroker_start required property validation
GST_START_TEST(CE_broker_missing_topic_error) {
    assert_pipeline_error_contains(
        "fakesrc num-buffers=0 ! dxmsgbroker broker-name=mqtt conn-info=127.0.0.1:1",
        "topic");
}
GST_END_TEST;

static Suite *dxmsgbroker_suite(void) {
    Suite *s = suite_create("dxmsgbroker");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CE_broker_invalid_type_error);
    tcase_add_test(tc, CE_broker_connect_fail_error);
    tcase_add_test(tc, CE_broker_missing_conn_info_error);
    tcase_add_test(tc, CE_broker_missing_topic_error);
    return s;
}

GST_CHECK_MAIN(dxmsgbroker);
