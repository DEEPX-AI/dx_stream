#include <dx_stream/gst-dxmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>

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

float computeIoU(const std::vector<float> box1, const std::vector<float> box2) {

    float x1 = std::max(box1[0], box2[0]);
    float y1 = std::max(box1[1], box2[1]);
    float x2 = std::min(box1[2], box2[2]);
    float y2 = std::min(box1[3], box2[3]);

    float intersection = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float area1 = (box1[2] - box1[0]) * (box1[3] - box1[1]);
    float area2 = (box2[2] - box2[0]) * (box2[3] - box2[1]);
    float unionArea = area1 + area2 - intersection;

    return unionArea > 0 ? (intersection / unionArea) : 0.0f;
}

static GstPadProbeReturn probe_primary(GstPad *pad, GstPadProbeInfo *info,
                                       gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    float max_conf = 0.0;

    std::vector<float> gt_box({245.000000, 14.500000, 415.000000, 355.500000});
    std::vector<float> gt_face({301.256226, 30.953709, 335.228210, 78.290771});

    std::vector<float> pred_box;
    std::vector<float> pred_face;

    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);

        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;
            // g_print("Stream #%d  PTS: %" GST_TIME_FORMAT " %s  ( %d x %d)\n",
            //         frame_meta->_stream_id, GST_TIME_ARGS(current_pts),
            //         frame_meta->_format, frame_meta->_width,
            //         frame_meta->_height);
        }

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
            if (object_meta->_confidence > max_conf) {
                if (object_meta->_box[2] <= 0 ||
                    object_meta->_face_box[2] <= 0) {
                    continue;
                }
                max_conf = (float)object_meta->_confidence;

                pred_box.push_back(object_meta->_box[0]);
                pred_box.push_back(object_meta->_box[1]);
                pred_box.push_back(object_meta->_box[2]);
                pred_box.push_back(object_meta->_box[3]);

                pred_face.push_back(object_meta->_face_box[0]);
                pred_face.push_back(object_meta->_face_box[1]);
                pred_face.push_back(object_meta->_face_box[2]);
                pred_face.push_back(object_meta->_face_box[3]);
            }
        }
    }
    fail_unless(max_conf > 0, "Max Confidence Lower than threshold");
    float box_iou = computeIoU(pred_box, gt_box);
    float face_iou = computeIoU(pred_face, gt_face);
    fail_unless(box_iou > 0.5 && face_iou > 0.5, "IOU Lower than threshold");

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_face_recognition_pipeline) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("dxgenbuffer", NULL);
    fail_unless(videosrc != NULL, "Failed to create dxgenbuffer element");

    g_object_set(videosrc, "image-path", "./../../test_resources/son.jpg",
                 NULL);
    GValue framerate = G_VALUE_INIT;
    g_value_init(&framerate, GST_TYPE_FRACTION);
    gst_value_set_fraction(&framerate, 5, 1);
    g_object_set_property(G_OBJECT(videosrc), "framerate", &framerate);
    g_value_unset(&framerate);
    g_object_set(videosrc, "num-buffers", 10, NULL);

    GstElement *jpegparse = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");
    g_object_set(preprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YoloV7/"
                 "preprocess_config.json",
                 NULL);

    GstElement *infer = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer, "model-path",
                 "./../../../dx_stream/samples/models/YoloV7.dxnn", NULL);
    g_object_set(infer, "preprocess-id", 1, NULL);
    g_object_set(infer, "inference-id", 1, NULL);

    GstElement *postprocess = gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess, "config-file-path",
                 "./../../../dx_stream/configs/Object_Detection/YoloV7/"
                 "postprocess_config.json",
                 NULL);

    GstElement *tee = gst_element_factory_make("tee", NULL);
    fail_unless(tee != NULL, "Failed to create tee element");
    GstElement *q1 = gst_element_factory_make("queue", NULL);
    GstElement *q2 = gst_element_factory_make("queue", NULL);

    GstElement *gather = gst_element_factory_make("dxgather", NULL);
    fail_unless(gather != NULL, "Failed to create GstDxGather element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    GstElement *preprocess_face =
        gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess_face != NULL,
                "Failed to create GstDxPreprocess element");
    g_object_set(preprocess_face, "config-file-path",
                 "./../../../dx_stream/configs/Face_Detection/SCRFD/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess_face, "interval", 0, NULL);

    GstElement *infer_face = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer_face != NULL, "Failed to create GstDxInfer element");
    g_object_set(infer_face, "model-path",
                 "./../../../dx_stream/samples/models/SCRFD500M_1.dxnn", NULL);
    g_object_set(infer_face, "preprocess-id", 4, NULL);
    g_object_set(infer_face, "inference-id", 4, NULL);
    g_object_set(infer_face, "secondary-mode", TRUE, NULL);

    GstElement *postprocess_face =
        gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess_face != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess_face, "config-file-path",
                 "./../../../dx_stream/configs/Face_Detection/SCRFD/"
                 "postprocess_config.json",
                 NULL);

    GstElement *preprocess_reid =
        gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess_reid != NULL,
                "Failed to create GstDxPreprocess element");
    g_object_set(preprocess_reid, "config-file-path",
                 "./../../../dx_stream/configs/Re-Identification/OSNet/"
                 "preprocess_config.json",
                 NULL);
    g_object_set(preprocess_reid, "interval", 0, NULL);

    GstElement *infer_reid = gst_element_factory_make("dxinfer", NULL);
    fail_unless(infer_reid != NULL, "Failed to create GstDxInfer element");
    g_object_set(
        infer_reid, "model-path",
        "./../../../dx_stream/samples/models/osnet_x0_5_market_256x128.dxnn",
        NULL);
    g_object_set(infer_reid, "preprocess-id", 3, NULL);
    g_object_set(infer_reid, "inference-id", 3, NULL);
    g_object_set(infer_reid, "secondary-mode", TRUE, NULL);

    GstElement *postprocess_reid =
        gst_element_factory_make("dxpostprocess", NULL);
    fail_unless(postprocess_reid != NULL,
                "Failed to create GstDxPostprocess element");
    g_object_set(postprocess_reid, "config-file-path",
                 "./../../../dx_stream/configs/Re-Identification/OSNet/"
                 "postprocess_config.json",
                 NULL);

    // ADD Elements
    gst_bin_add_many(GST_BIN(pipeline), videosrc, jpegparse, jpegdec,
                     preprocess, infer, postprocess, tee, q1, q2, gather,
                     preprocess_face, infer_face, postprocess_face,
                     preprocess_reid, infer_reid, postprocess_reid, fakesink,
                     NULL);

    // Link Elements
    fail_unless(gst_element_link_many(videosrc, jpegparse, jpegdec, preprocess,
                                      infer, postprocess, tee, NULL),
                "Failed to link");

    // Request dynamic pads from tee and link them to the queues
    GstPad *tee_src_pad1 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *tee_src_pad2 = gst_element_get_request_pad(tee, "src_%u");
    fail_unless(tee_src_pad1 && tee_src_pad2, "Failed to request tee pads");

    GstPad *q1_sink_pad = gst_element_get_static_pad(q1, "sink");
    GstPad *q2_sink_pad = gst_element_get_static_pad(q2, "sink");

    fail_unless(gst_pad_link(tee_src_pad1, q1_sink_pad) == GST_PAD_LINK_OK,
                "Failed to link tee to q1");
    fail_unless(gst_pad_link(tee_src_pad2, q2_sink_pad) == GST_PAD_LINK_OK,
                "Failed to link tee to q2");

    // Face Detection
    fail_unless(gst_element_link_many(q1, preprocess_face, infer_face,
                                      postprocess_face, NULL),
                "Failed to link");

    // ReID
    fail_unless(gst_element_link_many(q2, preprocess_reid, infer_reid,
                                      postprocess_reid, NULL),
                "Failed to link");

    link_static_src_to_dynamic_sink(postprocess_face, gather);
    link_static_src_to_dynamic_sink(postprocess_reid, gather);

    fail_unless(gst_element_link(gather, fakesink), "Failed to link");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_primary, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret =
        gst_element_get_state(pipeline, NULL, NULL, 10 * GST_SECOND);
    fail_unless(ret == GST_STATE_CHANGE_SUCCESS,
                "Pipeline state change to PLAYING timed out");

    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
GST_END_TEST

Suite *secondary_suite(void) {
    Suite *s = suite_create("Secondary TEST");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 60.0);

    tcase_add_test(tc_core, test_face_recognition_pipeline);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = secondary_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
