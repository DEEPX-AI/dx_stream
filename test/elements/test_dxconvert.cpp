// ---------------------------------------------------------------------------
// test_dxconvert.cpp — GStreamer Check tests for the dxconvert element
// ---------------------------------------------------------------------------
// OpenCV must be included before gstcheck.h to avoid the 'fail' macro clash.
#include <opencv2/opencv.hpp>
#include <cstring>
#include <string>
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

// ---------------------------------------------------------------------------
// Per-test state: expected output format + buffer counter
// ---------------------------------------------------------------------------
static const char *_expected_format = nullptr;
static int         _convert_bufs    = 0;

static GstPadProbeReturn convert_probe_cb(GstPad *pad, GstPadProbeInfo *info,
                                          gpointer user_data) {
    (void)info;
    (void)user_data;

    GstCaps *caps = gst_pad_get_current_caps(pad);
    fail_unless(caps != NULL, "No caps on fakesink pad");

    GstStructure *s = gst_caps_get_structure(caps, 0);
    const gchar *fmt = gst_structure_get_string(s, "format");
    fail_unless(fmt != NULL, "No format in caps");
    fail_unless_equals_string(fmt, _expected_format);

    gst_caps_unref(caps);
    _convert_bufs++;
    return GST_PAD_PROBE_OK;
}

// ---------------------------------------------------------------------------
// Generic conversion pipeline helper
//
//   videotestsrc ! capsfilter(src_fmt) ! dxconvert ! capsfilter(dst_fmt) ! fakesink
//
// Returns number of buffers that arrived at fakesink with correct format.
// ---------------------------------------------------------------------------
static int run_convert_pipeline(const char *src_format, int width, int height,
                                const char *dst_format, int num_buffers) {
    _expected_format = dst_format;
    _convert_bufs    = 0;

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("test-convert");
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, loop);

    GstElement *src      = gst_element_factory_make("videotestsrc", NULL);
    GstElement *capsf_in = gst_element_factory_make("capsfilter",   "cf-in");
    GstElement *convert  = gst_element_factory_make("dxconvert",    NULL);
    GstElement *capsf_out= gst_element_factory_make("capsfilter",   "cf-out");
    GstElement *fakesink = gst_element_factory_make("fakesink",     NULL);
    fail_unless(src && capsf_in && convert && capsf_out && fakesink);

    g_object_set(src, "num-buffers", num_buffers, NULL);

    // Input caps
    gchar *incaps_str = g_strdup_printf(
        "video/x-raw, format=%s, width=%d, height=%d, framerate=30/1",
        src_format, width, height);
    GstCaps *in_caps = gst_caps_from_string(incaps_str);
    g_object_set(capsf_in, "caps", in_caps, NULL);
    gst_caps_unref(in_caps);
    g_free(incaps_str);

    // Output caps (force target format, keep same dimensions)
    gchar *outcaps_str = g_strdup_printf(
        "video/x-raw, format=%s, width=%d, height=%d",
        dst_format, width, height);
    GstCaps *out_caps = gst_caps_from_string(outcaps_str);
    g_object_set(capsf_out, "caps", out_caps, NULL);
    gst_caps_unref(out_caps);
    g_free(outcaps_str);

    gst_bin_add_many(GST_BIN(pipeline), src, capsf_in, convert,
                     capsf_out, fakesink, NULL);
    fail_unless(gst_element_link_many(src, capsf_in, convert,
                                      capsf_out, fakesink, NULL));

    // Probe on fakesink sink pad
    GstPad *sink_pad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      convert_probe_cb, NULL, NULL);
    gst_object_unref(sink_pad);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    GstStateChangeReturn sret =
        gst_element_get_state(pipeline, NULL, NULL, 5 * GST_SECOND);
    fail_unless(sret != GST_STATE_CHANGE_FAILURE,
                "Pipeline %s->%s failed to reach PLAYING",
                src_format, dst_format);

    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    g_main_loop_unref(loop);

    return _convert_bufs;
}

// ===================================================================
// 1. Element creation
// ===================================================================
GST_START_TEST(test_dxconvert_creation) {
    GstElement *elem = gst_element_factory_make("dxconvert", NULL);
    fail_unless(elem != NULL, "Failed to create dxconvert element");
    gst_object_unref(elem);
}
GST_END_TEST

// ===================================================================
// 2. State transitions NULL → READY → PAUSED → PLAYING → NULL
// ===================================================================
GST_START_TEST(test_dxconvert_state_change) {
    GstElement *elem = gst_element_factory_make("dxconvert", NULL);
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
// 3. Pipeline: I420 → RGB
// ===================================================================
GST_START_TEST(test_dxconvert_i420_to_rgb) {
    int n = run_convert_pipeline("I420", 320, 240, "RGB", 20);
    fail_unless(n > 0, "I420→RGB: no buffers received");
}
GST_END_TEST

// ===================================================================
// 4. Pipeline: I420 → BGR
// ===================================================================
GST_START_TEST(test_dxconvert_i420_to_bgr) {
    int n = run_convert_pipeline("I420", 320, 240, "BGR", 20);
    fail_unless(n > 0, "I420→BGR: no buffers received");
}
GST_END_TEST

// ===================================================================
// 5. Pipeline: RGB → I420
// ===================================================================
GST_START_TEST(test_dxconvert_rgb_to_i420) {
    int n = run_convert_pipeline("RGB", 320, 240, "I420", 20);
    fail_unless(n > 0, "RGB→I420: no buffers received");
}
GST_END_TEST

// ===================================================================
// 6. Pipeline: I420 → NV12
// ===================================================================
GST_START_TEST(test_dxconvert_i420_to_nv12) {
    int n = run_convert_pipeline("I420", 320, 240, "NV12", 20);
    fail_unless(n > 0, "I420→NV12: no buffers received");
}
GST_END_TEST

// ===================================================================
// 7. Pipeline: RGB → BGR
// ===================================================================
GST_START_TEST(test_dxconvert_rgb_to_bgr) {
    int n = run_convert_pipeline("RGB", 320, 240, "BGR", 20);
    fail_unless(n > 0, "RGB→BGR: no buffers received");
}
GST_END_TEST

// ===================================================================
// 8. Pipeline: BGR → RGB
// ===================================================================
GST_START_TEST(test_dxconvert_bgr_to_rgb) {
    int n = run_convert_pipeline("BGR", 320, 240, "RGB", 20);
    fail_unless(n > 0, "BGR→RGB: no buffers received");
}
GST_END_TEST

// ===================================================================
// 9. Same-format passthrough: I420 → I420
//    (set_caps enables in-place, kernel is NULL)
// ===================================================================
GST_START_TEST(test_dxconvert_passthrough) {
    int n = run_convert_pipeline("I420", 320, 240, "I420", 10);
    fail_unless(n > 0, "Passthrough (I420→I420): no buffers received");
}
GST_END_TEST

// ===================================================================
// Quality-test helpers (OpenCV + appsink)
// ===================================================================

static std::string qc_resource_path(const char *filename) {
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

static cv::Mat qc_load_bgr(int w, int h) {
    std::string path = qc_resource_path("test.jpg");
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    fail_unless(!img.empty(), "Could not load test image: %s", path.c_str());
    cv::Mat out;
    cv::resize(img, out, cv::Size(w, h));
    return out;
}

/* Convert raw appsink bytes (in GStreamer fmt) to a BGR cv::Mat. */
static cv::Mat qc_raw_to_bgr(const uint8_t *data, int w, int h,
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

/* PSNR on the Y plane of the appsink output vs the Y channel of a BGR GT.
   Avoids sensitivity to chroma sub-sampling rounding (I420/NV12 output). */
static double qc_psnr_y(const uint8_t *data, int w, int h,
                         const cv::Mat &gt_bgr) {
    cv::Mat y_actual(h, w, CV_8UC1, (void *)data);
    cv::Mat gt_yuv;
    cv::cvtColor(gt_bgr, gt_yuv, cv::COLOR_BGR2YUV);
    std::vector<cv::Mat> ch;
    cv::split(gt_yuv, ch);
    return cv::PSNR(y_actual, ch[0]);
}

/* Build and run:
 *   multifilesrc → jpegdec → videoconvert → videoscale
 *     → capsfilter(src_fmt, w×h) → dxconvert
 *     → capsfilter(dst_fmt, w×h) → appsink
 *
 * Returns one GstSample (caller must gst_sample_unref), or NULL.        */
static GstSample *qc_run_convert_pipeline(const char *src_fmt,
                                           const char *dst_fmt,
                                           int w, int h) {
    std::string path = qc_resource_path("test.jpg");

    gchar *desc = g_strdup_printf(
        "multifilesrc location=\"%s\" num-buffers=1"
        " ! jpegdec"
        " ! videoscale"
        " ! video/x-raw,format=I420,width=%d,height=%d"
        " ! videoconvert"
        " ! video/x-raw,format=%s,width=%d,height=%d"
        " ! dxconvert"
        " ! video/x-raw,format=%s,width=%d,height=%d"
        " ! appsink name=sink sync=false",
        path.c_str(), w, h, src_fmt, w, h, dst_fmt, w, h);

    GError *err = nullptr;
    GstElement *pipeline = gst_parse_launch(desc, &err);
    g_free(desc);
    if (err || !pipeline) {
        if (err) {
            g_printerr("qc_run_convert_pipeline parse error: %s\n",
                       err->message);
            g_error_free(err);
        }
        fail("Failed to build convert quality pipeline");
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

/* Shared body for all quality cases.
 * packed_out=true → full BGR PSNR; false → Y-channel PSNR only.        */
static void run_quality_check(const char *src_fmt, const char *dst_fmt,
                               int w, int h, gboolean packed_out) {
    GstSample *sample = qc_run_convert_pipeline(src_fmt, dst_fmt, w, h);
    fail_unless(sample != NULL,
                "dxconvert quality %s→%s: no sample received",
                src_fmt, dst_fmt);

    GstBuffer *buf = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_READ);

    cv::Mat gt_bgr = qc_load_bgr(w, h);
    double psnr;
    if (packed_out) {
        cv::Mat actual_bgr = qc_raw_to_bgr(map.data, w, h, dst_fmt);
        psnr = cv::PSNR(gt_bgr, actual_bgr);
        g_print("  dxconvert quality %s→%s PSNR=%.2f dB\n",
                src_fmt, dst_fmt, psnr);
    } else {
        psnr = qc_psnr_y(map.data, w, h, gt_bgr);
        g_print("  dxconvert quality %s→%s Y-PSNR=%.2f dB\n",
                src_fmt, dst_fmt, psnr);
    }
    fail_unless(psnr >= 20.0,
                "dxconvert %s→%s quality too low: PSNR=%.2f dB (min 20)",
                src_fmt, dst_fmt, psnr);

    gst_buffer_unmap(buf, &map);
    gst_sample_unref(sample);
}

// ===================================================================
// Quality: all 12 cross-format conversion pairs (4 formats × 3 peers)
// ===================================================================

GST_START_TEST(test_dxconvert_quality_i420_to_rgb) {
    run_quality_check("I420", "RGB", 320, 240, TRUE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_i420_to_bgr) {
    run_quality_check("I420", "BGR", 320, 240, TRUE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_i420_to_nv12) {
    run_quality_check("I420", "NV12", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_rgb_to_i420) {
    run_quality_check("RGB", "I420", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_rgb_to_bgr) {
    run_quality_check("RGB", "BGR", 320, 240, TRUE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_rgb_to_nv12) {
    run_quality_check("RGB", "NV12", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_bgr_to_i420) {
    run_quality_check("BGR", "I420", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_bgr_to_rgb) {
    run_quality_check("BGR", "RGB", 320, 240, TRUE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_bgr_to_nv12) {
    run_quality_check("BGR", "NV12", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_nv12_to_i420) {
    run_quality_check("NV12", "I420", 320, 240, FALSE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_nv12_to_rgb) {
    run_quality_check("NV12", "RGB", 320, 240, TRUE);
}
GST_END_TEST

GST_START_TEST(test_dxconvert_quality_nv12_to_bgr) {
    run_quality_check("NV12", "BGR", 320, 240, TRUE);
}
GST_END_TEST

// ===================================================================
// Test suite
// ===================================================================
Suite *dxconvert_suite(void) {
    Suite *s  = suite_create("GstDxConvert");
    TCase *tc = tcase_create("Core");
    tcase_set_timeout(tc, 30.0);

    tcase_add_test(tc, test_dxconvert_creation);
    tcase_add_test(tc, test_dxconvert_state_change);
    tcase_add_test(tc, test_dxconvert_i420_to_rgb);
    tcase_add_test(tc, test_dxconvert_i420_to_bgr);
    tcase_add_test(tc, test_dxconvert_rgb_to_i420);
    tcase_add_test(tc, test_dxconvert_i420_to_nv12);
    tcase_add_test(tc, test_dxconvert_rgb_to_bgr);
    tcase_add_test(tc, test_dxconvert_bgr_to_rgb);
    tcase_add_test(tc, test_dxconvert_passthrough);
    tcase_add_test(tc, test_dxconvert_quality_i420_to_rgb);
    tcase_add_test(tc, test_dxconvert_quality_i420_to_bgr);
    tcase_add_test(tc, test_dxconvert_quality_i420_to_nv12);
    tcase_add_test(tc, test_dxconvert_quality_rgb_to_i420);
    tcase_add_test(tc, test_dxconvert_quality_rgb_to_bgr);
    tcase_add_test(tc, test_dxconvert_quality_rgb_to_nv12);
    tcase_add_test(tc, test_dxconvert_quality_bgr_to_i420);
    tcase_add_test(tc, test_dxconvert_quality_bgr_to_rgb);
    tcase_add_test(tc, test_dxconvert_quality_bgr_to_nv12);
    tcase_add_test(tc, test_dxconvert_quality_nv12_to_i420);
    tcase_add_test(tc, test_dxconvert_quality_nv12_to_rgb);
    tcase_add_test(tc, test_dxconvert_quality_nv12_to_bgr);

    suite_add_tcase(s, tc);
    return s;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    Suite *s    = dxconvert_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    int nfailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (nfailed == 0) ? 0 : 1;
}
