#include <gstdxstream/gst-dxframemeta.hpp>
#include <gstdxstream/gst-dxobjectmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>

// #include <algorithm>
// #include <iostream>
// #include <map>
// #include <vector>

using namespace std;

typedef map<int, vector<vector<float>>> DetectionMap;

DetectionMap gt;

float computeIoU(const vector<float> &box1, const vector<float> &box2) {
    float x1 = max(box1[0], box2[0]);
    float y1 = max(box1[1], box2[1]);
    float x2 = min(box1[2], box2[2]);
    float y2 = min(box1[3], box2[3]);

    float intersection = max(0.0f, x2 - x1) * max(0.0f, y2 - y1);
    float area1 = (box1[2] - box1[0]) * (box1[3] - box1[1]);
    float area2 = (box2[2] - box2[0]) * (box2[3] - box2[1]);
    float unionArea = area1 + area2 - intersection;

    return unionArea > 0 ? (intersection / unionArea) : 0.0f;
}

float evaluatePerformance(const DetectionMap &gt, const DetectionMap &pred,
                          float iouThreshold = 0.5) {
    int tp = 0, fp = 0, fn = 0;

    for (map<int, vector<vector<float>>>::const_iterator it = gt.begin();
         it != gt.end(); ++it) {
        int class_id = it->first;
        const vector<vector<float>> &gt_boxes = it->second;

        if (pred.find(class_id) == pred.end()) {
            fn += gt_boxes.size();
            continue;
        }

        vector<vector<float>> pred_boxes = pred.at(class_id);
        vector<bool> matched(pred_boxes.size(), false);

        for (size_t j = 0; j < gt_boxes.size(); ++j) {
            bool foundMatch = false;
            for (size_t i = 0; i < pred_boxes.size(); ++i) {
                if (!matched[i] &&
                    computeIoU(gt_boxes[j], pred_boxes[i]) >= iouThreshold) {
                    matched[i] = true;
                    foundMatch = true;
                    break;
                }
            }
            if (foundMatch) {
                tp++;
            } else {
                fn++;
            }
        }

        for (size_t i = 0; i < pred_boxes.size(); ++i) {
            if (!matched[i]) {
                fp++;
            }
        }
    }

    float precision = tp / static_cast<float>(tp + fp);
    float recall = tp / static_cast<float>(tp + fn);
    float f1 = (2 * precision * recall) / (precision + recall);

    return precision;
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS: {
        g_print("End-of-Stream received\n");
        g_main_loop_quit(loop);
    } break;
    case GST_MESSAGE_ERROR: {
        g_print("Error received in pipeline\n");
        g_main_loop_quit(loop);
    } break;
    default:
        break;
    }

    return TRUE;
}

static GstPadProbeReturn probe_primary(GstPad *pad, GstPadProbeInfo *info,
                                       gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");
    DetectionMap pred;
    
    DXFrameMeta *frame_meta = dx_get_frame_meta(buffer);
    if (frame_meta) {
        for (auto object_meta : frame_meta->_object_meta_list) {
            pred[object_meta->_label].push_back(
                {object_meta->_box[0], object_meta->_box[1],
                 object_meta->_box[2], object_meta->_box[3]});
        }
    }
    fail_unless(evaluatePerformance(gt, pred) > 0.1, "Precision < 0.1.");
    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

void yolo_pipeline(std::string models, int input_size, std::string postprocess_lib, std::string function_name) {
    g_print("Running for model: %s\n", models.c_str());
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc, "image-path", "./../../test_resources/test.jpg", NULL);
    GValue framerate = G_VALUE_INIT;
    g_value_init(&framerate, GST_TYPE_FRACTION);
    gst_value_set_fraction(&framerate, 30, 1);
    g_object_set_property(G_OBJECT(videosrc), "framerate", &framerate);
    g_value_unset(&framerate);
    g_object_set(videosrc, "num-buffers", 10, NULL);

    GstElement *jpegparse = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "preprocess-id", 1, NULL);
    g_object_set(preprocess, "resize-width", input_size, NULL);
    g_object_set(preprocess, "resize-height", input_size, NULL);
    g_object_set(preprocess, "keep-ratio", true, NULL);
    g_object_set(preprocess, "pad-value", 114, NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    std::string model_path =
        "./../../../dx_stream/samples/models/" + models + ".dxnn";
    g_object_set(infer, "model-path", model_path.c_str(), NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL, "Failed to create GstDxInfer element");
    g_object_set(postprocess, "inference-id", 1, NULL);
    g_object_set(postprocess, "library-file-path", postprocess_lib.c_str(), NULL);
    g_object_set(postprocess, "function-name", function_name.c_str(), NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    // ADD Elements
    gst_bin_add_many(GST_BIN(pipeline), videosrc, jpegparse, jpegdec,
                     preprocess, infer, postprocess, fakesink, NULL);

    // Link Elements
    fail_unless(gst_element_link_many(videosrc, jpegparse, jpegdec, preprocess,
                                      infer, postprocess, fakesink, NULL),
                "Failed to link");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_primary, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret =
        gst_element_get_state(pipeline, NULL, NULL, GST_SECOND);
    fail_unless(ret == GST_STATE_CHANGE_SUCCESS,
                "Pipeline state change to PLAYING timed out");

    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}

void yolo_pose_pipeline(std::string models, int input_size, std::string postprocess_lib, std::string function_name) {
    g_print("Running for model: %s\n", models.c_str());
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc, "image-path", "./../../test_resources/test.jpg", NULL);
    GValue framerate = G_VALUE_INIT;
    g_value_init(&framerate, GST_TYPE_FRACTION);
    gst_value_set_fraction(&framerate, 30, 1);
    g_object_set_property(G_OBJECT(videosrc), "framerate", &framerate);
    g_value_unset(&framerate);
    g_object_set(videosrc, "num-buffers", 3, NULL);

    GstElement *jpegparse = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "preprocess-id", 1, NULL);
    g_object_set(preprocess, "resize-width", input_size, NULL);
    g_object_set(preprocess, "resize-height", input_size, NULL);
    g_object_set(preprocess, "keep-ratio", true, NULL);
    g_object_set(preprocess, "pad-value", 0, NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    std::string model_path =
        "./../../../dx_stream/samples/models/" + models + ".dxnn";
    g_object_set(infer, "model-path", model_path.c_str(), NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL, "Failed to create GstDxInfer element");
    g_object_set(postprocess, "inference-id", 1, NULL);
    g_object_set(postprocess, "library-file-path", postprocess_lib.c_str(), NULL);
    g_object_set(postprocess, "function-name", function_name.c_str(), NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    // ADD Elements
    gst_bin_add_many(GST_BIN(pipeline), videosrc, jpegparse, jpegdec,
                     preprocess, infer, postprocess, fakesink, NULL);

    // Link Elements
    fail_unless(gst_element_link_many(videosrc, jpegparse, jpegdec, preprocess,
                                      infer, postprocess, fakesink, NULL),
                "Failed to link");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_primary, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret =
        gst_element_get_state(pipeline, NULL, NULL, GST_SECOND);
    fail_unless(ret == GST_STATE_CHANGE_SUCCESS,
                "Pipeline state change to PLAYING timed out");

    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}

GST_START_TEST(test_single_pipeline) {
    yolo_pipeline("YoloV5S_PPU", 640, "libpostprocess_ppu.so", "YOLOV5S_PPU");
    yolo_pipeline("yolo26n", 640, "libpostprocess_yolo26od.so", "PostProcess");

    yolo_pose_pipeline("YoloV5S", 640, "libpostprocess_yolov5s_6.so", "PostProcess");
    yolo_pose_pipeline("YoloV7", 640, "libpostprocess_yolov7.so", "PostProcess");
    yolo_pose_pipeline("YoloV8N", 640, "libpostprocess_yolov8n.so", "PostProcess");
    yolo_pose_pipeline("YoloV9S", 640, "libpostprocess_yolov9s.so", "PostProcess");
    yolo_pose_pipeline("YOLOV11N", 640, "libpostprocess_yolov11.so", "PostProcess");

    yolo_pose_pipeline("yolo26n-pose", 640, "libpostprocess_yolo26pose.so", "PostProcess");
    yolo_pose_pipeline("yolov8m_pose", 640, "libpostprocess_yolov8m_pose.so", "PostProcess");
}
GST_END_TEST

Suite *single_suite(void) {
    Suite *s = suite_create("Single TEST");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 120.0);
    tcase_add_test(tc_core, test_single_pipeline);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = single_suite();
    SRunner *sr = srunner_create(s);

    gt[0].push_back({124.547783, 62.005188, 246.898804, 329.931824});
    gt[0].push_back({269.850861, 73.818756, 367.612885, 332.555573});
    gt[0].push_back({395.051575, 100.663300, 484.869934, 332.248627});
    gt[1].push_back({392.320831, 81.118011, 623.422607, 279.040558});
    gt[13].push_back({79.094894, 151.225708, 527.124817, 324.877441});
    gt[16].push_back({408.001617, 165.888428, 451.949982, 227.317139});
    gt[24].push_back({94.010223, 89.505127, 186.151764, 230.614990});
    gt[39].push_back({255.098633, 199.996796, 267.930298, 243.062042});
    gt[67].push_back({313.397675, 141.667389, 329.099213, 150.474091});

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
