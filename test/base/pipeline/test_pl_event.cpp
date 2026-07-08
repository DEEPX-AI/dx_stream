// L2 common event/state verification — based on transform chain pipeline
// Calls common TC helpers to verify lifecycle, EOS, error recovery, latency, meta preservation.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "pipeline_tc_helpers.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static const char *TRANSFORM_PIPE =
    "videotestsrc num-buffers=10 "
    "! video/x-raw,format=I420,width=320,height=240,framerate=30/1 "
    "! dxscale width=160 height=120 "
    "! dxconvert "
    "! video/x-raw,format=RGB "
    "! fakesink sync=false";

static const char *TRANSFORM_PIPE_WITH_IDENTITY =
    "videotestsrc num-buffers=10 "
    "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
    "! identity name=meta_adder "
    "! dxscale width=32 height=32 "
    "! dxconvert "
    "! appsink name=sink sync=false";

static const char *TRANSFORM_PIPE_BAD =
    "videotestsrc num-buffers=5 "
    "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
    "! dxpreprocess resize-width=0 resize-height=0 "
    "! fakesink sync=false";

GST_START_TEST(PL_lifecycle_play_null_cycle) {
    test_lifecycle_cycle(TRANSFORM_PIPE, 5);
}
GST_END_TEST;

GST_START_TEST(PL_eos_propagation) {
    test_eos_propagation(TRANSFORM_PIPE);
}
GST_END_TEST;

GST_START_TEST(PL_error_recovery) {
    test_error_recovery(TRANSFORM_PIPE_BAD);
}
GST_END_TEST;

GST_START_TEST(PL_latency_query) {
    test_latency_query(TRANSFORM_PIPE);
}
GST_END_TEST;

GST_START_TEST(PL_meta_preservation) {
    test_meta_preservation(TRANSFORM_PIPE_WITH_IDENTITY,
                           "meta_adder", "sink", 0);
}
GST_END_TEST;

static Suite *pl_event_suite(void) {
    Suite *s = suite_create("pl_event");
    TCase *tc = tcase_create("lifecycle");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_lifecycle_play_null_cycle);
    tcase_add_test(tc, PL_eos_propagation);
    tcase_add_test(tc, PL_error_recovery);
    tcase_add_test(tc, PL_latency_query);
    tcase_add_test(tc, PL_meta_preservation);
    return s;
}

GST_CHECK_MAIN(pl_event);
