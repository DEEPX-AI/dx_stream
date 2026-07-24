// PL-E — dxrate framerate conversion verification
// videotestsrc(30fps) → dxrate(15fps) → appsink
// Verify framerate conversion by output buffer count and PTS interval

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "pipeline_tc_helpers.hpp"
#include "npu_env.hpp"

#include <vector>
#include <cmath>

using namespace dxtest;

static const char *RATE_PIPE =
    "videotestsrc num-buffers=30 "
    "! video/x-raw,format=I420,width=64,height=64,framerate=30/1 "
    "! dxrate framerate=15 "
    "! appsink name=sink sync=false drop=true";

static const char *RATE_PIPE_NOAPP =
    "videotestsrc num-buffers=30 "
    "! video/x-raw,format=I420,width=64,height=64,framerate=30/1 "
    "! dxrate framerate=15 "
    "! fakesink sync=false";

GST_START_TEST(PL_E_rate_reduces_framerate) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(RATE_PIPE, &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    std::vector<GstClockTime> pts_list;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              3 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        GstClockTime pts = GST_BUFFER_PTS(buf);
        if (GST_CLOCK_TIME_IS_VALID(pts))
            pts_list.push_back(pts);
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless((int)pts_list.size() <= 16,
                "30fps→15fps: output count must be ~15 (got %d)",
                (int)pts_list.size());
    fail_unless((int)pts_list.size() >= 10,
                "must produce at least 10 buffers (got %d)",
                (int)pts_list.size());

    if (pts_list.size() >= 2) {
        GstClockTime interval = pts_list[1] - pts_list[0];
        GstClockTime expected = GST_SECOND / 15;
        double ratio = (double)interval / (double)expected;
        fail_unless(ratio > 0.5 && ratio < 2.0,
                    "PTS interval must be ~66ms (got %" G_GUINT64_FORMAT " ns)",
                    interval);
    }

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_E_rate_eos) {
    test_eos_propagation(RATE_PIPE_NOAPP);
}
GST_END_TEST;

GST_START_TEST(PL_E_rate_lifecycle) {
    test_lifecycle_cycle(RATE_PIPE_NOAPP, 5);
}
GST_END_TEST;

GST_START_TEST(PL_E_rate_latency) {
    test_latency_query(RATE_PIPE_NOAPP);
}
GST_END_TEST;

static Suite *pl_interval_suite(void) {
    Suite *s = suite_create("pl_interval");
    TCase *tc = tcase_create("rate");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_E_rate_reduces_framerate);
    tcase_add_test(tc, PL_E_rate_eos);
    tcase_add_test(tc, PL_E_rate_lifecycle);
    tcase_add_test(tc, PL_E_rate_latency);
    return s;
}

GST_CHECK_MAIN(pl_interval);
