#pragma once

#include "transform_kernel_base.hpp"

#include <memory>

namespace dxt {

class GlesDevice;
struct GlesTarget;

// OpenGL ES 3.0 transform kernel: zero-copy dma-buf import, crop/letterbox/resize/CSC in one shader pass,
// packed RGB888 output. Secondary ROIs of a frame are rendered in a single GPU submission.
class GlesTransformKernel : public TransformKernelBase {
public:
    GlesTransformKernel();
    ~GlesTransformKernel() override;

    const char* backend_name() const override { return "gles"; }
    BackendCaps capabilities() const override;
    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;
    TransformResult transform(const FrameDesc& src, FrameDesc& dst, int slot_id = 0,
                              const DynamicOps* dynamic = nullptr) override;
    void transform_batch(const FrameDesc& src, const CropRect* crops, FrameDesc* dsts, int count, bool* ok,
                         int slot_id = 0) override;

private:
    bool run(const FrameDesc& src, const CropRect* crops, FrameDesc* const* dsts, int n, TransformResult* first);
    bool ensure_target(int w, int h);

    std::shared_ptr<GlesDevice> dev_;
    std::unique_ptr<GlesTarget> target_;
};

}  // namespace dxt
