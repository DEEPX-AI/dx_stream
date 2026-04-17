#pragma once

#ifdef HAVE_LIBRGA

#include "transform_kernel_base.hpp"
#include "libyuv_transform_kernel.hpp"   // internal fallback

#include <memory>

namespace dxt {

// ---------------------------------------------------------------------------
// RgaTransformKernel
//
// Hardware-accelerated video transform using Rockchip RGA2/RGA3.
// Supported platforms: any Rockchip SoC with librga (RK3588, RK3566, etc.)
//
// Capabilities (advertised):
//   src : NV12, RGB, BGR   (I420 excluded — 3-plane layout incompatible
//                            with RGA's single-pointer wrapbuffer model)
//   dst : NV12, RGB, BGR
//   ops : crop + scale + letterbox padding — all in ONE improcess() call
//   DMA-buf: YES — zero-copy when decoder outputs DMA-buf fd
//
// Per-frame validation strategy:
//   init()       — accepts any supported format pair. Only dst_template is stored.
//   every frame  — resolution range + scale ratio check, then imcheck().
//                  If HW rejects → transparently falls back to internal libyuv
//                  for that frame only. Next frame retries RGA.
//
// No hardcoded alignment tables.
// RGA3 cores are explicitly pinned (RGA2-Enhance has 32-bit IOMMU,
// which causes kernel panic when accessing memory above 4GB).
// ---------------------------------------------------------------------------

class RgaTransformKernel : public TransformKernelBase {
public:
    RgaTransformKernel()  = default;
    ~RgaTransformKernel() = default;

    const char* backend_name() const override { return "rga"; }
    BackendCaps capabilities()  const override;

    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    TransformResult transform(const FrameDesc&  src,
                              FrameDesc&        dst,
                              int               slot_id = 0,
                              const DynamicOps* dynamic  = nullptr) override;

private:
    // Even-aligned crop coordinates for YUV formats
    CropRect effective_crop(const FrameDesc& src, const DynamicOps* dynamic) const;

    void compute_dst_rect(int src_w, int src_h,
                          int& dst_x, int& dst_y,
                          int& dst_w, int& dst_h) const;

    // Fill padding area with appropriate color for the dst format
    void fill_padding(FrameDesc& dst) const;

    // Execute the RGA hardware path (called after imcheck succeeds)
    TransformResult rga_execute(const FrameDesc& src, FrameDesc& dst,
                                const CropRect& crop,
                                int dst_x, int dst_y, int dst_w, int dst_h);

    // Internal libyuv fallback — created once in init(), used if RGA rejects
    std::unique_ptr<LibyuvTransformKernel> libyuv_fallback_;
};

}  // namespace dxt

#endif  // HAVE_LIBRGA
