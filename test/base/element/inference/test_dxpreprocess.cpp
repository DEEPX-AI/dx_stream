// P3.1 — dxpreprocess contract tests
// Core: dxpreprocess is an in-place transform. Resizes + color converts video
// and stores tensor in DXFrameMeta._input_tensors[preprocess_id].
// resize-width/height required (0 → start() failure).

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include <gstdxstream/dxcommon.hpp>

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB_320 =
    "video/x-raw,format=RGB,width=320,height=240,framerate=30/1";

static GstBuffer *make_rgb_buffer(int w, int h, GstClockTime pts) {
    gsize sz = w * h * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

static GstHarness *make_preprocess_harness(guint rw, guint rh, guint pid = 0) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", rw, "resize-height", rh,
                 "preprocess-id", pid, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    return h;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    guint rw = 999, rh = 999, pid = 999, interval = 999;
    gboolean kr = FALSE, sm = TRUE;
    g_object_get(e, "resize-width", &rw, "resize-height", &rh,
                 "preprocess-id", &pid, "keep-ratio", &kr,
                 "secondary-mode", &sm, "interval", &interval, nullptr);
    fail_unless_equals_int(rw, 0);
    fail_unless_equals_int(rh, 0);
    fail_unless_equals_int(pid, 0);
    fail_unless(kr == TRUE);
    fail_unless(sm == FALSE);
    fail_unless_equals_int(interval, 0);

    g_object_set(e, "resize-width", 640u, "resize-height", 640u,
                 "preprocess-id", 1u, "keep-ratio", FALSE,
                 "secondary-mode", TRUE, "interval", 2u, nullptr);
    g_object_get(e, "resize-width", &rw, "resize-height", &rh,
                 "preprocess-id", &pid, "keep-ratio", &kr,
                 "secondary-mode", &sm, "interval", &interval, nullptr);
    fail_unless_equals_int(rw, 640);
    fail_unless_equals_int(rh, 640);
    fail_unless_equals_int(pid, 1);
    fail_unless(kr == FALSE);
    fail_unless(sm == TRUE);
    fail_unless_equals_int(interval, 2);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u, nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_preprocess_resize_zero_error: resize-width=0 → start() failure
// Target: gst_dxpreprocess_start L718-724 (width/height == 0 → GST_ELEMENT_ERROR)
// MUT: remove L718-724 check → crash or undefined
GST_START_TEST(CE_preprocess_resize_zero_error) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(
        "videotestsrc num-buffers=1 "
        "! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 "
        "! dxpreprocess resize-width=0 resize-height=0 "
        "! fakesink", &err);
    fail_unless(err == nullptr && pipe != nullptr);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    fail_unless(msg != nullptr, "expected error for resize=0");
    fail_unless_equals_int(GST_MESSAGE_TYPE(msg), GST_MESSAGE_ERROR);
    gst_message_unref(msg);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

// CE_preprocess_missing_custom_library_error: function without library → start() failure
// Target: gst_dxpreprocess_start (custom library/function pair validation)
GST_START_TEST(CE_preprocess_missing_custom_library_error) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "function-name", "PreProcess", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_preprocess_missing_custom_function_error: library without function → start() failure
// Target: gst_dxpreprocess_start (custom library/function pair validation)
GST_START_TEST(CE_preprocess_missing_custom_function_error) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "library-file-path", "/tmp/libpreprocess-placeholder.so", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_preprocess_bad_config_path_uses_properties: config-file-path load failure is non-fatal
// Target: gst_dxpreprocess_start validates resize/library properties, not config load status
GST_START_TEST(CE_preprocess_bad_config_path_uses_properties) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "config-file-path", "/nonexistent/dxpreprocess.json", nullptr);
    assert_state(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_preprocess_bad_custom_library_error: nonexistent custom library → start() failure
// Target: gst_dxpreprocess_start (dlopen failure → GST_ELEMENT_ERROR)
GST_START_TEST(CE_preprocess_bad_custom_library_error) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "library-file-path", "/nonexistent/libpreprocess.so",
                 "function-name", "PreProcess", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_preprocess_tensor_populated: RGB buffer → _input_tensors[0] created
// Target: Preprocessor::primary_process L356-358 (tensor → _input_tensors[id])
// MUT: remove L357 → _input_tensors empty → fail
GST_START_TEST(CE_preprocess_tensor_populated) {
    GstHarness *h = make_preprocess_harness(64, 64, 0);
    gst_harness_set_src_caps_str(h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    gst_harness_push(h, b);

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "preprocess must produce output");

    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr, "output must have DXFrameMeta");
    fail_unless(fm->_input_tensors.find(0) != fm->_input_tensors.end(),
                "input_tensors[0] must exist after preprocessing");

    const auto &tensors = fm->_input_tensors[0];
    fail_unless_equals_int(tensors._tensors.size(), 1);
    fail_unless(tensors._tensors[0]._shape.size() == 3,
                "tensor shape must be [H, W, C]");
    fail_unless_equals_int(tensors._tensors[0]._shape[0], 64);
    fail_unless_equals_int(tensors._tensors[0]._shape[1], 64);
    fail_unless_equals_int(tensors._tensors[0]._shape[2], 3);
    fail_unless(tensors.data_ptr() != nullptr, "tensor data must be allocated");

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_auto_frame_meta: buffer without DXFrameMeta → check_frame_meta auto-creates
// Target: Preprocessor::check_frame_meta L167-186 (null → dx_create_frame_meta)
// MUT: remove L169-170 → null deref → crash
GST_START_TEST(CE_preprocess_auto_frame_meta) {
    GstHarness *h = make_preprocess_harness(64, 64);
    gst_harness_set_src_caps_str(h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    gst_harness_push(h, b);

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "auto-meta buffer must produce output");

    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr,
                "check_frame_meta must auto-create DXFrameMeta");
    fail_unless_equals_int(fm->_width, 320);
    fail_unless_equals_int(fm->_height, 240);
    fail_unless_equals_int(fm->_stream_id, 0);

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_roi_applies: ROI setting → frame_meta._roi updated
// Target: Preprocessor::primary_process L303-307 (roi[0]!=-1 → roi apply)
// MUT: remove L303-307 → _roi stays at {-1,-1,-1,-1}
GST_START_TEST(CE_preprocess_roi_applies) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "roi", "10,20,200,180", nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    gst_harness_set_src_caps_str(h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    DXFrameMeta *fm = make_frame_meta(b, 0, 320, 240);
    gst_harness_push(h, b);

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "ROI buffer must produce output");

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr);
    fail_unless_equals_int(ofm->_roi[0], 10);
    fail_unless_equals_int(ofm->_roi[1], 20);
    fail_unless_equals_int(ofm->_roi[2], 200);
    fail_unless_equals_int(ofm->_roi[3], 180);

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_interval_skip: interval=2 → only 1 out of 3 frames gets tensor
// Target: Preprocessor::check_primary_interval L159-161 (cnt < interval → skip)
// MUT: remove interval check → all frames get tensors
GST_START_TEST(CE_preprocess_interval_skip) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", 64u, "resize-height", 64u,
                 "interval", 2u, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    gst_harness_set_src_caps_str(h, CAPS_RGB_320);

    int tensor_count = 0;
    for (int i = 0; i < 6; i++) {
        GstBuffer *b = make_rgb_buffer(320, 240, i * GST_SECOND / 30);
        DXFrameMeta *fm = make_frame_meta(b, 0, 320, 240);
        gst_harness_push(h, b);

        GstBuffer *out = gst_harness_try_pull(h);
        fail_unless(out != nullptr, "frame %d must produce output", i);

        DXFrameMeta *ofm = dx_get_frame_meta(out);
        if (ofm && ofm->_input_tensors.find(0) != ofm->_input_tensors.end()) {
            tensor_count++;
        }
        gst_buffer_unref(out);
    }
    // interval=2: skip 2, process 1 → 6 frames → 2 processed
    fail_unless_equals_int(tensor_count, 2);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_preprocess_id: preprocess-id=3 → stored in _input_tensors[3]
// Target: primary_process L357 (element->_preprocess.id as key)
// MUT: use 0 instead of id → _input_tensors[3] missing
GST_START_TEST(CE_preprocess_preprocess_id) {
    GstHarness *h = make_preprocess_harness(64, 64, 3);
    gst_harness_set_src_caps_str(h, CAPS_RGB_320);

    GstBuffer *b = make_rgb_buffer(320, 240, 0);
    gst_harness_push(h, b);

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "must produce output");

    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr);
    fail_unless(fm->_input_tensors.find(3) != fm->_input_tensors.end(),
                "tensor must be stored under preprocess_id=3, not 0");
    fail_unless(fm->_input_tensors.find(0) == fm->_input_tensors.end(),
                "preprocess_id=0 must NOT have tensor");

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *dxpreprocess_suite(void) {
    Suite *s = suite_create("dxpreprocess");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_preprocess_resize_zero_error);
    tcase_add_test(tc, CE_preprocess_missing_custom_library_error);
    tcase_add_test(tc, CE_preprocess_missing_custom_function_error);
    tcase_add_test(tc, CE_preprocess_bad_config_path_uses_properties);
    tcase_add_test(tc, CE_preprocess_bad_custom_library_error);
    tcase_add_test(tc, CE_preprocess_tensor_populated);
    tcase_add_test(tc, CE_preprocess_auto_frame_meta);
    tcase_add_test(tc, CE_preprocess_roi_applies);
    tcase_add_test(tc, CE_preprocess_interval_skip);
    tcase_add_test(tc, CE_preprocess_preprocess_id);
    return s;
}

GST_CHECK_MAIN(dxpreprocess);
