// P3.3 — dxpostprocess contract tests
// Core: dxpostprocess is an in-place transform. Loads postprocessing library via dlopen,
// calls _postproc_function to convert tensors → object detection results.
// Incomplete or invalid library/function → READY failure.
// No frame_meta → passthrough.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static std::string POSTPROC_LIB_PATH() {
    return dxtest::resolve_lib_path("libpostprocess_yolov5s_6.so");
}

static gboolean postproc_lib_available(void) {
    return g_file_test(POSTPROC_LIB_PATH().c_str(), G_FILE_TEST_EXISTS);
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    gchar *lib = nullptr, *func = nullptr;
    guint iid = 999;
    gboolean sm = TRUE;

    g_object_get(e, "library-file-path", &lib, "function-name", &func,
                 "inference-id", &iid, "secondary-mode", &sm, nullptr);
    fail_unless(lib == nullptr);
    fail_unless(func == nullptr);
    fail_unless_equals_int(iid, 0);
    fail_unless(sm == FALSE);
    g_free(lib);
    g_free(func);

    g_object_set(e, "library-file-path", "/tmp/test.so",
                 "function-name", "MyFunc",
                 "inference-id", 5u, "secondary-mode", TRUE, nullptr);
    g_object_get(e, "library-file-path", &lib, "function-name", &func,
                 "inference-id", &iid, "secondary-mode", &sm, nullptr);
    fail_unless_equals_string(lib, "/tmp/test.so");
    fail_unless_equals_string(func, "MyFunc");
    fail_unless_equals_int(iid, 5);
    fail_unless(sm == TRUE);
    g_free(lib);
    g_free(func);

    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_postproc_bad_library_error: nonexistent library → NULL→READY failure
// Target: dxpostprocess_change_state L170-175 (dlopen fail → GST_ELEMENT_ERROR)
// MUT: remove L171-175 → null library_handle used at dlsym → crash
GST_START_TEST(CE_postproc_bad_library_error) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", "/nonexistent/lib.so",
                 "function-name", "PostProcess", nullptr);
    assert_state_fails(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_postproc_bad_function_error: valid library, invalid function name → READY failure
// Target: dxpostprocess_change_state L178-188 (dlsym fail → GST_ELEMENT_ERROR)
// MUT: remove L179-188 → null func_ptr assigned → crash on call
GST_START_TEST(CE_postproc_bad_function_error) {
    if (!postproc_lib_available()) return;
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "NonExistentFunction_XYZ", nullptr);
    assert_state_fails(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_postproc_missing_library_error: function without library → READY failure
// Target: dxpostprocess_change_state (required library/function validation)
GST_START_TEST(CE_postproc_missing_library_error) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "function-name", "PostProcess", nullptr);
    assert_state_fails(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_postproc_missing_function_error: library without function → READY failure
// Target: dxpostprocess_change_state (required library/function validation)
GST_START_TEST(CE_postproc_missing_function_error) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", "/tmp/libpostprocess-placeholder.so", nullptr);
    assert_state_fails(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_postproc_bad_config_path_uses_properties: config-file-path load failure is non-fatal
// Target: dxpostprocess_change_state validates library/function properties, not config load status
GST_START_TEST(CE_postproc_bad_config_path_uses_properties) {
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "config-file-path", "/nonexistent/dxpostprocess.json", nullptr);
    assert_state(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_postproc_no_meta_passthrough: buffer without frame_meta → passthrough
// Target: gst_dxpostprocess_transform_ip L380-383 (frame_meta null → OK)
// MUT: remove L380-383 → null deref on frame_meta → crash
GST_START_TEST(CE_postproc_no_meta_passthrough) {
    if (!postproc_lib_available()) return;

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "PostProcess", nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h,
        "video/x-raw,format=RGB,width=4,height=4,framerate=1/1");

    GstBuffer *b = gst_buffer_new_allocate(nullptr, 4 * 4 * 3, nullptr);
    GST_BUFFER_PTS(b) = 0;
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "no-meta buffer must pass through (got %s)", gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "no-meta buffer must produce output");
    gst_buffer_unref(out);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_postproc_dlclose_reopen: READY→NULL→READY → library reload succeeds
// Target: dxpostprocess_change_state L204-209 (dlclose + null) + L166-194 (re-dlopen)
// MUT: remove L207 (_library_handle = nullptr) → second READY !_library_handle condition not met
//      → function pointer invalid (dlclosed) → crash on subsequent call
GST_START_TEST(CE_postproc_dlclose_reopen) {
    if (!postproc_lib_available()) return;
    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "PostProcess", nullptr);

    GstStateChangeReturn ret;
    ret = gst_element_set_state(e, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE, "first READY failed");

    gst_element_set_state(e, GST_STATE_NULL);

    ret = gst_element_set_state(e, GST_STATE_READY);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "second READY failed — library re-open must succeed");

    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxpostprocess_suite(void) {
    Suite *s = suite_create("dxpostprocess");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_postproc_bad_library_error);
    tcase_add_test(tc, CE_postproc_bad_function_error);
    tcase_add_test(tc, CE_postproc_missing_library_error);
    tcase_add_test(tc, CE_postproc_missing_function_error);
    tcase_add_test(tc, CE_postproc_bad_config_path_uses_properties);
    tcase_add_test(tc, CE_postproc_no_meta_passthrough);
    tcase_add_test(tc, CE_postproc_dlclose_reopen);
    return s;
}

GST_CHECK_MAIN(dxpostprocess);

