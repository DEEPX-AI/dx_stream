#include <gst/check/gstcheck.h>
#include <gst/gst.h>

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxtiler", NULL);
    fail_unless(element != NULL, "Failed to create GstDxTiler element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    GstElement *element = gst_element_factory_make("dxtiler", NULL);
    fail_unless(element != NULL, "Failed to create GstDxTiler element");

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
    GstElement *element = gst_element_factory_make("dxtiler", NULL);
    fail_unless(element != NULL, "Failed to create GstDxTiler element");

    g_object_set(element, "cols", 3, NULL);
    guint cols;
    g_object_get(element, "cols", &cols, NULL);
    fail_unless(cols == 3, "'cols' property not set correctly");

    g_object_set(element, "rows", 1, NULL);
    guint rows;
    g_object_get(element, "rows", &rows, NULL);
    fail_unless(rows == 1, "'rows' property not set correctly");

    g_object_set(element, "width", 113, NULL);
    guint width;
    g_object_get(element, "width", &width, NULL);
    fail_unless(width == 113, "'width' property not set correctly");

    g_object_set(element, "height", 224, NULL);
    guint height;
    g_object_get(element, "height", &height, NULL);
    fail_unless(height == 224, "'height' property not set correctly");

    // Json
    g_object_set(element, "config-file-path",
                 "./../../test_configs/tiler_config.json", NULL);
    gchar *config_file_path;
    g_object_get(element, "config-file-path", &config_file_path, NULL);
    fail_unless(g_strcmp0(config_file_path,
                          "./../../test_configs/tiler_config.json") == 0,
                "'config-file-path' properties not set correctly");
    g_free(config_file_path);

    g_object_get(element, "cols", &cols, "rows", &rows, "width", &width,
                 "height", &height, NULL);
    fail_unless(cols == 2 && rows == 2 && width == 640 && height == 480,
                "'cols' property not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

Suite *dxtiler_suite(void) {
    Suite *s = suite_create("GstDxTiler");
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
    Suite *s = dxtiler_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
