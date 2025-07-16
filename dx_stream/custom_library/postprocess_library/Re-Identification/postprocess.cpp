#include "gst-dxmeta.hpp"
#include <cmath>
#include <iostream>
#include <vector>

extern "C" void PostProcess(std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    object_meta->_body_feature.clear();

    float norm = 0.0f;
    int feature_length = network_output[0]._shape.size() - 1;
    float *vec = (float *)network_output[0]._data;
    for (int i = 0; i < network_output[0]._shape[feature_length]; i++) {
        float v = *(vec + i);
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
    for (int i = 0; i < network_output[0]._shape[feature_length]; i++) {
        float v = *(vec + i);
        object_meta->_body_feature.push_back(v / norm);
    }
}