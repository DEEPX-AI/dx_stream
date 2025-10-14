#include "dx_stream/gst-dxmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

#define sigmoid(x) (1 / (1 + std::exp(-x)))

struct BoundingBox {
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
    std::string class_name;
    std::vector<float> keypoints; // Optional: for models that predict keypoints
    
    BoundingBox(float x1, float y1, float x2, float y2, 
                float conf, int cls_id, const std::string& cls_name)
        : x1(x1), y1(y1), x2(x2), y2(y2), 
          confidence(conf), class_id(cls_id), class_name(cls_name) {}
    BoundingBox(float x1, float y1, float x2, float y2, 
                float conf, int cls_id, const std::string& cls_name,
                const std::vector<float>& kpts)
        : x1(x1), y1(y1), x2(x2), y2(y2), 
          confidence(conf), class_id(cls_id), class_name(cls_name), keypoints(kpts) {}
};

struct YoloConfig {
    int input_size = 0;

    float conf_threshold = 0.25f;
    float nms_threshold = 0.4f;
    
    int num_classes = 80;
    int num_keypoints = 0;
    
    std::vector<std::string> class_names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
        "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };

    std::vector<int> strides = {8, 16, 32};

    std::vector<std::vector<std::pair<float, float>>> anchors = {
        {{10.0f, 13.0f}, {16.0f, 30.0f}, {33.0f, 23.0f}},
        {{30.0f, 61.0f}, {62.0f, 45.0f}, {59.0f, 119.0f}},
        {{116.0f, 90.0f}, {156.0f, 198.0f}, {373.0f, 326.0f}}
    };
};

// extern "C" void SCRFDPostProcess(std::vector<dxs::DXTensor> outputs,
//                                  DXFrameMeta *frame_meta,
//                                  DXObjectMeta *object_meta,
//                                  SCRFDParams params) {
//     std::vector<std::vector<std::pair<float, int>>> ScoreIndices;
//     for (int i = 0; i < params.numClasses; i++) {
//         std::vector<std::pair<float, int>> v;
//         ScoreIndices.emplace_back(v);
//     }

//     std::vector<BBox> rawBoxes;
//     rawBoxes.clear();

//     int boxIdx = 0;
//     for (int b_idx = 0; b_idx < outputs[0]._shape[1]; b_idx++) {
//         uint8_t *raw_data = (uint8_t *)outputs[0]._data + (b_idx * 64);
//         dxs::DeviceFace_t *data =
//             static_cast<dxs::DeviceFace_t *>((void *)raw_data);
//         int stride = params.layerStride[data->layer_idx];

//         if (data->score >= params.scoreThreshold) {

//             ScoreIndices[0].emplace_back(data->score, boxIdx);

//             BBox bbox = {(data->grid_x - data->x) * stride,
//                          (data->grid_y - data->y) * stride,
//                          (data->grid_x + data->w) * stride,
//                          (data->grid_y + data->h) * stride,
//                          2 * data->x * stride,
//                          2 * data->x * stride,
//                          {dxs::Point_f(-1, -1, -1)}};

//             bbox._width = bbox._xmax - bbox._xmin;
//             bbox._height = bbox._ymax - bbox._ymin;

//             bbox._kpts.clear();
//             for (int k_idx = 0; k_idx < params.numKeypoints; k_idx++) {
//                 bbox._kpts.emplace_back(dxs::Point_f(
//                     (data->grid_x + data->kpts[k_idx][0]) * stride,
//                     (data->grid_y + data->kpts[k_idx][1]) * stride, 0.5f));
//             }

//             rawBoxes.emplace_back(bbox);
//             boxIdx += 1;
//         }
//     }

//     for (auto &indices : ScoreIndices) {
//         sort(indices.begin(), indices.end(), scoreComapre);
//     }

//     std::vector<BoundingBox> result;
//     nms(rawBoxes, ScoreIndices, params.iouThreshold, params.classNames,
//         params.numKeypoints, result);

//     if (result.size() > 0) {
//         BoundingBox ret = result[0];
//         int origin_w = object_meta->_box[2] - object_meta->_box[0];
//         int origin_h = object_meta->_box[3] - object_meta->_box[1];

//         float r, w_pad, h_pad, x1, y1, x2, y2, kx, ky, ks;

//         r = std::min(params.input_width / (float)origin_w,
//                      params.input_height / (float)origin_h);

//         w_pad = (params.input_width - origin_w * r) / 2.;
//         h_pad = (params.input_height - origin_h * r) / 2.;

//         x1 = (ret.box[0] - w_pad) / r;
//         y1 = (ret.box[1] - h_pad) / r;
//         x2 = (ret.box[2] - w_pad) / r;
//         y2 = (ret.box[3] - h_pad) / r;

//         x1 = std::min((float)origin_w, std::max((float)0.0, x1));
//         x2 = std::min((float)origin_w, std::max((float)0.0, x2));
//         y1 = std::min((float)origin_h, std::max((float)0.0, y1));
//         y2 = std::min((float)origin_h, std::max((float)0.0, y2));

//         object_meta->_face_landmarks.clear();
//         for (int k = 0; k < params.numKeypoints; k++) {

//             kx = (ret.kpt[k * 3 + 0] - w_pad) / r;
//             ky = (ret.kpt[k * 3 + 1] - h_pad) / r;
//             ks = ret.kpt[k * 3 + 2];

//             object_meta->_face_landmarks.push_back(dxs::Point_f(
//                 kx + object_meta->_box[0], ky + object_meta->_box[1], ks));
//         }

//         object_meta->_face_confidence = ret.score;
//         object_meta->_face_box[0] = x1 + object_meta->_box[0];
//         object_meta->_face_box[1] = y1 + object_meta->_box[1];
//         object_meta->_face_box[2] = x2 + object_meta->_box[0];
//         object_meta->_face_box[3] = y2 + object_meta->_box[1];
//     }
// }

float calculate_iou(const BoundingBox& box1, const BoundingBox& box2) {
    float x1 = std::max(box1.x1, box2.x1);
    float y1 = std::max(box1.y1, box2.y1);
    float x2 = std::min(box1.x2, box2.x2);
    float y2 = std::min(box1.y2, box2.y2);
    
    if (x2 < x1 || y2 < y1) return 0.0f;
    
    float intersection = (x2 - x1) * (y2 - y1);
    float area1 = (box1.x2 - box1.x1) * (box1.y2 - box1.y1);
    float area2 = (box2.x2 - box2.x1) * (box2.y2 - box2.y1);
    
    return intersection / (area1 + area2 - intersection);
}

std::vector<BoundingBox> nms(std::vector<BoundingBox>& boxes, float threshold) {
    if (boxes.empty()) return {};
    
    std::sort(boxes.begin(), boxes.end(), 
              [](const BoundingBox& a, const BoundingBox& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> suppressed(boxes.size(), false);
    std::vector<BoundingBox> result;
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (suppressed[i]) continue;
        
        result.push_back(boxes[i]);
        
        for (size_t j = i + 1; j < boxes.size(); ++j) {
            if (suppressed[j]) continue;
            
            if (boxes[i].class_id == boxes[j].class_id) {
                if (calculate_iou(boxes[i], boxes[j]) > threshold) {
                    suppressed[j] = true;
                }
            }
        }
    }
    
    return result;
}

// Decode bounding boxes for object detection (BBOX type)
std::vector<BoundingBox> decode_bbox(dxs::DXTensor &output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    int num_detections = output._shape[1];
    auto *dataSrc = static_cast<dxs::DeviceBoundingBox_t*>(output._data);
    
    for (int i = 0; i < num_detections; i++) {
        auto *data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        int gX = data->grid_x;
        int gY = data->grid_y;

        auto& anchors_for_layer = config.anchors[data->layer_idx];
        std::pair<float, float> anchor = anchors_for_layer[data->box_idx];

        float x, y, w, h;

        x = (data->x * 2. - 0.5 + gX) * config.strides[data->layer_idx];
        y = (data->y * 2. - 0.5 + gY) * config.strides[data->layer_idx];
        w = (data->w * data->w * 4.) * anchor.first;
        h = (data->h * data->h * 4.) * anchor.second;

        BoundingBox box(x - w / 2., y - h / 2., x + w / 2., y + h / 2., 
                       data->score, data->label, config.class_names[data->label]);
        boxes.push_back(box);
    }
    
    return boxes;
}

// Decode poses with keypoints (POSE type)
std::vector<BoundingBox> decode_pose(dxs::DXTensor &output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    int num_detections = output._shape[1];
    auto *dataSrc = static_cast<dxs::DevicePose_t*>(output._data);
    
    for (int i = 0; i < num_detections; i++) {
        auto *data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        int gX = data->grid_x;
        int gY = data->grid_y;

        auto& anchors_for_layer = config.anchors[data->layer_idx];
        std::pair<float, float> anchor = anchors_for_layer[data->box_idx];

        float x, y, w, h;

        x = (data->x * 2. - 0.5 + gX) * config.strides[data->layer_idx];
        y = (data->y * 2. - 0.5 + gY) * config.strides[data->layer_idx];
        w = (data->w * data->w * 4.) * anchor.first;
        h = (data->h * data->h * 4.) * anchor.second;

        // Extract keypoints
        std::vector<float> keypoints;
        if (config.num_keypoints > 0) {
            for (int k = 0; k < config.num_keypoints; k++) {
                keypoints.emplace_back((data->kpts[k][0] * 2. - 0.5 + gX) *
                                       config.strides[data->layer_idx]);
                keypoints.emplace_back((data->kpts[k][1] * 2. - 0.5 + gY) *
                                       config.strides[data->layer_idx]);
                keypoints.emplace_back(sigmoid(data->kpts[k][2]));
            }
        }

        BoundingBox box(x - w / 2., y - h / 2., x + w / 2., y + h / 2., 
                       data->score, 0, config.class_names[0], keypoints);
        boxes.push_back(box);
    }
    
    return boxes;
}

// Decode faces with keypoints (FACE type) - SCRFD style
std::vector<BoundingBox> decode_face(dxs::DXTensor &output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    int num_detections = output._shape[1];
    auto *dataSrc = static_cast<dxs::DeviceFace_t*>(output._data);

    for (int i = 0; i < num_detections; i++) {
        auto *data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        int stride = config.strides[data->layer_idx];

        // SCRFD bbox decoding - matches original implementation
        float x1 = (data->grid_x - data->x) * stride;
        float y1 = (data->grid_y - data->y) * stride;
        float x2 = (data->grid_x + data->w) * stride;
        float y2 = (data->grid_y + data->h) * stride;

        // Extract keypoints (SCRFD style - no offset adjustment)
        std::vector<float> keypoints;
        if (config.num_keypoints > 0) {
            for (int k = 0; k < config.num_keypoints; k++) {
                keypoints.emplace_back((data->grid_x + data->kpts[k][0]) * stride);
                keypoints.emplace_back((data->grid_y + data->kpts[k][1]) * stride);
                keypoints.emplace_back(0.5f);  // Fixed confidence for SCRFD landmarks
            }
        }

        BoundingBox box(x1, y1, x2, y2, 
                       data->score, 0, config.class_names[0], keypoints);
        boxes.push_back(box);
    }
    
    return boxes;
}

extern "C" void YOLOV5S_PPU(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {

    YoloConfig config;

    config.input_size = 320;

    std::vector<BoundingBox> all_boxes;

    if (network_output.size() != 1) {
        GST_ERROR("Unexpected number of outputs: %ld", network_output.size());
        return;
    }

    if (network_output[0]._type != dxs::DataType::BBOX) {
        GST_ERROR("Data type is not BBOX");
        return;
    }

    all_boxes = decode_bbox(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(config.input_size / (float)origin_w,
                           config.input_size / (float)origin_h);
        float w_pad = (config.input_size - origin_w * r) / 2.;
        float h_pad = (config.input_size - origin_h * r) / 2.;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *object_meta = dx_create_object_meta(buf);
        object_meta->_confidence = ret.confidence;
        object_meta->_label = ret.class_id;
        object_meta->_label_name = g_string_new(ret.class_name.c_str());
        object_meta->_box[0] = x1;
        object_meta->_box[1] = y1;
        object_meta->_box[2] = x2;
        object_meta->_box[3] = y2;
        dx_add_object_meta_to_frame_meta(object_meta, frame_meta);
    }
}


extern "C" void YOLOV5Pose_PPU(GstBuffer *buf,
                               std::vector<dxs::DXTensor> network_output,
                               DXFrameMeta *frame_meta,
                               DXObjectMeta *object_meta) {
    YoloConfig config;

    config.input_size = 640;
    config.num_keypoints = 17;
    config.num_classes = 1;
    config.class_names = {"person"};
    config.strides = {8, 16, 32, 64};
    config.anchors = {
        {{19.0f, 27.0f}, {44.0f, 40.0f}, {38.0f, 94.0f}},
        {{96.0f, 68.0f}, {86.0f, 152.0f}, {180.0f, 137.0f}},
        {{140.0f, 301.0f}, {303.0f, 264.0f}, {238.0f, 542.0f}},
        {{436.0f, 615.0f}, {739.0f, 380.0f}, {925.0f, 792.0f}}
    };

    std::vector<BoundingBox> all_boxes;

    if (network_output.size() != 1) {
        GST_ERROR("Unexpected number of outputs: %ld", network_output.size());
        return;
    }

    if (network_output[0]._type != dxs::DataType::POSE) {
        GST_ERROR("Data type is not POSE");
        return;
    }

    all_boxes = decode_pose(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(config.input_size / (float)origin_w,
                           config.input_size / (float)origin_h);
        float w_pad = (config.input_size - origin_w * r) / 2.;
        float h_pad = (config.input_size - origin_h * r) / 2.;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *object_meta = dx_create_object_meta(buf);
        object_meta->_confidence = ret.confidence;
        object_meta->_label = ret.class_id;
        object_meta->_label_name = g_string_new(ret.class_name.c_str());
        object_meta->_box[0] = x1;
        object_meta->_box[1] = y1;
        object_meta->_box[2] = x2;
        object_meta->_box[3] = y2;

        for (int k = 0; k < config.num_keypoints; k++) {
            float kx = (ret.keypoints[k * 3 + 0] - w_pad) / r;
            float ky = (ret.keypoints[k * 3 + 1] - h_pad) / r;
            float ks = ret.keypoints[k * 3 + 2];
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

extern "C" void SCRFD500M_PPU(GstBuffer *buf,
                              std::vector<dxs::DXTensor> network_output,
                              DXFrameMeta *frame_meta,
                              DXObjectMeta *object_meta) {

    YoloConfig config;

    config.input_size = 640;
    config.num_keypoints = 5;
    config.num_classes = 1;
    config.conf_threshold = 0.5f;
    config.nms_threshold = 0.45f;
    config.class_names = {"face"};
    config.strides = {8, 16, 32};
    config.anchors.clear();

    std::vector<BoundingBox> all_boxes;

    if (network_output.size() != 1) {
        GST_ERROR("Unexpected number of outputs: %ld", network_output.size());
        return;
    }

    if (network_output[0]._type != dxs::DataType::FACE) {
        GST_ERROR("Data type is not FACE");
        return;
    }

    all_boxes = decode_face(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(config.input_size / (float)origin_w,
                           config.input_size / (float)origin_h);
        float w_pad = (config.input_size - origin_w * r) / 2.;
        float h_pad = (config.input_size - origin_h * r) / 2.;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *object_meta = dx_create_object_meta(buf);
        object_meta->_face_confidence = ret.confidence;
        object_meta->_label = ret.class_id;
        object_meta->_label_name = g_string_new(ret.class_name.c_str());
        object_meta->_face_box[0] = x1;
        object_meta->_face_box[1] = y1;
        object_meta->_face_box[2] = x2;
        object_meta->_face_box[3] = y2;

        object_meta->_face_landmarks.clear();
        for (int k = 0; k < config.num_keypoints; k++) {
            float kx = (ret.keypoints[k * 3 + 0] - w_pad) / r;
            float ky = (ret.keypoints[k * 3 + 1] - h_pad) / r;
            float ks = ret.keypoints[k * 3 + 2];
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
                object_meta->_face_landmarks.push_back(dxs::Point_f(
                    kx + frame_meta->_roi[0], ky + frame_meta->_roi[1], ks));
            } else {
                object_meta->_face_landmarks.push_back(dxs::Point_f(kx, ky, ks));   
            }
        }

        dx_add_object_meta_to_frame_meta(object_meta, frame_meta);
    }
}
