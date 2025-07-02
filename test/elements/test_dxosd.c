#include <gst/check/gstcheck.h>
#include <gst/gst.h>

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxosd", NULL);
    fail_unless(element != NULL, "Failed to create GstDxOsd element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;
    GstElement *element = gst_element_factory_make("dxosd", NULL);
    fail_unless(element != NULL, "Failed to create GstDxOsd element");

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

Suite *dxosd_suite(void) {
    Suite *s = suite_create("GstDxOsd");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_element_initialization);
    tcase_add_test(tc_core, test_element_state_change);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxosd_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
