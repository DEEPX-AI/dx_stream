// Phase 5 — dxinfer secondary mode verification
// B17: secondary-mode=true → no push thread, synchronous inference
// Tests property, state behavior, and chain function secondary path

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <glib/gstdio.h>
#include <cstring>
#ifdef _WIN32
#include <io.h>
#define write _write
#define close _close
#else
#include <unistd.h>
#endif

using namespace dxtest;

static const char *CAPS_RGB G_GNUC_UNUSED =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

// CE_infer_secondary_property: secondary-mode property set/get works
// Target: gst_dxinfer class_init L380-381 (property registration)
GST_START_TEST(CE_infer_secondary_property) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    fail_unless(e != nullptr, "dxinfer must be registered");

    gboolean sec = TRUE;
    g_object_get(e, "secondary-mode", &sec, nullptr);
    fail_unless(sec == FALSE, "secondary-mode default must be FALSE");

    g_object_set(e, "secondary-mode", TRUE, nullptr);
    g_object_get(e, "secondary-mode", &sec, nullptr);
    fail_unless(sec == TRUE, "secondary-mode must be settable to TRUE");

    gst_object_unref(e);
}
GST_END_TEST;

// CE_infer_secondary_config_json: config-file-path with secondary_mode=true
// Target: gst_dxinfer load_config L99-101
GST_START_TEST(CE_infer_secondary_config_json) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);

    gchar *path = g_build_filename(g_get_tmp_dir(), "dxtest_infer_XXXXXX.json", NULL);
    int fd = g_mkstemp(path);
    fail_unless(fd >= 0, "g_mkstemp failed");
    const char *json =
        "{\n"
        "  \"model_path\": \"/tmp/dummy.dxnn\",\n"
        "  \"preprocess_id\": 1,\n"
        "  \"inference_id\": 2,\n"
        "  \"secondary_mode\": true\n"
        "}\n";
    gsize len = strlen(json);
    gsize written = 0;
    while (written < len) {
        gssize n = write(fd, json + written, len - written);
        fail_unless(n > 0, "write to temp file failed");
        written += (gsize)n;
    }
    close(fd);

    g_object_set(e, "config-file-path", path, nullptr);

    gboolean sec = FALSE;
    g_object_get(e, "secondary-mode", &sec, nullptr);
    fail_unless(sec == TRUE, "secondary-mode must be TRUE from config JSON");

    gchar *model = nullptr;
    g_object_get(e, "model-path", &model, nullptr);
    fail_unless_equals_string(model, "/tmp/dummy.dxnn");

    guint preproc_id = 0, infer_id = 0;
    g_object_get(e, "preprocess-id", &preproc_id,
                 "inference-id", &infer_id, nullptr);
    fail_unless_equals_int(preproc_id, 1);
    fail_unless_equals_int(infer_id, 2);

    g_free(model);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

// CE_infer_secondary_no_push_thread: secondary mode → PAUSED without push thread crash
// Target: handle_ready_to_paused L276 (if !_secondary_mode → start push thread)
// MUT: remove _secondary_mode check → thread created → crash on secondary_mode_infer
GST_START_TEST(CE_infer_secondary_no_push_thread) {
    DXTEST_SKIP_IF(!dxtest::npu_available(), "NPU not available");

    std::string model = dxtest::resolve_model_path("yolov5-s_640x640_ppu.dxnn");
    DXTEST_SKIP_IF(model.empty(), "model not found");

    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    g_object_set(e,
        "model-path", model.c_str(),
        "backend", 0,
        "secondary-mode", TRUE,
        nullptr);

    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "READY with secondary-mode must succeed");

    ret = gst_element_set_state(e, GST_STATE_PAUSED);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "PAUSED with secondary-mode must succeed (no push thread)");

    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxinfer_secondary_suite(void) {
    Suite *s = suite_create("dxinfer_secondary");
    TCase *tc = tcase_create("secondary_mode");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_infer_secondary_property);
    tcase_add_test(tc, CE_infer_secondary_config_json);
    tcase_add_test(tc, CE_infer_secondary_no_push_thread);
    return s;
}

GST_CHECK_MAIN(dxinfer_secondary);
