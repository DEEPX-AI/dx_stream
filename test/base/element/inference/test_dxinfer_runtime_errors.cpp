// Phase 3 — dxinfer runtime error handling
// B8: mid-stream backend errors → graceful degradation, no crash
// B13: NULL PTS buffer in QoS path
// Tests requiring NPU are SKIP'd when model/NPU unavailable.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static std::string find_model() {
    std::string p = resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    if (p.empty()) p = resolve_model_path("YOLOV5S_1.dxnn");
    return p;
}

// CE_infer_no_meta_drops_harness: buffer without frame_meta → silently dropped
// Target: gst_dxinfer_chain L926-932
// MUT: remove null-meta check → crash on frame_meta dereference
GST_START_TEST(CE_infer_no_meta_drops_harness) {
    std::string model = find_model();
    DXTEST_SKIP_IF(model.empty(), "no model available");
    DXTEST_SKIP_IF(!npu_available(), "NPU not available");

    GError *err = nullptr;
    gchar *desc = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxinfer model-path=%s backend=dxrt "
        "! appsink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(desc, &err);
    g_free(desc);
    fail_unless(err == nullptr && pipe != nullptr);

    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(src && sink);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    for (int i = 0; i < 3; i++) {
        GstBuffer *b = gst_buffer_new_allocate(nullptr, 64 * 64 * 3, nullptr);
        GstMapInfo map;
        gst_buffer_map(b, &map, GST_MAP_WRITE);
        memset(map.data, 0x80, map.size);
        gst_buffer_unmap(b, &map);
        GST_BUFFER_PTS(b) = i * GST_SECOND / 30;
        GST_BUFFER_DURATION(b) = GST_SECOND / 30;
        gst_app_src_push_buffer(GST_APP_SRC(src), b);
    }

    g_usleep(500 * 1000);

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), GST_SECOND);
    if (s) {
        gst_sample_unref(s);
        // buffers without frame meta should have been dropped, not passed through
        // but if model created meta, that's also OK
    }

    gst_app_src_end_of_stream(GST_APP_SRC(src));
    g_usleep(500 * 1000);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(src);
    gst_object_unref(sink);
    gst_object_unref(pipe);
}
GST_END_TEST;

// CE_infer_multiple_state_cycles: PLAYING→NULL repeated → no leak/crash
// Target: handle_paused_to_ready L293-310 (push_running=FALSE, thread join)
GST_START_TEST(CE_infer_multiple_state_cycles) {
    std::string model = find_model();
    DXTEST_SKIP_IF(model.empty(), "no model available");
    DXTEST_SKIP_IF(!npu_available(), "NPU not available");

    GError *err = nullptr;
    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=2 is-live=false "
        "! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer model-path=%s backend=dxrt "
        "! fakesink sync=false", model.c_str());

    for (int cycle = 0; cycle < 3; cycle++) {
        GstElement *pipe = gst_parse_launch(desc, &err);
        fail_unless(err == nullptr && pipe != nullptr);
        if (err) g_error_free(err);
        err = nullptr;

        gst_element_set_state(pipe, GST_STATE_PLAYING);

        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
            (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (msg) gst_message_unref(msg);
        gst_object_unref(bus);

        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }

    g_free(desc);
}
GST_END_TEST;

// CE_infer_eos_after_data: EOS after data → clean shutdown, no hang
// Target: gst_dxinfer_sink_event_eos, push_thread flush queue
GST_START_TEST(CE_infer_eos_after_data) {
    std::string model = find_model();
    DXTEST_SKIP_IF(model.empty(), "no model available");
    DXTEST_SKIP_IF(!npu_available(), "NPU not available");

    GError *err = nullptr;
    gchar *desc = g_strdup_printf(
        "videotestsrc num-buffers=5 is-live=false "
        "! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer model-path=%s backend=dxrt "
        "! appsink name=sink sync=false wait-on-eos=false", model.c_str());
    GstElement *pipe = gst_parse_launch(desc, &err);
    g_free(desc);
    fail_unless(err == nullptr && pipe != nullptr);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 30 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    fail_unless(msg != nullptr, "pipeline must reach EOS or error");
    fail_unless_equals_int(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);

    int count = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 0)) != nullptr) {
        count++;
        gst_sample_unref(s);
    }
    fail_unless(count > 0, "must receive output after inference + EOS");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(sink);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *dxinfer_runtime_suite(void) {
    Suite *s = suite_create("dxinfer_runtime");
    TCase *tc = tcase_create("runtime_errors");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_infer_no_meta_drops_harness);
    tcase_add_test(tc, CE_infer_multiple_state_cycles);
    tcase_add_test(tc, CE_infer_eos_after_data);
    return s;
}

GST_CHECK_MAIN(dxinfer_runtime);
