#include "video_transform_factory.hpp"

// ---------------------------------------------------------------------------
// Backend includes — guarded by compile-time defines.
// Add new backends here as they are implemented.
// ---------------------------------------------------------------------------

#ifdef DEEPX_V3
#include "v3_dsp_transform_kernel.hpp"
#endif

#ifdef HAVE_LIBRGA
#include "rga_transform_kernel.hpp"
#endif

// libyuv is always available as the universal software fallback.
#include "libyuv_transform_kernel.hpp"

#include <gst/gst.h>  // GST_WARNING / GST_ERROR for factory diagnostics

namespace dxt {

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

std::unique_ptr<IVideoTransformKernel> VideoTransformFactory::try_init(
    std::unique_ptr<IVideoTransformKernel> kernel,
    const FrameDesc&    dst_template,
    const TransformOps& ops)
{
    if (!kernel) return nullptr;
    if (kernel->init(dst_template, ops)) {
        return kernel;
    }
    GST_WARNING("VideoTransformFactory: backend '%s' rejected config, trying next",
                kernel->backend_name());
    return nullptr;
}

// ---------------------------------------------------------------------------
// create — auto-select best backend
// ---------------------------------------------------------------------------

std::unique_ptr<IVideoTransformKernel> VideoTransformFactory::create(
    const FrameDesc&    dst_template,
    const TransformOps& ops)
{
    std::unique_ptr<IVideoTransformKernel> result;

    // 1. V3 DSP (highest priority)
#ifdef DEEPX_V3
    result = try_init(std::make_unique<V3DspTransformKernel>(), dst_template, ops);
    if (result) return result;
#endif

    // 2. RGA hardware
#ifdef HAVE_LIBRGA
    result = try_init(std::make_unique<RgaTransformKernel>(), dst_template, ops);
    if (result) return result;
#endif

    // 3. libyuv software fallback (always available)
    result = try_init(std::make_unique<LibyuvTransformKernel>(), dst_template, ops);
    if (result) return result;

    GST_ERROR("VideoTransformFactory: no backend available for requested config");
    return nullptr;
}

// ---------------------------------------------------------------------------
// create_backend — explicit backend selection
// ---------------------------------------------------------------------------

std::unique_ptr<IVideoTransformKernel> VideoTransformFactory::create_backend(
    const std::string&  backend_name,
    const FrameDesc&    dst_template,
    const TransformOps& ops)
{
#ifdef HAVE_LIBRGA
    if (backend_name == "rga") {
        return try_init(std::make_unique<RgaTransformKernel>(), dst_template, ops);
    }
#endif

#ifdef DEEPX_V3
    if (backend_name == "v3dsp") {
        return try_init(std::make_unique<V3DspTransformKernel>(), dst_template, ops);
    }
#endif

    if (backend_name == "libyuv") {
        return try_init(std::make_unique<LibyuvTransformKernel>(), dst_template, ops);
    }

    GST_WARNING("VideoTransformFactory: unknown or unavailable backend '%s'",
                backend_name.c_str());
    return nullptr;
}

// ---------------------------------------------------------------------------
// available_backends
// ---------------------------------------------------------------------------

std::vector<std::string> VideoTransformFactory::available_backends() {
    std::vector<std::string> backends;

#ifdef DEEPX_V3
    backends.push_back("v3dsp");
#endif

#ifdef HAVE_LIBRGA
    backends.push_back("rga");
#endif

    backends.push_back("libyuv");

    return backends;
}

}  // namespace dxt
