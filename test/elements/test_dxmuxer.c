#include <dx_stream/gst-dxmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <unistd.h>

GstClockTime last_pts;
#define TARGET_SECONDS 5
#define ALLOWABLE_PERCENT 5.0

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
                                            GstElement *muxer) {
    GstPad *src_pad = gst_element_get_static_pad(element1, "src");
    fail_unless(src_pad, "Failed to get src pad from source");

    GstPad *sink_pad = gst_element_get_request_pad(muxer, "sink_%u");
    fail_unless(sink_pad, "Failed to request sink pad from muxer");

    // g_print(
    //     "Static src pad '%s' and requested sink pad '%s' ready for
    //     linking.\n", GST_PAD_NAME(src_pad), GST_PAD_NAME(sink_pad));

    GstPadLinkReturn ret = gst_pad_link(src_pad, sink_pad);
    fail_unless(ret == GST_PAD_LINK_OK, "Failed to link pads. Error");

    gst_object_unref(src_pad);
    gst_object_unref(sink_pad);
}

static GstPadProbeReturn probe_callback(GstPad *pad, GstPadProbeInfo *info,
                                        gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);

    GstMeta *meta;
    gpointer state = NULL;

    last_pts = GST_BUFFER_PTS(buffer);
    fail_unless(last_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    gboolean file_dx_frame_meta = FALSE;
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);

        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;
            // g_print("Stream #%d  PTS: %" GST_TIME_FORMAT " %s  ( %d x %d)\n",
            //         frame_meta->_stream_id, GST_TIME_ARGS(last_pts),
            //         frame_meta->_format, frame_meta->_width,
            //         frame_meta->_height);
            file_dx_frame_meta = TRUE;
        }
    }

    fail_unless(file_dx_frame_meta, "Buffer has no DXFrameMeta.");

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn
probe_multi_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);

    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    fail_unless(current_pts >= last_pts, "CURRENT PTS < LAST PTS");

    gboolean file_dx_frame_meta = FALSE;
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);

        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;
            // g_print("Stream #%d  PTS: %" GST_TIME_FORMAT " %s  ( %d x %d)\n",
            //         frame_meta->_stream_id, GST_TIME_ARGS(current_pts),
            //         frame_meta->_format, frame_meta->_width,
            //         frame_meta->_height);
            file_dx_frame_meta = TRUE;
        }
    }

    fail_unless(file_dx_frame_meta, "Buffer has no DXFrameMeta.");
    last_pts = current_pts;
    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxmuxer", NULL);

    fail_unless(element != NULL, "Failed to create GstDxMuxer element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstElement *element;
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    element = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(element != NULL, "Failed to create GstElement: dxmuxer");

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
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_properties) {
    GstElement *element = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(element != NULL, "Failed to create GstDxMuxer element");

    g_object_set(element, "live-source", TRUE, NULL);

    gboolean live_source;
    g_object_get(element, "live-source", &live_source, NULL);
    fail_unless(live_source == TRUE,
                "'live-source' property not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_single_stream_flow) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");

    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *muxer = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(muxer != NULL, "Failed to create GstDxMuxer element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc, muxer, fakesink, NULL);

    link_static_src_to_dynamic_sink(videosrc, muxer);

    fail_unless(gst_element_link(muxer, fakesink),
                "Failed to link muxer to fakesink");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_callback, NULL,
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
GST_END_TEST

GST_START_TEST(test_element_event) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");

    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *muxer = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(muxer != NULL, "Failed to create GstDxMuxer element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc, muxer, fakesink, NULL);

    link_static_src_to_dynamic_sink(videosrc, muxer);

    fail_unless(gst_element_link(muxer, fakesink),
                "Failed to link muxer to fakesink");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_callback, NULL,
                      NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn ret =
        gst_element_get_state(pipeline, NULL, NULL, GST_SECOND);
    fail_unless(ret == GST_STATE_CHANGE_SUCCESS,
                "Pipeline state change to PLAYING timed out");

    g_main_loop_run(loop);

    double elapsed_seconds = (double)last_pts / GST_SECOND;
    double target_time = TARGET_SECONDS;
    double lower_bound = target_time * (1.0 - ALLOWABLE_PERCENT / 100.0);
    double upper_bound = target_time * (1.0 + ALLOWABLE_PERCENT / 100.0);

    fail_unless(elapsed_seconds >= lower_bound &&
                    elapsed_seconds <= upper_bound,
                "PTS is out of range.");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
GST_END_TEST

void create_source_sub_pipeline(int num_buffers, GstElement *pipeline,
                                GstElement *muxer) {
    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");
    g_object_set(videosrc, "num-buffers", num_buffers, NULL);
    gst_bin_add_many(GST_BIN(pipeline), videosrc, NULL);

    link_static_src_to_dynamic_sink(videosrc, muxer);
}

GST_START_TEST(test_multi_stream_flow) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *muxer = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(muxer != NULL, "Failed to create GstDxMuxer element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), muxer, fakesink, NULL);

    create_source_sub_pipeline(100, pipeline, muxer);
    create_source_sub_pipeline(120, pipeline, muxer);
    create_source_sub_pipeline(120, pipeline, muxer);
    create_source_sub_pipeline(150, pipeline, muxer);

    fail_unless(gst_element_link(muxer, fakesink),
                "Failed to link muxer to fakesink");

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_multi_callback,
                      NULL, NULL);
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
GST_END_TEST

int get_sink_pad_count(GstElement *element) {
    int count = 0;
    GstIterator *pad_iter = gst_element_iterate_pads(element);
    GValue pad_value = G_VALUE_INIT;
    gboolean done = FALSE;

    while (!done) {
        switch (gst_iterator_next(pad_iter, &pad_value)) {
        case GST_ITERATOR_OK: {
            GstPad *pad = GST_PAD(g_value_get_object(&pad_value));
            if (GST_PAD_IS_SINK(pad)) {
                count++;
            }
            g_value_reset(&pad_value);
            break;
        }
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        case GST_ITERATOR_ERROR:
            g_printerr("Error while iterating over pads.\n");
            done = TRUE;
            break;
        default:
            break;
        }
    }

    g_value_unset(&pad_value);
    gst_iterator_free(pad_iter);

    return count;
}

GST_START_TEST(test_dynamic_pad_in_running_time) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *muxer = gst_element_factory_make("dxmuxer", NULL);
    fail_unless(muxer != NULL, "Failed to create GstDxMuxer element");

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");
    g_object_set(fakesink, "sync", TRUE, NULL);

    gst_bin_add_many(GST_BIN(pipeline), muxer, fakesink, NULL);

    fail_unless(gst_element_link(muxer, fakesink),
                "Failed to link muxer to fakesink");

    create_source_sub_pipeline(300, pipeline, muxer);

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER, probe_multi_callback,
                      NULL, NULL);
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
    // int pad_count = get_sink_pad_count(muxer);

    // sleep(1);
    // // gst_element_set_state(pipeline, GST_STATE_PAUSED);
    // // ret = gst_element_get_state(pipeline, NULL, NULL, GST_SECOND);
    // create_source_sub_pipeline(100, pipeline, muxer);
    // pad_count = get_sink_pad_count(muxer);
    // // g_print("Current sink pad count after second addition: %d\n",
    // pad_count);

    // // gst_element_set_state(pipeline, GST_STATE_PLAYING);
    // // ret = gst_element_get_state(pipeline, NULL, NULL, GST_SECOND);

    // sleep(10);
    //

    // gst_element_set_state(pipeline, GST_STATE_NULL);
    // gst_object_unref(pipeline);
}
GST_END_TEST

Suite *dxmuxer_suite(void) {
    Suite *s = suite_create("GstDxMuxer");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 20.0);

    tcase_add_test(tc_core, test_element_initialization);
    tcase_add_test(tc_core, test_element_state_change);
    tcase_add_test(tc_core, test_element_properties);

    tcase_add_test(tc_core, test_element_event);
    tcase_add_test(tc_core, test_single_stream_flow);
    tcase_add_test(tc_core, test_multi_stream_flow);

    // tcase_add_test(tc_core, test_dynamic_pad_in_running_time);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxmuxer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
