#include <gst/check/gstcheck.h>
#include <gst/gst.h>

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPostprocess element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    GstElement *element = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPostprocess element");

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
    GstElement *element = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPostprocess element");

    g_object_set(element, "inference-id", 6, NULL);
    gint inference_id;
    g_object_get(element, "inference-id", &inference_id, NULL);
    fail_unless(inference_id == 6, "'inference-id' property not set correctly");

    g_object_set(element, "function-name", "TEST_FUNC_POST", NULL);
    gchar *function_name;
    g_object_get(element, "function-name", &function_name, NULL);
    fail_unless(g_strcmp0(function_name, "TEST_FUNC_POST") == 0,
                "'function-name' properties not set correctly");

    g_object_set(element, "library-file-path", "TEST_POST_LIBRARY_PATH", NULL);
    gchar *library_file_path;
    g_object_get(element, "library-file-path", &library_file_path, NULL);
    fail_unless(g_strcmp0(library_file_path, "TEST_POST_LIBRARY_PATH") == 0,
                "'library-file-path' properties not set correctly");

    g_object_set(element, "secondary-mode", TRUE, NULL);
    gboolean secondary_mode;
    g_object_get(element, "secondary-mode", &secondary_mode, NULL);
    fail_unless(secondary_mode == TRUE,
                "'secondary-mode' property not set correctly");

    // Json
    g_object_set(element, "config-file-path",
                 "./../../test_configs/postprocess_config.json", NULL);
    gchar *config_file_path;
    g_object_get(element, "config-file-path", &config_file_path, NULL);
    fail_unless(g_strcmp0(config_file_path,
                          "./../../test_configs/postprocess_config.json") == 0,
                "'config-file-path' properties not set correctly");
    g_free(config_file_path);

    g_object_get(element, "inference-id", &inference_id, NULL);
    fail_unless(inference_id == 1, "'inference-id' property not set correctly");

    g_object_get(element, "library-file-path", &library_file_path, NULL);
    fail_unless(g_strcmp0(library_file_path,
                          "/usr/share/dx-stream/lib/libpostprocess_yolo.so") ==
                    0,
                "'library-file-path' properties not set correctly");

    g_object_get(element, "function-name", &function_name, NULL);
    fail_unless(g_strcmp0(function_name, "YOLOV5S_1") == 0,
                "'function-name' properties not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

Suite *dxpostprocess_suite(void) {
    Suite *s = suite_create("GstDxPostprocess");
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
    Suite *s = dxpostprocess_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
