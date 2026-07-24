// Phase 3 — dxpostprocess runtime edge cases
// B8: postprocess exception handling in both primary and secondary mode
// Tests: no output_tensors for infer_id → skip, meta with objects but no tensors → skip

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB =
    "video/x-raw,format=RGB,width=4,height=4,framerate=1/1";
static const guint BUF_SIZE = 4 * 4 * 3;

static std::string POSTPROC_LIB_PATH() {
    return dxtest::resolve_lib_path("libpostprocess_ppu.so");
}

static gboolean lib_available() {
    return g_file_test(POSTPROC_LIB_PATH().c_str(), G_FILE_TEST_EXISTS);
}

// CE_postproc_no_tensors_primary: frame meta with no output_tensors → passthrough
// Target: gst_dxpostprocess_transform_ip L396 (iter == end → skip processing)
// MUT: remove L396 check → undefined access on empty map → crash
GST_START_TEST(CE_postproc_no_tensors_primary) {
    DXTEST_SKIP_IF(!lib_available(), "postprocess library not found");

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "YOLOV5S_PPU",
                 "inference-id", 1u, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_RGB);

    GstBuffer *b = gst_buffer_new_allocate(nullptr, BUF_SIZE, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, map.size);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 0;
    make_frame_meta(b, 0, 4, 4);

    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "no output_tensors must passthrough (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "passthrough must produce output");
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_postproc_secondary_no_tensors: secondary mode, objects without matching tensors → skip
// Target: process_secondary_mode L348-353 (iter == end → continue, empty tensors → continue)
GST_START_TEST(CE_postproc_secondary_no_tensors) {
    DXTEST_SKIP_IF(!lib_available(), "postprocess library not found");

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "YOLOV5S_PPU",
                 "inference-id", 1u,
                 "secondary-mode", TRUE, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_RGB);

    GstBuffer *b = gst_buffer_new_allocate(nullptr, BUF_SIZE, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, map.size);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 0;
    DXFrameMeta *fm = make_frame_meta(b, 0, 4, 4);
    add_object_to_frame(fm, 1, 0.9f, 0, 0, 2, 2);

    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "secondary with no matching tensors must passthrough (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "passthrough must produce output");
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_postproc_wrong_infer_id: buffer has tensors for id=0 but element expects id=99 → skip
// Target: gst_dxpostprocess_transform_ip L396 (find(99) → end)
GST_START_TEST(CE_postproc_wrong_infer_id) {
    DXTEST_SKIP_IF(!lib_available(), "postprocess library not found");

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "YOLOV5S_PPU",
                 "inference-id", 99u, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);

    gst_harness_set_src_caps_str(h, CAPS_RGB);

    GstBuffer *b = gst_buffer_new_allocate(nullptr, BUF_SIZE, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, map.size);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = 0;
    make_frame_meta(b, 0, 4, 4);

    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "mismatched infer_id must passthrough (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr);
    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_postproc_dlclose_multiple: multiple READY→NULL→READY cycles → no leak
// Target: dxpostprocess_change_state (dlopen/dlclose symmetry)
GST_START_TEST(CE_postproc_dlclose_multiple) {
    DXTEST_SKIP_IF(!lib_available(), "postprocess library not found");

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "library-file-path", POSTPROC_LIB_PATH().c_str(),
                 "function-name", "YOLOV5S_PPU", nullptr);

    for (int i = 0; i < 10; i++) {
        GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_READY);
        fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                    "READY cycle %d failed", i);
        gst_element_set_state(e, GST_STATE_NULL);
    }

    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxpostprocess_runtime_suite(void) {
    Suite *s = suite_create("dxpostprocess_runtime");
    TCase *tc = tcase_create("runtime_edge");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_postproc_no_tensors_primary);
    tcase_add_test(tc, CE_postproc_secondary_no_tensors);
    tcase_add_test(tc, CE_postproc_wrong_infer_id);
    tcase_add_test(tc, CE_postproc_dlclose_multiple);
    return s;
}

GST_CHECK_MAIN(dxpostprocess_runtime);
