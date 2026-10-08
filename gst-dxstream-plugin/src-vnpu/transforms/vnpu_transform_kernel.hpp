#pragma once

#include "transform_kernel_base.hpp"
#include <dxvnpu/dxvnpu_c_api.h>

namespace dxt {

class VnpuTransformKernel : public TransformKernelBase {
public:
    VnpuTransformKernel() = default;
    ~VnpuTransformKernel() override;

    const char* backend_name() const override { return "vnpu"; }
    BackendCaps capabilities() const override;

    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    TransformResult transform(const FrameDesc& src,
                              FrameDesc& dst,
                              int slot_id = 0,
                              const DynamicOps* dynamic = nullptr) override;

private:
    dxvnpu_pipeline_t processor_ = nullptr;

    int input_width_ = 0;
    int input_height_ = 0;
    VideoFormat input_format_ = VideoFormat::NV12;
    bool dynamic_crop_ = false;

    static dxvnpu_color_format_t to_dxvnpu_format(VideoFormat fmt);
    static VideoFormat from_dxvnpu_format(dxvnpu_color_format_t fmt);
};

}
