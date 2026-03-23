#pragma once

#ifdef HAVE_LIBRGA

#include "transform_kernel_base.hpp"

namespace dxt {

// ---------------------------------------------------------------------------
// RgaTransformKernel
//
// Hardware-accelerated video transform using Rockchip RGA2/RGA3.
// Supported platforms: RK3588 (Orange Pi 5 Plus, etc.)
//
// Capabilities:
//   src : NV12 only (hardware constraint)
//   dst : RGB or BGR
//   ops : crop + scale + letterbox padding — all in ONE improcess() call (zero extra copy)
//   DMA-buf: YES — zero-copy when decoder outputs DMA-buf fd
//
// RGA scale ratio constraint: 1/8 ~ 8 per axis
// RGA src resolution: 68x2 ~ 8176x8176
// RGA dst resolution: 68x2 ~ 8128x8128
// Output alignment: dst width must be 16-aligned, dst height 2-aligned
// ---------------------------------------------------------------------------

class RgaTransformKernel : public TransformKernelBase {
public:
    RgaTransformKernel()  = default;
    ~RgaTransformKernel() = default;

    const char* backend_name() const override { return "rga"; }
    BackendCaps capabilities()  const override;

    // Validates dst dimensions and stores configuration.
    // Returns false if dst format/alignment is unsupported.
    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    // Executes crop + scale + letterbox + colorconvert in a single improcess() call.
    // src must be NV12 (CPU_VIRTUAL with data populated, or DMA_BUF with fd set).
    // dst must be CPU_VIRTUAL RGB/BGR with data pointer set by caller.
    // slot_id is unused (RGA is stateless; no per-slot scratch buffers needed).
    TransformResult transform(const FrameDesc&  src,
                              FrameDesc&        dst,
                              int               slot_id = 0,
                              const DynamicOps* dynamic  = nullptr) override;

private:
    // RGA-specific: enforces even-aligned coordinates required for NV12
    CropRect effective_crop(const FrameDesc& src, const DynamicOps* dynamic) const;

    // Local wrapper keeping transform() body unchanged (param names: dst_w, dst_h)
    void compute_dst_rect(int src_w, int src_h,
                          int& dst_x, int& dst_y,
                          int& dst_w, int& dst_h) const;
};

}  // namespace dxt

#endif  // HAVE_LIBRGA
