#include <gst/check/gstcheck.h>
#include <gst/gst.h>

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxmsgbroker", NULL);
    fail_unless(element != NULL, "Failed to create GstDxMsgBroker element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    GstElement *element = gst_element_factory_make("dxmsgbroker", NULL);
    fail_unless(element != NULL, "Failed to create GstDxMsgBroker element");

    g_object_set(element, "conn-info", "localhost:1883", NULL);
    gchar *conn_info;
    g_object_get(element, "conn-info", &conn_info, NULL);
    fail_unless(g_strcmp0(conn_info, "localhost:1883") == 0,
                "'conn-info' properties not set correctly");
    g_free(conn_info);

    ret = gst_element_set_state(element, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to READY");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_READY,
                "State should be READY but got different state");

    ret = gst_element_set_state(element, GST_STATE_PAUSED);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to PAUSED");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_PAUSED,
                "State should be PAUSED but got different state");

    ret = gst_element_set_state(element, GST_STATE_PLAYING);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to PLAYING");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_PLAYING,
                "State should be PLAYING but got different state");

    gst_element_set_state(element, GST_STATE_NULL);
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_properties) {
    GstElement *element = gst_element_factory_make("dxmsgbroker", NULL);
    fail_unless(element != NULL, "Failed to create GstDxMsgBroker element");

    g_object_set(element, "broker-name", "TEST_BROKER", NULL);
    gchar *broker_name;
    g_object_get(element, "broker-name", &broker_name, NULL);
    fail_unless(g_strcmp0(broker_name, "TEST_BROKER") == 0,
                "'broker-name' properties not set correctly");
    g_free(broker_name);

    g_object_set(element, "topic", "TEST_TOPIC", NULL);
    gchar *topic;
    g_object_get(element, "topic", &topic, NULL);
    fail_unless(g_strcmp0(topic, "TEST_TOPIC") == 0,
                "'topic' properties not set correctly");
    g_free(topic);

    g_object_set(element, "conn-info", "localhost:1883", NULL);
    gchar *conn_info;
    g_object_get(element, "conn-info", &conn_info, NULL);
    fail_unless(g_strcmp0(conn_info, "localhost:1883") == 0,
                "'conn-info' properties not set correctly");
    g_free(conn_info);

    g_object_set(element, "config", "TEST_CONFIG", NULL);
    gchar *config;
    g_object_get(element, "config", &config, NULL);
    fail_unless(g_strcmp0(config, "TEST_CONFIG") == 0,
                "'config' properties not set correctly");
    g_free(config);

    gst_object_unref(element);
}
GST_END_TEST

Suite *dxmsgbroker_suite(void) {
    Suite *s = suite_create("GstDxMsgBroker");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_element_initialization);
    tcase_add_test(tc_core, test_element_state_change);
    tcase_add_test(tc_core, test_element_properties);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxmsgbroker_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
