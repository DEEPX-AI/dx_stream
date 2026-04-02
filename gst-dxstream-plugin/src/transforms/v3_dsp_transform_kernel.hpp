#pragma once

#ifdef DEEPX_V3

#include "transform_kernel_base.hpp"

namespace dxt {

// ---------------------------------------------------------------------------
// V3DspTransformKernel
//
// Hardware-accelerated video transform using DeepX V3 DSP (dxcvext library).
// Supported platforms: DeepX V3 SoC only.
//
// Capabilities:
//   src : I420, RGB, BGR  (NV12 NOT supported by V3 DSP)
//   dst : RGB, BGR        (packed output for inference)
//   ops : crop + scale + letterbox padding via DSP-accelerated primitives
//
// Memory model:
//   DSP buffer allocation via dxcvext::allocDspBuffer().
//   Uses ping-pong double buffering (buffer_0 / buffer_1) to avoid
//   extra allocations across the crop → scale → convert pipeline.
//
// Execution pipeline (same as original V3DspPreprocessor):
//   1. memcpy src → DSP buffer_0
//   2. Crop  (if needed): buffer_0 → buffer_1, swap
//   3. Scale (if needed): current → work, swap
//   4. Color convert (if needed): current → work, swap
//   5. Padding + copy to dst output
// ---------------------------------------------------------------------------

class V3DspTransformKernel : public TransformKernelBase {
public:
    V3DspTransformKernel();
    ~V3DspTransformKernel() override;

    const char* backend_name() const override { return "v3dsp"; }
    BackendCaps capabilities()  const override;

    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

    TransformResult transform(const FrameDesc&  src,
                              FrameDesc&        dst,
                              int               slot_id = 0,
                              const DynamicOps* dynamic  = nullptr) override;

private:
    // DSP double-buffer (ping-pong)
    uint8_t* dsp_buffer_0_ = nullptr;
    uint8_t* dsp_buffer_1_ = nullptr;

    // DSP-accelerated operations
    bool crop_i420(const uint8_t* src, int src_stride_y, int src_stride_u, int src_stride_v,
                   size_t offset_y, size_t offset_u, size_t offset_v,
                   uint8_t* dst, int src_w, int src_h, int crop_x, int crop_y,
                   int crop_w, int crop_h) const;
    bool crop_rgb(const uint8_t* src, uint8_t* dst,
                  int src_w, int src_h, int crop_x, int crop_y,
                  int crop_w, int crop_h) const;

    bool scale_i420(const uint8_t* src, uint8_t* dst,
                    int src_w, int src_h, int dst_w, int dst_h) const;
    bool scale_rgb(const uint8_t* src, uint8_t* dst,
                   int src_w, int src_h, int dst_w, int dst_h,
                   VideoFormat fmt) const;

    bool convert_i420_to_packed(const uint8_t* src, uint8_t* dst,
                                int width, int height, VideoFormat dst_fmt) const;
};

}  // namespace dxt

#endif  // DEEPX_V3
