// Phase 6 — Demo pipeline smoke tests (headless)
// A10: single/multi/secondary pipeline scenarios run without error
// Each test builds a real inference pipeline with fakesink/appsink, verifies output

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "npu_env.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *MODEL = "yolov5-s_640x640_ppu.dxnn";
static std::string POSTLIB_PATH() {
    return dxtest::resolve_lib_path("libpostprocess_ppu.so");
}

static bool demo_resources_available() {
    if (!npu_available()) return false;
    std::string model = resolve_model_path(MODEL);
    if (model.empty()) return false;
    if (!path_exists(POSTLIB_PATH())) return false;
    return true;
}

// PL_demo_single_infer: single-stream inference pipeline runs to EOS
// Target: full chain (preprocess→infer→postprocess→osd) with real model + synthetic input
GST_START_TEST(PL_demo_single_infer) {
    DXTEST_SKIP_IF(!demo_resources_available(), "NPU/model/POSTLIB_PATH().c_str() not available");

    std::string model = resolve_model_path(MODEL);

    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=5 "
        "! video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 preprocess-id=1 "
        "! queue max-size-buffers=2 "
        "! dxinfer model-path=%s preprocess-id=1 inference-id=1 "
        "! queue max-size-buffers=2 "
        "! dxpostprocess library-file-path=%s function-name=YOLOV5S_PPU inference-id=1 "
        "! queue max-size-buffers=2 "
        "! dxosd "
        "! fakesink name=sink sync=false",
        model.c_str(), POSTLIB_PATH().c_str());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    g_free(desc);
    fail_unless(err == nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");

    GstStateChangeReturn ret = gst_element_set_state(pipe, GST_STATE_PLAYING);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE, "pipeline PLAYING failed");

    GstBus *bus = gst_element_get_bus(pipe);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 60 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless(msg != nullptr, "timeout waiting for EOS/ERROR");
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "expected EOS, got %s", GST_MESSAGE_TYPE_NAME(msg));

    gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// PL_demo_single_detections: inference pipeline produces frames with DXFrameMeta
// Target: preprocess→infer→postprocess chain populates metadata on output buffers
GST_START_TEST(PL_demo_single_detections) {
    DXTEST_SKIP_IF(!demo_resources_available(), "NPU/model/POSTLIB_PATH().c_str() not available");

    std::string model = resolve_model_path(MODEL);

    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=5 "
        "! video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 preprocess-id=1 "
        "! queue max-size-buffers=2 "
        "! dxinfer model-path=%s preprocess-id=1 inference-id=1 "
        "! queue max-size-buffers=2 "
        "! dxpostprocess library-file-path=%s function-name=YOLOV5S_PPU inference-id=1 "
        "! appsink name=sink sync=false drop=true",
        model.c_str(), POSTLIB_PATH().c_str());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    g_free(desc);
    fail_unless(err == nullptr);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    int total_frames = 0;
    int frames_with_meta = 0;

    for (int i = 0; i < 5; i++) {
        GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10 * GST_SECOND);
        if (!s) break;
        total_frames++;
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm)
            frames_with_meta++;
        gst_sample_unref(s);
    }

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(sink);
    gst_object_unref(pipe);

    fail_unless(total_frames > 0, "no frames received from inference chain");
    fail_unless(frames_with_meta > 0,
                "inference chain must produce DXFrameMeta (got 0/%d)", total_frames);
}
GST_END_TEST;

// PL_demo_with_tracker: inference + tracker pipeline runs without error
// Target: dxtracker processes frames after postprocess
GST_START_TEST(PL_demo_with_tracker) {
    DXTEST_SKIP_IF(!demo_resources_available(), "NPU/model/POSTLIB_PATH().c_str() not available");

    std::string root = dx_stream_root();
    std::string model = resolve_model_path(MODEL);
    std::string tracker_cfg = root + "/dx_stream/configs/tracker_config.json";
    DXTEST_SKIP_IF(!path_exists(tracker_cfg), "tracker_config.json not found");

    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=10 "
        "! video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 preprocess-id=1 "
        "! queue max-size-buffers=2 "
        "! dxinfer model-path=%s preprocess-id=1 inference-id=1 "
        "! queue max-size-buffers=2 "
        "! dxpostprocess library-file-path=%s function-name=YOLOV5S_PPU inference-id=1 "
        "! queue max-size-buffers=2 "
        "! dxtracker config-file-path=%s "
        "! appsink name=sink sync=false drop=true",
        model.c_str(), POSTLIB_PATH().c_str(), tracker_cfg.c_str());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc, &err);
    g_free(desc);
    fail_unless(err == nullptr);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    int total_frames = 0;
    for (int i = 0; i < 10; i++) {
        GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10 * GST_SECOND);
        if (!s) break;
        total_frames++;
        gst_sample_unref(s);
    }

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(sink);
    gst_object_unref(pipe);

    fail_unless(total_frames > 0,
                "tracker pipeline must produce output (got 0 frames)");
}
GST_END_TEST;

// PL_demo_lifecycle: inference pipeline survives PLAYING→NULL×3
GST_START_TEST(PL_demo_lifecycle) {
    DXTEST_SKIP_IF(!demo_resources_available(), "NPU/model/POSTLIB_PATH().c_str() not available");

    std::string model = resolve_model_path(MODEL);

    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 preprocess-id=1 "
        "! dxinfer model-path=%s preprocess-id=1 inference-id=1 "
        "! dxpostprocess library-file-path=%s function-name=YOLOV5S_PPU inference-id=1 "
        "! fakesink sync=false",
        model.c_str(), POSTLIB_PATH().c_str());

    for (int cycle = 0; cycle < 3; cycle++) {
        GError *err = nullptr;
        GstElement *pipe = gst_parse_launch(desc, &err);
        fail_unless(err == nullptr, "cycle %d parse failed", cycle);

        GstStateChangeReturn ret = gst_element_set_state(pipe, GST_STATE_PLAYING);
        fail_unless(ret != GST_STATE_CHANGE_FAILURE, "cycle %d PLAYING failed", cycle);

        GstBus *bus = gst_element_get_bus(pipe);
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 30 * GST_SECOND,
            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

        if (msg) {
            fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                        "cycle %d: expected EOS, got %s",
                        cycle, GST_MESSAGE_TYPE_NAME(msg));
            gst_message_unref(msg);
        }

        gst_object_unref(bus);
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }

    g_free(desc);
}
GST_END_TEST;

static Suite *demo_pipelines_suite(void) {
    Suite *s = suite_create("demo_pipelines");
    TCase *tc = tcase_create("demo");
    tcase_set_timeout(tc, 120.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_demo_single_infer);
    tcase_add_test(tc, PL_demo_single_detections);
    tcase_add_test(tc, PL_demo_with_tracker);
    tcase_add_test(tc, PL_demo_lifecycle);
    return s;
}

GST_CHECK_MAIN(demo_pipelines);
