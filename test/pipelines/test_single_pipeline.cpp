#include <dx_stream/gst-dxmeta.hpp>
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
    case GST_MESSAGE_EOS:
        g_print("End-of-Stream received\n");
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_ERROR:
        g_print("Error received in pipeline\n");
        g_main_loop_quit(loop);
        break;
    default:
        break;
    }

    return TRUE;
}

static GstPadProbeReturn probe_primary(GstPad *pad, GstPadProbeInfo *info,
                                       gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");
    DetectionMap pred;
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);
        if (meta_type == DX_OBJECT_META_API_TYPE) {
            DXObjectMeta *object_meta = (DXObjectMeta *)meta;
            pred[object_meta->_label].push_back(
                {object_meta->_box[0], object_meta->_box[1],
                 object_meta->_box[2], object_meta->_box[3]});
        }
    }
    fail_unless(evaluatePerformance(gt, pred) > 0.1, "Precision < 0.1.");
    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

void yolo_pipeline(std::string models) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc, "image-path", "./../../test_resources/1.jpg", NULL);
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
    std::string preprocess_config_path =
        "./../../../dx_stream/configs/Object_Detection/" + models +
        "/preprocess_config.json";
    g_object_set(preprocess, "config-file-path", preprocess_config_path.c_str(),
                 NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    std::string model_path =
        "./../../../dx_stream/samples/models/" + models + ".dxnn";
    g_object_set(infer, "model-path", model_path.c_str(), NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL, "Failed to create GstDxInfer element");
    std::string postprocess_config_path =
        "./../../../dx_stream/configs/Object_Detection/" + models +
        "/postprocess_config.json";
    g_object_set(postprocess, "config-file-path",
                 postprocess_config_path.c_str(), NULL);

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

void yolo_pose_pipeline(std::string models) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc, "image-path", "./../../test_resources/1.jpg", NULL);
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
    std::string preprocess_config_path =
        "./../../../dx_stream/configs/Pose_Estimation/" + models +
        "/preprocess_config.json";
    g_object_set(preprocess, "config-file-path", preprocess_config_path.c_str(),
                 NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    std::string model_path =
        "./../../../dx_stream/samples/models/" + models + ".dxnn";
    g_object_set(infer, "model-path", model_path.c_str(), NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL, "Failed to create GstDxInfer element");
    std::string postprocess_config_path =
        "./../../../dx_stream/configs/Pose_Estimation/" + models +
        "/postprocess_config.json";
    g_object_set(postprocess, "config-file-path",
                 postprocess_config_path.c_str(), NULL);

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
    yolo_pipeline("YOLOV3_1");
    yolo_pipeline("YOLOV4_3");
    yolo_pipeline("YOLOV5S_1");
    yolo_pipeline("YOLOV5S_3");
    yolo_pipeline("YOLOV5S_4");
    yolo_pipeline("YOLOV5S_6");
    yolo_pipeline("YOLOV5X_2");
    yolo_pipeline("YOLOv7_512");
    yolo_pipeline("YoloV7");
    yolo_pipeline("YoloV8N");
    yolo_pipeline("YOLOV9S");
    yolo_pipeline("YOLOX-S_1");
    yolo_pose_pipeline("YOLOV5Pose640_1");
}
GST_END_TEST

Suite *single_suite(void) {
    Suite *s = suite_create("Single TEST");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 60.0);
    tcase_add_test(tc_core, test_single_pipeline);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = single_suite();
    SRunner *sr = srunner_create(s);

    gt[0].push_back({384.375000, 67.250000, 498.125000, 349.749969});
    gt[0].push_back({0.625000, 262.250000, 59.375000, 307.250000});
    gt[39].push_back({128.125000, 101.625000, 139.375000, 137.875000});
    gt[41].push_back({141.250000, 267.875000, 171.250000, 301.625000});
    gt[44].push_back({532.500000, 42.875004, 552.500000, 124.125000});
    gt[44].push_back({135.000000, 249.125000, 150.000000, 272.875000});
    gt[45].push_back({57.500000, 288.500000, 135.000000, 328.499969});
    gt[45].push_back({30.625000, 342.874969, 99.375000, 384.124969});
    gt[45].push_back({156.250000, 168.500000, 181.250000, 183.500000});
    gt[45].push_back({0.000000, 302.875000, 85.625000, 361.624969});
    gt[58].push_back({0.000000, 0.375004, 60.000000, 151.625000});
    gt[69].push_back({490.625000, 203.500000, 616.875000, 343.499969});
    gt[69].push_back({242.500000, 131.625000, 327.500000, 245.375000});
    gt[69].push_back({1.250000, 187.250000, 191.250000, 294.750000});
    gt[69].push_back({333.750000, 199.750000, 396.250000, 324.749969});

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
