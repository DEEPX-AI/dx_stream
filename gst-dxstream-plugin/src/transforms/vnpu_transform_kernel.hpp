#pragma once

// ---------------------------------------------------------------------------
// VnpuTransformKernel
//
// Hardware-accelerated video transform using DEEPX VNPU VideoProcessor.
//
// Key constraint: VNPU VideoProcessor requires fixed input AND output
// configuration at construction time. Unlike RGA/libyuv, the input
// format and dimensions cannot change after init.
//
// This kernel uses lazy initialization: the VideoProcessor is created on
// the first transform() call using the actual source frame dimensions.
// BackendCaps::supports_dynamic_input_size is false, so the kernel pool
// (TransformKernelPool) creates separate instances per input config.
//
// Supported formats:
//   src: NV12, RGB, BGR
//   dst: NV12, RGB, BGR
// ---------------------------------------------------------------------------

#ifdef HAVE_DXVNPU

#include "transform_kernel_base.hpp"
#include <dxvnpu/dxvnpu_api.h>
#include <memory>

namespace dxt {

class VnpuTransformKernel : public TransformKernelBase {
public:
    VnpuTransformKernel()  = default;
    ~VnpuTransformKernel() override = default;

    const char* backend_name() const override { return "vnpu"; }
    BackendCaps capabilities()  const override;

    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    TransformResult transform(const FrameDesc&  src,
                              FrameDesc&        dst,
                              int               slot_id = 0,
                              const DynamicOps* dynamic  = nullptr) override;

private:
    std::unique_ptr<dxvnpu::VideoProcessor> processor_;

    // Input config captured on first transform (used for mismatch detection)
    int input_width_  = 0;
    int input_height_ = 0;
    VideoFormat input_format_ = VideoFormat::NV12;

    static dxvnpu::ColorFormat to_dxvnpu_format(VideoFormat fmt);
    static VideoFormat from_dxvnpu_format(dxvnpu::ColorFormat fmt);
};

}  // namespace dxt

#endif  // HAVE_DXVNPU
