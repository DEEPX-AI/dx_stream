#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <dxrt/dxrt_api.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <tuple>
#include <glib.h>
#include <gst/gst.h>

extern "C" void PostProcess(GstBuffer* buf,
                            const dxrt::TensorPtrs& network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = frame_meta;

    object_meta->_body_feature.clear();

    float norm = 0.0f;
    const int feature_length = static_cast<int>(network_output[0]->shape().size()) - 1;
    const auto* vec = static_cast<float*>(network_output[0]->data());
    for (int i = 0; i < network_output[0]->shape()[feature_length]; ++i) {
        const float v = vec[i];
        norm += v * v;
    }
    norm = std::sqrt(norm);

    // Avoid division by zero
    if (norm == 0.0f) {
        std::cerr
            << "Warning: Norm of the vector is zero. Normalization skipped."
            << std::endl;
        return;
    }

    // Normalize the vector
    for (int i = 0; i < network_output[0]->shape()[feature_length]; ++i) {
        object_meta->_body_feature.push_back(vec[i] / norm);
    }
}