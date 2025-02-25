#include <dx_stream/gst-dxmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

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

bool isBoxInsideROI(const vector<float> &box, const vector<float> &roi) {
    float x1 = box[0], y1 = box[1], x2 = box[2], y2 = box[3];
    float rx1 = roi[0], ry1 = roi[1], rx2 = roi[2], ry2 = roi[3];

    return (x1 >= rx1 && y1 >= ry1 && x2 <= rx2 && y2 <= ry2);
}

bool filterDetectionsByROI(const DetectionMap &detections,
                           const vector<float> &roi) {
    for (auto it = detections.begin(); it != detections.end(); ++it) {
        int id = it->first;
        const vector<vector<float>> &boxes = it->second;

        vector<vector<float>> filteredBoxes;
        for (size_t i = 0; i < boxes.size(); ++i) {
            if (boxes[i].size() != 4 || !isBoxInsideROI(boxes[i], roi)) {
                return false;
            }
        }
    }
    return true;
}

static GstPadProbeReturn probe_primary(GstPad *pad, GstPadProbeInfo *info,
                                       gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    vector<float> roi({0, 0, 200, 300});

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

    fail_unless(filterDetectionsByROI(pred, roi), "Out of ROI");
    fail_unless(evaluatePerformance(gt, pred) > 0.1, "Precision < 0.1.");
    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_primary) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    // ------------------------------ Stream 0 ------------------------------//
    GstElement *videosrc0 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc0 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc0, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc0), "framerate", 10, 1, NULL);
    g_object_set(videosrc0, "num-buffers", 100, NULL);

    GstElement *jpegparse0 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse0 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec0 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec0 != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_1/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess, "roi", "0,0,200,300", NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer, "model-path",
                 "./../../../dx_stream/samples/models/YOLOV5S_1.dxnn", NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_1/"
                 "postprocess_config.json",
                 NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    // -------------------------- LINK -----------------------//

    gst_bin_add_many(GST_BIN(pipeline), videosrc0, jpegparse0, jpegdec0,
                     preprocess, infer, postprocess, fakesink, NULL);
    fail_unless(gst_element_link_many(videosrc0, jpegparse0, jpegdec0,
                                      preprocess, infer, postprocess, fakesink,
                                      NULL),
                "Failed to link");
    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_primary, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    // -------------------------- RUN -----------------------//
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
GST_END_TEST

static GstPadProbeReturn probe_secondary(GstPad *pad, GstPadProbeInfo *info,
                                         gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    vector<float> roi({240, 10, 420, 360});

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    vector<float> pred_face;

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);
        if (meta_type == DX_OBJECT_META_API_TYPE) {
            DXObjectMeta *object_meta = (DXObjectMeta *)meta;
            // g_print("PTS: %" GST_TIME_FORMAT
            //         " Label : %d  Conf : %f Track : %d BOX : [%f %f %f %f] "
            //         "FACE BOX : [%f %f %f %f]\n",
            //         GST_TIME_ARGS(current_pts), object_meta->_label,
            //         object_meta->_confidence, object_meta->_track_id,
            //         object_meta->_box[0], object_meta->_box[1],
            //         object_meta->_box[2], object_meta->_box[3],
            //         object_meta->_face_box[0], object_meta->_face_box[1],
            //         object_meta->_face_box[2], object_meta->_face_box[3]);
            if (object_meta->_face_box[0] != 0 &&
                object_meta->_face_box[0] != 0 &&
                object_meta->_face_box[0] != 0 &&
                object_meta->_face_box[0] != 0) {
                pred_face.push_back(object_meta->_face_box[0]);
                pred_face.push_back(object_meta->_face_box[1]);
                pred_face.push_back(object_meta->_face_box[2]);
                pred_face.push_back(object_meta->_face_box[3]);
            }
        }
    }
    fail_unless(pred_face.size() == 4, "pred face not 1");
    fail_unless(isBoxInsideROI(pred_face, roi), "Out of ROI");

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_secondary) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    // ------------------------------ Stream 0 ------------------------------//
    GstElement *videosrc0 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc0 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc0, "image-path", "./../../test_resources/son.jpg",
                 NULL);
    g_object_set(G_OBJECT(videosrc0), "framerate", 10, 1, NULL);
    g_object_set(videosrc0, "num-buffers", 100, NULL);

    GstElement *jpegparse0 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse0 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec0 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec0 != NULL, "Failed to create jpegdec element");

    GstElement *preprocess0 = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess0 != NULL,
                "Failed to create GstDxPreprocess element");
    g_object_set(preprocess0, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_1/"
                 "preprocess_config.json",
                 NULL);

    GstElement *infer0 = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer0 != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer0, "model-path",
                 "./../../../dx_stream/samples/models/YOLOV5S_1.dxnn", NULL);
    g_object_set(infer0, "preprocess-id", 1, NULL);
    g_object_set(infer0, "inference-id", 1, NULL);

    GstElement *postprocess0 = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess0 != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess0, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_1/"
                 "postprocess_config.json",
                 NULL);

    GstElement *preprocess1 = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess1 != NULL,
                "Failed to create GstDxPreprocess element");
    g_object_set(preprocess1, "config-file-path",
                 "./../../../dx_stream/configs/Face_Detection/SCRFD/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess1, "interval", 0, NULL);
    g_object_set(preprocess1, "roi", "240,10,420,360", NULL);

    GstElement *infer1 = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer1 != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer1, "model-path",
                 "./../../../dx_stream/samples/models/SCRFD500M_1.dxnn", NULL);
    g_object_set(infer1, "preprocess-id", 4, NULL);
    g_object_set(infer1, "inference-id", 4, NULL);
    g_object_set(infer1, "secondary-mode", TRUE, NULL);

    GstElement *postprocess1 = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess1 != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess1, "config-file-path",
                 "./../../../dx_stream/configs/Face_Detection/SCRFD/"
                 "postprocess_config.json",
                 NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    // -------------------------- LINK -----------------------//

    gst_bin_add_many(GST_BIN(pipeline), videosrc0, jpegparse0, jpegdec0,
                     preprocess0, infer0, postprocess0, preprocess1, infer1,
                     postprocess1, fakesink, NULL);
    fail_unless(gst_element_link_many(videosrc0, jpegparse0, jpegdec0,
                                      preprocess0, infer0, postprocess0,
                                      preprocess1, infer1, postprocess1,
                                      fakesink, NULL),
                "Failed to link");
    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_secondary,
                      NULL, NULL);
    gst_object_unref(sink_pad);

    // -------------------------- RUN -----------------------//
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
GST_END_TEST

Suite *roi_suite(void) {
    Suite *s = suite_create("ROI TEST");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_primary);
    tcase_add_test(tc_core, test_secondary);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = roi_suite();
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
