// Phase 2 — State cycle stress test
// NULL→PLAYING→NULL x10 for multiple pipeline topologies.
// Detects resource leaks, thread join failures, dlclose misses.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "pipeline_tc_helpers.hpp"
#include "npu_env.hpp"
#include "yolo_pipeline.hpp"

using namespace dxtest;

static const char *TRANSFORM_PIPE =
    "videotestsrc num-buffers=3 "
    "! video/x-raw,format=I420,width=64,height=64,framerate=30/1 "
    "! dxscale width=32 height=32 "
    "! dxconvert ! video/x-raw,format=RGB "
    "! fakesink sync=false";

static const char *RATE_PIPE =
    "videotestsrc num-buffers=10 "
    "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
    "! dxrate framerate=15 "
    "! fakesink sync=false";

static const char *TRACKER_PIPE =
    "videotestsrc num-buffers=3 "
    "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
    "! dxtracker "
    "! fakesink sync=false";

GST_START_TEST(PL_state_stress_transform_10x) {
    test_lifecycle_cycle(TRANSFORM_PIPE, 10);
}
GST_END_TEST;

GST_START_TEST(PL_state_stress_rate_10x) {
    test_lifecycle_cycle(RATE_PIPE, 10);
}
GST_END_TEST;

GST_START_TEST(PL_state_stress_tracker_10x) {
    test_lifecycle_cycle(TRACKER_PIPE, 10);
}
GST_END_TEST;

GST_START_TEST(PL_state_stress_infer_5x) {
    DXTEST_SKIP_IF(!npu_available(), "NPU not available");
    std::string desc = build_infer_pipeline(YOLO_DEFAULT, "fakesink sync=false");
    DXTEST_SKIP_IF(desc.empty(), "model/postprocess not available");
    test_lifecycle_cycle(desc.c_str(), 5);
}
GST_END_TEST;

GST_START_TEST(PL_state_stress_rapid_toggle) {
    for (int i = 0; i < 20; i++) {
        GError *err = nullptr;
        GstElement *pipe = gst_parse_launch(TRANSFORM_PIPE, &err);
        fail_unless(pipe != nullptr);
        if (err) g_error_free(err);

        gst_element_set_state(pipe, GST_STATE_PLAYING);
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }
}
GST_END_TEST;

GST_START_TEST(PL_state_stress_paused_playing) {
    for (int i = 0; i < 10; i++) {
        GError *err = nullptr;
        GstElement *pipe = gst_parse_launch(TRANSFORM_PIPE, &err);
        fail_unless(pipe != nullptr);
        if (err) g_error_free(err);

        gst_element_set_state(pipe, GST_STATE_PAUSED);
        gst_element_get_state(pipe, nullptr, nullptr, 2 * GST_SECOND);
        gst_element_set_state(pipe, GST_STATE_PLAYING);
        g_usleep(50 * 1000);
        gst_element_set_state(pipe, GST_STATE_PAUSED);
        gst_element_get_state(pipe, nullptr, nullptr, 2 * GST_SECOND);
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }
}
GST_END_TEST;

static Suite *pl_state_stress_suite(void) {
    Suite *s = suite_create("pl_state_stress");
    TCase *tc = tcase_create("state_cycles");
    tcase_set_timeout(tc, 120.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_state_stress_transform_10x);
    tcase_add_test(tc, PL_state_stress_rate_10x);
    tcase_add_test(tc, PL_state_stress_tracker_10x);
    tcase_add_test(tc, PL_state_stress_infer_5x);
    tcase_add_test(tc, PL_state_stress_rapid_toggle);
    tcase_add_test(tc, PL_state_stress_paused_playing);
    return s;
}

GST_CHECK_MAIN(pl_state_stress);
