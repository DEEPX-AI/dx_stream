#include <dx_stream/gst-dxmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <unistd.h>

#define LABEL 101
#define CONFIDENCE 0.975
#define TRACK_ID 11

#define BOX_X1 100
#define BOX_Y1 150
#define BOX_X2 1100
#define BOX_Y2 1500

#define FACE_BOX_X1 10
#define FACE_BOX_Y1 15
#define FACE_BOX_X2 110
#define FACE_BOX_Y2 150

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

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxgather", NULL);
    fail_unless(element != NULL, "Failed to create GstDxGather element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstElement *element;
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    element = gst_element_factory_make("dxgather", NULL);
    fail_unless(element != NULL, "Failed to create GstElement: dxgather");

    ret = gst_element_set_state(element, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to READY");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_READY,
                "State should be READY but got different state");

    ret = gst_element_set_state(element, GST_STATE_PAUSED);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to PAUSED");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_PAUSED,
                "State should be PAUSED but got different state");

    ret = gst_element_set_state(element, GST_STATE_PLAYING);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "Failed to change state to PLAYING");

    gst_element_get_state(element, &current_state, &pending_state,
                          GST_CLOCK_TIME_NONE);
    fail_unless(current_state == GST_STATE_PLAYING,
                "State should be PLAYING but got different state");

    gst_element_set_state(element, GST_STATE_NULL);
    if (GST_IS_ELEMENT(element)) {
        gst_object_unref(element);
    }
}
GST_END_TEST

static gboolean timeout_callback(GMainLoop *loop) {
    // g_print("Timeout reached, quitting the main loop.\n");
    g_main_loop_quit(loop);
    return FALSE;
}

static void link_static_src_to_dynamic_sink(GstElement *element1,
                                            GstElement *gather) {
    GstPad *src_pad = gst_element_get_static_pad(element1, "src");
    fail_unless(src_pad, "Failed to get src pad from source");

    GstPad *sink_pad = gst_element_get_request_pad(gather, "sink_%u");
    fail_unless(sink_pad, "Failed to request sink pad from gather");

    // g_print(
    //     "Static src pad '%s' and requested sink pad '%s' ready for
    //     linking.\n", GST_PAD_NAME(src_pad), GST_PAD_NAME(sink_pad));

    GstPadLinkReturn ret = gst_pad_link(src_pad, sink_pad);
    fail_unless(ret == GST_PAD_LINK_OK, "Failed to link pads. Error");

    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
}

static GstPadProbeReturn
probe_create_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
    }

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_add_meta(buffer, DX_FRAME_META_INFO, NULL);
    frame_meta->_stream_id = 0;
    frame_meta->_format = "I420";
    frame_meta->_name = "test";

    DXObjectMeta *object_meta =
        (DXObjectMeta *)gst_buffer_add_meta(buffer, DX_OBJECT_META_INFO, NULL);

    dx_add_object_meta_to_frame_meta(object_meta, frame_meta);

    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn probe_callback(GstPad *pad, GstPadProbeInfo *info,
                                        gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");
    GstMeta *meta;
    gpointer state = NULL;

    gboolean is_valid = FALSE;
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);

        if (meta_type == DX_OBJECT_META_API_TYPE) {
            DXObjectMeta *object_meta = (DXObjectMeta *)meta;
            // g_print("PTS: %" GST_TIME_FORMAT
            //         " Label : %d  Conf : %f Track : %d BOX : [%f %f %f %f] "
            //         "FACE BOX : [%f %f %f %f]\n",
            //         GST_TIME_ARGS(current_pts), object_meta->_label,
            //         object_meta->_confidence, object_meta->_box[0],
            //         object_meta->_track_id, object_meta->_box[1],
            //         object_meta->_box[2], object_meta->_box[3],
            //         object_meta->_face_box[0], object_meta->_face_box[1],
            //         object_meta->_face_box[2], object_meta->_face_box[3]);
            if (object_meta->_box[0] != 0) {
                if (object_meta->_box[0] != BOX_X1 ||
                    object_meta->_box[1] != BOX_Y1 ||
                    object_meta->_box[2] != BOX_X2 ||
                    object_meta->_box[3] != BOX_Y2) {
                    continue;
                }
            }
            if (object_meta->_face_box[0] != 0) {
                if (object_meta->_face_box[0] != FACE_BOX_X1 ||
                    object_meta->_face_box[1] != FACE_BOX_Y1 ||
                    object_meta->_face_box[2] != FACE_BOX_X2 ||
                    object_meta->_face_box[3] != FACE_BOX_Y2) {
                    continue;
                }
            }
            if (object_meta->_label != -1 && object_meta->_label != LABEL) {
                continue;
            }
            if (object_meta->_confidence != (gfloat)-1 &&
                object_meta->_confidence != (gfloat)CONFIDENCE) {
                continue;
            }
            if (object_meta->_track_id != -1 &&
                object_meta->_track_id != TRACK_ID) {
                continue;
            }
            is_valid = TRUE;
        }
    }

    fail_unless(is_valid == TRUE, "Not Valid DXObjectMeta.");
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_flow) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");
    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *tee = gst_element_factory_make("tee", NULL);
    fail_unless(tee != NULL, "Failed to create tee element");

    GstElement *q1 = gst_element_factory_make("queue", NULL);
    GstElement *q2 = gst_element_factory_make("queue", NULL);
    GstElement *q3 = gst_element_factory_make("queue", NULL);

    GstElement *genobj1 = gst_element_factory_make("dxgenobj", NULL);
    g_object_set(genobj1, "box", TRUE, NULL);
    GstElement *genobj2 = gst_element_factory_make("dxgenobj", NULL);
    g_object_set(genobj2, "face-box", TRUE, NULL);
    GstElement *genobj3 = gst_element_factory_make("dxgenobj", NULL);
    g_object_set(genobj3, "confidence", TRUE, NULL);

    GstElement *gather = gst_element_factory_make("dxgather", NULL);
    fail_unless(gather != NULL, "Failed to create GstDxGather element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc, tee, q1, q2, q3, genobj1,
                     genobj2, genobj3, gather, fakesink, NULL);

    // Link videosrc to tee
    fail_unless(gst_element_link(videosrc, tee), "Failed to link source & tee");

    GstPad *tee_sink_pad = gst_element_get_static_pad(tee, "sink");
    gst_pad_add_probe(tee_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      probe_create_callback, NULL, NULL);
    gst_object_unref(tee_sink_pad);

    // Request dynamic pads from tee and link them to the queues
    GstPad *tee_src_pad1 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *tee_src_pad2 = gst_element_get_request_pad(tee, "src_%u");
    GstPad *tee_src_pad3 = gst_element_get_request_pad(tee, "src_%u");
    fail_unless(tee_src_pad1 && tee_src_pad2 && tee_src_pad3,
                "Failed to request tee pads");

    GstPad *q1_sink_pad = gst_element_get_static_pad(q1, "sink");
    GstPad *q2_sink_pad = gst_element_get_static_pad(q2, "sink");
    GstPad *q3_sink_pad = gst_element_get_static_pad(q3, "sink");

    fail_unless(gst_pad_link(tee_src_pad1, q1_sink_pad) == GST_PAD_LINK_OK,
                "Failed to link tee to q1");
    fail_unless(gst_pad_link(tee_src_pad2, q2_sink_pad) == GST_PAD_LINK_OK,
                "Failed to link tee to q2");
    fail_unless(gst_pad_link(tee_src_pad3, q3_sink_pad) == GST_PAD_LINK_OK,
                "Failed to link tee to q3");

    gst_object_unref(tee_src_pad1);
    gst_object_unref(tee_src_pad2);
    gst_object_unref(tee_src_pad3);
    gst_object_unref(q1_sink_pad);
    gst_object_unref(q2_sink_pad);
    gst_object_unref(q3_sink_pad);

    fail_unless(gst_element_link(q1, genobj1), "Failed to link queue & genobj");
    fail_unless(gst_element_link(q2, genobj2), "Failed to link queue & genobj");
    fail_unless(gst_element_link(q3, genobj3), "Failed to link queue & genobj");

    // Link queues to gather
    link_static_src_to_dynamic_sink(genobj1, gather);
    link_static_src_to_dynamic_sink(genobj2, gather);
    link_static_src_to_dynamic_sink(genobj3, gather);

    fail_unless(gst_element_link(gather, fakesink),
                "Failed to link gather & sink");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_callback, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    // Set pipeline to PLAYING state
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

Suite *dxgather_suite(void) {
    Suite *s = suite_create("GstDxGather");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 10.0);

    tcase_add_test(tc_core, test_element_initialization);
    tcase_add_test(tc_core, test_element_state_change);

    tcase_add_test(tc_core, test_flow);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxgather_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
