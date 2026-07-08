// PL-A1 — Multi-model inference quality verification
// Runs each of the 9 YOLO models against test.jpg GT with IoU precision > 0.1
// SKIP: model file, NPU, or postprocess library not available

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"
#include "iou_eval.hpp"
#include "yolo_pipeline.hpp"

using namespace dxtest;

static DetectionMap collect_detections(const char *pipeline_desc) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(pipeline_desc, &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    DetectionMap all;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_box[2] > 0 && obj->_box[3] > 0) {
                    all[obj->_label].push_back(
                        {obj->_box[0], obj->_box[1], obj->_box[2], obj->_box[3]});
                }
            }
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

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
    return all;
}

static bool can_run_model(const YoloModel &m) {
    return npu_available() && !build_infer_pipeline(m).empty();
}

static void run_model_precision_test(const YoloModel &m) {
    std::string desc = build_infer_pipeline(m);
    fail_unless(!desc.empty(), "pipeline build failed for %s", m.model_name);

    DetectionMap pred = collect_detections(desc.c_str());
    DetectionMap gt = test_jpg_gt();
    EvalResult r = evaluate_detections(gt, pred, 0.5f);

    fail_unless(r.precision > 0.1f,
                "model %s: precision %.3f must be > 0.1 (TP=%d FP=%d FN=%d)",
                m.model_name, r.precision, r.tp, r.fp, r.fn);
}

// --- Per-model TCs ---

GST_START_TEST(PL_A1_YoloV5S_PPU_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[0]),
                   "YoloV5S_PPU model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[0]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_yolo26n_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[1]),
                   "yolo26n model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[1]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_YoloV5S_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[2]),
                   "YoloV5S model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[2]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_YoloV7_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[3]),
                   "YoloV7 model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[3]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_YoloV8N_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[4]),
                   "YoloV8N model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[4]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_YoloV9S_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[5]),
                   "YoloV9S model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[5]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_YOLOV11N_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[6]),
                   "YOLOV11N model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[6]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_yolo26n_pose_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[7]),
                   "yolo26n-pose model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[7]);
}
GST_END_TEST;

GST_START_TEST(PL_A1_yolov8m_pose_precision) {
    DXTEST_SKIP_IF(!can_run_model(YOLO_MODELS[8]),
                   "yolov8m_pose model/NPU/postprocess not available");
    run_model_precision_test(YOLO_MODELS[8]);
}
GST_END_TEST;

static Suite *pl_infer_models_suite(void) {
    Suite *s = suite_create("pl_infer_models");
    TCase *tc = tcase_create("multi_model_precision");
    tcase_set_timeout(tc, 120.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_A1_YoloV5S_PPU_precision);
    tcase_add_test(tc, PL_A1_yolo26n_precision);
    tcase_add_test(tc, PL_A1_YoloV5S_precision);
    tcase_add_test(tc, PL_A1_YoloV7_precision);
    tcase_add_test(tc, PL_A1_YoloV8N_precision);
    tcase_add_test(tc, PL_A1_YoloV9S_precision);
    tcase_add_test(tc, PL_A1_YOLOV11N_precision);
    tcase_add_test(tc, PL_A1_yolo26n_pose_precision);
    tcase_add_test(tc, PL_A1_yolov8m_pose_precision);
    return s;
}

GST_CHECK_MAIN(pl_infer_models);
