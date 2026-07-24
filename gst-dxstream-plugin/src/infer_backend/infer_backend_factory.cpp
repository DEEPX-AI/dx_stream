#include "infer_backend_factory.hpp"

#ifdef HAVE_DXRT
#include "dxrt_backend.hpp"
#endif

#ifdef HAVE_DXVNPU
#include "dxvnpu_backend.hpp"
#endif

#include <gst/gst.h>

#define GST_CAT_DEFAULT inference_backend_cat
GST_DEBUG_CATEGORY_EXTERN(inference_backend_cat);

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
#ifdef HAVE_DXVNPU
        return std::make_unique<DxvnpuBackend>();
#else
        GST_ERROR("dxvnpu backend not compiled in");
        return nullptr;
#endif

    case BackendType::AUTO:
    default:
#ifdef HAVE_DXRT
        GST_INFO("Auto backend: using dxrt");
        return std::make_unique<DxrtBackend>();
#elif defined(HAVE_DXVNPU)
        GST_INFO("Auto backend: using dxvnpu");
        return std::make_unique<DxvnpuBackend>();
#else
        GST_ERROR("No inference backend available");
        return nullptr;
#endif
    }
}
