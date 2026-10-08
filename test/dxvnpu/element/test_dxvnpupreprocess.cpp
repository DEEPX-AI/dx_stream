#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>

#include "meta_helpers.hpp"
#include <gstdxstream/dxcommon.hpp>

#include <cstring>

using namespace dxtest;

static gboolean require_device() {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") == 0) return TRUE;
    g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
    return FALSE;
}

static GstBuffer *make_nv12_buffer(int width, int height, GstClockTime pts) {
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, width * height * 3 / 2, nullptr);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
            map.data[row * width + column] = (row + column) & 0xff;
    memset(map.data + width * height, 128, width * height / 2);
    gst_buffer_unmap(buffer, &map);
    GST_BUFFER_PTS(buffer) = pts;
    GST_BUFFER_DURATION(buffer) = GST_SECOND / 30;
    return buffer;
}

static GstBuffer *make_split_nv12_buffer(int width, int height) {
    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, width * height * 3 / 2, nullptr);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_WRITE);
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
            map.data[row * width + column] = column < width / 2 ? 32 : 192;
    memset(map.data + width * height, 128, width * height / 2);
    gst_buffer_unmap(buffer, &map);
    GST_BUFFER_PTS(buffer) = 0;
    GST_BUFFER_DURATION(buffer) = GST_SECOND / 30;
    return buffer;
}

static int preprocess_crop_center_luma(const char *roi, int input_width,
                                       int input_height, int output_width,
                                       int output_height) {
    GstElement *element = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(element, "resize-width", output_width, "resize-height", output_height,
                 "keep-ratio", FALSE, nullptr);
    if (roi)
        g_object_set(element, "roi", roi, nullptr);
    GstHarness *harness = gst_harness_new_with_element(element, "sink", "src");
    gst_object_unref(element);
    char caps[96];
    g_snprintf(caps, sizeof(caps),
               "video/x-raw,format=NV12,width=%d,height=%d,framerate=30/1",
               input_width, input_height);
    gst_harness_set_src_caps_str(harness, caps);
    gst_harness_push(harness, make_split_nv12_buffer(input_width, input_height));

    GstBuffer *out = gst_harness_try_pull(harness);
    fail_unless(out != nullptr, "preprocess must produce output");
    DXFrameMeta *frame_meta = dx_get_frame_meta(out);
    fail_unless(frame_meta != nullptr);
    const auto &tensor = frame_meta->_input_tensors[0];
    const uint8_t *data = static_cast<const uint8_t *>(tensor.data_ptr());
    int luma = data[3 * ((output_height / 2) * output_width + output_width / 2)];

    gst_buffer_unref(out);
    gst_harness_teardown(harness);
    return luma;
}

GST_START_TEST(CE_preprocess_vnpu_host_tensor) {
    if (!require_device()) return;

    GstElement *element = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(element, "resize-width", 640u, "resize-height", 360u, nullptr);
    GstHarness *harness = gst_harness_new_with_element(element, "sink", "src");
    gst_object_unref(element);
    gst_harness_set_src_caps_str(
        harness, "video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1");
    gst_harness_push(harness, make_nv12_buffer(1920, 1080, 0));

    GstBuffer *out = gst_harness_try_pull(harness);
    fail_unless(out != nullptr, "preprocess must produce output");
    DXFrameMeta *frame_meta = dx_get_frame_meta(out);
    fail_unless(frame_meta != nullptr);
    fail_unless(frame_meta->_input_tensors.find(0) != frame_meta->_input_tensors.end());

    const auto &tensor = frame_meta->_input_tensors[0];
    fail_unless_equals_int(tensor._tensors.size(), 1);
    fail_unless(tensor._tensors[0]._shape ==
                std::vector<int64_t>({360, 640, 3}));
    fail_unless_equals_int(tensor._tensors[0]._type, dxs::UINT8);
    fail_unless_equals_int(tensor._mem_size, 640 * 360 * 3);
    fail_unless(tensor.data_ptr() != nullptr);
    fail_unless(static_cast<const uint8_t *>(tensor.data_ptr())[3 * (640 * 180 + 320)] != 0);

    gst_buffer_unref(out);
    gst_harness_teardown(harness);
}
GST_END_TEST;

GST_START_TEST(CE_preprocess_vnpu_crop_applies_to_tensor) {
    if (!require_device()) return;

    const int left = preprocess_crop_center_luma("0,0,320,480", 640, 480, 320, 240);
    const int right = preprocess_crop_center_luma("320,0,640,480", 640, 480, 320, 240);
    fail_unless(right > left + 100,
                "crop must select distinct source regions (left=%d, right=%d)",
                left, right);
}
GST_END_TEST;

GST_START_TEST(CE_preprocess_vnpu_crop_allows_large_full_frame_scale) {
    if (!require_device()) return;

    const int luma = preprocess_crop_center_luma("960,0,1600,540", 1920, 1080, 224, 224);
    fail_unless(luma > 150,
                "crop must be used instead of full-frame 1920x1080 scale (luma=%d)", luma);
}
GST_END_TEST;

GST_START_TEST(CE_preprocess_vnpu_secondary_object_crop_allows_large_full_frame_scale) {
    if (!require_device()) return;

    GstElement *element = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(element, "resize-width", 224u, "resize-height", 224u,
                 "keep-ratio", FALSE, "secondary-mode", TRUE, nullptr);
    GstHarness *harness = gst_harness_new_with_element(element, "sink", "src");
    gst_object_unref(element);
    gst_harness_set_src_caps_str(
        harness, "video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1");

    GstBuffer *input = make_split_nv12_buffer(1920, 1080);
    DXFrameMeta *frame_meta = make_frame_meta(input, 0, 1920, 1080, "NV12");
    add_object_to_frame(frame_meta, 0, 1.0f, 960, 0, 1600, 540);
    gst_harness_push(harness, input);

    GstBuffer *out = gst_harness_try_pull(harness);
    fail_unless(out != nullptr, "secondary preprocess must produce output");
    frame_meta = dx_get_frame_meta(out);
    fail_unless(frame_meta != nullptr);
    fail_unless_equals_int(frame_meta->_object_meta_list.size(), 1);
    const auto tensor_it = frame_meta->_object_meta_list[0]->_input_tensors.find(0);
    fail_unless(tensor_it != frame_meta->_object_meta_list[0]->_input_tensors.end());
    const uint8_t *data = static_cast<const uint8_t *>(tensor_it->second.data_ptr());
    fail_unless(data != nullptr);
    fail_unless(data[3 * (112 * 224 + 112)] > 150,
                "secondary object crop must select the bright right-side ROI");

    gst_buffer_unref(out);
    gst_harness_teardown(harness);
}
GST_END_TEST;

GST_START_TEST(CE_preprocess_vnpu_accepts_rga_min_resolution) {
    if (!require_device()) return;

    const int luma = preprocess_crop_center_luma(nullptr, 68, 2, 68, 2);
    fail_unless(luma > 0, "68x2 RGA minimum resolution must be processed (luma=%d)", luma);
}
GST_END_TEST;

GST_START_TEST(CE_preprocess_vnpu_accepts_eightfold_scale) {
    if (!require_device()) return;

    const int luma = preprocess_crop_center_luma(nullptr, 1792, 1792, 224, 224);
    fail_unless(luma > 0, "exact 8x scale must be processed (luma=%d)", luma);
}
GST_END_TEST;

static Suite *dxvnpupreprocess_suite() {
    Suite *suite = suite_create("dxvnpupreprocess");
    TCase *testcase = tcase_create("acceptance");
    tcase_set_timeout(testcase, 30.0);
    suite_add_tcase(suite, testcase);
    tcase_add_test(testcase, CE_preprocess_vnpu_host_tensor);
    tcase_add_test(testcase, CE_preprocess_vnpu_crop_applies_to_tensor);
    tcase_add_test(testcase, CE_preprocess_vnpu_crop_allows_large_full_frame_scale);
    tcase_add_test(testcase,
                   CE_preprocess_vnpu_secondary_object_crop_allows_large_full_frame_scale);
    tcase_add_test(testcase, CE_preprocess_vnpu_accepts_rga_min_resolution);
    tcase_add_test(testcase, CE_preprocess_vnpu_accepts_eightfold_scale);
    return suite;
}

GST_CHECK_MAIN(dxvnpupreprocess);
