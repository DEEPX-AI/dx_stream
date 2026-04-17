#pragma once

// ---------------------------------------------------------------------------
// TransformKernelBase
//
// Concrete base for all IVideoTransformKernel backend implementations.
// Provides:
//   - Common state: dst_template_, ops_, initialized_, scratch_
//   - Default init() that stores config (subclass can override to add HW setup)
//   - Shared helpers: effective_crop(), compute_dst_rect()
//
// Backends that generate intermediate scaled buffers (libyuv, v3dsp) use
// scratch_ for per-slot storage to support multi-stream concurrency.
// ---------------------------------------------------------------------------

#include "video_transform_kernel.hpp"
#include <unordered_map>
#include <vector>

namespace dxt {

class TransformKernelBase : public IVideoTransformKernel {
public:
    ~TransformKernelBase() override = default;

    // Stores dst_template + ops and marks initialized_ = true.
    // Subclasses that need HW resource allocation should override, do their
    // setup, then call TransformKernelBase::init() at the end.
    bool init(const FrameDesc& dst_template, const TransformOps& ops) override;

protected:
    FrameDesc    dst_template_;
    TransformOps ops_;
    bool         initialized_ = false;

    // Per-slot scratch buffer for intermediate scaled data.
    // Key = slot_id (stream_id in multi-stream; 0 for single-stream).
    std::unordered_map<int, std::vector<uint8_t>> scratch_;

    // ---------------------------------------------------------------------------
    // Helpers — shared identical implementations across backends
    // ---------------------------------------------------------------------------

    // Resolve effective crop for this call.
    // Priority: dynamic->crop_override > ops_.crop > full-frame (no crop).
    CropRect effective_crop(const FrameDesc& src, const DynamicOps* dynamic) const;

    // Compute letterbox content placement within dst buffer.
    // When keep_aspect_ratio == false: dst_x = dst_y = 0, content = full dst.
    // When keep_aspect_ratio == true:  content is centred, maintaining src ratio.
    void compute_dst_rect(int src_w, int src_h,
                          int& dst_x, int& dst_y,
                          int& content_w, int& content_h) const;
};

}  // namespace dxt
