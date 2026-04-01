#pragma once

#include "transform_kernel_base.hpp"

namespace dxt {

// ---------------------------------------------------------------------------
// LibyuvTransformKernel
//
// Software fallback video transform using libyuv + OpenCV (for RGB resize).
// Always available — no hardware dependencies.
//
// Supported conversions (full 4×4 matrix):
//   I420 → I420, NV12, RGB, BGR
//   NV12 → I420, NV12, RGB, BGR
//   RGB  → I420, NV12, RGB, BGR
//   BGR  → I420, NV12, RGB, BGR
//
// Note: RGB/BGR → NV12 uses a two-step path (packed → I420 → NV12).
//
// Copy minimisation strategy:
//   - Crop: pointer arithmetic (zero copy)
//   - Scale-only OR Convert-only: src → output  (1 copy)
//   - Scale + Convert: src → scratch(scaled YUV) → output  (2 copies)
//   - Letterbox: convert dst_stride trick writes directly into padded output
// ---------------------------------------------------------------------------

class LibyuvTransformKernel : public TransformKernelBase {
public:
    LibyuvTransformKernel()  = default;
    ~LibyuvTransformKernel() override = default;

    const char* backend_name() const override { return "libyuv"; }
    BackendCaps capabilities()  const override;

    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    TransformResult transform(const FrameDesc&  src,
                              FrameDesc&        dst,
                              int               slot_id = 0,
                              const DynamicOps* dynamic  = nullptr) override;

private:

    // Format-specific helpers — all operate on raw pointers, no GStreamer deps
    bool scale_i420(const uint8_t* src_y, int src_stride_y,
                    const uint8_t* src_u, int src_stride_u,
                    const uint8_t* src_v, int src_stride_v,
                    int src_w, int src_h,
                    uint8_t* dst_y, int dst_stride_y,
                    uint8_t* dst_u, int dst_stride_u,
                    uint8_t* dst_v, int dst_stride_v,
                    int dst_w, int dst_h) const;

    bool scale_nv12(const uint8_t* src_y, int src_stride_y,
                    const uint8_t* src_uv, int src_stride_uv,
                    int src_w, int src_h,
                    uint8_t* dst_y, int dst_stride_y,
                    uint8_t* dst_uv, int dst_stride_uv,
                    int dst_w, int dst_h) const;

    bool scale_rgb(const uint8_t* src, int src_stride,
                   int src_w, int src_h,
                   uint8_t* dst, int dst_stride,
                   int dst_w, int dst_h) const;

    bool convert_color(VideoFormat src_fmt,
                       int src_w, int src_h,
                       int src_stride_y, int src_stride_u, int src_stride_v,
                       const uint8_t* src_y, const uint8_t* src_u, const uint8_t* src_v,
                       const uint8_t* src_uv,
                       uint8_t* dst_0, int dst_stride_0,
                       uint8_t* dst_1, int dst_stride_1,
                       uint8_t* dst_2, int dst_stride_2,
                       VideoFormat dst_fmt) const;

    void fill_padding(uint8_t* dst, int width, int height, int stride,
                      VideoFormat fmt) const;
};

}  // namespace dxt
