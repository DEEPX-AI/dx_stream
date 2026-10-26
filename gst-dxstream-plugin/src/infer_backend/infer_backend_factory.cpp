#include "infer_backend_factory.hpp"

#ifdef HAVE_DXRT
#include "dxrt_backend.hpp"
#endif

#include "./../../general/dx_dlfcn.h"
#include <gst/gst.h>

#define GST_CAT_DEFAULT inference_backend_cat
GST_DEBUG_CATEGORY_EXTERN(inference_backend_cat);

#ifdef _WIN32
#define DXVNPU_BACKEND_LIB "gstdxstream-vnpu.dll"
#else
#define DXVNPU_BACKEND_LIB "libgstdxstream-vnpu.so"
#endif

// dxvnpu is not linked into this plugin. Loaded on demand from the sibling
// plugin (gstdxstream-vnpu), which is installed with with_dxvnpu=enabled.
static std::unique_ptr<IInferBackend> CreateDxvnpuBackend() {
    using CreateFn = IInferBackend* (*)();
    static const CreateFn create_fn = [] {
        void* handle = dlopen(DXVNPU_BACKEND_LIB, RTLD_NOW);
        if (!handle) {
            GST_ERROR("dxvnpu backend requested but %s not found (%s). "
                      "Is dx_vnpu SDK / gst-dxstream-vnpu installed?",
                      DXVNPU_BACKEND_LIB, dlerror());
            return static_cast<CreateFn>(nullptr);
        }

        auto create = reinterpret_cast<CreateFn>(
            dlsym(handle, "dx_create_dxvnpu_backend"));
        if (!create) {
            GST_ERROR("dx_create_dxvnpu_backend symbol not found in %s (%s)",
                      DXVNPU_BACKEND_LIB, dlerror());
            dlclose(handle);
        }
        return create;
    }();

    return create_fn ? std::unique_ptr<IInferBackend>(create_fn()) : nullptr;
}

std::unique_ptr<IInferBackend> InferBackendFactory::Create(BackendType type) {

    switch (type) {
    case BackendType::DXRT:
#ifdef HAVE_DXRT
        return std::make_unique<DxrtBackend>();
#else
        GST_ERROR("dxrt backend not compiled in");
        return nullptr;
#endif

    case BackendType::DXVNPU:
        return CreateDxvnpuBackend();

    case BackendType::AUTO:
    default:
#ifdef HAVE_DXRT
        GST_INFO("Auto backend: using dxrt");
        return std::make_unique<DxrtBackend>();
#else
        GST_INFO("Auto backend: trying dxvnpu");
        return CreateDxvnpuBackend();
#endif
    }
}
