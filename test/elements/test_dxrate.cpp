#include <dx_stream/gst-dxmeta.hpp>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <unistd.h>

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
    GstElement *element = gst_element_factory_make("dxrate", NULL);

    fail_unless(element != NULL, "Failed to create GstDxRate element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstElement *element;
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    element = gst_element_factory_make("dxrate", NULL);
    fail_unless(element != NULL, "Failed to create GstElement: dxrate");

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
    GstElement *element = gst_element_factory_make("dxrate", NULL);
    fail_unless(element != NULL, "Failed to create GstDxRate element");

    g_object_set(element, "framerate", 15, NULL);
    gint framerate;
    g_object_get(element, "framerate", &framerate, NULL);
    fail_unless(framerate == 15, "'framerate' property not set correctly");

    g_object_set(element, "throttle", TRUE, NULL);
    gboolean throttle;
    g_object_get(element, "throttle", &throttle, NULL);
    fail_unless(throttle == TRUE, "'throttle' property not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

GstClockTime last_pts = GST_CLOCK_TIME_NONE;
#define TARGET_FRAMERATE 15
#define ALLOWABLE_PERCENT 5.0

static GstPadProbeReturn probe_callback(GstPad *pad, GstPadProbeInfo *info,
                                        gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    if (last_pts == GST_CLOCK_TIME_NONE) {
        last_pts = current_pts;
        return GST_PAD_PROBE_OK;
    }

    GstClockTime diff_pts = current_pts - last_pts;
    double duration = (double)diff_pts / GST_SECOND;
    double target_duration = 1 / (double)TARGET_FRAMERATE;

    double lower_bound = target_duration * (1.0 - ALLOWABLE_PERCENT / 100.0);
    double upper_bound = target_duration * (1.0 + ALLOWABLE_PERCENT / 100.0);

    fail_unless(duration >= lower_bound && duration <= upper_bound,
                "framerate is out of range.");

    last_pts = current_pts;

    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_framerate) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");

    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *rate = gst_element_factory_make("dxrate", NULL);
    fail_unless(rate != NULL, "Failed to create GstDxRate element");
    g_object_set(rate, "framerate", TARGET_FRAMERATE, NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc, rate, fakesink, NULL);
    fail_unless(gst_element_link_many(videosrc, rate, fakesink, NULL),
                "Failed to link");

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

static GstPadProbeReturn probe_callback_throttle(GstPad *pad,
                                                 GstPadProbeInfo *info,
                                                 gpointer user_data) {
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);

    if (event && GST_EVENT_TYPE(event) == GST_EVENT_QOS) {
        // QoS 이벤트에서 필요한 정보 추출
        GstQOSType qos_type;
        gdouble proportion;
        GstClockTimeDiff diff;
        GstClockTime timestamp;

        gst_event_parse_qos(event, &qos_type, &proportion, &diff, &timestamp);
        fail_unless(qos_type == GST_QOS_TYPE_THROTTLE,
                    "Received QoS event, but not of type THROTTLE");
        double target_duration = 1 / (double)TARGET_FRAMERATE;
        double duration = (double)diff / GST_SECOND;
        double lower_bound =
            target_duration * (1.0 - ALLOWABLE_PERCENT / 100.0);
        double upper_bound =
            target_duration * (1.0 + ALLOWABLE_PERCENT / 100.0);
        fail_unless(duration >= lower_bound && duration <= upper_bound,
                    "Throttle Delay is out of range.");
    }

    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_throttle) {
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-pipeline");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *videosrc = gst_element_factory_make("videotestsrc", NULL);
    fail_unless(videosrc != NULL, "Failed to create videotestsrc element");

    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *rate = gst_element_factory_make("dxrate", NULL);
    fail_unless(rate != NULL, "Failed to create GstDxRate element");
    g_object_set(rate, "framerate", TARGET_FRAMERATE, NULL);
    g_object_set(rate, "throttle", TRUE, NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    gst_bin_add_many(GST_BIN(pipeline), videosrc, rate, fakesink, NULL);
    fail_unless(gst_element_link_many(videosrc, rate, fakesink, NULL),
                "Failed to link");

    GstPad *src_pad = gst_element_get_static_pad(videosrc, "src");
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_EVENT_UPSTREAM,
                      probe_callback_throttle, NULL, NULL);
    gst_object_unref(src_pad);

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

Suite *dxrate_suite(void) {
    Suite *s = suite_create("GstDxRate");
    TCase *tc_core = tcase_create("Core");
    tcase_set_timeout(tc_core, 20.0);

    tcase_add_test(tc_core, test_element_initialization);
    tcase_add_test(tc_core, test_element_state_change);
    tcase_add_test(tc_core, test_element_properties);

    tcase_add_test(tc_core, test_framerate);
    tcase_add_test(tc_core, test_throttle);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxrate_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
