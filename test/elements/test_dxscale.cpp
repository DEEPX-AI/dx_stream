// ---------------------------------------------------------------------------
// test_dxscale.cpp — GStreamer Check tests for the dxscale element
// ---------------------------------------------------------------------------
// OpenCV must be included before gstcheck.h to avoid the 'fail' macro clash.
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <unistd.h>
#include <gst/app/gstappsink.h>
#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/video/video.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    (void)bus;
    GMainLoop *loop = (GMainLoop *)data;
    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
        g_main_loop_quit(loop);
        break;
    case GST_MESSAGE_ERROR: {
        gchar *debug = nullptr;
        GError *err  = nullptr;
        gst_message_parse_error(msg, &err, &debug);
        g_printerr("ERROR: %s\n%s\n", err->message, debug ? debug : "");
        g_clear_error(&err);
        g_free(debug);
        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

// ===================================================================
// 1. Element creation
// ===================================================================
GST_START_TEST(test_dxscale_creation) {
    GstElement *elem = gst_element_factory_make("dxscale", NULL);
    fail_unless(elem != NULL, "Failed to create dxscale element");
    gst_object_unref(elem);
}
GST_END_TEST

// ===================================================================
// 2. State transitions NULL → READY → PAUSED → PLAYING → NULL
// ===================================================================
GST_START_TEST(test_dxscale_state_change) {
    GstElement *elem = gst_element_factory_make("dxscale", NULL);
    fail_unless(elem != NULL);

    GstStateChangeReturn ret;
    GstState cur, pending;

    ret = gst_element_set_state(elem, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE);
    gst_element_get_state(elem, &cur, &pending, GST_CLOCK_TIME_NONE);
    fail_unless_equals_int(cur, GST_STATE_READY);

    ret = gst_element_set_state(elem, GST_STATE_PAUSED);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE);
    gst_element_get_state(elem, &cur, &pending, GST_CLOCK_TIME_NONE);
    fail_unless_equals_int(cur, GST_STATE_PAUSED);

    ret = gst_element_set_state(elem, GST_STATE_PLAYING);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE);
    gst_element_get_state(elem, &cur, &pending, GST_CLOCK_TIME_NONE);
    fail_unless_equals_int(cur, GST_STATE_PLAYING);

    gst_element_set_state(elem, GST_STATE_NULL);
    gst_object_unref(elem);
}
GST_END_TEST

// ===================================================================
// 3. Property get/set
// ===================================================================
GST_START_TEST(test_dxscale_properties) {
    GstElement *elem = gst_element_factory_make("dxscale", NULL);
    fail_unless(elem != NULL);

    // Defaults
    guint w = 999, h = 999;
    g_object_get(elem, "width", &w, "height", &h, NULL);
    fail_unless_equals_int(w, 0);
    fail_unless_equals_int(h, 0);

    // Set
    g_object_set(elem, "width", 320u, "height", 240u, NULL);
    g_object_get(elem, "width", &w, "height", &h, NULL);
    fail_unless_equals_int(w, 320);
    fail_unless_equals_int(h, 240);

    gst_object_unref(elem);
}
GST_END_TEST

// ===================================================================
// 4. Pipeline: I420 640×480 → dxscale 320×240 → fakesink
//    Verify output dimensions via pad probe on fakesink sink pad.
// ===================================================================
static int _scale_buffers_received = 0;

static GstPadProbeReturn scale_probe_cb(GstPad *pad, GstPadProbeInfo *info,
                                        gpointer user_data) {
    (void)info;
    (void)user_data;

    GstCaps *caps = gst_pad_get_current_caps(pad);
    fail_unless(caps != NULL, "No caps on fakesink pad");

    GstStructure *s = gst_caps_get_structure(caps, 0);
    gint w = 0, h = 0;
    gst_structure_get_int(s, "width", &w);
    gst_structure_get_int(s, "height", &h);

    fail_unless_equals_int(w, 320);
    fail_unless_equals_int(h, 240);

    gst_caps_unref(caps);
    _scale_buffers_received++;
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_dxscale_pipeline_i420) {
    _scale_buffers_received = 0;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-scale-i420");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *src      = gst_element_factory_make("videotestsrc", NULL);
    GstElement *capsf    = gst_element_factory_make("capsfilter", NULL);
    GstElement *scale    = gst_element_factory_make("dxscale", NULL);
    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(src && capsf && scale && fakesink);

    g_object_set(src, "num-buffers", 30, NULL);
    GstCaps *in_caps = gst_caps_from_string(
        "video/x-raw, format=I420, width=640, height=480, framerate=30/1");
    g_object_set(capsf, "caps", in_caps, NULL);
    gst_caps_unref(in_caps);

    g_object_set(scale, "width", 320u, "height", 240u, NULL);

    gst_bin_add_many(GST_BIN(pipeline), src, capsf, scale, fakesink, NULL);
    fail_unless(gst_element_link_many(src, capsf, scale, fakesink, NULL));

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      scale_probe_cb, NULL, NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn sret =
        gst_element_get_state(pipeline, NULL, NULL, 5 * GST_SECOND);
    fail_unless(sret != GST_STATE_CHANGE_FAILURE,
                "Pipeline failed to reach PLAYING");

    g_main_loop_run(loop);

    fail_unless(_scale_buffers_received > 0,
                "No buffers received at fakesink");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
GST_END_TEST

// ===================================================================
// 5. Pipeline: RGB 320×240 → dxscale 160×120 → fakesink
// ===================================================================
static int _scale_rgb_buffers = 0;

static GstPadProbeReturn scale_rgb_probe_cb(GstPad *pad, GstPadProbeInfo *info,
                                            gpointer user_data) {
    (void)info;
    (void)user_data;

    GstCaps *caps = gst_pad_get_current_caps(pad);
    fail_unless(caps != NULL);
    GstStructure *s = gst_caps_get_structure(caps, 0);
    gint w = 0, h = 0;
    gst_structure_get_int(s, "width", &w);
    gst_structure_get_int(s, "height", &h);
    fail_unless_equals_int(w, 160);
    fail_unless_equals_int(h, 120);
    gst_caps_unref(caps);
    _scale_rgb_buffers++;
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_dxscale_pipeline_rgb) {
    _scale_rgb_buffers = 0;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-scale-rgb");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *src      = gst_element_factory_make("videotestsrc", NULL);
    GstElement *capsf    = gst_element_factory_make("capsfilter", NULL);
    GstElement *scale    = gst_element_factory_make("dxscale", NULL);
    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(src && capsf && scale && fakesink);

    g_object_set(src, "num-buffers", 20, NULL);
    GstCaps *in_caps = gst_caps_from_string(
        "video/x-raw, format=RGB, width=320, height=240, framerate=30/1");
    g_object_set(capsf, "caps", in_caps, NULL);
    gst_caps_unref(in_caps);

    g_object_set(scale, "width", 160u, "height", 120u, NULL);

    gst_bin_add_many(GST_BIN(pipeline), src, capsf, scale, fakesink, NULL);
    fail_unless(gst_element_link_many(src, capsf, scale, fakesink, NULL));

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      scale_rgb_probe_cb, NULL, NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn sret =
        gst_element_get_state(pipeline, NULL, NULL, 5 * GST_SECOND);
    fail_unless(sret != GST_STATE_CHANGE_FAILURE);

    g_main_loop_run(loop);
    fail_unless(_scale_rgb_buffers > 0, "No RGB buffers received");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
GST_END_TEST

// ===================================================================
// 6. Passthrough: same size (width=0, height=0) — buffers flow through
// ===================================================================
static int _passthrough_bufs = 0;

static GstPadProbeReturn passthrough_probe_cb(GstPad *pad,
                                              GstPadProbeInfo *info,
                                              gpointer user_data) {
    (void)pad;
    (void)info;
    (void)user_data;
    _passthrough_bufs++;
    return GST_PAD_PROBE_OK;
}

GST_START_TEST(test_dxscale_passthrough) {
    _passthrough_bufs = 0;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-scale-passthrough");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *src      = gst_element_factory_make("videotestsrc", NULL);
    GstElement *capsf    = gst_element_factory_make("capsfilter", NULL);
    GstElement *scale    = gst_element_factory_make("dxscale", NULL);
    GstElement *fakesink = gst_element_factory_make("fakesink", NULL);
    fail_unless(src && capsf && scale && fakesink);

    g_object_set(src, "num-buffers", 10, NULL);
    GstCaps *in_caps = gst_caps_from_string(
        "video/x-raw, format=I420, width=320, height=240, framerate=30/1");
    g_object_set(capsf, "caps", in_caps, NULL);
    gst_caps_unref(in_caps);

    // width=0, height=0 → passthrough (no resize)
    // dxscale transform_caps won't fix output size → in/out dimensions match

    gst_bin_add_many(GST_BIN(pipeline), src, capsf, scale, fakesink, NULL);
    fail_unless(gst_element_link_many(src, capsf, scale, fakesink, NULL));

    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      passthrough_probe_cb, NULL, NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn sret =
        gst_element_get_state(pipeline, NULL, NULL, 5 * GST_SECOND);
    fail_unless(sret != GST_STATE_CHANGE_FAILURE);

    g_main_loop_run(loop);
    fail_unless(_passthrough_bufs > 0, "No buffers in passthrough mode");

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);
}
GST_END_TEST

// ===================================================================
// Quality-test helpers (OpenCV + appsink)
// ===================================================================

/* Resolve the path to a file in test/test_resources/ relative to the
   running binary (expected to live in test/elements/bin/).              */
static std::string q_resource_path(const char *filename) {
    char buf[4096] = {};
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    std::string dir;
    if (n > 0) {
        buf[n] = '\0';
        std::string exe(buf);
        dir = exe.substr(0, exe.rfind('/'));   // .../test/elements/bin
        dir += "/../../test_resources";        // .../test/test_resources
    } else {
        dir = "../test_resources";
    }
    return dir + "/" + filename;
}

/* Load 1.jpg and resize to (w, h) in BGR. Aborts the test if missing. */
static cv::Mat q_load_bgr(int w, int h) {
    std::string path = q_resource_path("1.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    fail_unless(!img.empty(), "Could not load test image: %s", path.c_str());
    cv::Mat out;
    cv::resize(img, out, cv::Size(w, h));
    return out;
}

/* Convert raw appsink bytes to a BGR cv::Mat for comparison. */
static cv::Mat q_raw_to_bgr(const uint8_t *data, int w, int h,
                              const char *fmt) {
    cv::Mat src, bgr;
    if (strcmp(fmt, "RGB") == 0) {
        src = cv::Mat(h, w, CV_8UC3, (void *)data).clone();
        cv::cvtColor(src, bgr, cv::COLOR_RGB2BGR);
    } else if (strcmp(fmt, "BGR") == 0) {
        bgr = cv::Mat(h, w, CV_8UC3, (void *)data).clone();
    } else if (strcmp(fmt, "I420") == 0) {
        src = cv::Mat(h * 3 / 2, w, CV_8UC1, (void *)data);
        cv::cvtColor(src, bgr, cv::COLOR_YUV2BGR_I420);
    } else if (strcmp(fmt, "NV12") == 0) {
        src = cv::Mat(h * 3 / 2, w, CV_8UC1, (void *)data);
        cv::cvtColor(src, bgr, cv::COLOR_YUV2BGR_NV12);
    }
    return bgr;
}

/* PSNR between the Y plane of the appsink output and the Y channel of a BGR GT.
   Used for YUV formats where chroma sub-sampling rounding creates minor diffs. */
static double q_psnr_y(const uint8_t *data, int w, int h,
                        const cv::Mat &gt_bgr) {
    cv::Mat y_actual(h, w, CV_8UC1, (void *)data);  // Y plane is always first
    cv::Mat gt_yuv;
    cv::cvtColor(gt_bgr, gt_yuv, cv::COLOR_BGR2YUV);
    std::vector<cv::Mat> ch;
    cv::split(gt_yuv, ch);
    return cv::PSNR(y_actual, ch[0]);
}

/* Build and run the pipeline:
 *   multifilesrc → jpegdec → videoconvert → videoscale
 *     → capsfilter(fmt, in_w×in_h) → dxscale(out_w×out_h) → appsink
 *
 * Returns one pulled GstSample (caller must gst_sample_unref it),
 * or NULL on failure.                                                   */
static GstSample *q_run_scale_pipeline(const char *fmt,
                                        int in_w, int in_h,
                                        int out_w, int out_h) {
    std::string path = q_resource_path("1.jpg");

    gchar *desc = g_strdup_printf(
        "multifilesrc location=\"%s\" num-buffers=1"
        " ! jpegdec"
        " ! videoscale"
        " ! video/x-raw,format=I420,width=%d,height=%d"
        " ! videoconvert"
        " ! video/x-raw,format=%s,width=%d,height=%d"
        " ! dxscale width=%d height=%d"
        " ! appsink name=sink sync=false",
        path.c_str(), in_w, in_h, fmt, in_w, in_h, out_w, out_h);

    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(desc, &err);
    g_free(desc);
    if (err || !pipeline) {
        if (err) {
            g_printerr("q_run_scale_pipeline parse error: %s\n", err->message);
            g_error_free(err);
        }
        fail("Failed to build scale quality pipeline");
        return nullptr;
    }

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    fail_unless(sink != NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    GstSample *sample =
        gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 10 * GST_SECOND);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(sink);
    gst_object_unref(pipeline);
    return sample;
}

// ===================================================================
// 7. Quality: RGB 640×480 → dxscale 320×240, PSNR vs OpenCV GT ≥ 20 dB
// ===================================================================
GST_START_TEST(test_dxscale_quality_rgb) {
    GstSample *sample = q_run_scale_pipeline("RGB", 640, 480, 320, 240);
    fail_unless(sample != NULL, "dxscale quality RGB: no sample received");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    cv::Mat gt_bgr = q_load_bgr(320, 240);
    cv::Mat actual_bgr = q_raw_to_bgr(map.data, 320, 240, "RGB");
    double psnr = cv::PSNR(gt_bgr, actual_bgr);
    g_print("  dxscale quality RGB PSNR=%.2f dB\n", psnr);
    fail_unless(psnr >= 20.0,
                "dxscale RGB quality too low: PSNR=%.2f dB (min 20)", psnr);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
}
GST_END_TEST

// ===================================================================
// 8. Quality: BGR 640×480 → dxscale 320×240, PSNR ≥ 20 dB
// ===================================================================
GST_START_TEST(test_dxscale_quality_bgr) {
    GstSample *sample = q_run_scale_pipeline("BGR", 640, 480, 320, 240);
    fail_unless(sample != NULL, "dxscale quality BGR: no sample received");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    cv::Mat gt_bgr = q_load_bgr(320, 240);
    cv::Mat actual_bgr = q_raw_to_bgr(map.data, 320, 240, "BGR");
    double psnr = cv::PSNR(gt_bgr, actual_bgr);
    g_print("  dxscale quality BGR PSNR=%.2f dB\n", psnr);
    fail_unless(psnr >= 20.0,
                "dxscale BGR quality too low: PSNR=%.2f dB (min 20)", psnr);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
}
GST_END_TEST

// ===================================================================
// 9. Quality: I420 640×480 → dxscale 320×240, Y-channel PSNR ≥ 20 dB
// ===================================================================
GST_START_TEST(test_dxscale_quality_i420) {
    GstSample *sample = q_run_scale_pipeline("I420", 640, 480, 320, 240);
    fail_unless(sample != NULL, "dxscale quality I420: no sample received");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    cv::Mat gt_bgr = q_load_bgr(320, 240);
    double psnr = q_psnr_y(map.data, 320, 240, gt_bgr);
    g_print("  dxscale quality I420 Y-PSNR=%.2f dB\n", psnr);
    fail_unless(psnr >= 20.0,
                "dxscale I420 quality too low: Y-PSNR=%.2f dB (min 20)", psnr);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
}
GST_END_TEST

// ===================================================================
// 10. Quality: NV12 640×480 → dxscale 320×240, Y-channel PSNR ≥ 20 dB
// ===================================================================
GST_START_TEST(test_dxscale_quality_nv12) {
    GstSample *sample = q_run_scale_pipeline("NV12", 640, 480, 320, 240);
    fail_unless(sample != NULL, "dxscale quality NV12: no sample received");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    cv::Mat gt_bgr = q_load_bgr(320, 240);
    double psnr = q_psnr_y(map.data, 320, 240, gt_bgr);
    g_print("  dxscale quality NV12 Y-PSNR=%.2f dB\n", psnr);
    fail_unless(psnr >= 20.0,
                "dxscale NV12 quality too low: Y-PSNR=%.2f dB (min 20)", psnr);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
}
GST_END_TEST

// ===================================================================
// Test suite
// ===================================================================
Suite *dxscale_suite(void) {
    Suite *s   = suite_create("GstDxScale");
    TCase *tc  = tcase_create("Core");
    tcase_set_timeout(tc, 30.0);

    tcase_add_test(tc, test_dxscale_creation);
    tcase_add_test(tc, test_dxscale_state_change);
    tcase_add_test(tc, test_dxscale_properties);
    tcase_add_test(tc, test_dxscale_pipeline_i420);
    tcase_add_test(tc, test_dxscale_pipeline_rgb);
    tcase_add_test(tc, test_dxscale_passthrough);
    tcase_add_test(tc, test_dxscale_quality_rgb);
    tcase_add_test(tc, test_dxscale_quality_bgr);
    tcase_add_test(tc, test_dxscale_quality_i420);
    tcase_add_test(tc, test_dxscale_quality_nv12);

    suite_add_tcase(s, tc);
    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    Suite *s      = dxscale_suite();
    SRunner *sr   = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int nfailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (nfailed == 0) ? 0 : 1;
}
