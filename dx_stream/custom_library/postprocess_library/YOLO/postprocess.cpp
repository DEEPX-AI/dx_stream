#include "dxcommon.hpp"
#include "dxrt/dxrt_api.h"
#include "gst-dxmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

#define sigmoid(x) (1 / (1 + std::exp(-x)))

struct Decoded {
    int label;
    std::string labelname;
    float score;
    float box[4];
    std::vector<float> kpts;
    ~Decoded(void);
    Decoded(void);
    Decoded(unsigned int _label, std::string _labelname, float _score,
            float data1, float data2, float data3, float data4);
    Decoded(unsigned int _label, std::string _labelname, float _score,
            float data1, float data2, float data3, float data4,
            float *keypoints, int numKeypoints);
};

Decoded::~Decoded(void) {}
Decoded::Decoded(void) {}
Decoded::Decoded(unsigned int _label, const std::string _labelname,
                 float _score, float data1, float data2, float data3,
                 float data4)
    : label(_label), score(_score), labelname(_labelname) {

    box[0] = data1;
    box[1] = data2;
    box[2] = data3;
    box[3] = data4;
}

Decoded::Decoded(unsigned int _label, const std::string _labelname,
                 float _score, float data1, float data2, float data3,
                 float data4, float *keypoints, int numKeypoints)
    : label(_label), score(_score), labelname(_labelname) {

    box[0] = data1;
    box[1] = data2;
    box[2] = data3;
    box[3] = data4;

    kpts.clear();
    kpts = std::vector<float>(3 * numKeypoints);

    for (int i = 0; i < 3 * numKeypoints; i++) {
        kpts[i] = keypoints[i];
    }
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
    int numKeypoints;
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

void FilterWithSort(
    std::vector<shared_ptr<dxrt::Tensor>> outputs,
    std::vector<std::vector<std::pair<float, int>>> &ScoreIndices,
    std::vector<float> &Boxes, std::vector<float> &Keypoints, YoloParam param) {

    int boxIdx = 0;
    int x, y, w, h;

    if (outputs.size() <= 0) {
        return;
    }

    int numElements = outputs.front()->shape().front();
    if (numElements <= 0) {
        return;
    }

    if (outputs.size() == 1) {
        if (outputs.front()->type() == dxrt::DataType::BBOX) {

            dxrt::DeviceBoundingBox_t *dataSrc =
                (dxrt::DeviceBoundingBox_t *)outputs.front()->data();

            for (int i = 0; i < numElements; i++) {
                dxrt::DeviceBoundingBox_t *data = dataSrc + i;
                auto layer = param.layers[data->layer_idx];
                int strideX = param.width / layer.numGridX;
                int strideY = param.height / layer.numGridY;
                int gX = data->grid_x;
                int gY = data->grid_y;
                float scale_x_y = layer.scaleX;

                ScoreIndices[data->label].emplace_back(data->score, boxIdx);

                if (layer.anchorHeight.size() > 0) {
                    if (scale_x_y == 0) {
                        x = (data->x * 2. - 0.5 + gX) * strideX;
                        y = (data->y * 2. - 0.5 + gY) * strideY;
                    } else {
                        x = (data->x * scale_x_y - 0.5 * (scale_x_y - 1) + gX) *
                            strideX;
                        y = (data->y * scale_x_y - 0.5 * (scale_x_y - 1) + gY) *
                            strideY;
                    }
                    w = (data->w * data->w * 4.) *
                        layer.anchorWidth[data->box_idx];
                    h = (data->h * data->h * 4.) *
                        layer.anchorHeight[data->box_idx];
                } else {
                    x = (gX + data->x) * strideX;
                    y = (gY + data->y) * strideY;
                    w = exp(data->w) * strideX;
                    h = exp(data->h) * strideY;
                }

                Boxes.emplace_back(x - w / 2.); /*x1*/
                Boxes.emplace_back(y - h / 2.); /*y1*/
                Boxes.emplace_back(x + w / 2.); /*x2*/
                Boxes.emplace_back(y + h / 2.); /*y2*/

                boxIdx++;
            }
        } else if (outputs.front()->type() == dxrt::DataType::POSE) {

            dxrt::DevicePose_t *dataSrc =
                (dxrt::DevicePose_t *)outputs.front()->data();

            for (int i = 0; i < numElements; i++) {
                dxrt::DevicePose_t *data = dataSrc + i;

                auto layer = param.layers[data->layer_idx];
                int strideX = param.width / layer.numGridX;
                int strideY = param.height / layer.numGridY;
                int gX = data->grid_x;
                int gY = data->grid_y;
                float scale_x_y = layer.scaleX;

                ScoreIndices[0].emplace_back(data->score, boxIdx);
                if (layer.anchorHeight.size() > 0) {
                    if (scale_x_y == 0) {
                        x = (data->x * 2. - 0.5 + gX) * strideX;
                        y = (data->y * 2. - 0.5 + gY) * strideY;
                    } else {
                        x = (data->x * scale_x_y - 0.5 * (scale_x_y - 1) + gX) *
                            strideX;
                        y = (data->y * scale_x_y - 0.5 * (scale_x_y - 1) + gY) *
                            strideY;
                    }
                    w = (data->w * data->w * 4.) *
                        layer.anchorWidth[data->box_idx];
                    h = (data->h * data->h * 4.) *
                        layer.anchorHeight[data->box_idx];
                } else {
                    x = (gX + data->x) * strideX;
                    y = (gY + data->y) * strideY;
                    w = exp(data->w) * strideX;
                    h = exp(data->h) * strideY;
                }

                Boxes.emplace_back(x - w / 2.); /*x1*/
                Boxes.emplace_back(y - h / 2.); /*y1*/
                Boxes.emplace_back(x + w / 2.); /*x2*/
                Boxes.emplace_back(y + h / 2.); /*y2*/

                for (int k = 0; k < param.numKeypoints; k++) {
                    Keypoints.emplace_back((data->kpts[k][0] * 2. - 0.5 + gX) *
                                           strideX);
                    Keypoints.emplace_back((data->kpts[k][1] * 2. - 0.5 + gY) *
                                           strideY);
                    Keypoints.emplace_back(sigmoid(data->kpts[k][2]));
                }

                boxIdx++;
            }
        } else if (outputs.front()->type() ==
                   dxrt::DataType::FLOAT) { // USE_ORT=ON

            auto *dataSrc = (float *)outputs.front()->data();

            if (outputs.front()->shape()[1] <
                outputs.front()->shape()[2]) { // yolov8, yolov9 has an output
                                               // of shape (batchSize, 84, 8400)
                int dimensions = outputs.front()->shape()[1];
                int rows = outputs.front()->shape()[2];

                float dataSrc_t[rows * dimensions];
                for (int i = 0; i < dimensions; i++) {
                    float *data_ = (float *)dataSrc + (rows * i);
                    for (int j = 0; j < rows; j++) {
                        dataSrc_t[dimensions * j + i] = data_[j];
                    }
                }

                for (int i = 0; i < rows; i++) {
                    float *data = (float *)dataSrc_t + (dimensions * i);
                    float *classesScores = data + 4;

                    int ClassId = 0;
                    float maxClassScore = classesScores[0];

                    for (int class_idx = 0; class_idx < param.numClasses;
                         class_idx++) {
                        if (classesScores[class_idx] > maxClassScore) {
                            maxClassScore = classesScores[class_idx];
                            ClassId = class_idx;
                        }
                    }

                    if (maxClassScore > param.scoreThreshold) {
                        ScoreIndices[ClassId].emplace_back(maxClassScore,
                                                           boxIdx);

                        Boxes.emplace_back(data[0] - data[2] / 2.); /*x1*/
                        Boxes.emplace_back(data[1] - data[3] / 2.); /*y1*/
                        Boxes.emplace_back(data[0] + data[2] / 2.); /*x2*/
                        Boxes.emplace_back(data[1] + data[3] / 2.); /*y2*/

                        boxIdx++;
                    }
                }
            } else { // yolov5 has an output of shape (batchSize, 25200, 85)
                     // (Num classes + box[x,y,w,h] + confidence[c])
                auto rows = outputs.front()->shape()[1];
                auto dimensions = outputs.front()->shape()[2];

                for (int i = 0; i < rows; i++) {
                    float *data = (float *)dataSrc + (dimensions * i);
                    float *classesScores = data + 5;

                    int ClassId = 0;
                    float maxClassScore = classesScores[0];

                    for (int class_idx = 0; class_idx < param.numClasses;
                         class_idx++) {
                        if (classesScores[class_idx] > maxClassScore) {
                            maxClassScore = classesScores[class_idx];
                            ClassId = class_idx;
                        }
                    }

                    if (maxClassScore * data[4] > param.scoreThreshold) {

                        ScoreIndices[ClassId].emplace_back(maxClassScore,
                                                           boxIdx);

                        Boxes.emplace_back(data[0] - data[2] / 2.); /*x1*/
                        Boxes.emplace_back(data[1] - data[3] / 2.); /*y1*/
                        Boxes.emplace_back(data[0] + data[2] / 2.); /*x2*/
                        Boxes.emplace_back(data[1] + data[3] / 2.); /*y2*/

                        boxIdx++;
                    }
                }
            }
        } else {
            // Unknown Output Type
            return;
        }

    } else if (outputs.size() == 3) {

        float conf_threshold = param.confThreshold;
        float rawThreshold = std::log(conf_threshold / (1 - conf_threshold));
        float confidence, objectness, box_temp[4];
        float *data;

        for (auto &layer : param.layers) {
            int strideX = param.width / layer.numGridX;
            int strideY = param.height / layer.numGridY;
            int numGridX = layer.numGridX;
            int numGridY = layer.numGridY;
            int tensorIdx = layer.tensorIdx[0];
            float scale_x_y = layer.scaleX;
            for (int gY = 0; gY < numGridY; gY++) {
                for (int gX = 0; gX < numGridX; gX++) {
                    for (int box = 0; box < layer.numBoxes; box++) {
                        data = (float *)(outputs[tensorIdx]->data(
                            gY, gX, box * (4 + 1 + param.numClasses)));
                        if (data[4] > rawThreshold) {
                            objectness = sigmoid(data[4]);
                            /* Step1 - obj_conf > CONF_THRESHOLD */
                            if (objectness > conf_threshold) {

                                int ClassId = 0;
                                float maxClassScore = data[5];

                                for (int cls = 0; cls < param.numClasses;
                                     cls++) {
                                    if (data[5 + cls] > maxClassScore) {
                                        maxClassScore = data[5 + cls];
                                        ClassId = cls;
                                    }
                                }

                                confidence =
                                    objectness * sigmoid(maxClassScore);
                                if (confidence > param.scoreThreshold) {
                                    ScoreIndices[ClassId].emplace_back(
                                        confidence, boxIdx);

                                    if (scale_x_y == 0) {
                                        x = (sigmoid(data[0]) * 2. - 0.5 + gX) *
                                            strideX;
                                        y = (sigmoid(data[1]) * 2. - 0.5 + gY) *
                                            strideY;
                                    } else {
                                        x = (sigmoid(data[0]) * scale_x_y -
                                             0.5 * (scale_x_y - 1) + gX) *
                                            strideX;

                                        y = (sigmoid(data[1]) * scale_x_y -
                                             0.5 * (scale_x_y - 1) + gY) *
                                            strideY;
                                    }

                                    w = pow((sigmoid(data[2]) * 2.), 2) *
                                        layer.anchorWidth[box];
                                    h = pow((sigmoid(data[3]) * 2.), 2) *
                                        layer.anchorHeight[box];

                                    Boxes.emplace_back(x - w / 2.); /*x1*/
                                    Boxes.emplace_back(y - h / 2.); /*y1*/
                                    Boxes.emplace_back(x + w / 2.); /*x2*/
                                    Boxes.emplace_back(y + h / 2.); /*y2*/

                                    boxIdx++;
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        return;
    }

    for (int cls = 0; cls < (int)param.numClasses; cls++) {
        std::sort(ScoreIndices[cls].begin(), ScoreIndices[cls].end(),
                  scoreComapre);
    }
}

static bool compare(const Decoded &r1, const Decoded &r2) {
    return r1.score > r2.score;
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
            decoded = Decoded(cls, (char *)param.classNames[cls].c_str(),
                              ScoreIndices[cls][i].first,
                              Boxes[4 * ScoreIndices[cls][i].second],
                              Boxes[4 * ScoreIndices[cls][i].second + 1],
                              Boxes[4 * ScoreIndices[cls][i].second + 2],
                              Boxes[4 * ScoreIndices[cls][i].second + 3],
                              &Keypoints[3 * param.numKeypoints *
                                         ScoreIndices[cls][i].second],
                              param.numKeypoints);
        } else {
            decoded =
                Decoded(cls, param.classNames[cls], ScoreIndices[cls][i].first,
                        Boxes[4 * ScoreIndices[cls][i].second],
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
    for (size_t cls = 0; cls < param.numClasses; cls++) {
        NmsOneClass(cls, ScoreIndices, Boxes, Keypoints, Result, param);
    }
}

void YOLOPostProcess(std::vector<shared_ptr<dxrt::Tensor>> network_output,
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

        DXObjectMeta *object_meta = dx_create_object_meta(frame_meta->_buf);
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
        dx_add_object_meta_to_frame_meta(object_meta, frame_meta);
    }
}

extern "C" void YOLOV5S_1(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                          DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 512,
        .width = 512,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers =
            {
                createYoloLayerParam(64, 64, 3, {10.0, 16.0, 33.0},
                                     {13.0, 30.0, 23.0}, {0}),
                createYoloLayerParam(32, 32, 3, {30.0, 62.0, 59.0},
                                     {61.0, 45.0, 119.0}, {1}),
                createYoloLayerParam(16, 16, 3, {116.0, 156.0, 373.0},
                                     {90.0, 198.0, 326.0}, {2}),
            },
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV5S_3(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                          DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 512,
        .width = 512,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers =
            {
                createYoloLayerParam(64, 64, 3, {10.0, 16.0, 33.0},
                                     {13.0, 30.0, 23.0}, {0}),
                createYoloLayerParam(32, 32, 3, {30.0, 62.0, 59.0},
                                     {61.0, 45.0, 119.0}, {1}),
                createYoloLayerParam(16, 16, 3, {116.0, 156.0, 373.0},
                                     {90.0, 198.0, 326.0}, {2}),
            },
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV5S_4(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                          DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 320,
        .width = 320,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers = {createYoloLayerParam(40, 40, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(20, 20, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(10, 10, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})

        },
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV5S_6(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                          DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})},
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV5X_2(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                          DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers = {createYoloLayerParam(80, 80, 3, {10.0, 16.0, 33.0},
                                        {13.0, 30.0, 23.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {30.0, 62.0, 59.0},
                                        {61.0, 45.0, 119.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {116.0, 156.0, 373.0},
                                        {90.0, 198.0, 326.0}, {2})},
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void
YOLOV5Pose_1(std::vector<shared_ptr<dxrt::Tensor>> network_output,
             DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .confThreshold = 0.3,
        .scoreThreshold = 0.5,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 1,
        .numKeypoints = 17,
        .layers = {createYoloLayerParam(80, 80, 3, {19.0, 44.0, 38.0},
                                        {27.0, 40.0, 94.0}, {1, 0}),
                   createYoloLayerParam(40, 40, 3, {96.0, 86.0, 180.0},
                                        {68.0, 152.0, 137.0}, {3, 2}),
                   createYoloLayerParam(20, 20, 3, {140.0, 303.0, 238.0},
                                        {301.0, 264.0, 542.0}, {5, 4}),
                   createYoloLayerParam(10, 10, 3, {436.0, 739.0, 925.0},
                                        {615.0, 380.0, 792.0}, {7, 6})},
        .classNames = {"person"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV7_512(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                           DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 512,
        .width = 512,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers = {createYoloLayerParam(64, 64, 3, {12.0, 19.0, 40.0},
                                        {16.0, 36.0, 28.0}, {0}),
                   createYoloLayerParam(32, 32, 3, {36.0, 76.0, 72.0},
                                        {75.0, 55.0, 146.0}, {1}),
                   createYoloLayerParam(16, 16, 3, {142.0, 192.0, 459.0},
                                        {110.0, 243.0, 401.0}, {2})},
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV7_640(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                           DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .confThreshold = 0.25,
        .scoreThreshold = 0.3,
        .iouThreshold = 0.4,
        .numBoxes = -1, // check from layer info.
        .numClasses = 80,
        .layers = {createYoloLayerParam(80, 80, 3, {12.0, 19.0, 40.0},
                                        {16.0, 36.0, 28.0}, {0}),
                   createYoloLayerParam(40, 40, 3, {36.0, 76.0, 72.0},
                                        {75.0, 55.0, 146.0}, {1}),
                   createYoloLayerParam(20, 20, 3, {142.0, 192.0, 459.0},
                                        {110.0, 243.0, 401.0}, {2})},
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV8N(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                        DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .scoreThreshold = 0.3,
        .numClasses = 80,
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}

extern "C" void YOLOV9S(std::vector<shared_ptr<dxrt::Tensor>> network_output,
                        DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    YoloParam param = {
        .height = 640,
        .width = 640,
        .scoreThreshold = 0.3,
        .numClasses = 80,
        .classNames =
            {"person",       "bicycle",      "car",          "motorcycle",
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
             "scissors",     "teddybear",    "hairdrier",    "toothbrush"},
    };

    YOLOPostProcess(network_output, frame_meta, param);
}