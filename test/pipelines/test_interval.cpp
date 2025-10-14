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
map<int, int> multi_frame_cnt;
int single_frame_cnt;

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

static void link_static_src_to_dynamic_sink(GstElement *element1,
                                            GstElement *gather) {
    GstPad *src_pad = gst_element_get_static_pad(element1, "src");
    fail_unless(src_pad, "Failed to get src pad from source");

    GstPad *sink_pad = gst_element_get_request_pad(gather, "sink_%u");
    fail_unless(sink_pad, "Failed to request sink pad from gather");

    GstPadLinkReturn ret = gst_pad_link(src_pad, sink_pad);
    fail_unless(ret == GST_PAD_LINK_OK, "Failed to link pads. Error");

    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
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

static GstPadProbeReturn probe_single(GstPad *pad, GstPadProbeInfo *info,
                                      gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    single_frame_cnt++;

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);
        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;

            int objects_size = g_list_length(frame_meta->_object_meta_list);
            if (single_frame_cnt == 4) {
                DetectionMap pred;
                for (int o = 0; o < objects_size; o++) {
                    DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                        frame_meta->_object_meta_list, o);
                    pred[object_meta->_label].push_back(
                        {object_meta->_box[0], object_meta->_box[1],
                         object_meta->_box[2], object_meta->_box[3]});
                }
                fail_unless(evaluatePerformance(gt, pred) > 0.1,
                            "Precision < 0.1.");
                single_frame_cnt = 0;
            } else {
                fail_unless(objects_size == 0, "Object exist in Skip Frame");
            }
        }
    }

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_single_stream) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    // ------------------------------ Stream 0 ------------------------------//
    GstElement *videosrc0 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc0 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc0, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc0), "framerate", 10, 1, NULL);
    g_object_set(videosrc0, "num-buffers", 10, NULL);

    GstElement *jpegparse0 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse0 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec0 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec0 != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_3/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess, "interval", 3, NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer, "model-path",
                 "./../../../dx_stream/samples/models/YOLOV5S_3.dxnn", NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_3/"
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
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_single, NULL,
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

static GstPadProbeReturn probe_multi(GstPad *pad, GstPadProbeInfo *info,
                                     gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);
        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;

            multi_frame_cnt[frame_meta->_stream_id] += 1;
            int objects_size = g_list_length(frame_meta->_object_meta_list);
            if (multi_frame_cnt[frame_meta->_stream_id] == 4) {
                DetectionMap pred;
                for (int o = 0; o < objects_size; o++) {
                    DXObjectMeta *object_meta = (DXObjectMeta *)g_list_nth_data(
                        frame_meta->_object_meta_list, o);
                    pred[object_meta->_label].push_back(
                        {object_meta->_box[0], object_meta->_box[1],
                         object_meta->_box[2], object_meta->_box[3]});
                }
                fail_unless(evaluatePerformance(gt, pred) > 0.1,
                            "Precision < 0.1.");
                multi_frame_cnt[frame_meta->_stream_id] = 0;
            } else {
                fail_unless(objects_size == 0, "Object exist in Skip Frame");
            }
        }
    }

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_multi_stream) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    // ---------------------------- Stream 0 ------------------------------//
    GstElement *videosrc0 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc0 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc0, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc0), "framerate", 10, 1, NULL);
    g_object_set(videosrc0, "num-buffers", 10, NULL);

    GstElement *jpegparse0 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse0 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec0 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec0 != NULL, "Failed to create jpegdec element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc0, jpegparse0, jpegdec0, NULL);
    fail_unless(gst_element_link_many(videosrc0, jpegparse0, jpegdec0, NULL),
                "Failed to link");

    // ---------------------------- Stream 1------------------------------//

    GstElement *videosrc1 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc1 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc1, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc1), "framerate", 10, 1, NULL);
    g_object_set(videosrc1, "num-buffers", 10, NULL);

    GstElement *jpegparse1 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse1 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec1 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec1 != NULL, "Failed to create jpegdec element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc1, jpegparse1, jpegdec1, NULL);
    fail_unless(gst_element_link_many(videosrc1, jpegparse1, jpegdec1, NULL),
                "Failed to link");

    // ---------------------------- Stream 2------------------------------//

    GstElement *videosrc2 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc2 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc2, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc2), "framerate", 10, 1, NULL);
    g_object_set(videosrc2, "num-buffers", 10, NULL);

    GstElement *jpegparse2 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse2 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec2 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec2 != NULL, "Failed to create jpegdec element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc2, jpegparse2, jpegdec2, NULL);
    fail_unless(gst_element_link_many(videosrc2, jpegparse2, jpegdec2, NULL),
                "Failed to link");

    // ------------------------------ Stream 3------------------------------//

    GstElement *videosrc3 = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc3 != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc3, "image-path", "./../../test_resources/1.jpg", NULL);
    g_object_set(G_OBJECT(videosrc3), "framerate", 10, 1, NULL);
    g_object_set(videosrc3, "num-buffers", 10, NULL);

    GstElement *jpegparse3 = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse3 != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec3 = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec3 != NULL, "Failed to create jpegdec element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc3, jpegparse3, jpegdec3, NULL);
    fail_unless(gst_element_link_many(videosrc3, jpegparse3, jpegdec3, NULL),
                "Failed to link");

    // ----------------------- Inference pipeline -----------------------//

    GstElement *inputselector =
        gst_element_factory_make("dxinputselector", NULL);
    fail_unless(inputselector != NULL,
                "Failed to create GstDxInputSelector element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_3/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess, "interval", 3, NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer, "model-path",
                 "./../../../dx_stream/samples/models/YOLOV5S_3.dxnn", NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YOLOV5S_3/"
                 "postprocess_config.json",
                 NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), inputselector, preprocess, infer,
                     postprocess, fakesink, NULL);
    fail_unless(gst_element_link_many(inputselector, preprocess, infer,
                                      postprocess, fakesink, NULL),
                "Failed to link");

    // -------------------------- LINK -----------------------//
    link_static_src_to_dynamic_sink(jpegdec0, inputselector);
    link_static_src_to_dynamic_sink(jpegdec1, inputselector);
    link_static_src_to_dynamic_sink(jpegdec2, inputselector);
    link_static_src_to_dynamic_sink(jpegdec3, inputselector);

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_multi, NULL,
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

Suite *interval_suite(void) {
    Suite *s = suite_create("Interval TEST");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 60.0);
    tcase_add_test(tc_core, test_single_stream);
    tcase_add_test(tc_core, test_multi_stream);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = interval_suite();
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

    multi_frame_cnt[0] = 0;
    multi_frame_cnt[1] = 0;
    multi_frame_cnt[2] = 0;
    multi_frame_cnt[3] = 0;

    single_frame_cnt = 0;

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
