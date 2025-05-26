#include <cmath>
#include <numeric>

#include "dxcommon.hpp"
#include "gst-dxmeta.hpp"

struct segmentationParams {
    bool needArgmax;
    int input_width;
    int input_height;
    int numClasses;
};

// max index를 찾는 함수 분리
int argmax_float(const float *input, int h, int w, int width, int height,
                 int numClasses, int align) {
    int maxIdx = 0;
    if (align == 32) {
        for (int c = 0; c < numClasses; c++) {
            if (input[(width * h + w) * align + maxIdx] <
                input[(width * h + w) * align + c]) {
                maxIdx = c;
            }
        }
    } else {
        for (int c = 0; c < numClasses; c++) {
            if (input[(height * width * maxIdx) + (width * h) + w] <
                input[(height * width * c) + (width * h) + w]) {
                maxIdx = c;
            }
        }
    }
    return maxIdx;
}

// 클래스 값 얻는 함수 분리 (uint16_t 버전)
uint16_t get_class_uint16(const uint16_t *input, int h, int w, int width,
                          int numClasses) {
    int cls = input[width * h + w];
    return (cls < numClasses) ? cls
                              : 0; // numClasses 넘으면 0으로 처리 (필요시 변경)
}

void Segmentation(std::vector<dxs::DXTensor> outputs, DXFrameMeta *frame_meta,
                  segmentationParams &params) {
    DXObjectMeta *object_meta = dx_create_object_meta(frame_meta->_buf);

    int width = params.input_width;
    int height = params.input_height;
    int numClasses = params.numClasses;

    object_meta->_seg_cls_map.width = width;
    object_meta->_seg_cls_map.height = height;
    object_meta->_seg_cls_map.data.resize(width * height);

    if (params.needArgmax) {
        float *input = (float *)outputs[0]._data;
        int align = outputs[0]._shape.back();

        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                object_meta->_seg_cls_map.data[width * h + w] =
                    argmax_float(input, h, w, width, height, numClasses, align);
            }
        }
    } else {
        uint16_t *input = (uint16_t *)outputs[0]._data;

        for (int h = 0; h < height; h++) {
            for (int w = 0; w < width; w++) {
                object_meta->_seg_cls_map.data[width * h + w] =
                    get_class_uint16(input, h, w, width, numClasses);
            }
        }
    }

    dx_add_object_meta_to_frame_meta(object_meta, frame_meta);
}

extern "C" void PostProcess(std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {
    segmentationParams params = {.needArgmax = true,
                                 .input_width = 640,
                                 .input_height = 640,
                                 .numClasses = 19};

    Segmentation(network_output, frame_meta, params);
}
