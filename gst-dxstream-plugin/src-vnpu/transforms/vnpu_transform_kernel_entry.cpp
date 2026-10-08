#include "vnpu_transform_kernel.hpp"
#include "../../general/dxcommon.hpp"

DX_CUSTOM_EXPORT dxt::IVideoTransformKernel* dx_create_vnpu_transform_kernel() {
    return new dxt::VnpuTransformKernel();
}
