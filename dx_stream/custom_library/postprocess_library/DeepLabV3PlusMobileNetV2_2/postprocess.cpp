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

int argmax_float(const float* input, int h, int w, int width, int height,
                 int numClasses, int align) {
    int maxIdx = 0;
    if (align == 32) {
        for (int c = 0; c < numClasses; ++c) {
            if (input[(width * h + w) * align + maxIdx] <
                input[(width * h + w) * align + c]) {
                maxIdx = c;
            }
        }
    } else {
        for (int c = 0; c < numClasses; ++c) {
            if (input[(height * width * maxIdx) + (width * h) + w] <
                input[(height * width * c) + (width * h) + w]) {
                maxIdx = c;
            }
        }
    }
    return maxIdx;
}

uint16_t get_class_uint16(const uint16_t* input, int h, int w, int width,
                          int numClasses) {
    int cls = input[width * h + w];
    return (cls < numClasses) ? static_cast<uint16_t>(cls)
                              : static_cast<uint16_t>(0);
}

void Segmentation(GstBuffer* buf,
                    const dxrt::TensorPtrs& outputs, 
                    DXFrameMeta* frame_meta,
                    const segmentationParams& params) {
    std::ignore = buf;
    DXObjectMeta* object_meta = dx_acquire_obj_meta_from_pool();

    const int width = params.input_width;
    const int height = params.input_height;
    const int numClasses = params.numClasses;

    object_meta->_seg_cls_map.width = width;
    object_meta->_seg_cls_map.height = height;
    object_meta->_seg_cls_map.data.resize(width * height);

    if (params.needArgmax) {
        const auto* input = static_cast<const float*>(outputs[0]->data());
        const auto align = static_cast<int>(outputs[0]->shape().back());

        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                object_meta->_seg_cls_map.data[width * h + w] =
                    static_cast<unsigned char>(argmax_float(input, h, w, width, height, numClasses, align));
            }
        }
    } else {
        const auto* input = static_cast<const uint16_t*>(outputs[0]->data());

        for (int h = 0; h < height; ++h) {
            for (int w = 0; w < width; ++w) {
                object_meta->_seg_cls_map.data[width * h + w] =
                    static_cast<unsigned char>(get_class_uint16(input, h, w, width, numClasses));
            }
        }
    }

    dx_add_obj_meta_to_frame(frame_meta, object_meta);
}

extern "C" void PostProcess(GstBuffer* buf,
                            const dxrt::TensorPtrs& network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = object_meta;
    segmentationParams params = {.needArgmax = true,
                                 .input_width = 640,
                                 .input_height = 640,
                                 .numClasses = 19};

    Segmentation(buf, network_output, frame_meta, params);
}
