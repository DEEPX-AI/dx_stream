#include "gles_transform_kernel.hpp"
#include "../../general/dxcommon.hpp"

DX_CUSTOM_EXPORT dxt::IVideoTransformKernel* dx_create_gles_transform_kernel() {
    return new dxt::GlesTransformKernel();
}
