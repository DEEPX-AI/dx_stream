#include <gst/check/gstcheck.h>
#include <gst/gst.h>

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxinfer", NULL);
    fail_unless(element != NULL, "Failed to create GstDxInfer element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    GstElement *element = gst_element_factory_make("dxinfer", NULL);
    fail_unless(element != NULL, "Failed to create GstDxInfer element");
    g_object_set(element, "model-path",
                 "./../../../dx_stream/samples/models/YOLOV5S_1.dxnn", NULL);

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
    GstElement *element = gst_element_factory_make("dxinfer", NULL);
    fail_unless(element != NULL, "Failed to create GstDxInfer element");

    g_object_set(element, "preprocess-id", 22, NULL);
    guint preprocess_id;
    g_object_get(element, "preprocess-id", &preprocess_id, NULL);
    fail_unless(preprocess_id == 22,
                "'preprocess-id' property not set correctly");

    g_object_set(element, "inference-id", 21, NULL);
    gint inference_id;
    g_object_get(element, "inference-id", &inference_id, NULL);
    fail_unless(inference_id == 21,
                "'inference-id' property not set correctly");

    g_object_set(element, "model-path", "/home/test/test.dxnn", NULL);
    gchar *model_path;
    g_object_get(element, "model-path", &model_path, NULL);
    fail_unless(g_strcmp0(model_path, "/home/test/test.dxnn") == 0,
                "'model-path' properties not set correctly");
    g_free(model_path);

    g_object_set(element, "secondary-mode", TRUE, NULL);
    gboolean secondary_mode;
    g_object_get(element, "secondary-mode", &secondary_mode, NULL);
    fail_unless(secondary_mode == TRUE,
                "'secondary-mode' property not set correctly");

    // Json
    g_object_set(element, "config-file-path",
                 "./../../test_configs/inference_config.json", NULL);
    gchar *config_file_path;
    g_object_get(element, "config-file-path", &config_file_path, NULL);
    fail_unless(g_strcmp0(config_file_path,
                          "./../../test_configs/inference_config.json") == 0,
                "'config-file-path' properties not set correctly");
    g_free(config_file_path);

    g_object_get(element, "preprocess-id", &preprocess_id, NULL);
    fail_unless(preprocess_id == 1,
                "'preprocess-id' property not set correctly");

    g_object_get(element, "inference-id", &inference_id, NULL);
    fail_unless(inference_id == 1, "'inference-id' property not set correctly");

    g_object_get(element, "model-path", &model_path, NULL);
    fail_unless(
        g_strcmp0(model_path,
                  "./../../../dx_stream/samples/models/YOLOV5S_1.dxnn") == 0,
        "'model-path' properties not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

Suite *dxinfer_suite(void) {
    Suite *s = suite_create("GstDxInfer");
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
    Suite *s = dxinfer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
