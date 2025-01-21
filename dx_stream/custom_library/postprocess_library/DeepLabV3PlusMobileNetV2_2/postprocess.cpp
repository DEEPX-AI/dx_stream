#include "dxcommon.hpp"
#include "dxrt/dxrt_api.h"
#include "gst-dxmeta.hpp"
#include <cmath>
#include <numeric>

struct segmentationParams {
    bool needArgmax;
    int input_width;
    int input_height;
    int numClasses;
};

void Segmentation(std::vector<shared_ptr<dxrt::Tensor>> outputs,
                  DXFrameMeta *frame_meta, segmentationParams &params) {

    DXObjectMeta *object_meta = dx_create_object_meta(frame_meta->_buf);

    object_meta->_seg_cls_map.width = params.input_width;
    object_meta->_seg_cls_map.height = params.input_height;

    object_meta->_seg_cls_map.data =
        new unsigned char[params.input_width * params.input_height];

    for (int h = 0; h < params.input_height; h++) {
        for (int w = 0; w < params.input_width; w++) {
            if (params.needArgmax) {

                float *input = (float *)outputs.front()->data();

                int maxIdx = 0;
                int align = outputs.front()->shape().back();
                for (int c = 0; c < params.numClasses; c++) {
                    if (input[(params.input_width * h + w) * align + maxIdx] <
                        input[(params.input_width * h + w) * align + c]) {
                        maxIdx = c;
                    }
                }

                object_meta->_seg_cls_map.data[params.input_width * h + w] =
                    maxIdx;

            } else {

                uint16_t *input = (uint16_t *)outputs.front()->data();

                int cls = input[params.input_width * h + w];
                if (cls < params.numClasses) {
                    object_meta->_seg_cls_map.data[params.input_width * h + w] =
                        cls;
                }
            }
        }
    }
    dx_add_object_meta_to_frame_meta(object_meta, frame_meta);
}

extern "C" void
PostProcess(std::vector<shared_ptr<dxrt::Tensor>> network_output,
            DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    segmentationParams params = {.needArgmax = true,
                                 .input_width = 640,
                                 .input_height = 640,
                                 .numClasses = 19};

    Segmentation(network_output, frame_meta, params);
}
