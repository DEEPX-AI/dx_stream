// P3.2 — dxinfer contract tests
// Core: dxinfer directly inherits GstElement. Performs inference via backend Put/Get in chain function.
// Delivers output asynchronously via push_thread. model-path required (missing → READY failure).
// Buffers without frame_meta are dropped.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static std::string resolve_test_model() {
    std::string p = resolve_model_path("YoloV5S_PPU.dxnn");
    if (p.empty()) p = resolve_model_path("YOLOV5S_1.dxnn");
    return p;
}

static gboolean model_available(void) {
    return !resolve_test_model().empty();
}

static void assert_pipeline_error_contains(const gchar *launch, const gchar *needle) {
    GError *parse_error = nullptr;
    GstElement *pipe = gst_parse_launch(launch, &parse_error);
    fail_unless(parse_error == nullptr && pipe != nullptr,
                "pipeline parse failed: %s", parse_error ? parse_error->message : "unknown");

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PAUSED);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND, GST_MESSAGE_ERROR);
    fail_unless(msg != nullptr, "expected pipeline error containing '%s'", needle);

    GError *error = nullptr;
    gchar *debug = nullptr;
    gst_message_parse_error(msg, &error, &debug);
    fail_unless(error != nullptr && g_strrstr(error->message, needle) != nullptr,
                "expected error message containing '%s', got '%s'", needle,
                error ? error->message : "<null>");
    g_clear_error(&error);
    g_free(debug);
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    gchar *mp = nullptr;
    guint pid = 999, iid = 999;
    gboolean sm = TRUE, ort = FALSE;

    g_object_get(e, "model-path", &mp, "preprocess-id", &pid,
                 "inference-id", &iid, "secondary-mode", &sm,
                 "use-ort", &ort, nullptr);
    fail_unless(mp == nullptr);
    fail_unless_equals_int(pid, 0);
    fail_unless_equals_int(iid, 0);
    fail_unless(sm == FALSE);
    fail_unless(ort == TRUE);
    g_free(mp);

    g_object_set(e, "model-path", "/tmp/test.dxnn",
                 "preprocess-id", 2u, "inference-id", 3u,
                 "secondary-mode", TRUE, "use-ort", FALSE, nullptr);
    g_object_get(e, "model-path", &mp, "preprocess-id", &pid,
                 "inference-id", &iid, "secondary-mode", &sm,
                 "use-ort", &ort, nullptr);
    fail_unless_equals_string(mp, "/tmp/test.dxnn");
    fail_unless_equals_int(pid, 2);
    fail_unless_equals_int(iid, 3);
    fail_unless(sm == TRUE);
    fail_unless(ort == FALSE);
    g_free(mp);

    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_infer_no_model_error: model-path not set → NULL→READY failure
// Target: handle_null_to_ready L224-229 (model_path == nullptr → GST_ELEMENT_ERROR)
// MUT: remove L225-229 → null deref on model_path at L232
GST_START_TEST(CE_infer_no_model_error) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    assert_state_fails(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_infer_empty_model_error: empty model-path → NULL→READY failure before backend init
// Target: handle_null_to_ready validates required model-path property value
GST_START_TEST(CE_infer_empty_model_error) {
    assert_pipeline_error_contains(
        "fakesrc num-buffers=0 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=1/1 "
        "! dxinfer model-path=\"\" "
        "! fakesink",
        "model-path property is required");
}
GST_END_TEST;

// CE_infer_bad_model_error: nonexistent model-path → NULL→READY failure before backend init
// Target: handle_null_to_ready validates model-path file existence
// MUT: remove file check → backend Init() reports generic backend failure
GST_START_TEST(CE_infer_bad_model_error) {
    assert_pipeline_error_contains(
        "fakesrc num-buffers=0 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=1/1 "
        "! dxinfer model-path=/nonexistent/model.dxnn "
        "! fakesink",
        "model-path file does not exist");
}
GST_END_TEST;

// CE_infer_bad_config_path_uses_properties: config-file-path load failure is non-fatal
// Target: handle_null_to_ready should validate model-path, not config load status
GST_START_TEST(CE_infer_bad_config_path_uses_properties) {
    if (!model_available()) return;
    std::string model = resolve_test_model();
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    g_object_set(e, "config-file-path", "/nonexistent/dxinfer.json",
                 "model-path", model.c_str(), nullptr);
    gst_util_set_object_arg(G_OBJECT(e), "backend", "dxrt");
    assert_state(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_infer_thread_join: valid model full state cycle → push thread join succeeds
// Target: handle_paused_to_ready L293-310 (push_running=FALSE, thread join)
// MUT: remove L305-307 (g_thread_join) → thread leak, use-after-free
GST_START_TEST(CE_infer_thread_join) {
    if (!model_available()) return;
    std::string model = resolve_test_model();
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    g_object_set(e, "model-path", model.c_str(), nullptr);
    gst_util_set_object_arg(G_OBJECT(e), "backend", "dxrt");
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_infer_no_meta_drops: buffer without frame_meta → drop (no crash)
// Target: gst_dxinfer_chain L928-932 (frame_meta == null → unref + return OK)
// MUT: removing L928-932 → null deref at L936 → crash
GST_START_TEST(CE_infer_no_meta_drops) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxinfer model-path=%s backend=dxrt "
        "! fakesink", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    fail_unless(err == nullptr && pipe != nullptr);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 15 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    fail_unless(msg != nullptr, "expected EOS (no-meta buffers silently dropped)");
    fail_unless_equals_int(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);

    gst_message_unref(msg);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

// CE_infer_output_tensors: after inference, result stored in _output_tensors[0]
// Target: primary_mode_infer L881-886 (allocate + Put) + push_thread L700-720 (Get)
// MUT: remove L881-882 → _output_tensors empty → fail
GST_START_TEST(CE_infer_output_tensors) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "videotestsrc num-buffers=1 "
        "! video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer model-path=%s backend=dxrt "
        "! appsink name=sink", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    fail_unless(err == nullptr && pipe != nullptr);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    fail_unless(sample != nullptr, "must receive at least 1 sample after inference");

    GstBuffer *buf = gst_sample_get_buffer(sample);
    fail_unless(buf != nullptr);

    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fail_unless(fm != nullptr, "output must have DXFrameMeta");
    fail_unless(fm->_output_tensors.find(0) != fm->_output_tensors.end(),
                "output_tensors[0] must exist after inference");

    const auto &ot = fm->_output_tensors[0];
    fail_unless(ot.data_ptr() != nullptr, "output tensor data must be allocated");
    fail_unless(ot._tensors.size() > 0,
                "output tensor must have at least 1 tensor entry");

    gst_sample_unref(sample);
    gst_object_unref(sink);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *dxinfer_suite(void) {
    Suite *s = suite_create("dxinfer");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CE_infer_no_model_error);
    tcase_add_test(tc, CE_infer_empty_model_error);
    tcase_add_test(tc, CE_infer_bad_model_error);
    tcase_add_test(tc, CE_infer_bad_config_path_uses_properties);
    tcase_add_test(tc, CE_infer_thread_join);
    tcase_add_test(tc, CE_infer_no_meta_drops);
    tcase_add_test(tc, CE_infer_output_tensors);
    return s;
}

GST_CHECK_MAIN(dxinfer);
