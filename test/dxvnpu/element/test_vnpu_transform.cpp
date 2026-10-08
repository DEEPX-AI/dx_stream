#include <gst/check/gstcheck.h>

#include "../../../gst-dxstream-plugin/src/transforms/gst_frame_desc.hpp"
#include "../../../gst-dxstream-plugin/src/transforms/video_transform_factory.hpp"

#include "../../../gst-dxstream-plugin/general/dx_dlfcn.h"

#include <algorithm>
#include <memory>
#include <vector>

using CreateKernel = dxt::IVideoTransformKernel* (*)();

static std::unique_ptr<dxt::IVideoTransformKernel> create_kernel(void** library) {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") != 0) {
        g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
        return nullptr;
    }

#ifdef _WIN32
    *library = dlopen("gstdxstream-vnpu.dll", RTLD_NOW | RTLD_LOCAL);
#else
    *library = dlopen("libgstdxstream-vnpu.so", RTLD_NOW | RTLD_LOCAL);
#endif
    fail_unless(*library != nullptr, "failed to load VNPU transform plugin: %s", dlerror());
    auto create = reinterpret_cast<CreateKernel>(
        dlsym(*library, "dx_create_vnpu_transform_kernel"));
    fail_unless(create != nullptr, "VNPU transform factory is unavailable: %s", dlerror());

    std::unique_ptr<dxt::IVideoTransformKernel> kernel(create());
    fail_unless(kernel != nullptr);
    fail_unless(g_strcmp0(kernel->backend_name(), "vnpu") == 0);
    return kernel;
}

static dxt::FrameDesc nv12_frame(std::vector<uint8_t>& data, int width, int height) {
    data.resize(static_cast<size_t>(width) * height * 3 / 2);
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
            data[static_cast<size_t>(row) * width + column] = (row + column) & 0xff;
    std::fill(data.begin() + static_cast<size_t>(width) * height, data.end(), 128);
    return dxt::make_output_frame_desc(data.data(), width, height, dxt::VideoFormat::NV12);
}

static dxt::FrameDesc red_nv12_frame(std::vector<uint8_t>& data, int width, int height) {
    data.resize(static_cast<size_t>(width) * height * 3 / 2);
    std::fill(data.begin(), data.begin() + static_cast<size_t>(width) * height, 82);
    for (size_t i = static_cast<size_t>(width) * height; i < data.size(); i += 2) {
        data[i] = 90;
        data[i + 1] = 240;
    }
    return dxt::make_output_frame_desc(data.data(), width, height, dxt::VideoFormat::NV12);
}

static dxt::FrameDesc gradient_nv12_frame(std::vector<uint8_t>& data, int width, int height) {
    data.resize(static_cast<size_t>(width) * height * 3 / 2);
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column)
            data[static_cast<size_t>(row) * width + column] =
                static_cast<uint8_t>(20 + (column + row) / 6);
    }
    std::fill(data.begin() + static_cast<size_t>(width) * height, data.end(), 128);
    return dxt::make_output_frame_desc(data.data(), width, height, dxt::VideoFormat::NV12);
}

GST_START_TEST(CA1_vnpu_scale_host_nv12) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(640, 360, dxt::VideoFormat::NV12);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(640 * 360 * 3 / 2);
    const auto input = nv12_frame(input_data, 1920, 1080);
    auto output = dxt::make_output_frame_desc(output_data.data(), 640, 360,
                                               dxt::VideoFormat::NV12);

    const auto result = kernel->transform(input, output);
    fail_unless(result.success);
    fail_unless_equals_int(output.planes[0].stride, 640);
    fail_unless_equals_int(output.planes[1].stride, 640);
    fail_unless(output_data[640 * 180 + 320] != 0);

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA2_vnpu_convert_host_nv12_to_rgb) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(640, 360, dxt::VideoFormat::RGB);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(640 * 360 * 3);
    const auto input = nv12_frame(input_data, 1920, 1080);
    auto output = dxt::make_output_frame_desc(output_data.data(), 640, 360,
                                               dxt::VideoFormat::RGB);

    const auto result = kernel->transform(input, output);
    fail_unless(result.success);
    fail_unless_equals_int(output.planes[0].stride, 640 * 3);
    fail_unless(output_data[3 * (640 * 180 + 320)] != 0);

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA3_vnpu_letterbox_uses_configured_padding) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(640, 640, dxt::VideoFormat::RGB);
    dxt::TransformOps ops;
    ops.keep_aspect_ratio = true;
    ops.padding.enabled = true;
    ops.padding.pad_r = 114;
    ops.padding.pad_g = 114;
    ops.padding.pad_b = 114;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(640 * 640 * 3);
    const auto input = nv12_frame(input_data, 1920, 1080);
    auto output = dxt::make_output_frame_desc(output_data.data(), 640, 640,
                                               dxt::VideoFormat::RGB);

    fail_unless(kernel->transform(input, output).success);
    fail_unless_equals_int(output_data[0], 114);
    fail_unless_equals_int(output_data[1], 114);
    fail_unless_equals_int(output_data[2], 114);

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA4_vnpu_nv12_to_rgb_preserves_channel_order) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(640, 640, dxt::VideoFormat::RGB);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(640 * 640 * 3);
    const auto input = red_nv12_frame(input_data, 640, 640);
    auto output = dxt::make_output_frame_desc(output_data.data(), 640, 640,
                                               dxt::VideoFormat::RGB);

    fail_unless(kernel->transform(input, output).success);
    fail_unless(output_data[0] > 200, "red channel must remain red");
    fail_unless(output_data[1] < 50, "green channel must remain low");
    fail_unless(output_data[2] < 50, "blue channel must remain low");

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA5_letterbox_uses_vnpu_backend) {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") != 0) {
        g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
        return;
    }

    const auto destination_template =
        dxt::make_dst_template(640, 640, dxt::VideoFormat::RGB);
    dxt::TransformOps ops;
    ops.keep_aspect_ratio = true;
    ops.padding.enabled = true;
    ops.padding.pad_r = 114;
    ops.padding.pad_g = 114;
    ops.padding.pad_b = 114;
    auto kernel = dxt::VideoTransformFactory::create(
        destination_template, ops, dxt::VideoFormat::NV12);

    fail_unless(kernel != nullptr);
    fail_unless_equals_string(kernel->backend_name(), "vnpu");
}
GST_END_TEST;

GST_START_TEST(CA6_vnpu_dynamic_crop_applies_to_output) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(640, 240, dxt::VideoFormat::NV12);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(640 * 240 * 3 / 2);
    const auto input = gradient_nv12_frame(input_data, 640, 480);
    auto output = dxt::make_output_frame_desc(output_data.data(), 640, 240,
                                               dxt::VideoFormat::NV12);
    const dxt::CropRect crops[] = {
        {0,   0,   640, 480, true}, {0,   0,   320, 240, true},
        {320, 0,   320, 240, true}, {0,   240, 320, 240, true},
        {320, 240, 320, 240, true}, {80,  60,  480, 360, true},
        {0,   120, 640, 240, true}, {160, 0,   320, 480, true},
        {240, 180, 160, 120, true}, {480, 300, 160, 180, true},
    };

    for (const auto& crop : crops) {
        const dxt::DynamicOps dynamic = {&crop};
        fail_unless(kernel->transform(input, output, 0, &dynamic).success);
        unsigned int luma_sum = 0;
        for (int pixel = 0; pixel < 640 * 240; ++pixel)
            luma_sum += output_data[pixel];
        const int actual = static_cast<int>(luma_sum / (640 * 240));
        const int expected = 20 + (crop.x + crop.y + crop.w / 2 + crop.h / 2) / 6;
        fail_unless(actual >= expected - 12 && actual <= expected + 12,
                    "crop %d,%d %dx%d produced luma %d, expected about %d",
                    crop.x, crop.y, crop.w, crop.h, actual, expected);
    }

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA7_vnpu_exact_8x_downscale_1792x1792_to_224x224) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(224, 224, dxt::VideoFormat::NV12);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(224 * 224 * 3 / 2);
    const auto input = nv12_frame(input_data, 1792, 1792);
    auto output = dxt::make_output_frame_desc(output_data.data(), 224, 224,
                                               dxt::VideoFormat::NV12);

    const auto result = kernel->transform(input, output);
    fail_unless(result.success, "VNPU kernel must succeed for 1792x1792->224x224");
    fail_unless_equals_int(output.planes[0].stride, 224);
    fail_unless_equals_int(output.planes[1].stride, 224);
    fail_unless(output_data[224 * 112 + 112] != 0);

    kernel.reset();
    dlclose(library);
}
GST_END_TEST;

GST_START_TEST(CA8_vnpu_rejects_68x2_for_fallback) {
    void* library = nullptr;
    auto kernel = create_kernel(&library);
    if (!kernel)
        return;

    const auto destination_template =
        dxt::make_dst_template(68, 2, dxt::VideoFormat::NV12);
    dxt::TransformOps ops;
    fail_unless(kernel->init(destination_template, ops));

    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data(68 * 2 * 3 / 2);
    const auto input = nv12_frame(input_data, 68, 2);
    auto output = dxt::make_output_frame_desc(output_data.data(), 68, 2,
                                               dxt::VideoFormat::NV12);

    const auto result = kernel->transform(input, output);
    fail_if(result.success, "68x2 must bypass the VNPU backend");

    kernel.reset();
}
GST_END_TEST;

static Suite* vnpu_transform_suite() {
    Suite* suite = suite_create("vnpu_transform");
    TCase* testcase = tcase_create("hardware");
    tcase_add_test(testcase, CA1_vnpu_scale_host_nv12);
    tcase_add_test(testcase, CA2_vnpu_convert_host_nv12_to_rgb);
    tcase_add_test(testcase, CA3_vnpu_letterbox_uses_configured_padding);
    tcase_add_test(testcase, CA4_vnpu_nv12_to_rgb_preserves_channel_order);
    tcase_add_test(testcase, CA5_letterbox_uses_vnpu_backend);
    tcase_add_test(testcase, CA6_vnpu_dynamic_crop_applies_to_output);
    tcase_add_test(testcase, CA7_vnpu_exact_8x_downscale_1792x1792_to_224x224);
    tcase_add_test(testcase, CA8_vnpu_rejects_68x2_for_fallback);
    suite_add_tcase(suite, testcase);
    return suite;
}

GST_CHECK_MAIN(vnpu_transform);
