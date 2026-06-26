// PL-G variant — Real video file decoding → transform chain verification
// Resource condition: SKIP if boat.mp4 not found

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "pipeline_tc_helpers.hpp"
#include "npu_env.hpp"

#include <string>

using namespace dxtest;

static std::string video_pipe_desc() {
    std::string video = resolve_video_path("boat.mp4");
    if (video.empty()) return "";
    return "filesrc location=" + video +
           " ! decodebin ! videoconvert"
           " ! video/x-raw,format=RGB"
           " ! dxscale width=320 height=240"
           " ! dxconvert"
           " ! video/x-raw,format=BGR"
           " ! appsink name=sink sync=false drop=true";
}

static std::string video_pipe_noapp() {
    std::string video = resolve_video_path("boat.mp4");
    if (video.empty()) return "";
    return "filesrc location=" + video +
           " ! decodebin ! videoconvert"
           " ! video/x-raw,format=RGB"
           " ! dxscale width=320 height=240"
           " ! dxconvert"
           " ! video/x-raw,format=BGR"
           " ! fakesink sync=false";
}

GST_START_TEST(PL_video_decode_transform) {
    std::string desc = video_pipe_desc();
    DXTEST_SKIP_IF(desc.empty(), "boat.mp4 not found");

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    int count = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              2 * GST_SECOND)) != nullptr) {
        GstCaps *caps = gst_sample_get_caps(s);
        GstVideoInfo info;
        fail_unless(gst_video_info_from_caps(&info, caps));
        fail_unless_equals_int(GST_VIDEO_INFO_WIDTH(&info), 320);
        fail_unless_equals_int(GST_VIDEO_INFO_HEIGHT(&info), 240);
        fail_unless_equals_int(GST_VIDEO_INFO_FORMAT(&info), GST_VIDEO_FORMAT_BGR);
        count++;
        gst_sample_unref(s);
        if (count >= 30) break;
    }
    gst_object_unref(sink);

    fail_unless(count > 0, "must produce output buffers (got %d)", count);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_video_lifecycle) {
    std::string desc = video_pipe_noapp();
    DXTEST_SKIP_IF(desc.empty(), "boat.mp4 not found");
    test_lifecycle_cycle(desc.c_str(), 3);
}
GST_END_TEST;

static Suite *pl_video_suite(void) {
    Suite *s = suite_create("pl_video");
    TCase *tc = tcase_create("video_data");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_video_decode_transform);
    tcase_add_test(tc, PL_video_lifecycle);
    return s;
}

GST_CHECK_MAIN(pl_video);
