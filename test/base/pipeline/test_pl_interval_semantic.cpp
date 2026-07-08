// PL-A4 — Interval semantic verification
// With interval=3: first 3 frames skipped, frame 3 inferred, frames 4-6 skipped, frame 7 inferred, etc.
// check_primary_interval skips cnt < interval frames, then processes on cnt == interval.
// Verifies the MEANING (inferred vs skipped), not just total count.
// Uses imagefreeze to generate multiple frames from a single test image.
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

static const YoloModel INTERVAL_MODEL = {
    "yolo26n", 640, "libpostprocess_yolo26od.so", "PostProcess", 114
};

static bool can_run_interval() {
    return npu_available() &&
           !build_multiframe_infer_pipeline(INTERVAL_MODEL, 12, 3).empty();
}

GST_START_TEST(PL_A4_interval_infer_vs_skip) {
    DXTEST_SKIP_IF(!can_run_interval(),
                   "yolo26n model/NPU/postprocess not available");

    std::string desc =
        build_multiframe_infer_pipeline(INTERVAL_MODEL, 12, 3);
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

    int frame_idx = 0;
    int inferred_frames = 0;
    int skipped_frames = 0;

    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        int objects = fm ? (int)fm->_object_meta_list.size() : 0;

        if (frame_idx % 4 == 3) {
            if (objects > 0) {
                DetectionMap pred;
                for (auto *obj : fm->_object_meta_list) {
                    if (obj->_box[2] > 0 && obj->_box[3] > 0) {
                        pred[obj->_label].push_back(
                            {obj->_box[0], obj->_box[1],
                             obj->_box[2], obj->_box[3]});
                    }
                }
                DetectionMap gt = test_jpg_gt();
                EvalResult r = evaluate_detections(gt, pred, 0.5f);
                fail_unless(r.precision > 0.1f,
                            "frame %d: precision %.3f must be > 0.1",
                            frame_idx, r.precision);
            }
            inferred_frames++;
        } else {
            fail_unless(objects == 0,
                        "frame %d: skip frame must have 0 objects (got %d)",
                        frame_idx, objects);
            skipped_frames++;
        }

        frame_idx++;
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

    fail_unless(frame_idx > 0, "must produce frames");
    fail_unless(inferred_frames > 0, "must have at least 1 inferred frame");
    fail_unless(skipped_frames > 0, "must have at least 1 skipped frame");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A4_interval_skip_ratio) {
    DXTEST_SKIP_IF(!can_run_interval(),
                   "yolo26n model/NPU/postprocess not available");

    std::string desc =
        build_multiframe_infer_pipeline(INTERVAL_MODEL, 12, 3);
    fail_unless(!desc.empty());

    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    int with_objects = 0;
    int without_objects = 0;

    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              10 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        int objects = fm ? (int)fm->_object_meta_list.size() : 0;
        if (objects > 0) with_objects++;
        else without_objects++;
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) {
        fail_unless(GST_MESSAGE_TYPE(msg) != GST_MESSAGE_ERROR);
        gst_message_unref(msg);
    }

    int total = with_objects + without_objects;
    fail_unless(total >= 4, "need at least 4 frames (got %d)", total);

    float ratio = (float)with_objects / (float)total;
    fail_unless(ratio < 0.5f,
                "inferred ratio %.2f must be < 0.5 (got %d/%d)",
                ratio, with_objects, total);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_A4_interval_eos) {
    DXTEST_SKIP_IF(!can_run_interval(),
                   "yolo26n model/NPU/postprocess not available");
    std::string desc =
        build_multiframe_infer_pipeline(INTERVAL_MODEL, 12, 3,
            "fakesink sync=false");
    fail_unless(!desc.empty());
    test_eos_propagation(desc.c_str(), 30 * GST_SECOND);
}
GST_END_TEST;

static Suite *pl_interval_semantic_suite(void) {
    Suite *s = suite_create("pl_interval_semantic");
    TCase *tc = tcase_create("interval_semantics");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_A4_interval_infer_vs_skip);
    tcase_add_test(tc, PL_A4_interval_skip_ratio);
    tcase_add_test(tc, PL_A4_interval_eos);
    return s;
}

GST_CHECK_MAIN(pl_interval_semantic);
