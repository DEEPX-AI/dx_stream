#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <dxrt/dxrt_api.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <tuple>
#include <glib.h>
#include <gst/gst.h>

#define sigmoid(x) (1 / (1 + std::exp(-x)))

struct BoundingBox {
    float x1;
    float y1;
    float x2;
    float y2;
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

// Helper function: Perform NMS on boxes of a single class
std::vector<BoundingBox> nms_single_class(std::vector<BoundingBox>& boxes_per_class, float threshold) {
    if (boxes_per_class.empty()) return {};
    
    // Sort boxes by confidence (highest first)
    std::sort(boxes_per_class.begin(), boxes_per_class.end(), 
              [](const BoundingBox& a, const BoundingBox& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> suppressed(boxes_per_class.size(), false);
    std::vector<BoundingBox> result;
    
    for (size_t i = 0; i < boxes_per_class.size(); ++i) {
        if (suppressed[i]) continue;
        
        result.push_back(boxes_per_class[i]);
        
        // Check overlap with remaining boxes
        for (size_t j = i + 1; j < boxes_per_class.size(); ++j) {
            if (suppressed[j]) continue;
            
            if (calculate_iou(boxes_per_class[i], boxes_per_class[j]) > threshold) {
                suppressed[j] = true;
            }
        }
    }
    
    return result;
}

std::vector<BoundingBox> nms(const std::vector<BoundingBox>& boxes, float threshold, int num_classes) {
    if (boxes.empty()) return {};
    
    // Group boxes by class
    std::vector<std::vector<BoundingBox>> class_boxes(num_classes);
    for (const auto& box : boxes) {
        if (box.class_id >= 0 && box.class_id < num_classes) {
            class_boxes[box.class_id].push_back(box);
        }
    }
    
    std::vector<BoundingBox> final_boxes;
    
    // Perform NMS for each class separately
    for (auto& boxes_per_class : class_boxes) {
        if (boxes_per_class.empty()) continue;
        
        auto class_result = nms_single_class(boxes_per_class, threshold);
        final_boxes.insert(final_boxes.end(), class_result.begin(), class_result.end());
    }
    
    // Sort final results by confidence
    std::sort(final_boxes.begin(), final_boxes.end(), 
              [](const BoundingBox& a, const BoundingBox& b) {
                  return a.confidence > b.confidence;
              });
    
    return final_boxes;
}

// Decode bounding boxes for object detection (BBOX type)
std::vector<BoundingBox> decode_bbox(const std::shared_ptr<dxrt::Tensor>& output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    const auto num_detections = static_cast<int>(output->shape()[1]);
    const auto* dataSrc = static_cast<dxrt::DeviceBoundingBox_t*>(output->data());
    
    for (int i = 0; i < num_detections; i++) {
        const auto *data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        int gX = data->grid_x;
        int gY = data->grid_y;

        const auto& anchors_for_layer = config.anchors[data->layer_idx];
        const std::pair<float, float> anchor = anchors_for_layer[data->box_idx];

        float x = (data->x * 2.0f - 0.5f + static_cast<float>(gX)) * static_cast<float>(config.strides[data->layer_idx]);
        float y = (data->y * 2.0f - 0.5f + static_cast<float>(gY)) * static_cast<float>(config.strides[data->layer_idx]);
        float w = (data->w * data->w * 4.0f) * anchor.first;
        float h = (data->h * data->h * 4.0f) * anchor.second;

        BoundingBox box(x - w / 2.0f, y - h / 2.0f, x + w / 2.0f, y + h / 2.0f, 
                       data->score, data->label, config.class_names[data->label]);
        boxes.push_back(box);
    }
    
    return boxes;
}

// Decode poses with keypoints (POSE type)
std::vector<BoundingBox> decode_pose(const std::shared_ptr<dxrt::Tensor>& output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    const auto num_detections = static_cast<int>(output->shape()[1]);
    const auto* dataSrc = static_cast<dxrt::DevicePose_t*>(output->data());
    
    for (int i = 0; i < num_detections; ++i) {
        const auto* data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        const int gX = data->grid_x;
        const int gY = data->grid_y;

        const auto& anchors_for_layer = config.anchors[data->layer_idx];
        const std::pair<float, float> anchor = anchors_for_layer[data->box_idx];

        float x = (data->x * 2.0f - 0.5f + static_cast<float>(gX)) * static_cast<float>(config.strides[data->layer_idx]);
        float y = (data->y * 2.0f - 0.5f + static_cast<float>(gY)) * static_cast<float>(config.strides[data->layer_idx]);
        float w = (data->w * data->w * 4.0f) * anchor.first;
        float h = (data->h * data->h * 4.0f) * anchor.second;
        // Extract keypoints
        std::vector<float> keypoints;
        if (config.num_keypoints > 0) {
            for (int k = 0; k < config.num_keypoints; ++k) {
                keypoints.emplace_back((data->kpts[k][0] * 2. - 0.5 + gX) *
                                       config.strides[data->layer_idx]);
                keypoints.emplace_back((data->kpts[k][1] * 2. - 0.5 + gY) *
                                       config.strides[data->layer_idx]);
                keypoints.emplace_back(sigmoid(data->kpts[k][2]));
            }
        }

        BoundingBox box(x - w / 2.0f, y - h / 2.0f, x + w / 2.0f, y + h / 2.0f, 
                       data->score, 0, config.class_names[0], keypoints);
        boxes.push_back(box);
    }
    
    return boxes;
}

// Decode faces with keypoints (FACE type) - SCRFD style
std::vector<BoundingBox> decode_face(const std::shared_ptr<dxrt::Tensor>& output, const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    const auto num_detections = static_cast<int>(output->shape()[1]);
    const auto* dataSrc = static_cast<dxrt::DeviceFace_t*>(output->data());

    for (int i = 0; i < num_detections; ++i) {
        const auto* data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        const int stride = config.strides[data->layer_idx];

        // SCRFD bbox decoding - matches original implementation
        float x1 = (data->grid_x - data->x) * static_cast<float>(stride);
        float y1 = (data->grid_y - data->y) * static_cast<float>(stride);
        float x2 = (data->grid_x + data->w) * static_cast<float>(stride);
        float y2 = (data->grid_y + data->h) * static_cast<float>(stride);

        // Extract keypoints (SCRFD style - no offset adjustment)
        std::vector<float> keypoints;
        if (config.num_keypoints > 0) {
            for (int k = 0; k < config.num_keypoints; ++k) {
                keypoints.emplace_back((data->grid_x + data->kpts[k][0]) * static_cast<float>(stride));
                keypoints.emplace_back((data->grid_y + data->kpts[k][1]) * static_cast<float>(stride));
                keypoints.emplace_back(0.5f);  // Fixed confidence for SCRFD landmarks
            }
        }

        BoundingBox box(x1, y1, x2, y2, 
                       data->score, 0, config.class_names[0], keypoints);
        boxes.push_back(box);
    }
    
    return boxes;
}

extern "C" void YOLOV5S_PPU(GstBuffer* buf,
                            const dxrt::TensorPtrs& network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;

    YoloConfig config;

    config.input_size = 640;

    std::vector<BoundingBox> all_boxes;

    if (network_output.size() != 1) {
        GST_ERROR("Unexpected number of outputs: %ld", network_output.size());
        return;
    }

    all_boxes = decode_bbox(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold, config.num_classes);

    for (const auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(static_cast<float>(config.input_size) / static_cast<float>(origin_w),
                           static_cast<float>(config.input_size) / static_cast<float>(origin_h));
        float w_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_w) * r) / 2.0f;
        float h_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_h) * r) / 2.0f;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min(static_cast<float>(origin_w), std::max(0.0f, x1));
        x2 = std::min(static_cast<float>(origin_w), std::max(0.0f, x2));
        y1 = std::min(static_cast<float>(origin_h), std::max(0.0f, y1));
        y2 = std::min(static_cast<float>(origin_h), std::max(0.0f, y2));

        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = ret.confidence;
        obj_meta->_label = ret.class_id;
        obj_meta->_label_name = ret.class_name;
        obj_meta->_box[0] = x1;
        obj_meta->_box[1] = y1;
        obj_meta->_box[2] = x2;
        obj_meta->_box[3] = y2;
        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}


extern "C" void YOLOV5Pose_PPU(GstBuffer* buf,
                               const dxrt::TensorPtrs& network_output,
                               DXFrameMeta* frame_meta,
                               DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
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

    if (network_output[0]->type() != dxrt::DataType::POSE) {
        GST_ERROR("Data type is not POSE");
        return;
    }

    all_boxes = decode_pose(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold, config.num_classes);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(static_cast<float>(config.input_size) / static_cast<float>(origin_w),
                           static_cast<float>(config.input_size) / static_cast<float>(origin_h));
        float w_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_w) * r) / 2.0f;
        float h_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_h) * r) / 2.0f;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min(static_cast<float>(origin_w), std::max(0.0f, x1));
        x2 = std::min(static_cast<float>(origin_w), std::max(0.0f, x2));
        y1 = std::min(static_cast<float>(origin_h), std::max(0.0f, y1));
        y2 = std::min(static_cast<float>(origin_h), std::max(0.0f, y2));

        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = ret.confidence;
        obj_meta->_label = ret.class_id;
        obj_meta->_label_name = ret.class_name;
        obj_meta->_box[0] = x1;
        obj_meta->_box[1] = y1;
        obj_meta->_box[2] = x2;
        obj_meta->_box[3] = y2;

        for (int k = 0; k < config.num_keypoints; ++k) {
            float kx = (ret.keypoints[k * 3 + 0] - w_pad) / r;
            float ky = (ret.keypoints[k * 3 + 1] - h_pad) / r;
            float ks = ret.keypoints[k * 3 + 2];
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
                obj_meta->_keypoints.push_back(kx + static_cast<float>(frame_meta->_roi[0]));
                obj_meta->_keypoints.push_back(ky + static_cast<float>(frame_meta->_roi[1]));
            } else {
                obj_meta->_keypoints.push_back(kx);
                obj_meta->_keypoints.push_back(ky);
            }

            obj_meta->_keypoints.push_back(ks);
        }

        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}

extern "C" void SCRFD500M_PPU(GstBuffer* buf,
                              const dxrt::TensorPtrs& network_output,
                              DXFrameMeta* frame_meta,
                              DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;

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

    if (network_output[0]->type() != dxrt::DataType::FACE) {
        GST_ERROR("Data type is not FACE");
        return;
    }

    all_boxes = decode_face(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold, config.num_classes);

    for (auto &ret : results) {
        int origin_w = frame_meta->_width;
        int origin_h = frame_meta->_height;
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
            origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
        }

        float r = std::min(static_cast<float>(config.input_size) / static_cast<float>(origin_w),
                           static_cast<float>(config.input_size) / static_cast<float>(origin_h));
        float w_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_w) * r) / 2.0f;
        float h_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_h) * r) / 2.0f;

        float x1 = (ret.x1 - w_pad) / r;
        float x2 = (ret.x2 - w_pad) / r;
        float y1 = (ret.y1 - h_pad) / r;
        float y2 = (ret.y2 - h_pad) / r;

        x1 = std::min((float)origin_w, std::max((float)0.0, x1));
        x2 = std::min((float)origin_w, std::max((float)0.0, x2));
        y1 = std::min((float)origin_h, std::max((float)0.0, y1));
        y2 = std::min((float)origin_h, std::max((float)0.0, y2));

        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_face_confidence = ret.confidence;
        obj_meta->_label = ret.class_id;
        obj_meta->_label_name = ret.class_name;
        obj_meta->_face_box[0] = x1;
        obj_meta->_face_box[1] = y1;
        obj_meta->_face_box[2] = x2;
        obj_meta->_face_box[3] = y2;

        obj_meta->_face_landmarks.clear();
        for (int k = 0; k < config.num_keypoints; ++k) {
            float kx = (ret.keypoints[k * 3 + 0] - w_pad) / r;
            float ky = (ret.keypoints[k * 3 + 1] - h_pad) / r;
            float ks = ret.keypoints[k * 3 + 2];
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
                obj_meta->_face_landmarks.push_back(kx + static_cast<float>(frame_meta->_roi[0]));
                obj_meta->_face_landmarks.push_back(ky + static_cast<float>(frame_meta->_roi[1]));
                obj_meta->_face_landmarks.push_back(ks);
            } else {
                obj_meta->_face_landmarks.push_back(kx);
                obj_meta->_face_landmarks.push_back(ky);
                obj_meta->_face_landmarks.push_back(ks);
            }
        }

        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}

extern "C" void SCRFD500M_PPU_SECOND(GstBuffer* buf,
                                     const dxrt::TensorPtrs& network_output,
                                     DXFrameMeta* frame_meta,
                                     DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = frame_meta;

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

    if (network_output[0]->type() != dxrt::DataType::FACE) {
        GST_ERROR("Data type is not FACE");
        return;
    }

    all_boxes = decode_face(network_output[0], config);

    auto results = nms(all_boxes, config.nms_threshold, config.num_classes);

    if (results.empty()) {
        return;
    }

    const auto& ret = results[0];
    
    auto origin_w = static_cast<int>(object_meta->_box[2] - object_meta->_box[0]);
    auto origin_h = static_cast<int>(object_meta->_box[3] - object_meta->_box[1]);
    if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
        frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
        origin_w = frame_meta->_roi[2] - frame_meta->_roi[0];
        origin_h = frame_meta->_roi[3] - frame_meta->_roi[1];
    }

    float r = std::min(static_cast<float>(config.input_size) / static_cast<float>(origin_w),
                           static_cast<float>(config.input_size) / static_cast<float>(origin_h));
    float w_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_w) * r) / 2.0f;
    float h_pad = (static_cast<float>(config.input_size) - static_cast<float>(origin_h) * r) / 2.0f;

    float x1 = (ret.x1 - w_pad) / r;
    float x2 = (ret.x2 - w_pad) / r;
    float y1 = (ret.y1 - h_pad) / r;
    float y2 = (ret.y2 - h_pad) / r;

    x1 = std::min(static_cast<float>(origin_w), std::max(0.0f, x1));
    x2 = std::min(static_cast<float>(origin_w), std::max(0.0f, x2));
    y1 = std::min(static_cast<float>(origin_h), std::max(0.0f, y1));
    y2 = std::min(static_cast<float>(origin_h), std::max(0.0f, y2));

    object_meta->_face_confidence = ret.confidence;
    object_meta->_label = ret.class_id;
    object_meta->_label_name = ret.class_name;
    object_meta->_face_box[0] = x1 + object_meta->_box[0];
    object_meta->_face_box[1] = y1 + object_meta->_box[1];
    object_meta->_face_box[2] = x2 + object_meta->_box[0];
    object_meta->_face_box[3] = y2 + object_meta->_box[1];

    object_meta->_face_landmarks.clear();
    for (int k = 0; k < config.num_keypoints; ++k) {
        float kx = (ret.keypoints[k * 3 + 0] - w_pad) / r;
        float ky = (ret.keypoints[k * 3 + 1] - h_pad) / r;
        float ks = ret.keypoints[k * 3 + 2];

        kx = std::max(0.0f, std::min(static_cast<float>(origin_w), kx));
        ky = std::max(0.0f, std::min(static_cast<float>(origin_h), ky));

        object_meta->_face_landmarks.push_back(kx + object_meta->_box[0]);
        object_meta->_face_landmarks.push_back(ky + object_meta->_box[1]);
        object_meta->_face_landmarks.push_back(ks);
    }

}
