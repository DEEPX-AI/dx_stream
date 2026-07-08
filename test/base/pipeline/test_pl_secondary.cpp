// PL-A2 — Secondary inference flow verification
// primary detection -> tee -> (classify + face detection) -> gather
// Validates: IoU > 0.5 for both detection box and face box
// SKIP: model files, NPU, or postprocess libraries not available

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"
#include "iou_eval.hpp"
#include "yolo_pipeline.hpp"
#include "pipeline_tc_helpers.hpp"

using namespace dxtest;

static bool can_run_secondary() {
    if (!npu_available()) return false;

    std::string primary_model = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    std::string classify_model = resolve_model_path("efficientnet-lite0_256x256.dxnn");
    std::string face_model = resolve_model_path("scrfd-500m_640x640.dxnn");
    if (primary_model.empty() || classify_model.empty() || face_model.empty())
        return false;

    std::string pp1 = dxtest::resolve_lib_path("libpostprocess_ppu.so");
    std::string pp2 = dxtest::resolve_lib_path("libpostprocess_object_class.so");
    std::string pp3 = dxtest::resolve_lib_path("libpostprocess_scrfd500m.so");
    return path_exists(pp1) && path_exists(pp2) &&
           path_exists(pp3);
}

static void link_src_to_gather(GstElement *element, GstElement *gather) {
    GstPad *src = gst_element_get_static_pad(element, "src");
    fail_unless(src != nullptr);
    GstPad *sink = gst_element_get_request_pad(gather, "sink_%u");
    fail_unless(sink != nullptr);
    fail_unless(gst_pad_link(src, sink) == GST_PAD_LINK_OK);
    gst_object_unref(src);
    gst_object_unref(sink);
}

GST_START_TEST(PL_A2_secondary_iou_validation) {
    DXTEST_SKIP_IF(!can_run_secondary(),
                   "secondary models/NPU/postprocess not available");

    std::string image = resolve_resource_path("images/test.jpg");
    std::string primary_model = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    std::string cls_model = resolve_model_path("efficientnet-lite0_256x256.dxnn");
    std::string face_model = resolve_model_path("scrfd-500m_640x640.dxnn");
    std::string pp_primary = dxtest::resolve_lib_path("libpostprocess_ppu.so");
    std::string pp_cls = dxtest::resolve_lib_path("libpostprocess_object_class.so");
    std::string pp_face = dxtest::resolve_lib_path("libpostprocess_scrfd500m.so");

    GstElement *pipe = gst_pipeline_new(nullptr);

    // source -> decode -> preprocess -> infer -> postprocess -> tee
    GstElement *filesrc = gst_element_factory_make("filesrc", "filesrc");
    GstElement *jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    GstElement *vconv = gst_element_factory_make("videoconvert", "vconv");
    GstElement *capsf = gst_element_factory_make("capsfilter", "capsf");
    GstElement *preprocess0 = gst_element_factory_make("dxpreprocess", "preprocess0");
    GstElement *infer0 = gst_element_factory_make("dxinfer", "infer0");
    GstElement *postprocess0 = gst_element_factory_make("dxpostprocess", "postprocess0");
    GstElement *tee = gst_element_factory_make("tee", "tee");

    // classification branch
    GstElement *q1 = gst_element_factory_make("queue", "q1");
    GstElement *preprocess_cls = gst_element_factory_make("dxpreprocess", "preprocess_cls");
    GstElement *infer_cls = gst_element_factory_make("dxinfer", "infer_cls");
    GstElement *postprocess_cls = gst_element_factory_make("dxpostprocess", "postprocess_cls");

    // face detection branch
    GstElement *q2 = gst_element_factory_make("queue", "q2");
    GstElement *preprocess_face = gst_element_factory_make("dxpreprocess", "preprocess_face");
    GstElement *infer_face = gst_element_factory_make("dxinfer", "infer_face");
    GstElement *postprocess_face = gst_element_factory_make("dxpostprocess", "postprocess_face");

    // gather -> sink
    GstElement *gather = gst_element_factory_make("dxgather", "gather");
    GstElement *sink = gst_element_factory_make("appsink", "sink");

    // configure source
    g_object_set(filesrc, "location", image.c_str(), nullptr);
    GstCaps *caps = gst_caps_from_string("video/x-raw,format=RGB");
    g_object_set(capsf, "caps", caps, nullptr);
    gst_caps_unref(caps);

    // primary stage
    g_object_set(preprocess0, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 0, nullptr);
    g_object_set(infer0, "model-path", primary_model.c_str(),
                 "backend", 0, "inference-id", (guint)1, nullptr);
    g_object_set(postprocess0, "library-file-path", pp_primary.c_str(),
                 "function-name", "YOLOV5S_PPU", "inference-id", (guint)1, nullptr);

    // classification branch
    g_object_set(preprocess_cls, "resize-width", 224, "resize-height", 224,
                 "keep-ratio", FALSE, "secondary-mode", TRUE,
                 "preprocess-id", (guint)2, "interval", (guint)0, nullptr);
    g_object_set(infer_cls, "model-path", cls_model.c_str(),
                 "backend", 0, "secondary-mode", TRUE,
                 "preprocess-id", (guint)2, "inference-id", (guint)2, nullptr);
    g_object_set(postprocess_cls, "library-file-path", pp_cls.c_str(),
                 "function-name", "PostProcess", "secondary-mode", TRUE,
                 "inference-id", (guint)2, nullptr);

    // face detection branch
    g_object_set(preprocess_face, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 114,
                 "secondary-mode", TRUE, "target-class-id", 0,
                 "preprocess-id", (guint)3, "interval", (guint)0, nullptr);
    g_object_set(infer_face, "model-path", face_model.c_str(),
                 "backend", 0, "secondary-mode", TRUE,
                 "preprocess-id", (guint)3, "inference-id", (guint)3, nullptr);
    g_object_set(postprocess_face, "library-file-path", pp_face.c_str(),
                 "function-name", "PostProcess", "secondary-mode", TRUE,
                 "inference-id", (guint)3, nullptr);

    g_object_set(sink, "sync", FALSE, "drop", TRUE, nullptr);

    // add all to pipeline
    gst_bin_add_many(GST_BIN(pipe), filesrc, jpegdec, vconv, capsf,
                     preprocess0, infer0, postprocess0, tee,
                     q1, preprocess_cls, infer_cls, postprocess_cls,
                     q2, preprocess_face, infer_face, postprocess_face,
                     gather, sink, nullptr);

    // link primary chain
    fail_unless(gst_element_link_many(filesrc, jpegdec, vconv, capsf,
                                       preprocess0, infer0, postprocess0, tee, nullptr));

    // link tee -> q1 -> classification chain
    GstPad *tee_src1 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *q1_sink = gst_element_get_static_pad(q1, "sink");
    gst_pad_link(tee_src1, q1_sink);
    gst_object_unref(tee_src1);
    gst_object_unref(q1_sink);
    fail_unless(gst_element_link_many(q1, preprocess_cls, infer_cls,
                                       postprocess_cls, nullptr));

    // link tee -> q2 -> face chain
    GstPad *tee_src2 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *q2_sink = gst_element_get_static_pad(q2, "sink");
    gst_pad_link(tee_src2, q2_sink);
    gst_object_unref(tee_src2);
    gst_object_unref(q2_sink);
    fail_unless(gst_element_link_many(q2, preprocess_face, infer_face,
                                       postprocess_face, nullptr));

    // link both branches to gather
    link_src_to_gather(postprocess_cls, gather);
    link_src_to_gather(postprocess_face, gather);

    // gather -> sink
    fail_unless(gst_element_link(gather, sink));

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    float max_conf = 0.0f;
    Box best_box, best_face;

    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              15 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm) {
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_confidence > max_conf &&
                    obj->_box[2] > 0 && obj->_box[3] > 0 &&
                    obj->_face_box[2] > 0 && obj->_face_box[3] > 0) {
                    max_conf = (float)obj->_confidence;
                    best_box = {obj->_box[0], obj->_box[1],
                                obj->_box[2], obj->_box[3]};
                    best_face = {obj->_face_box[0], obj->_face_box[1],
                                 obj->_face_box[2], obj->_face_box[3]};
                }
            }
        }
        gst_sample_unref(s);
    }

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 15 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR,
                    "unexpected bus ERROR in secondary pipeline");
        gst_message_unref(msg);
    }
    gst_object_unref(bus);

    fail_unless(max_conf > 0.0f, "must detect at least one object with face");

    Box gt_box = test_jpg_face_gt_box();
    Box gt_face = test_jpg_face_gt_face();

    float box_iou = compute_iou(best_box, gt_box);
    float face_iou = compute_iou(best_face, gt_face);

    fail_unless(box_iou > 0.5f,
                "detection box IoU must be > 0.5 (got %.3f)", box_iou);
    fail_unless(face_iou > 0.5f,
                "face box IoU must be > 0.5 (got %.3f)", face_iou);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A2_secondary_eos) {
    DXTEST_SKIP_IF(!can_run_secondary(),
                   "secondary models/NPU/postprocess not available");

    std::string image = resolve_resource_path("images/test.jpg");
    std::string primary_model = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    std::string cls_model = resolve_model_path("efficientnet-lite0_256x256.dxnn");
    std::string face_model = resolve_model_path("scrfd-500m_640x640.dxnn");
    std::string pp_primary = dxtest::resolve_lib_path("libpostprocess_ppu.so");
    std::string pp_cls = dxtest::resolve_lib_path("libpostprocess_object_class.so");
    std::string pp_face = dxtest::resolve_lib_path("libpostprocess_scrfd500m.so");

    // same pipeline with fakesink for EOS test
    GstElement *pipe = gst_pipeline_new(nullptr);

    GstElement *filesrc = gst_element_factory_make("filesrc", "filesrc");
    GstElement *jpegdec = gst_element_factory_make("jpegdec", nullptr);
    GstElement *vconv = gst_element_factory_make("videoconvert", nullptr);
    GstElement *capsf = gst_element_factory_make("capsfilter", nullptr);
    GstElement *preprocess0 = gst_element_factory_make("dxpreprocess", nullptr);
    GstElement *infer0 = gst_element_factory_make("dxinfer", nullptr);
    GstElement *postprocess0 = gst_element_factory_make("dxpostprocess", nullptr);
    GstElement *tee = gst_element_factory_make("tee", nullptr);
    GstElement *q1 = gst_element_factory_make("queue", nullptr);
    GstElement *pp_cls_e = gst_element_factory_make("dxpreprocess", nullptr);
    GstElement *inf_cls = gst_element_factory_make("dxinfer", nullptr);
    GstElement *post_cls = gst_element_factory_make("dxpostprocess", nullptr);
    GstElement *q2 = gst_element_factory_make("queue", nullptr);
    GstElement *pp_face_e = gst_element_factory_make("dxpreprocess", nullptr);
    GstElement *inf_face = gst_element_factory_make("dxinfer", nullptr);
    GstElement *post_face = gst_element_factory_make("dxpostprocess", nullptr);
    GstElement *gather = gst_element_factory_make("dxgather", nullptr);
    GstElement *fsink = gst_element_factory_make("fakesink", "fakesink");

    g_object_set(filesrc, "location", image.c_str(), nullptr);
    GstCaps *caps = gst_caps_from_string("video/x-raw,format=RGB");
    g_object_set(capsf, "caps", caps, nullptr);
    gst_caps_unref(caps);

    g_object_set(preprocess0, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 0, nullptr);
    g_object_set(infer0, "model-path", primary_model.c_str(),
                 "backend", 0, "inference-id", (guint)1, nullptr);
    g_object_set(postprocess0, "library-file-path", pp_primary.c_str(),
                 "function-name", "YOLOV5S_PPU", "inference-id", (guint)1, nullptr);

    g_object_set(pp_cls_e, "resize-width", 224, "resize-height", 224,
                 "keep-ratio", FALSE, "secondary-mode", TRUE,
                 "preprocess-id", (guint)2, "interval", (guint)0, nullptr);
    g_object_set(inf_cls, "model-path", cls_model.c_str(),
                 "backend", 0, "secondary-mode", TRUE,
                 "preprocess-id", (guint)2, "inference-id", (guint)2, nullptr);
    g_object_set(post_cls, "library-file-path", pp_cls.c_str(),
                 "function-name", "PostProcess", "secondary-mode", TRUE,
                 "inference-id", (guint)2, nullptr);

    g_object_set(pp_face_e, "resize-width", 640, "resize-height", 640,
                 "keep-ratio", TRUE, "pad-value", 114,
                 "secondary-mode", TRUE, "target-class-id", 0,
                 "preprocess-id", (guint)3, "interval", (guint)0, nullptr);
    g_object_set(inf_face, "model-path", face_model.c_str(),
                 "backend", 0, "secondary-mode", TRUE,
                 "preprocess-id", (guint)3, "inference-id", (guint)3, nullptr);
    g_object_set(post_face, "library-file-path", pp_face.c_str(),
                 "function-name", "PostProcess", "secondary-mode", TRUE,
                 "inference-id", (guint)3, nullptr);

    g_object_set(fsink, "sync", FALSE, nullptr);

    gst_bin_add_many(GST_BIN(pipe), filesrc, jpegdec, vconv, capsf,
                     preprocess0, infer0, postprocess0, tee,
                     q1, pp_cls_e, inf_cls, post_cls,
                     q2, pp_face_e, inf_face, post_face,
                     gather, fsink, nullptr);

    gst_element_link_many(filesrc, jpegdec, vconv, capsf,
                          preprocess0, infer0, postprocess0, tee, nullptr);

    GstPad *ts1 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *qs1 = gst_element_get_static_pad(q1, "sink");
    gst_pad_link(ts1, qs1);
    gst_object_unref(ts1);
    gst_object_unref(qs1);
    gst_element_link_many(q1, pp_cls_e, inf_cls, post_cls, nullptr);

    GstPad *ts2 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *qs2 = gst_element_get_static_pad(q2, "sink");
    gst_pad_link(ts2, qs2);
    gst_object_unref(ts2);
    gst_object_unref(qs2);
    gst_element_link_many(q2, pp_face_e, inf_face, post_face, nullptr);

    link_src_to_gather(post_cls, gather);
    link_src_to_gather(post_face, gather);
    gst_element_link(gather, fsink);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 30 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout waiting for EOS in secondary pipeline");
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "expected EOS, got ERROR");
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_secondary_suite(void) {
    Suite *s = suite_create("pl_secondary");
    TCase *tc = tcase_create("secondary_inference");
    tcase_set_timeout(tc, 120.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_A2_secondary_iou_validation);
    tcase_add_test(tc, PL_A2_secondary_eos);
    return s;
}

GST_CHECK_MAIN(pl_secondary);

