#pragma once

#include "dxcommon.hpp"
#include "video_transform_kernel.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dxt {

// ---------------------------------------------------------------------------
// VideoTransformFactory
//
// Selects the best available backend at runtime:
//   Priority: V3 DSP  >  VNPU  >  RGA  >  libyuv
//
// Hardware backends are discovered from sibling plugins at runtime.
// libyuv is always available as the software fallback.
//
// Factory validates each backend via capabilities() before calling init().
// If init() fails, it falls through to the next candidate automatically.
// ---------------------------------------------------------------------------

// DX_API: called directly (not just via GStreamer registry) by white-box
// tests under test/dxvnpu — same cross-DLL export requirement as
// TransformKernelBase (see that header for the full rationale).
class DX_API VideoTransformFactory {
public:
    // Auto-select best available backend.
    // src_format: source pixel format hint for backend selection.
    //   Backends whose capabilities().src_formats do not include src_format
    //   are skipped, ensuring a compatible backend is selected.
    // Returns nullptr only when ALL backends refuse the configuration.
    // In practice libyuv always accepts, so nullptr should not happen.
    static std::unique_ptr<IVideoTransformKernel> create(
        const FrameDesc&    dst_template,
        const TransformOps& ops,
        VideoFormat         src_format);

    // Explicitly request a named backend ("rga", "vnpu", "v3dsp", "libyuv").
    // Useful for unit tests and per-platform benchmarks.
    // Returns nullptr if the backend is unavailable or refuses the config.
    static std::unique_ptr<IVideoTransformKernel> create_backend(
        const std::string&  backend_name,
        const FrameDesc&    dst_template,
        const TransformOps& ops);

    // Return names of installed hardware backends plus libyuv.
    static std::vector<std::string> available_backends();

private:
    // Helper: try to init a kernel; return it on success, null on failure.
    // When check_formats is true, validates src/dst format support.
    static std::unique_ptr<IVideoTransformKernel> try_init(
        std::unique_ptr<IVideoTransformKernel> kernel,
        const FrameDesc&    dst_template,
        const TransformOps& ops,
        VideoFormat         src_format,
        bool                check_formats = true);
};

}  // namespace dxt
