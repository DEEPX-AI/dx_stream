// PL-A — Inference full path verification (real model + result validation)
// filesrc(test.jpg) → jpegdec → videoconvert → videoscale → capsfilter
// → dxpreprocess → dxinfer → dxpostprocess → appsink
// SKIP: model file or NPU not available

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "pipeline_tc_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <string>

using namespace dxtest;

static std::string PP_LIB_PATH() {
    return dxtest::resolve_lib_path("libpostprocess_ppu.so");
}

static std::string infer_pipe_desc() {
    std::string model = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    std::string image = resolve_resource_path("images/test.jpg");
    if (model.empty() || image.empty()) return "";
    if (!path_exists(PP_LIB_PATH())) return "";

    return "filesrc location=" + image +
           " ! jpegdec ! videoconvert ! videoscale"
           " ! video/x-raw,format=RGB,width=640,height=640"
           " ! dxpreprocess resize-width=640 resize-height=640"
           " ! dxinfer model-path=" + model + " backend=dxrt"
           " ! dxpostprocess library-file-path=" + std::string(PP_LIB_PATH().c_str()) +
           " function-name=YOLOV5S_PPU"
           " ! appsink name=sink sync=false drop=true";
}

static bool can_run_infer() {
    return !infer_pipe_desc().empty() && npu_available();
}

GST_START_TEST(PL_A_infer_produces_detections) {
    DXTEST_SKIP_IF(!can_run_infer(), "model/NPU/postprocess not available");

    std::string desc = infer_pipe_desc();
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    int total_objects = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            total_objects += (int)fm->_object_meta_list.size();
        }
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR,
                    "unexpected bus ERROR during inference");
        gst_message_unref(msg);
    }

    fail_unless(total_objects > 0,
                "inference must produce detections (got %d objects)", total_objects);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A_infer_detection_fields) {
    DXTEST_SKIP_IF(!can_run_infer(), "model/NPU/postprocess not available");

    std::string desc = infer_pipe_desc();
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    bool checked = false;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm && !fm->_object_meta_list.empty()) {
            for (auto *obj : fm->_object_meta_list) {
                fail_unless(obj->_confidence > 0.0f && obj->_confidence <= 1.0f,
                            "confidence must be in (0,1], got %f", obj->_confidence);
                fail_unless(obj->_box[0] >= 0 && obj->_box[0] <= 640,
                            "box x out of range: %f", obj->_box[0]);
                fail_unless(obj->_box[1] >= 0 && obj->_box[1] <= 640,
                            "box y out of range: %f", obj->_box[1]);
                fail_unless(obj->_box[2] > 0, "box w must be positive: %f", obj->_box[2]);
                fail_unless(obj->_box[3] > 0, "box h must be positive: %f", obj->_box[3]);
                checked = true;
            }
        }
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless(checked, "must have at least one detection to validate fields");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static std::string infer_pipe_noapp() {
    std::string model = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    std::string image = resolve_resource_path("images/test.jpg");
    if (model.empty() || image.empty()) return "";
    if (!path_exists(PP_LIB_PATH().c_str())) return "";

    return "filesrc location=" + image +
           " ! jpegdec ! videoconvert ! videoscale"
           " ! video/x-raw,format=RGB,width=640,height=640"
           " ! dxpreprocess resize-width=640 resize-height=640"
           " ! dxinfer model-path=" + model + " backend=dxrt"
           " ! dxpostprocess library-file-path=" + std::string(PP_LIB_PATH().c_str()) +
           " function-name=YOLOV5S_PPU"
           " ! fakesink sync=false";
}

GST_START_TEST(PL_A_infer_eos) {
    DXTEST_SKIP_IF(!can_run_infer(), "model/NPU/postprocess not available");
    test_eos_propagation(infer_pipe_noapp().c_str(), 30 * GST_SECOND);
}
GST_END_TEST;

GST_START_TEST(PL_A_infer_lifecycle) {
    DXTEST_SKIP_IF(!can_run_infer(), "model/NPU/postprocess not available");
    test_lifecycle_cycle(infer_pipe_noapp().c_str(), 3);
}
GST_END_TEST;

GST_START_TEST(PL_A_infer_no_bus_error) {
    DXTEST_SKIP_IF(!can_run_infer(), "model/NPU/postprocess not available");

    std::string desc = infer_pipe_noapp();
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 30 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout waiting for EOS");

    if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *gerr = nullptr;
        gst_message_parse_error(msg, &gerr, nullptr);
        fail("unexpected bus ERROR: %s", gerr ? gerr->message : "unknown");
        g_error_free(gerr);
    }
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_infer_suite(void) {
    Suite *s = suite_create("pl_infer");
    TCase *tc = tcase_create("inference");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_A_infer_produces_detections);
    tcase_add_test(tc, PL_A_infer_detection_fields);
    tcase_add_test(tc, PL_A_infer_eos);
    tcase_add_test(tc, PL_A_infer_lifecycle);
    tcase_add_test(tc, PL_A_infer_no_bus_error);
    return s;
}

GST_CHECK_MAIN(pl_infer);
