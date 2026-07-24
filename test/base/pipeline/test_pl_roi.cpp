// PL-A5 — ROI clipping verification
// Primary: all detections must be inside ROI bounds
// Secondary: face detections must be inside ROI bounds
// SKIP: model/NPU/postprocess not available

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"
#include "iou_eval.hpp"
#include "yolo_pipeline.hpp"
#include "pipeline_tc_helpers.hpp"

using namespace dxtest;

static const YoloModel ROI_MODEL = {
    "yolo26n", 640, "libpostprocess_yolo26od.so", "PostProcess", 114
};

static bool can_run_roi() {
    return npu_available() && !build_infer_pipeline(ROI_MODEL).empty();
}

static bool can_run_roi_secondary() {
    if (!can_run_roi()) return false;
    std::string face_model = resolve_model_path("scrfd-500m_640x640.dxnn");
    std::string pp_face = dxtest::resolve_lib_path("libpostprocess_scrfd500m.so");
    return !face_model.empty() && path_exists(pp_face);
}

GST_START_TEST(PL_A5_primary_roi_all_inside) {
    DXTEST_SKIP_IF(!can_run_roi(),
                   "yolo26n model/NPU/postprocess not available");

    std::string desc = build_infer_pipeline_with_roi(
        ROI_MODEL, "100,30,400,340");
    fail_unless(!desc.empty());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    Box roi = {100.0f, 30.0f, 400.0f, 340.0f};
    int frames_with_objects = 0;

    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm && !fm->_object_meta_list.empty()) {
            DetectionMap pred;
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_box[2] > 0 && obj->_box[3] > 0) {
                    pred[obj->_label].push_back(
                        {obj->_box[0], obj->_box[1],
                         obj->_box[2], obj->_box[3]});
                }
            }
            fail_unless(all_detections_inside_roi(pred, roi),
                        "all detections must be inside ROI(100,30,400,340)");
            frames_with_objects++;
        }
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR,
                    "unexpected bus ERROR");
        gst_message_unref(msg);
    }

    fail_unless(frames_with_objects > 0,
                "must have at least 1 frame with ROI-clipped detections");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A5_primary_roi_precision) {
    DXTEST_SKIP_IF(!can_run_roi(),
                   "yolo26n model/NPU/postprocess not available");

    std::string desc = build_infer_pipeline_with_roi(
        ROI_MODEL, "100,30,400,340");
    fail_unless(!desc.empty());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    DetectionMap all_pred;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_box[2] > 0 && obj->_box[3] > 0) {
                    all_pred[obj->_label].push_back(
                        {obj->_box[0], obj->_box[1],
                         obj->_box[2], obj->_box[3]});
                }
            }
        }
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR);
        gst_message_unref(msg);
    }

    DetectionMap gt = test_jpg_gt();
    EvalResult r = evaluate_detections(gt, all_pred, 0.5f);
    fail_unless(r.precision > 0.1f,
                "ROI pipeline precision %.3f must be > 0.1", r.precision);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A5_secondary_roi_faces_inside) {
    DXTEST_SKIP_IF(!can_run_roi_secondary(),
                   "yolo26n + SCRFD500M model/NPU/postprocess not available");

    std::string image = resolve_resource_path("images/test.jpg");
    std::string primary_model = resolve_model_path("yolo26-n_640x640.dxnn");
    std::string face_model = resolve_model_path("scrfd-500m_640x640.dxnn");
    std::string pp_primary = dxtest::resolve_lib_path("libpostprocess_yolo26od.so");
    std::string pp_face = dxtest::resolve_lib_path("libpostprocess_scrfd500m.so");

    GstElement *pipe = gst_pipeline_new(nullptr);

    GstElement *filesrc = gst_element_factory_make("filesrc", nullptr);
    GstElement *jpegdec = gst_element_factory_make("jpegdec", nullptr);
    GstElement *vconv = gst_element_factory_make("videoconvert", nullptr);
    GstElement *capsf = gst_element_factory_make("capsfilter", nullptr);
    GstElement *preprocess0 = gst_element_factory_make("dxpreprocess", nullptr);
    GstElement *infer0 = gst_element_factory_make("dxinfer", nullptr);
    GstElement *postprocess0 = gst_element_factory_make("dxpostprocess", nullptr);
    GstElement *preprocess1 = gst_element_factory_make("dxpreprocess", nullptr);
    GstElement *infer1 = gst_element_factory_make("dxinfer", nullptr);
    GstElement *postprocess1 = gst_element_factory_make("dxpostprocess", nullptr);
    GstElement *sink = gst_element_factory_make("appsink", "sink");

    g_object_set(filesrc, "location", image.c_str(), nullptr);
    GstCaps *caps = gst_caps_from_string("video/x-raw,format=RGB");
    g_object_set(capsf, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // primary stage: no ROI
    g_object_set(preprocess0, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 114, nullptr);
    g_object_set(infer0, "model-path", primary_model.c_str(),
                 "backend", 0, "inference-id", (guint)1, nullptr);
    g_object_set(postprocess0, "library-file-path", pp_primary.c_str(),
                 "function-name", "PostProcess", "inference-id", (guint)1, nullptr);

    // secondary stage: face detection with ROI
    g_object_set(preprocess1, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 114,
                 "secondary-mode", TRUE, "target-class-id", 0,
                 "roi", "200,30,400,350",
                 "preprocess-id", (guint)2, "interval", (guint)0, nullptr);
    g_object_set(infer1, "model-path", face_model.c_str(),
                 "backend", 0, "secondary-mode", TRUE,
                 "preprocess-id", (guint)2, "inference-id", (guint)2, nullptr);
    g_object_set(postprocess1, "library-file-path", pp_face.c_str(),
                 "function-name", "PostProcess", "secondary-mode", TRUE,
                 "inference-id", (guint)2, nullptr);

    g_object_set(sink, "sync", FALSE, "drop", TRUE, nullptr);

    gst_bin_add_many(GST_BIN(pipe), filesrc, jpegdec, vconv, capsf,
                     preprocess0, infer0, postprocess0,
                     preprocess1, infer1, postprocess1, sink, nullptr);
    fail_unless(gst_element_link_many(filesrc, jpegdec, vconv, capsf,
                                       preprocess0, infer0, postprocess0,
                                       preprocess1, infer1, postprocess1,
                                       sink, nullptr));

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    Box roi = {200.0f, 30.0f, 400.0f, 350.0f};
    int face_count = 0;

    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              15 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_face_box[2] > 0 && obj->_face_box[3] > 0) {
                    Box face = {obj->_face_box[0], obj->_face_box[1],
                                obj->_face_box[2], obj->_face_box[3]};
                    fail_unless(is_box_inside_roi(face, roi),
                                "face box (%.1f,%.1f,%.1f,%.1f) outside ROI",
                                face[0], face[1], face[2], face[3]);
                    face_count++;
                }
            }
        }
        gst_sample_unref(s);
    }

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 15 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR,
                    "unexpected bus ERROR in secondary ROI pipeline");
        gst_message_unref(msg);
    }

    fail_unless(face_count >= 1,
                "must detect at least 1 face inside ROI (got %d)", face_count);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_roi_suite(void) {
    Suite *s = suite_create("pl_roi");
    TCase *tc = tcase_create("roi_clipping");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_A5_primary_roi_all_inside);
    tcase_add_test(tc, PL_A5_primary_roi_precision);
    tcase_add_test(tc, PL_A5_secondary_roi_faces_inside);
    return s;
}

GST_CHECK_MAIN(pl_roi);

