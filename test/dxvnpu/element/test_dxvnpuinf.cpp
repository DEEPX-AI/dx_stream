#include <gst/app/gstappsink.h>
#include <gst/check/gstcheck.h>

#include "meta_helpers.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static std::string resolve_test_model() {
    std::string p = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    if (p.empty()) p = resolve_model_path("YOLOV5S_1.dxnn");
    return p;
}

static gboolean require_device() {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") == 0) return TRUE;
    g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
    return FALSE;
}

GST_START_TEST(CE_infer_dxvnpu_async_fifo) {
    if (!require_device() || resolve_test_model().empty()) return;
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "videotestsrc num-buffers=8 "
        "! video/x-raw,format=NV12,width=640,height=640,framerate=30/1 "
        "! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 "
        "! dxinfer preprocess-id=1 inference-id=1 model-path=%s "
        "backend=dxvnpu use-ort=false "
        "! appsink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    fail_unless(err == nullptr && pipe != nullptr);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    for (guint i = 0; i < 8; ++i) {
        GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        fail_unless(sample != nullptr, "frame %u must be emitted after inference", i);
        GstBuffer *buf = gst_sample_get_buffer(sample);
        fail_unless_equals_uint64(GST_BUFFER_PTS(buf), i * GST_SECOND / 30);

        DXFrameMeta *fm = dx_get_frame_meta(buf);
        fail_unless(fm != nullptr, "frame %u must retain DXFrameMeta", i);
        auto output = fm->_output_tensors.find(1);
        fail_unless(output != fm->_output_tensors.end(),
                    "frame %u must contain inference-id 1 output", i);
        fail_unless(output->second.data_ptr() != nullptr,
                    "frame %u output data must be allocated", i);
        fail_unless(!output->second._tensors.empty(),
                    "frame %u must expose at least one output tensor", i);
        gst_sample_unref(sample);
    }

    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    fail_unless(msg != nullptr && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "async inference pipeline must finish with EOS");
    gst_message_unref(msg);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(sink);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *dxvnpuinf_suite() {
    Suite *suite = suite_create("dxvnpuinf");
    TCase *testcase = tcase_create("acceptance");
    tcase_set_timeout(testcase, 30.0);
    suite_add_tcase(suite, testcase);
    tcase_add_test(testcase, CE_infer_dxvnpu_async_fifo);
    return suite;
}

GST_CHECK_MAIN(dxvnpuinf);
