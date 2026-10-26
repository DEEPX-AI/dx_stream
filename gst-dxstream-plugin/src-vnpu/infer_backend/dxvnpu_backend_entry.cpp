#include "dxvnpu_backend.hpp"
#include "../../general/dxcommon.hpp"

DX_CUSTOM_EXPORT IInferBackend* dx_create_dxvnpu_backend() {
    return new DxvnpuBackend();
}
