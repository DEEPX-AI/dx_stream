#include "dx_stream/gst-dxmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// YOLO Post-Processing Library for DX Stream
// This is a template implementation for YOLO object detection post-processing.
// Users can modify this code to adapt to their own YOLO models.

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Simple bounding box structure for detected objects
 */
struct BoundingBox {
    float x1, y1, x2, y2;      // Bounding box coordinates (left, top, right, bottom)
    float confidence;           // Detection confidence (0.0 to 1.0)
    int class_id;              // Class ID (0-based index)
    std::string class_name;    // Human-readable class name
    
    BoundingBox(float x1, float y1, float x2, float y2, 
                float conf, int cls_id, const std::string& cls_name)
        : x1(x1), y1(y1), x2(x2), y2(y2), 
          confidence(conf), class_id(cls_id), class_name(cls_name) {}
};

/**
 * @brief Configuration structure for YOLO post-processing
 */
struct YoloConfig {
    // Model input dimensions (must match your model's input size)
    int input_width = 512;
    int input_height = 512;
    
    // Detection thresholds (adjust based on your requirements)
    float conf_threshold = 0.25f;    // Minimum confidence for detection
    float nms_threshold = 0.4f;      // IoU threshold for NMS
    
    // Number of classes in your dataset
    int num_classes = 80;
    
    // COCO dataset class names (modify for your dataset)
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
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get the index of a tensor by its name
 * @param network_output Vector of network output tensors
 * @param tensor_name Name of the tensor to search for
 * @return Index of the tensor if found, -1 otherwise
 */
 inline int get_index_by_tensor_name(const std::vector<dxs::DXTensor>& network_output, const std::string& tensor_name) {
    for (size_t i = 0; i < network_output.size(); i++) {
        if (network_output[i]._name == tensor_name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

/**
 * @brief Sigmoid activation function
 */
inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

/**
 * @brief Calculate Intersection over Union (IoU) between two bounding boxes
 */
float calculate_iou(const BoundingBox& box1, const BoundingBox& box2) {
    // Calculate intersection rectangle
    float x1 = std::max(box1.x1, box2.x1);
    float y1 = std::max(box1.y1, box2.y1);
    float x2 = std::min(box1.x2, box2.x2);
    float y2 = std::min(box1.y2, box2.y2);
    
    // No intersection
    if (x2 < x1 || y2 < y1) return 0.0f;
    
    // Calculate areas
    float intersection = (x2 - x1) * (y2 - y1);
    float area1 = (box1.x2 - box1.x1) * (box1.y2 - box1.y1);
    float area2 = (box2.x2 - box2.x1) * (box2.y2 - box2.y1);
    
    return intersection / (area1 + area2 - intersection);
}

/**
 * @brief Non-Maximum Suppression (NMS) to remove overlapping detections
 */
std::vector<BoundingBox> nms(std::vector<BoundingBox>& boxes, float threshold) {
    if (boxes.empty()) return {};
    
    // Sort boxes by confidence (highest first)
    std::sort(boxes.begin(), boxes.end(), 
              [](const BoundingBox& a, const BoundingBox& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> suppressed(boxes.size(), false);
    std::vector<BoundingBox> result;
    
    for (size_t i = 0; i < boxes.size(); ++i) {
        if (suppressed[i]) continue;
        
        // Keep the current box
        result.push_back(boxes[i]);
        
        // Check overlap with remaining boxes
        for (size_t j = i + 1; j < boxes.size(); ++j) {
            if (suppressed[j]) continue;
            
            // Only suppress boxes of the same class
            if (boxes[i].class_id == boxes[j].class_id) {
                if (calculate_iou(boxes[i], boxes[j]) > threshold) {
                    suppressed[j] = true;
                }
            }
        }
    }
    
    return result;
}

// ============================================================================
// YOLO Output Parsing Functions
// ============================================================================

/**
 * @brief Parse single output format (e.g., ONNX converted models with "output" blob)
 * 
 * This function handles models that output a single tensor with format:
 * [num_detections, 5 + num_classes] where each row contains:
 * [x_center, y_center, width, height, objectness, class_scores...]
 */
std::vector<BoundingBox> parse_single_output(const dxs::DXTensor& output, 
                                             const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    const float* data = static_cast<const float*>(output._data);
    
    // Tensor shape: [batch, num_detections, 5 + num_classes]
    int num_detections = output._shape[1];
    int features_per_detection = output._shape[2];  // 5 + num_classes
    
    for (int i = 0; i < num_detections; i++) {
        // Get data for current detection
        const float* detection_data = data + (features_per_detection * i);
        const float* class_scores = detection_data + 5;  // Skip [x, y, w, h, obj]
        
        // Find the class with highest score
        int best_class = 0;
        float max_class_score = class_scores[0];
        for (int cls = 0; cls < config.num_classes; cls++) {
            if (class_scores[cls] > max_class_score) {
                max_class_score = class_scores[cls];
                best_class = cls;
            }
        }
        
        // Calculate final confidence
        float confidence = max_class_score * detection_data[4];  // obj * class_score
        if (confidence <= config.conf_threshold) continue;
        
        // Convert center coordinates to corner coordinates
        // Note: Coordinates are already normalized (0.0 to 1.0)
        float center_x = detection_data[0];
        float center_y = detection_data[1];
        float width = detection_data[2];
        float height = detection_data[3];
        
        float x1 = center_x - width / 2.0f;
        float y1 = center_y - height / 2.0f;
        float x2 = center_x + width / 2.0f;
        float y2 = center_y + height / 2.0f;
        
        boxes.emplace_back(x1, y1, x2, y2, confidence, best_class, config.class_names[best_class]);
    }
    
    return boxes;
}

// ============================================================================
// Main Post-Processing Function
// ============================================================================

/**
 * @brief Main post-processing function for YOLO object detection
 * 
 * This function is the entry point for YOLO post-processing. It automatically
 * detects the output format and applies appropriate parsing:
 * 
 * 1. Single Output: Looks for "output" blob (ONNX converted models)
 * 
 * The function then applies NMS and scales coordinates to original image space.
 */
extern "C" void PostProcess(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {
    // Configuration setup
    YoloConfig config;
    
    int ort_idx = get_index_by_tensor_name(network_output, "output");

    // Output parsing
    std::vector<BoundingBox> all_boxes;

    if (ort_idx != -1) {
        all_boxes = parse_single_output(network_output[ort_idx], config);
    } else {
        GST_ERROR("YOLOX-S_1 support only single output\n");
    }
    
    // Non-Maximum Suppression
    auto final_boxes = nms(all_boxes, config.nms_threshold);
    
    // Get original image dimensions
    int orig_width = frame_meta->_width;
    int orig_height = frame_meta->_height;
    
    // Handle ROI (Region of Interest) if specified
    if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
        frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
        orig_width = frame_meta->_roi[2] - frame_meta->_roi[0];
        orig_height = frame_meta->_roi[3] - frame_meta->_roi[1];
    }
    
    // Convert bounding boxes to DX Stream format
    for (const auto& box : final_boxes) {
        // Scale coordinates to original image space
        // Calculate scaling ratio (maintains aspect ratio)
        float r = std::min(static_cast<float>(config.input_width) / orig_width,
                           static_cast<float>(config.input_height) / orig_height);
        
        // Calculate padding that was added during preprocessing
        float w_pad = (config.input_width - orig_width * r) / 2.0f;
        float h_pad = (config.input_height - orig_height * r) / 2.0f;
        
        // Remove padding and scale to original image coordinates
        float x1 = (box.x1 - w_pad) / r;
        float y1 = (box.y1 - h_pad) / r;
        float x2 = (box.x2 - w_pad) / r;
        float y2 = (box.y2 - h_pad) / r;
        
        // Clamp coordinates to image boundaries
        x1 = std::max(0.0f, std::min(static_cast<float>(orig_width), x1));
        y1 = std::max(0.0f, std::min(static_cast<float>(orig_height), y1));
        x2 = std::max(0.0f, std::min(static_cast<float>(orig_width), x2));
        y2 = std::max(0.0f, std::min(static_cast<float>(orig_height), y2));
        
        // Create DX Stream object metadata
        DXObjectMeta *obj_meta = dx_create_object_meta(buf);
        obj_meta->_confidence = box.confidence;
        obj_meta->_label = box.class_id;
        obj_meta->_label_name = g_string_new(box.class_name.c_str());
        obj_meta->_box[0] = x1;
        obj_meta->_box[1] = y1;
        obj_meta->_box[2] = x2;
        obj_meta->_box[3] = y2;
        
        // Adjust coordinates if ROI is specified
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            obj_meta->_box[0] += frame_meta->_roi[0];
            obj_meta->_box[1] += frame_meta->_roi[1];
            obj_meta->_box[2] += frame_meta->_roi[0];
            obj_meta->_box[3] += frame_meta->_roi[1];
        }
        
        // Add object to frame metadata
        dx_add_object_meta_to_frame_meta(obj_meta, frame_meta);
    }
}
