#pragma once

#include "video_transform_kernel.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dxt {

// ---------------------------------------------------------------------------
// VideoTransformFactory
//
// Selects the best available backend at runtime:
//   Priority: V3 DSP  >  RGA  >  libyuv
//
// Backend availability is determined by compile-time defines:
//   DEEPX_V3    — V3 DSP backend
//   HAVE_LIBRGA — RGA hardware backend
//   (always)    — libyuv software backend (fallback)
//
// Factory validates each backend via capabilities() before calling init().
// If init() fails, it falls through to the next candidate automatically.
// ---------------------------------------------------------------------------

class VideoTransformFactory {
public:
    // Auto-select best available backend.
    // Returns nullptr only when ALL backends refuse the configuration.
    // In practice libyuv always accepts, so nullptr should not happen.
    static std::unique_ptr<IVideoTransformKernel> create(
        const FrameDesc&    dst_template,
        const TransformOps& ops);

    // Explicitly request a named backend ("rga", "v3dsp", "libyuv").
    // Useful for unit tests and per-platform benchmarks.
    // Returns nullptr if the backend is unavailable or refuses the config.
    static std::unique_ptr<IVideoTransformKernel> create_backend(
        const std::string&  backend_name,
        const FrameDesc&    dst_template,
        const TransformOps& ops);

    // Return names of backends compiled into this build.
    static std::vector<std::string> available_backends();

private:
    // Helper: try to init a kernel; return it on success, null on failure.
    static std::unique_ptr<IVideoTransformKernel> try_init(
        std::unique_ptr<IVideoTransformKernel> kernel,
        const FrameDesc&    dst_template,
        const TransformOps& ops);
};

}  // namespace dxt
