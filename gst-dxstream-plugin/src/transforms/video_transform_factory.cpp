#include "video_transform_factory.hpp"

#include <algorithm>
#include <mutex>

// ---------------------------------------------------------------------------
// Core-only backend includes.
// ---------------------------------------------------------------------------

// libyuv is always available as the universal software fallback.
#include "libyuv_transform_kernel.hpp"

#include "./../../general/dx_dlfcn.h"
#include <gst/gst.h>

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

namespace dxt {

#ifdef _WIN32
#define DXVNPU_BACKEND_LIB "gstdxstream-vnpu.dll"
#define DXRGA_BACKEND_LIB "gstdxstream-rga.dll"
#define DXV3_BACKEND_LIB "gstdxstream-v3.dll"
#else
#define DXVNPU_BACKEND_LIB "libgstdxstream-vnpu.so"
#define DXRGA_BACKEND_LIB "libgstdxstream-rga.so"
#define DXV3_BACKEND_LIB "libgstdxstream-v3.so"
#define DXGLES_BACKEND_LIB "libgstdxstream-gles.so"
#endif

using CreateTransformKernelFn = IVideoTransformKernel* (*)();

struct TransformBackendFactory {
    const char* library;
    const char* symbol;
    void* handle = nullptr;
    CreateTransformKernelFn create = nullptr;
    std::once_flag load_once;
};

static CreateTransformKernelFn LoadTransformFactory(TransformBackendFactory& factory) {
    std::call_once(factory.load_once, [&factory] {
        factory.handle = dlopen(factory.library, RTLD_NOW);
        if (!factory.handle) {
            GST_DEBUG("transform backend unavailable: %s not found (%s)",
                      factory.library, dlerror());
            return;
        }

        factory.create = reinterpret_cast<CreateTransformKernelFn>(
            dlsym(factory.handle, factory.symbol));
        if (!factory.create) {
            GST_ERROR("%s not found in %s (%s)",
                      factory.symbol, factory.library, dlerror());
            dlclose(factory.handle);
            factory.handle = nullptr;
        }
    });
    return factory.create;
}

static TransformBackendFactory v3_factory = {
    DXV3_BACKEND_LIB, "dx_create_v3_transform_kernel"};
static TransformBackendFactory vnpu_factory = {
    DXVNPU_BACKEND_LIB, "dx_create_vnpu_transform_kernel"};
static TransformBackendFactory rga_factory = {
    DXRGA_BACKEND_LIB, "dx_create_rga_transform_kernel"};
#ifndef _WIN32
static TransformBackendFactory gles_factory = {
    DXGLES_BACKEND_LIB, "dx_create_gles_transform_kernel"};
#endif

static void ensure_transform_kernel_debug_category() {
    static gsize initialized = 0;
    if (g_once_init_enter(&initialized)) {
        if (!transform_kernel_cat) {
            GST_DEBUG_CATEGORY_INIT(transform_kernel_cat, "transform_kernel", 0,
                                    "Video transform kernels");
        }
        g_once_init_leave(&initialized, 1);
    }
}

static std::unique_ptr<IVideoTransformKernel> CreateV3TransformKernel() {
    auto create = LoadTransformFactory(v3_factory);
    return create ? std::unique_ptr<IVideoTransformKernel>(create()) : nullptr;
}

// vnpu is not linked into this plugin. Loaded on demand from the sibling
// plugin (gstdxstream-vnpu), same as the dxvnpu infer backend.
static std::unique_ptr<IVideoTransformKernel> CreateVnpuTransformKernel() {
    auto create = LoadTransformFactory(vnpu_factory);
    return create ? std::unique_ptr<IVideoTransformKernel>(create()) : nullptr;
}

// rga is not linked into this plugin. Loaded on demand from the sibling
// plugin (gstdxstream-rga), same pattern as the vnpu transform kernel.
static std::unique_ptr<IVideoTransformKernel> CreateRgaTransformKernel() {
    auto create = LoadTransformFactory(rga_factory);
    return create ? std::unique_ptr<IVideoTransformKernel>(create()) : nullptr;
}

// OpenGL ES (any EGL dma-buf capable GPU) is loaded on demand from the sibling plugin (gstdxstream-gles).
static std::unique_ptr<IVideoTransformKernel> CreateGlesTransformKernel() {
#ifdef _WIN32
    return nullptr;
#else
    auto create = LoadTransformFactory(gles_factory);
    return create ? std::unique_ptr<IVideoTransformKernel>(create()) : nullptr;
#endif
}

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
    ensure_transform_kernel_debug_category();
    std::unique_ptr<IVideoTransformKernel> result;

    // 1. V3 DSP (highest priority)
    result = try_init(CreateV3TransformKernel(), dst_template, ops, src_format);
    if (result) return result;

    // 2. VNPU hardware
    result = try_init(CreateVnpuTransformKernel(), dst_template, ops, src_format);
    if (result) return result;

    // 3. RGA hardware
    result = try_init(CreateRgaTransformKernel(), dst_template, ops, src_format);
    if (result) return result;

    // 4. OpenGL ES GPU (generic; after SoC-specific engines)
    result = try_init(CreateGlesTransformKernel(), dst_template, ops, src_format);
    if (result) return result;

    // 5. libyuv software fallback (always available)
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
    ensure_transform_kernel_debug_category();
    if (backend_name == "vnpu") {
        return try_init(CreateVnpuTransformKernel(), dst_template, ops,
                        VideoFormat::NV12, false);
    }

    if (backend_name == "rga") {
        return try_init(CreateRgaTransformKernel(), dst_template, ops,
                        VideoFormat::NV12, false);
    }

    if (backend_name == "v3dsp") {
        return try_init(CreateV3TransformKernel(), dst_template, ops,
                        VideoFormat::NV12, false);
    }

    if (backend_name == "gles") {
        return try_init(CreateGlesTransformKernel(), dst_template, ops,
                        VideoFormat::NV12, false);
    }

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
    ensure_transform_kernel_debug_category();
    std::vector<std::string> backends;

    if (LoadTransformFactory(v3_factory)) backends.push_back("v3dsp");
    if (LoadTransformFactory(vnpu_factory)) backends.push_back("vnpu");
    if (LoadTransformFactory(rga_factory)) backends.push_back("rga");
#ifndef _WIN32
    if (LoadTransformFactory(gles_factory)) backends.push_back("gles");
#endif

    backends.push_back("libyuv");

    return backends;
}

}  // namespace dxt
