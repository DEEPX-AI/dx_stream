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

GST_START_TEST(test_element_initialization) {
    GstElement *element = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPreprocess element");
    gst_object_unref(element);
}
GST_END_TEST

GST_START_TEST(test_element_state_change) {
    GstStateChangeReturn ret;
    GstState current_state, pending_state;

    GstElement *element = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPreprocess element");

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
    GstElement *element = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(element != NULL, "Failed to create GstDxPreprocess element");

    g_object_set(element, "color-format", 1, NULL);
    guint color_format;
    g_object_get(element, "color-format", &color_format, NULL);
    fail_unless(color_format == 1, "'color-format' property not set correctly");

    g_object_set(element, "resize-width", 640, "resize-height", 480, NULL);
    guint resize_width, resize_height;
    g_object_get(element, "resize-width", &resize_width, "resize-height",
                 &resize_height, NULL);
    fail_unless(
        resize_width == 640 && resize_height == 480,
        "'resize-width' and 'resize-height' properties not set correctly");

    g_object_set(element, "function-name", "TEST_FUNC", NULL);
    gchar *function_name;
    g_object_get(element, "function-name", &function_name, NULL);
    fail_unless(g_strcmp0(function_name, "TEST_FUNC") == 0,
                "'function-name' properties not set correctly");

    g_object_set(element, "interval", 15, NULL);
    guint interval;
    g_object_get(element, "interval", &interval, NULL);
    fail_unless(interval == 15, "'interval' property not set correctly");

    g_object_set(element, "keep-ratio", FALSE, NULL);
    gboolean keep_ratio;
    g_object_get(element, "keep-ratio", &keep_ratio, NULL);
    fail_unless(keep_ratio == FALSE, "'keep-ratio' property not set correctly");

    g_object_set(element, "library-file-path", "TEST_LIBRARY_PATH", NULL);
    gchar *library_file_path;
    g_object_get(element, "library-file-path", &library_file_path, NULL);
    fail_unless(g_strcmp0(library_file_path, "TEST_LIBRARY_PATH") == 0,
                "'library-file-path' properties not set correctly");

    g_object_set(element, "min-object-width", 170, "min-object-height", 340,
                 NULL);
    guint min_object_width, min_object_height;
    g_object_get(element, "min-object-width", &min_object_width,
                 "min-object-height", &min_object_height, NULL);
    fail_unless(min_object_width == 170 && min_object_height == 340,
                "'min-object-width' and 'min-object-height' properties not set "
                "correctly");

    g_object_set(element, "pad-value", 231, NULL);
    guint pad_value;
    g_object_get(element, "pad-value", &pad_value, NULL);
    fail_unless(pad_value == 231, "'pad-value' property not set correctly");

    g_object_set(element, "preprocess-id", 13, NULL);
    guint preprocess_id;
    g_object_get(element, "preprocess-id", &preprocess_id, NULL);
    fail_unless(preprocess_id == 13,
                "'preprocess-id' property not set correctly");

    g_object_set(element, "target-class-id", 2, NULL);
    gint target_class_id;
    g_object_get(element, "target-class-id", &target_class_id, NULL);
    fail_unless(target_class_id == 2,
                "'target-class-id' property not set correctly");

    g_object_set(element, "secondary-mode", TRUE, NULL);
    gboolean secondary_mode;
    g_object_get(element, "secondary-mode", &secondary_mode, NULL);
    fail_unless(secondary_mode == TRUE,
                "'secondary-mode' property not set correctly");

    const gchar *expected_roi = "12,132,12,3";
    g_object_set(element, "roi", expected_roi, NULL);
    gchar *retrieved_roi;
    g_object_get(element, "roi", &retrieved_roi, NULL);
    fail_unless(strcmp(retrieved_roi, expected_roi) == 0,
                "ROI property mismatch: expected %s, got %s", expected_roi,
                retrieved_roi);
    g_free(retrieved_roi);

    // Json
    g_object_set(element, "config-file-path",
                 "./../../test_configs/preprocess_config.json", NULL);
    gchar *config_file_path;
    g_object_get(element, "config-file-path", &config_file_path, NULL);
    fail_unless(g_strcmp0(config_file_path,
                          "./../../test_configs/preprocess_config.json") == 0,
                "'config-file-path' properties not set correctly");
    g_free(config_file_path);

    g_object_get(element, "resize-width", &resize_width, "resize-height",
                 &resize_height, NULL);
    fail_unless(
        resize_width == 512 && resize_height == 512,
        "'resize-width' and 'resize-height' properties not set correctly");
    g_object_get(element, "preprocess-id", &preprocess_id, NULL);
    fail_unless(preprocess_id == 1,
                "'preprocess-id' property not set correctly");
    g_object_get(element, "keep-ratio", &keep_ratio, NULL);
    fail_unless(keep_ratio == TRUE, "'keep-ratio' property not set correctly");
    g_object_get(element, "pad-value", &pad_value, NULL);
    fail_unless(pad_value == 114, "'pad-value' property not set correctly");
    g_object_get(element, "interval", &interval, NULL);
    fail_unless(interval == 0, "'interval' property not set correctly");

    gst_object_unref(element);
}
GST_END_TEST

int _interval = 0;
static GstPadProbeReturn probe_primary(GstPad *pad, GstPadProbeInfo *info,
                                       gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    buffer = gst_buffer_ref(buffer);
    GstMeta *meta;
    gpointer state = NULL;

    GstClockTime current_pts = GST_BUFFER_PTS(buffer);
    fail_unless(current_pts != GST_CLOCK_TIME_NONE, "Buffer has no PTS.");

    _interval++;
    while ((meta = gst_buffer_iterate_meta(buffer, &state))) {
        GType meta_type = meta->info->api;
        const gchar *type_name = g_type_name(meta_type);

        if (meta_type == DX_FRAME_META_API_TYPE) {
            DXFrameMeta *frame_meta = (DXFrameMeta *)meta;
            if (_interval == 3) {
                fail_unless(frame_meta->_input_tensors.find(2) !=
                                frame_meta->_input_tensors.end(),
                            "Preprocess ID Failed");
                _interval = 0;
            } else {
                fail_unless(frame_meta->_input_tensors.find(2) ==
                                frame_meta->_input_tensors.end(),
                            "Preprocess ID Exist");
            }
        }
    }
    gst_buffer_unref(buffer);
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_primary_preprocess) {
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
    g_object_set(videosrc, "num-buffers", 150, NULL);

    GstElement *jpegparse = gst_element_factory_make("jpegparse", NULL);
    fail_unless(jpegparse != NULL, "Failed to create jpegparse element");

    GstElement *jpegdec = gst_element_factory_make("jpegdec", NULL);
    fail_unless(jpegdec != NULL, "Failed to create jpegdec element");

    GstElement *preprocess = gst_element_factory_make("dxpreprocess", NULL);
    fail_unless(preprocess != NULL, "Failed to create GstDxPreprocess element");

    // Preprocess
    g_object_set(preprocess, "color-format", 1, NULL);
    g_object_set(preprocess, "resize-width", 640, "resize-height", 640, NULL);
    g_object_set(preprocess, "interval", 2, NULL);
    // g_object_set(preprocess, "keep-ratio", FALSE, NULL);
    // g_object_set(preprocess, "pad-value", 200, NULL);
    g_object_set(preprocess, "preprocess-id", 2, NULL);
    const gchar *expected_roi = "0,0,300,300";
    g_object_set(preprocess, "roi", expected_roi, NULL);

    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(fakesink != NULL, "Failed to create fakesink element");

    // ADD Elements
    gst_bin_add_many(GST_BIN(pipeline), videosrc, jpegparse, jpegdec,
                     preprocess, fakesink, NULL);

    // Link Elements
    fail_unless(gst_element_link_many(videosrc, jpegparse, jpegdec, preprocess,
                                      fakesink, NULL),
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
GST_END_TEST

Suite *dxpreprocess_suite(void) {
    Suite *s = suite_create("GstDxPreprocess");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_element_initialization);
    // tcase_add_test(tc_core, test_element_state_change);
    // tcase_add_test(tc_core, test_element_properties);

    // tcase_add_test(tc_core, test_primary_preprocess);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    int number_failed;
    Suite *s = dxpreprocess_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
