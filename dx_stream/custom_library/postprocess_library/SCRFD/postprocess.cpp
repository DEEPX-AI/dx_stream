#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <functional>
#include <iostream>

#include "dxcommon.hpp"
#include "dxrt/dxrt_api.h"
#include "gst-dxmeta.hpp"

template <typename _T> struct Size_ {
    _T _width;
    _T _height;

    bool operator==(const Size_ &a) {
        if (_width == a._width && _height == a._height) {
            return true;
        } else {
            return false;
        }
    };
    Size_<_T>(_T width, _T height) {
        this->_width = width;
        this->_height = height;
    };
    Size_<_T>() {
        this->_width = 0;
        this->_height = 0;
    };
};

typedef Size_<int> Size;
typedef Size_<float> Size_f;

struct BBox {
    float _xmin;
    float _ymin;
    float _xmax;
    float _ymax;
    float _width;
    float _height;
    std::vector<dxs::Point_f> _kpts;
};

struct BoundingBox {
    int label;
    std::string labelname;
    float score;
    std::vector<float> box;
    std::vector<float> kpt;
    ~BoundingBox(void);
    BoundingBox(void);
    BoundingBox(unsigned int _label, std::string _labelname, float _score,
                float data1, float data2, float data3, float data4,
                std::vector<dxs::Point_f> keypoints, int numKeypoints);
};

BoundingBox::~BoundingBox(void) {}
BoundingBox::BoundingBox(void) {}

BoundingBox::BoundingBox(unsigned int _label, std::string _labelname,
                         float _score, float data1, float data2, float data3,
                         float data4, std::vector<dxs::Point_f> keypoints,
                         int numKeypoints)
    : label(_label), score(_score) {
    kpt.clear();
    kpt = std::vector<float>(3 * numKeypoints);

    box.clear();
    box = std::vector<float>(4);
    box[0] = data1;
    box[1] = data2;
    box[2] = data3;
    box[3] = data4;

    for (int i = 0; i < numKeypoints; i++) {
        kpt[3 * i] = keypoints[i]._x;
        kpt[3 * i + 1] = keypoints[i]._y;
        kpt[3 * i + 2] = keypoints[i]._z;
    }
    labelname = _labelname;
}

struct SCRFDParams {
    std::vector<std::string> classNames;
    std::vector<int> layerStride;
    float scoreThreshold;
    float iouThreshold;
    int numClasses;
    int numKeypoints;
    int input_width;
    int input_height;
};

static bool scoreComapre(const std::pair<float, int> &a,
                         const std::pair<float, int> &b) {
    if (a.first > b.first)
        return true;
    else
        return false;
};

static bool compare(const BoundingBox &r1, const BoundingBox &r2) {
    return r1.score > r2.score;
}

float calcIoU(BBox a, BBox b) {
    float a_w = a._xmax - a._xmin;
    float a_h = a._ymax - a._ymin;
    float b_w = b._xmax - b._xmin;
    float b_h = b._ymax - b._ymin;
    float overlap_w = std::min(a._xmax, b._xmax) - std::max(a._xmin, b._xmin);
    if (overlap_w < 0)
        overlap_w = 0.f;
    float overlap_h = std::min(a._ymax, b._ymax) - std::max(a._ymin, b._ymin);
    if (overlap_h < 0)
        overlap_h = 0.f;

    float overlap_area = overlap_w * overlap_h;
    float a_area = a_w * a_h;
    float b_area = b_w * b_h;

    return overlap_area / (a_area + b_area - overlap_area);
}

void nms(std::vector<BBox> rawBoxes,
         std::vector<std::vector<std::pair<float, int>>> &scoreIndices,
         float IouThreshold, std::vector<std::string> &ClassNames,
         int numKeypoints, std::vector<BoundingBox> &Result) {
    for (size_t idx = 0; idx < scoreIndices.size(); idx++) // class
    {
        auto &_indices = scoreIndices[idx];
        for (size_t j = 0; j < _indices.size(); j++) {
            if (_indices[j].first == 0.0f)
                continue;

            for (size_t k = j + 1; k < _indices.size(); k++) {
                if (_indices[k].first == 0.f)
                    continue;
                float iou = calcIoU(rawBoxes[_indices[j].second],
                                    rawBoxes[_indices[k].second]);
                if (iou >= IouThreshold) {
                    _indices[k].first = 0.0f;
                }
            }
        }
    }
    for (size_t cls = 0; cls < scoreIndices.size(); cls++) {
        auto _indices = scoreIndices[cls];
        for (size_t i = 0; i < _indices.size(); i++) {
            if (_indices[i].first > 00.f) {
                auto box = BoundingBox(cls, (char *)ClassNames[cls].c_str(),
                                       scoreIndices[cls][i].first,
                                       rawBoxes[_indices[i].second]._xmin,
                                       rawBoxes[_indices[i].second]._ymin,
                                       rawBoxes[_indices[i].second]._xmax,
                                       rawBoxes[_indices[i].second]._ymax,
                                       rawBoxes[_indices[i].second]._kpts,
                                       numKeypoints);
                Result.emplace_back(box);
            }
        }
    }
    sort(Result.begin(), Result.end(), compare);
};

extern "C" void SCRFDPostProcess(std::vector<shared_ptr<dxrt::Tensor>> outputs,
                                 DXFrameMeta *frame_meta,
                                 DXObjectMeta *object_meta,
                                 SCRFDParams params) {
    std::vector<std::vector<std::pair<float, int>>> ScoreIndices;
    for (size_t i = 0; i < params.numClasses; i++) {
        std::vector<std::pair<float, int>> v;
        ScoreIndices.emplace_back(v);
    }

    std::vector<BBox> rawBoxes;
    rawBoxes.clear();

    int numBoxes = outputs.front()->shape()[0];

    int boxIdx = 0;
    for (int b_idx = 0; b_idx < numBoxes; b_idx++) {
        uint8_t *raw_data = (uint8_t *)outputs.front()->data() + (b_idx * 64);
        dxrt::DeviceFace_t *data =
            static_cast<dxrt::DeviceFace_t *>((void *)raw_data);
        int stride = params.layerStride[data->layer_idx];

        if (data->score >= params.scoreThreshold) {

            ScoreIndices[0].emplace_back(data->score, boxIdx);

            BBox bbox = {(data->grid_x - data->x) * stride,
                         (data->grid_y - data->y) * stride,
                         (data->grid_x + data->w) * stride,
                         (data->grid_y + data->h) * stride,
                         2 * data->x * stride,
                         2 * data->x * stride,
                         {dxs::Point_f(-1, -1, -1)}};

            bbox._width = bbox._xmax - bbox._xmin;
            bbox._height = bbox._ymax - bbox._ymin;

            bbox._kpts.clear();
            for (int k_idx = 0; k_idx < params.numKeypoints; k_idx++) {
                bbox._kpts.emplace_back(dxs::Point_f(
                    (data->grid_x + data->kpts[k_idx][0]) * stride,
                    (data->grid_y + data->kpts[k_idx][1]) * stride, 0.5f));
            }

            rawBoxes.emplace_back(bbox);
            boxIdx += 1;
        }
    }

    for (auto &indices : ScoreIndices) {
        sort(indices.begin(), indices.end(), scoreComapre);
    }

    std::vector<BoundingBox> result;
    nms(rawBoxes, ScoreIndices, params.iouThreshold, params.classNames,
        params.numKeypoints, result);

    for (auto &ret : result) {
        int origin_w = object_meta->_box[2] - object_meta->_box[0];
        int origin_h = object_meta->_box[3] - object_meta->_box[1];

        float r, w_pad, h_pad, x1, y1, x2, y2, kx, ky, ks;

        r = std::min(params.input_width / (float)origin_w,
                     params.input_height / (float)origin_h);

        w_pad = (params.input_width - origin_w * r) / 2.;
        h_pad = (params.input_height - origin_h * r) / 2.;

        x1 = (ret.box[0] - w_pad) / r;
        y1 = (ret.box[1] - h_pad) / r;
        x2 = (ret.box[2] - w_pad) / r;
        y2 = (ret.box[3] - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        object_meta->_face_landmarks.clear();
        for (int k = 0; k < params.numKeypoints; k++) {

            kx = (ret.kpt[k * 3 + 0] - w_pad) / r;
            ky = (ret.kpt[k * 3 + 1] - h_pad) / r;
            ks = ret.kpt[k * 3 + 2];

            object_meta->_face_landmarks.push_back(dxs::Point_f(
                kx + object_meta->_box[0], ky + object_meta->_box[1], ks));
        }

        object_meta->_face_confidence = ret.score;
        object_meta->_face_box[0] = x1 + object_meta->_box[0];
        object_meta->_face_box[1] = y1 + object_meta->_box[1];
        object_meta->_face_box[2] = x2 + object_meta->_box[0];
        object_meta->_face_box[3] = y2 + object_meta->_box[1];
        break;
    }
}

extern "C" void
PostProcess(std::vector<shared_ptr<dxrt::Tensor>> network_output,
            DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {

    SCRFDParams params = {.classNames = {"face"},
                          .layerStride = {32, 16, 8}, /* layer re-ordering */
                          .scoreThreshold = 0.5,
                          .iouThreshold = 0.45,
                          .numClasses = 1,
                          .numKeypoints = 5,
                          .input_width = 640,
                          .input_height = 640};

    SCRFDPostProcess(network_output, frame_meta, object_meta, params);
}