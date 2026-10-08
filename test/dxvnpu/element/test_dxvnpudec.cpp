#include <gst/app/gstappsink.h>
#include <gst/check/gstcheck.h>
#include <gst/video/video.h>

#include "npu_env.hpp"

using namespace dxtest;

static gboolean require_device() {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") == 0) return TRUE;
    g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
    return FALSE;
}

static void decode_sample(const char *filename, const char *parser) {
    if (!require_device()) return;

    const std::string video = resolve_video_path(filename);
    if (video.empty()) {
        g_test_skip("DXVNPU sample video is not installed; run ./setup.sh");
        return;
    }

    GError *error = nullptr;
    gchar *launch = g_strdup_printf(
        "filesrc location=%s ! qtdemux ! %s ! dxvnpudec ! "
        "appsink name=sink sync=false max-buffers=4 drop=false",
        video.c_str(), parser);
    GstElement *pipeline = gst_parse_launch(launch, &error);
    g_free(launch);
    fail_unless(error == nullptr && pipeline != nullptr,
                "pipeline parse failed: %s", error ? error->message : "unknown");
    g_clear_error(&error);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    GstBus *bus = gst_element_get_bus(pipeline);
    fail_unless(sink != nullptr && bus != nullptr);
    fail_unless(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    guint samples = 0;
    while (GstSample *sample = gst_app_sink_try_pull_sample(
               GST_APP_SINK(sink), 10 * GST_SECOND)) {
        GstVideoInfo info;
        fail_unless(gst_video_info_from_caps(&info, gst_sample_get_caps(sample)));
        fail_unless_equals_int(GST_VIDEO_INFO_FORMAT(&info), GST_VIDEO_FORMAT_NV12);
        fail_unless(GST_VIDEO_INFO_WIDTH(&info) > 0 && GST_VIDEO_INFO_HEIGHT(&info) > 0);
        const gsize minimum_size =
            static_cast<gsize>(GST_VIDEO_INFO_WIDTH(&info)) * GST_VIDEO_INFO_HEIGHT(&info) * 3 / 2;
        fail_unless(gst_buffer_get_size(gst_sample_get_buffer(sample)) >= minimum_size);
        ++samples;
        gst_sample_unref(sample);
    }

    GstMessage *message = gst_bus_timed_pop_filtered(
        bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(message != nullptr && GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS,
                "decoder must finish the sample without a bus error");
    gst_message_unref(message);
    fail_unless(samples > 0, "decoder must produce NV12 frames");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(sink);
    gst_object_unref(pipeline);
}

GST_START_TEST(CE_dxvnpudec_h264_sample) {
    decode_sample("codec_test_clip_h264_16Mbps.mp4", "h264parse");
}
GST_END_TEST;

GST_START_TEST(CE_dxvnpudec_h265_sample) {
    decode_sample("codec_test_clip_h265_8Mbps.mp4", "h265parse");
}
GST_END_TEST;

static Suite *dxvnpudec_suite() {
    Suite *suite = suite_create("dxvnpudec");
    TCase *testcase = tcase_create("acceptance");
    tcase_set_timeout(testcase, 60.0);
    suite_add_tcase(suite, testcase);
    tcase_add_test(testcase, CE_dxvnpudec_h264_sample);
    tcase_add_test(testcase, CE_dxvnpudec_h265_sample);
    return suite;
}

GST_CHECK_MAIN(dxvnpudec);
