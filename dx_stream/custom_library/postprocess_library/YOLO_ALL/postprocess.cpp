#include "dx_stream/gst-dxframemeta.hpp"
#include "dx_stream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

#define sigmoid(x) (1 / (1 + std::exp(-x)))

struct Decoded {
    std::string labelname;
    std::vector<float> kpts;
    float box[4];
    float score;
    int label;

    ~Decoded() = default;
    Decoded() = default;
    Decoded(std::string _labelname, float _score, unsigned int _label,
            float data1, float data2, float data3, float data4);
    Decoded(std::string _labelname, float _score, unsigned int _label,
            float data1, float data2, float data3, float data4,
            float *keypoints, int numKeypoints);
};

Decoded::Decoded(std::string _labelname, float _score, unsigned int _label,
                 float data1, float data2, float data3, float data4)
    : labelname(std::move(_labelname)), score(_score), label(_label) {
    box[0] = data1;
    box[1] = data2;
    box[2] = data3;
    box[3] = data4;
}

Decoded::Decoded(std::string _labelname, float _score, unsigned int _label,
                 float data1, float data2, float data3, float data4,
                 float *keypoints, int numKeypoints)
    : labelname(std::move(_labelname)), score(_score), label(_label) {
    box[0] = data1;
    box[1] = data2;
    box[2] = data3;
    box[3] = data4;

    kpts.assign(keypoints, keypoints + 3 * numKeypoints);
}

struct YoloLayerParam {
    int numGridX;
    int numGridY;
    int numBoxes;
    std::vector<float> anchorWidth;
    std::vector<float> anchorHeight;
    std::vector<int> tensorIdx;
    float scaleX = 0;
    float scaleY = 0;
    YoloLayerParam() = default;
    YoloLayerParam(int _gx, int _gy, int _numB,
                   const std::vector<float> &_vAnchorW,
                   const std::vector<float> &_vAnchorH,
                   const std::vector<int> &_vTensorIdx, float _sx = 0.f,
                   float _sy = 0.f)
        : numGridX(_gx), numGridY(_gy), numBoxes(_numB), anchorWidth(_vAnchorW),
          anchorHeight(_vAnchorH), tensorIdx(_vTensorIdx), scaleX(_sx),
          scaleY(_sy) {}
};

struct YoloParam {
    int height;
    int width;
    float confThreshold;
    float scoreThreshold;
    float iouThreshold;
    int numBoxes;
    int numClasses;
    int numKeypoints=0;
    std::vector<YoloLayerParam> layers;
    std::vector<std::string> classNames;
};

YoloLayerParam createYoloLayerParam(int _gx, int _gy, int _numB,
                                    const std::vector<float> &_vAnchorW,
                                    const std::vector<float> &_vAnchorH,
                                    const std::vector<int> &_vTensorIdx,
                                    float _sx = 0.f, float _sy = 0.f) {
    YoloLayerParam s;
    s.numGridX = _gx;
    s.numGridY = _gy;
    s.numBoxes = _numB;
    s.anchorWidth = _vAnchorW;
    s.anchorHeight = _vAnchorH;
    s.tensorIdx = _vTensorIdx;
    s.scaleX = _sx;
    s.scaleY = _sy;
    return s;
}

static bool scoreComapre(const std::pair<float, int> &a,
                         const std::pair<float, int> &b) {
    if (a.first > b.first)
        return true;
    else
        return false;
};

inline void ComputeBox(const YoloParam &param, const YoloLayerParam &layer,
                       const int gX, const int gY, const float scale_x_y,
                       const float x_raw, const float y_raw, const float w_raw,
                       const float h_raw, const int box_idx, int strideX,
                       int strideY, std::vector<float> &Boxes) {
    int x, y, w, h;

    if (!layer.anchorHeight.empty()) {
        if (scale_x_y == 0) {
            x = (x_raw * 2. - 0.5 + gX) * strideX;
            y = (y_raw * 2. - 0.5 + gY) * strideY;
        } else {
            x = (x_raw * scale_x_y - 0.5 * (scale_x_y - 1) + gX) * strideX;
            y = (y_raw * scale_x_y - 0.5 * (scale_x_y - 1) + gY) * strideY;
        }
        w = (w_raw * w_raw * 4.) * layer.anchorWidth[box_idx];
        h = (h_raw * h_raw * 4.) * layer.anchorHeight[box_idx];
    } else {
        x = (gX + x_raw) * strideX;
        y = (gY + y_raw) * strideY;
        w = exp(w_raw) * strideX;
        h = exp(h_raw) * strideY;
    }

    Boxes.emplace_back(x - w / 2.);
    Boxes.emplace_back(y - h / 2.);
    Boxes.emplace_back(x + w / 2.);
    Boxes.emplace_back(y + h / 2.);
}

void ProcessBBOX(const dxs::DXTensor &output,
                 std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
                 std::vector<float> &Boxes, YoloParam &param, int &boxIdx) {
    auto *dataSrc = (dxs::DeviceBoundingBox_t *)output._data;

    for (int i = 0; i < output._shape[1]; i++) {
        auto *data = dataSrc + i;
        const auto &layer = param.layers[data->layer_idx];

        int strideX = param.width / layer.numGridX;
        int strideY = param.height / layer.numGridY;
        int gX = data->grid_x;
        int gY = data->grid_y;

        ScoreIndices[data->label].emplace_back(data->score, boxIdx);

        ComputeBox(param, layer, gX, gY, layer.scaleX, data->x, data->y,
                   data->w, data->h, data->box_idx, strideX, strideY, Boxes);

        boxIdx++;
    }
}

void ProcessPOSE(const dxs::DXTensor &output,
                 std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
                 std::vector<float> &Boxes, std::vector<float> &Keypoints,
                 YoloParam &param, int &boxIdx) {
    auto *dataSrc = (dxs::DevicePose_t *)output._data;

    for (int i = 0; i < output._shape[1]; i++) {
        auto *data = dataSrc + i;
        const auto &layer = param.layers[data->layer_idx];

        int strideX = param.width / layer.numGridX;
        int strideY = param.height / layer.numGridY;
        int gX = data->grid_x;
        int gY = data->grid_y;

        ScoreIndices[0].emplace_back(data->score, boxIdx);

        ComputeBox(param, layer, gX, gY, layer.scaleX, data->x, data->y,
                   data->w, data->h, data->box_idx, strideX, strideY, Boxes);

        for (int k = 0; k < param.numKeypoints; k++) {
            Keypoints.emplace_back((data->kpts[k][0] * 2. - 0.5 + gX) *
                                   strideX);
            Keypoints.emplace_back((data->kpts[k][1] * 2. - 0.5 + gY) *
                                   strideY);
            Keypoints.emplace_back(sigmoid(data->kpts[k][2]));
        }

        boxIdx++;
    }
}

void ProcessYoloV8V9(
    const float *dataSrc, int dimensions, int rows,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, YoloParam &param, int &boxIdx) {
    // Transpose data (84, 8400) -> (8400, 84)
    std::vector<float> dataSrc_t_vec(static_cast<size_t>(rows) * dimensions);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < rows; j++) {
            dataSrc_t_vec[static_cast<size_t>(j) * dimensions + i] =
                dataSrc[static_cast<size_t>(i) * rows + j];
        }
    }

    for (int i = 0; i < rows; i++) {
        size_t base_idx = static_cast<size_t>(i) * dimensions;
        if (4 + param.numClasses > dimensions) {
            continue;
        }

        int ClassId = 0;
        float maxClassScore = dataSrc_t_vec[base_idx + 4];

        for (int class_idx = 0; class_idx < param.numClasses; ++class_idx) {
            if (dataSrc_t_vec[base_idx + 4 + class_idx] > maxClassScore) {
                maxClassScore = dataSrc_t_vec[base_idx + 4 + class_idx];
                ClassId = class_idx;
            }
        }

        if (maxClassScore <= param.scoreThreshold) {
            continue;
        }

        ScoreIndices[ClassId].emplace_back(maxClassScore, boxIdx);

        float cx = dataSrc_t_vec[base_idx + 0];
        float cy = dataSrc_t_vec[base_idx + 1];
        float w = dataSrc_t_vec[base_idx + 2];
        float h = dataSrc_t_vec[base_idx + 3];

        Boxes.emplace_back(cx - w / 2.0f);
        Boxes.emplace_back(cy - h / 2.0f);
        Boxes.emplace_back(cx + w / 2.0f);
        Boxes.emplace_back(cy + h / 2.0f);

        boxIdx++;
    }
}

void ProcessYoloV5Face(
    const float *dataSrc, int rows, int dimensions,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, YoloParam &param,
    int &boxIdx) {
    for (int i = 0; i < rows; i++) {
        const float *data = dataSrc + (dimensions * i);

        float obj_conf = data[4];
        float cls_conf = data[15];
        float conf = obj_conf * cls_conf;

        if (conf < param.scoreThreshold) {
            continue;
        }

        ScoreIndices[0].emplace_back(conf, boxIdx);

        Boxes.emplace_back(data[0] - data[2] / 2.0f);
        Boxes.emplace_back(data[1] - data[3] / 2.0f);
        Boxes.emplace_back(data[0] + data[2] / 2.0f);
        Boxes.emplace_back(data[1] + data[3] / 2.0f);

        for (int k = 0; k < param.numKeypoints; k++) {
            int kptIdx = 5 + (2 * k);
            Keypoints.emplace_back(data[kptIdx]);
            Keypoints.emplace_back(data[kptIdx + 1]);
            Keypoints.emplace_back(0.5f);
        }

        boxIdx++;
    }
}

void ProcessYoloV5Normal(
    const float *dataSrc, int rows, int dimensions,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, YoloParam &param, int &boxIdx) {
    for (int i = 0; i < rows; i++) {
        const float *data = dataSrc + (dimensions * i);
        const float *classesScores = data + 5;

        int ClassId = 0;
        float maxClassScore = classesScores[0];

        for (int class_idx = 0; class_idx < param.numClasses; class_idx++) {
            if (classesScores[class_idx] > maxClassScore) {
                maxClassScore = classesScores[class_idx];
                ClassId = class_idx;
            }
        }

        if (maxClassScore * data[4] <= param.scoreThreshold) {
            continue;
        }

        ScoreIndices[ClassId].emplace_back(maxClassScore, boxIdx);

        Boxes.emplace_back(data[0] - data[2] / 2.0f);
        Boxes.emplace_back(data[1] - data[3] / 2.0f);
        Boxes.emplace_back(data[0] + data[2] / 2.0f);
        Boxes.emplace_back(data[1] + data[3] / 2.0f);

        boxIdx++;
    }
}

void ProcessFLOAT(const dxs::DXTensor &output,
                  std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
                  std::vector<float> &Boxes, std::vector<float> &Keypoints,
                  YoloParam &param, int &boxIdx) {
    auto *dataSrc = (float *)output._data;

    if (output._shape[1] == 84 && output._shape[2] == 8400) {
        ProcessYoloV8V9(dataSrc, output._shape[1], output._shape[2],
                        ScoreIndices, Boxes, param, boxIdx);
    } else if (output._shape[1] == 25200 && output._shape[2] == 16) {
        ProcessYoloV5Face(dataSrc, output._shape[1], output._shape[2],
                          ScoreIndices, Boxes, Keypoints, param, boxIdx);
    } else {
        ProcessYoloV5Normal(dataSrc, output._shape[1], output._shape[2],
                            ScoreIndices, Boxes, param, boxIdx);
    }
}

void ProcessBoxesForGridCell(
    const uint8_t *basePtr, int gY, int gX, const dxs::DXTensor &outputTensor,
    const YoloLayerParam &layer, int strideX, int strideY, float rawThreshold,
    float confThreshold, YoloParam &param,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, int &boxIdx) {
    uint32_t inc = outputTensor._shape[3] * outputTensor._elemSize;

    int box_element_size = 4 + 1 + param.numClasses + 2 * param.numKeypoints;

    for (int box = 0; box < layer.numBoxes; box++) {
        auto *data = reinterpret_cast<float *>(
            const_cast<uint8_t *>(basePtr) + gY * outputTensor._shape[2] * inc +
            gX * inc + outputTensor._elemSize * (box * box_element_size));

        if (data[4] <= rawThreshold)
            continue;

        float objectness = sigmoid(data[4]);
        if (objectness <= confThreshold)
            continue;

        int ClassId = 0;
        float maxClassScore = data[5];
        for (int cls = 0; cls < param.numClasses; cls++) {
            if (data[5 + cls] > maxClassScore) {
                maxClassScore = data[5 + cls];
                ClassId = cls;
            }
        }

        float confidence = objectness * sigmoid(maxClassScore);
        if (confidence <= param.scoreThreshold)
            continue;

        ScoreIndices[ClassId].emplace_back(confidence, boxIdx);

        float scale_x_y = layer.scaleX;

        float x, y, w, h;
        if (scale_x_y == 0) {
            x = (sigmoid(data[0]) * 2.0f - 0.5f + gX) * strideX;
            y = (sigmoid(data[1]) * 2.0f - 0.5f + gY) * strideY;
        } else {
            x = (sigmoid(data[0]) * scale_x_y - 0.5f * (scale_x_y - 1) + gX) *
                strideX;
            y = (sigmoid(data[1]) * scale_x_y - 0.5f * (scale_x_y - 1) + gY) *
                strideY;
        }

        w = std::pow(sigmoid(data[2]) * 2.0f, 2) * layer.anchorWidth[box];
        h = std::pow(sigmoid(data[3]) * 2.0f, 2) * layer.anchorHeight[box];

        Boxes.emplace_back(x - w / 2.0f);
        Boxes.emplace_back(y - h / 2.0f);
        Boxes.emplace_back(x + w / 2.0f);
        Boxes.emplace_back(y + h / 2.0f);

        if (param.numKeypoints > 0) {
            int kpt_offset = 4 + param.numClasses;
            for (int k = 0; k < param.numKeypoints; k++) {
                float kpt_x, kpt_y;
                kpt_x = (data[kpt_offset + 2 * k] * layer.anchorWidth[box]) +
                        (gX * strideX);
                kpt_y =
                    (data[kpt_offset + 2 * k + 1] * layer.anchorHeight[box]) +
                    (gY * strideY);
                Keypoints.emplace_back(kpt_x);
                Keypoints.emplace_back(kpt_y);
                Keypoints.emplace_back(0.5f);
            }
        }

        boxIdx++;
    }
}

void ProcessMultiOutput(
    const std::vector<dxs::DXTensor> &outputs,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, YoloParam &param,
    int &boxIdx) {
    float confThreshold = param.confThreshold;
    float rawThreshold = std::log(confThreshold / (1 - confThreshold));

    for (const auto &layer : param.layers) {
        int strideX = param.width / layer.numGridX;
        int strideY = param.height / layer.numGridY;
        int numGridX = layer.numGridX;
        int numGridY = layer.numGridY;
        int tensorIdx = layer.tensorIdx[0];

        const auto *basePtr =
            static_cast<const uint8_t *>(outputs[tensorIdx]._data);

        for (int gY = 0; gY < numGridY; gY++) {
            for (int gX = 0; gX < numGridX; gX++) {
                ProcessBoxesForGridCell(basePtr, gY, gX, outputs[tensorIdx],
                                        layer, strideX, strideY, rawThreshold,
                                        confThreshold, param, ScoreIndices,
                                        Boxes, Keypoints, boxIdx);
            }
        }
    }
}

void ProcessOutputSingle(
    const dxs::DXTensor &output,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, YoloParam &param,
    int &boxIdx) {
    if (output._type == dxs::DataType::BBOX) {
        ProcessBBOX(output, ScoreIndices, Boxes, param, boxIdx);
    } else if (output._type == dxs::DataType::POSE) {
        ProcessPOSE(output, ScoreIndices, Boxes, Keypoints, param, boxIdx);
    } else if (output._type == dxs::DataType::FLOAT) {
        ProcessFLOAT(output, ScoreIndices, Boxes, Keypoints, param, boxIdx);
    } else {
        return;
    }
}

void FilterWithSort(
    std::vector<dxs::DXTensor> outputs,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, YoloParam param) {

    int boxIdx = 0;

    if (outputs.empty())
        return;

    if (outputs.size() == 1) {
        if (outputs[0]._type == dxs::DataType::BBOX) {
            ProcessBBOX(outputs[0], ScoreIndices, Boxes, param, boxIdx);
        } else if (outputs[0]._type == dxs::DataType::POSE) {
            ProcessPOSE(outputs[0], ScoreIndices, Boxes, Keypoints, param,
                        boxIdx);
        } else if (outputs[0]._type == dxs::DataType::FLOAT) {
            ProcessFLOAT(outputs[0], ScoreIndices, Boxes, Keypoints, param,
                         boxIdx);
        } else {
            return; // Unknown type
        }
    } else if (outputs.size() == 3) {
        ProcessMultiOutput(outputs, ScoreIndices, Boxes, Keypoints, param,
                           boxIdx);
    } else {
        g_error("Post-process: Not supported format\n");
        return;
    }

    for (int cls = 0; cls < static_cast<int>(param.numClasses); cls++) {
        std::sort(ScoreIndices[cls].begin(), ScoreIndices[cls].end(),
                  scoreComapre);
    }
}

float CalcIOU(float *box, float *truth) {
    float ovr_left = std::max(box[0], truth[0]);
    float ovr_right = std::min(box[2], truth[2]);
    float ovr_top = std::max(box[1], truth[1]);
    float ovr_bottom = std::min(box[3], truth[3]);
    float ovr_width = ovr_right - ovr_left;
    float ovr_height = ovr_bottom - ovr_top;
    if (ovr_width < 0 || ovr_height < 0)
        return 0;
    float overlap_area = ovr_width * ovr_height;
    float union_area = (box[2] - box[0]) * (box[3] - box[1]) +
                       (truth[2] - truth[0]) * (truth[3] - truth[1]) -
                       overlap_area;
    return overlap_area * 1.0 / union_area;
}

void NmsOneClass(unsigned int cls,
                 std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
                 std::vector<float> Boxes, std::vector<float> Keypoints,
                 std::vector<Decoded> &Result, YoloParam param) {
    Decoded decoded;
    float iou;
    int i, j;
    int numCandidates = ScoreIndices[cls].size();
    bool valid[numCandidates];
    std::fill_n(valid, numCandidates, true);
    for (i = 0; i < numCandidates; i++) {
        if (!valid[i]) {
            continue;
        }
        if (Keypoints.size() > 0) {
            decoded = Decoded(param.classNames[cls], ScoreIndices[cls][i].first,
                              cls, Boxes[4 * ScoreIndices[cls][i].second],
                              Boxes[4 * ScoreIndices[cls][i].second + 1],
                              Boxes[4 * ScoreIndices[cls][i].second + 2],
                              Boxes[4 * ScoreIndices[cls][i].second + 3],
                              &Keypoints[3 * param.numKeypoints *
                                         ScoreIndices[cls][i].second],
                              param.numKeypoints);
        } else {
            decoded = Decoded(param.classNames[cls], ScoreIndices[cls][i].first,
                              cls, Boxes[4 * ScoreIndices[cls][i].second],
                              Boxes[4 * ScoreIndices[cls][i].second + 1],
                              Boxes[4 * ScoreIndices[cls][i].second + 2],
                              Boxes[4 * ScoreIndices[cls][i].second + 3]);
        }

        Result.emplace_back(decoded);
        for (j = i + 1; j < numCandidates; j++) {
            if (!valid[j]) {
                continue;
            }
            iou = CalcIOU(&Boxes[4 * ScoreIndices[cls][j].second],
                          &Boxes[4 * ScoreIndices[cls][i].second]);
            if (iou > param.iouThreshold) {
                valid[j] = false;
            }
        }
    }
}

void Nms(std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
         std::vector<float> &Boxes, std::vector<float> &Keypoints,
         std::vector<Decoded> &Result, YoloParam param) {
    for (int cls = 0; cls < param.numClasses; cls++) {
        NmsOneClass(cls, ScoreIndices, Boxes, Keypoints, Result, param);
    }
}

void YOLOPostProcess(GstBuffer *buf,
                    std::vector<dxs::DXTensor> network_output,
                    DXFrameMeta *frame_meta, YoloParam param) {

    std::vector<Decoded> results;
    std::vector<std::vector<std::pair<float, int>>> ScoreIndices;
    std::vector<float> Boxes;
    std::vector<float> Keypoints;

    for (int cls = 0; cls < param.numClasses; cls++) {
        std::vector<std::pair<float, int>> v;
        ScoreIndices.push_back(v);
    }

    FilterWithSort(network_output, ScoreIndices, Boxes, Keypoints, param);

    Nms(ScoreIndices, Boxes, Keypoints, results, param);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(param.width / (float)origin_w,
                           param.height / (float)origin_h);
        float w_pad = (param.width - origin_w * r) / 2.;
        float h_pad = (param.height - origin_h * r) / 2.;

        float x1 = (ret.box[0] - w_pad) / r;
        float x2 = (ret.box[2] - w_pad) / r;
        float y1 = (ret.box[1] - h_pad) / r;
        float y2 = (ret.box[3] - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *object_meta = dx_acquire_obj_meta_from_pool();
        object_meta->_confidence = ret.score;
        object_meta->_label = ret.label;
        object_meta->_label_name = g_string_new(ret.labelname.c_str());
        object_meta->_box[0] = x1;
        object_meta->_box[1] = y1;
        object_meta->_box[2] = x2;
        object_meta->_box[3] = y2;

        for (int k = 0; k < param.numKeypoints; k++) {
            float kx = (ret.kpts[k * 3 + 0] - w_pad) / r;
            float ky = (ret.kpts[k * 3 + 1] - h_pad) / r;
            float ks = ret.kpts[k * 3 + 2];
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
                object_meta->_keypoints.push_back(kx + frame_meta->_roi[0]);
                object_meta->_keypoints.push_back(ky + frame_meta->_roi[1]);
            } else {
                object_meta->_keypoints.push_back(kx);
                object_meta->_keypoints.push_back(ky);
            }

            object_meta->_keypoints.push_back(ks);
        }
        dx_add_obj_meta_to_frame(frame_meta, object_meta);
    }
}

void YOLOFacePostProcess(GstBuffer *buf,
                        std::vector<dxs::DXTensor> network_output,
                        DXFrameMeta *frame_meta, YoloParam param) {

    std::vector<Decoded> results;
    std::vector<std::vector<std::pair<float, int>>> ScoreIndices;
    std::vector<float> Boxes;
    std::vector<float> Keypoints;

    for (int cls = 0; cls < param.numClasses; cls++) {
        std::vector<std::pair<float, int>> v;
        ScoreIndices.push_back(v);
    }

    FilterWithSort(network_output, ScoreIndices, Boxes, Keypoints, param);

    Nms(ScoreIndices, Boxes, Keypoints, results, param);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(param.width / (float)origin_w,
                           param.height / (float)origin_h);
        float w_pad = (param.width - origin_w * r) / 2.;
        float h_pad = (param.height - origin_h * r) / 2.;

        float x1 = (ret.box[0] - w_pad) / r;
        float x2 = (ret.box[2] - w_pad) / r;
        float y1 = (ret.box[1] - h_pad) / r;
        float y2 = (ret.box[3] - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *object_meta = dx_acquire_obj_meta_from_pool();
        object_meta->_confidence = ret.score;
        object_meta->_label = ret.label;
        object_meta->_label_name = g_string_new(ret.labelname.c_str());
        object_meta->_face_box[0] = x1;
        object_meta->_face_box[1] = y1;
        object_meta->_face_box[2] = x2;
        object_meta->_face_box[3] = y2;

        for (int k = 0; k < param.numKeypoints; k++) {
            float kx = (ret.kpts[k * 3 + 0] - w_pad) / r;
            float ky = (ret.kpts[k * 3 + 1] - h_pad) / r;
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {

                object_meta->_face_landmarks.push_back(dxs::Point_f(
                    kx + frame_meta->_roi[0], ky + frame_meta->_roi[1]));
            } else {
                object_meta->_face_landmarks.push_back(dxs::Point_f(kx, ky));
            }
        }
        dx_add_obj_meta_to_frame(frame_meta, object_meta);
    }
}

const std::vector<std::string> COCO_CLASSES = {
    "person",       "bicycle",      "car",          "motorcycle",
    "airplane",     "bus",          "train",        "truck",
    "boat",         "trafficlight", "firehydrant",  "stopsign",
    "parkingmeter", "bench",        "bird",         "cat",
    "dog",          "horse",        "sheep",        "cow",
    "elephant",     "bear",         "zebra",        "giraffe",
    "backpack",     "umbrella",     "handbag",      "tie",
    "suitcase",     "frisbee",      "skis",         "snowboard",
    "sportsball",   "kite",         "baseballbat",  "baseballglove",
    "skateboard",   "surfboard",    "tennisracket", "bottle",
    "wineglass",    "cup",          "fork",         "knife",
    "spoon",        "bowl",         "banana",       "apple",
    "sandwich",     "orange",       "broccoli",     "carrot",
    "hotdog",       "pizza",        "donut",        "cake",
    "chair",        "couch",        "pottedplant",  "bed",
    "diningtable",  "toilet",       "tv",           "laptop",
    "mouse",        "remote",       "keyboard",     "cellphone",
    "microwave",    "oven",         "toaster",      "sink",
    "refrigerator", "book",         "clock",        "vase",
    "scissors",     "teddybear",    "hairdrier",    "toothbrush"};

extern "C" void YOLOV5S_1(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 512;
    param.width = 512;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {
        createYoloLayerParam(64, 64, 3, {10.0, 16.0, 33.0},
                             {13.0, 30.0, 23.0}, {0}),
        createYoloLayerParam(32, 32, 3, {30.0, 62.0, 59.0},
                             {61.0, 45.0, 119.0}, {1}),
        createYoloLayerParam(16, 16, 3, {116.0, 156.0, 373.0},
                             {90.0, 198.0, 326.0}, {2}),
    };
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5S_3(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 512;
    param.width = 512;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {
        createYoloLayerParam(64, 64, 3, {10.0, 16.0, 33.0},
                             {13.0, 30.0, 23.0}, {0}),
        createYoloLayerParam(32, 32, 3, {30.0, 62.0, 59.0},
                             {61.0, 45.0, 119.0}, {1}),
        createYoloLayerParam(16, 16, 3, {116.0, 156.0, 373.0},
                             {90.0, 198.0, 326.0}, {2}),
    };
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5S_4(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 320;
    param.width = 320;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(40, 40, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(20, 20, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(10, 10, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5S_6(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5S_Face(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.0f; // not used for face
    param.scoreThreshold = 0.25;
    param.iouThreshold = 0.4;
    param.numBoxes = -1;
    param.numClasses = 1;
    param.numKeypoints = 5;
    param.layers = {createYoloLayerParam(80, 80, 3, {4.0, 8.0, 13.0},
                                        {5.0, 10.0, 16.0}, {2}),
                   createYoloLayerParam(40, 40, 3, {23.0, 43.0, 73.0},
                                        {29.0, 55.0, 105.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {146.0, 231.0, 335.0},
                                        {217.0, 300.0, 433.0}, {0})};
    param.classNames = {"person"};

    YOLOFacePostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5X_2(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV5Pose_1(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.3;
    param.scoreThreshold = 0.5;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 1;
    param.numKeypoints = 17;
    param.layers = {createYoloLayerParam(80, 80, 3, {19.0, 44.0, 38.0},
                                        {27.0, 40.0, 94.0}, {1, 0}),
                   createYoloLayerParam(40, 40, 3, {96.0, 86.0, 180.0},
                                        {68.0, 152.0, 137.0}, {3, 2}),
                   createYoloLayerParam(20, 20, 3, {140.0, 303.0, 238.0},
                                        {301.0, 264.0, 542.0}, {5, 4}),
                   createYoloLayerParam(10, 10, 3, {436.0, 739.0, 925.0},
                                        {615.0, 380.0, 792.0}, {7, 6})};
    param.classNames = {"person"};

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV7_512(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 512;
    param.width = 512;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(64, 64, 3, {12.0, 19.0, 40.0},
                                        {16.0, 36.0, 28.0}, {0}),
                   createYoloLayerParam(32, 32, 3, {36.0, 76.0, 72.0},
                                        {75.0, 55.0, 146.0}, {1}),
                   createYoloLayerParam(16, 16, 3, {142.0, 192.0, 459.0},
                                        {110.0, 243.0, 401.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV7_640(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(80, 80, 3, {12.0, 19.0, 40.0},
                                        {16.0, 36.0, 28.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {36.0, 76.0, 72.0},
                                        {75.0, 55.0, 146.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {142.0, 192.0, 459.0},
                                        {110.0, 243.0, 401.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV8N(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}

extern "C" void YOLOV9S(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloParam param;
    param.height = 640;
    param.width = 640;
    param.confThreshold = 0.25;
    param.scoreThreshold = 0.3;
    param.iouThreshold = 0.4;
    param.numBoxes = -1; // check from layer info.
    param.numClasses = 80;
    param.layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})};
    param.classNames = COCO_CLASSES;

    YOLOPostProcess(buf, network_output, frame_meta, param);
}
