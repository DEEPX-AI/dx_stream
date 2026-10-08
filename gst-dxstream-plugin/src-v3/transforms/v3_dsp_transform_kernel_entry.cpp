#include "v3_dsp_transform_kernel.hpp"
#include "../../general/dxcommon.hpp"

DX_CUSTOM_EXPORT dxt::IVideoTransformKernel* dx_create_v3_transform_kernel() {
    return new dxt::V3DspTransformKernel();
}
