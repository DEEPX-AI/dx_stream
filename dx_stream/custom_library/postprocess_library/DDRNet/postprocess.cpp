#include <cmath>
#include <numeric>

#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <dxrt/dxrt_api.h>
#include <tuple>
#include <glib.h>
#include <gst/gst.h>

struct segmentationParams {
    bool needArgmax;
    int input_width;
    int input_height;
    int numClasses;
};

void Segmentation(GstBuffer* buf,
                    const dxrt::TensorPtrs& outputs, 
                    DXFrameMeta* frame_meta,
                    const segmentationParams& params) {
    std::ignore = buf;
    DXObjectMeta* object_meta = dx_acquire_obj_meta_from_pool();

    const int width = params.input_width;
    const int height = params.input_height;

    object_meta->_seg_cls_map.width = width;
    object_meta->_seg_cls_map.height = height;
    object_meta->_seg_cls_map.data.resize(width * height);

    // Optimized: single loop with direct pointer access
    const auto* input = static_cast<const uint16_t*>(outputs[0]->data());
    const int total_pixels = width * height;
    const int max_cls = params.numClasses;
    
    for (int i = 0; i < total_pixels; ++i) {
        int cls = input[i];
        object_meta->_seg_cls_map.data[i] = static_cast<unsigned char>((cls < max_cls) ? cls : 0);
    }

    dx_add_obj_meta_to_frame(frame_meta, object_meta);
}

extern "C" void PostProcess(GstBuffer* buf,
                            const dxrt::TensorPtrs& network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = object_meta;
    segmentationParams params = {.needArgmax = false,
                                 .input_width = 768,
                                 .input_height = 384,
                                 .numClasses = 3};

    Segmentation(buf, network_output, frame_meta, params);
}
