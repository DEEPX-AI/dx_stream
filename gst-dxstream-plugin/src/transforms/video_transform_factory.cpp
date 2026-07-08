#include "video_transform_factory.hpp"

#include <algorithm>

// ---------------------------------------------------------------------------
// Backend includes — guarded by compile-time defines.
// Add new backends here as they are implemented.
// ---------------------------------------------------------------------------

#ifdef DEEPX_V3
#include "v3_dsp_transform_kernel.hpp"
#endif

#ifdef HAVE_DXVNPU
#include "vnpu_transform_kernel.hpp"
#endif

#ifdef HAVE_LIBRGA
#include "rga_transform_kernel.hpp"
#endif

// libyuv is always available as the universal software fallback.
#include "libyuv_transform_kernel.hpp"

#include <gst/gst.h>

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

namespace dxt {

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

std::unique_ptr<IVideoTransformKernel> VideoTransformFactory::try_init(
    std::unique_ptr<IVideoTransformKernel> kernel,
    const FrameDesc&    dst_template,
    const TransformOps& ops,
    VideoFormat         src_format,
    bool                check_formats)
{
    if (!kernel) return nullptr;

    if (check_formats) {
        const auto& caps = kernel->capabilities();

        // Check src format support
        const auto& src_fmts = caps.src_formats;
        if (std::find(src_fmts.begin(), src_fmts.end(), src_format) == src_fmts.end()) {
            GST_DEBUG("VideoTransformFactory: backend '%s' does not support src format, skipping",
                      caps.name);
            return nullptr;
        }

        // Check dst format support
        const auto& dst_fmts = caps.dst_formats;
        if (std::find(dst_fmts.begin(), dst_fmts.end(), dst_template.format) == dst_fmts.end()) {
            GST_DEBUG("VideoTransformFactory: backend '%s' does not support dst format, skipping",
                      caps.name);
            return nullptr;
        }
    }

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
    const TransformOps& ops,
    VideoFormat         src_format)
{
    std::unique_ptr<IVideoTransformKernel> result;

    // 1. V3 DSP (highest priority)
#ifdef DEEPX_V3
    result = try_init(std::make_unique<V3DspTransformKernel>(), dst_template, ops, src_format);
    if (result) return result;
#endif

    // 2. VNPU hardware
#ifdef HAVE_DXVNPU
    result = try_init(std::make_unique<VnpuTransformKernel>(), dst_template, ops, src_format);
    if (result) return result;
#endif

    // 3. RGA hardware
#ifdef HAVE_LIBRGA
    result = try_init(std::make_unique<RgaTransformKernel>(), dst_template, ops, src_format);
    if (result) return result;
#endif

    // 4. libyuv software fallback (always available)
    result = try_init(std::make_unique<LibyuvTransformKernel>(), dst_template, ops, src_format);
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
#ifdef HAVE_DXVNPU
    if (backend_name == "vnpu") {
        return try_init(std::make_unique<VnpuTransformKernel>(), dst_template, ops,
                        VideoFormat::NV12, false);
    }
#endif

#ifdef HAVE_LIBRGA
    if (backend_name == "rga") {
        return try_init(std::make_unique<RgaTransformKernel>(), dst_template, ops,
                        VideoFormat::NV12, false);
    }
#endif

#ifdef DEEPX_V3
    if (backend_name == "v3dsp") {
        return try_init(std::make_unique<V3DspTransformKernel>(), dst_template, ops,
                        VideoFormat::NV12, false);
    }
#endif

    if (backend_name == "libyuv") {
        return try_init(std::make_unique<LibyuvTransformKernel>(), dst_template, ops,
                        VideoFormat::NV12, false);
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

#ifdef HAVE_DXVNPU
    backends.push_back("vnpu");
#endif

#ifdef HAVE_LIBRGA
    backends.push_back("rga");
#endif

    backends.push_back("libyuv");

    return backends;
}

}  // namespace dxt
