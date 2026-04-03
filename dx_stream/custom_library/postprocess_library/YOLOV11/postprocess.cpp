#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <map>
#include <tuple>
#include <limits>
#include <glib.h>
#include <gst/gst.h>
#include <string>

// YOLOv11 Post-Processing Library for DX Stream
// This implementation is specifically designed for YOLOv11 models with DFL decoding.

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
 * @brief Configuration structure for YOLOv11 post-processing
 */
struct YoloConfig {
    // Model input dimensions (must match your model's input size)
    int input_width = 640;
    int input_height = 640;
    
    // Detection thresholds (adjust based on your requirements)
    float conf_threshold = 0.25f;    // Minimum confidence for detection
    float nms_threshold = 0.4f;      // IoU threshold for NMS
    
    // Number of classes in your dataset
    int num_classes = 80;
    
    // DFL parameters (Distribution Focal Loss)
    int dfl_bins = 16;  // Number of bins for distance distribution
    
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
 inline int get_index_by_tensor_name(std::vector<dxs::DXTensor> network_output, const std::string& tensor_name) {
    for (size_t i = 0; i < network_output.size(); i++) {
        if (network_output[i]._name == tensor_name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int find_tensor_index_by_shape(const std::vector<dxs::DXTensor>& outputs,
                               int channel, int spatial) {
    for (size_t i = 0; i < outputs.size(); i++) {
        if (static_cast<int>(outputs[i]._shape[1]) == channel &&
            static_cast<int>(outputs[i]._shape[2]) == spatial &&
            static_cast<int>(outputs[i]._shape[3]) == spatial) {
            return static_cast<int>(i);
        }
    }
    return -1;
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
 * @brief Class-wise Non-Maximum Suppression (NMS) to remove overlapping detections
 */
std::vector<BoundingBox> nms(std::vector<BoundingBox>& boxes, float threshold) {
    if (boxes.empty()) return {};
    
    // Group boxes by class ID
    std::map<int, std::vector<BoundingBox>> class_boxes;
    for (const auto& box : boxes) {
        class_boxes[box.class_id].push_back(box);
    }
    
    std::vector<BoundingBox> result;
    
    // Apply NMS for each class independently
    for (auto& class_pair : class_boxes) {
        std::vector<BoundingBox>& class_box_list = class_pair.second;
        
        // Sort boxes by confidence (highest first)
        std::sort(class_box_list.begin(), class_box_list.end(), 
                  [](const BoundingBox& a, const BoundingBox& b) {
                      return a.confidence > b.confidence;
                  });
        
        std::vector<bool> suppressed(class_box_list.size(), false);
        
        for (size_t i = 0; i < class_box_list.size(); ++i) {
            if (suppressed[i]) continue;
            
            // Keep the current box
            result.push_back(class_box_list[i]);
            
            // Check overlap with remaining boxes of the same class
            for (size_t j = i + 1; j < class_box_list.size(); ++j) {
                if (suppressed[j]) continue;
                
                if (calculate_iou(class_box_list[i], class_box_list[j]) > threshold) {
                    suppressed[j] = true;
                }
            }
        }
    }
    
    return result;
}

// ============================================================================
// YOLOv8N Output Parsing
// ============================================================================

/**
 * @brief Parse YOLOv8N single output format
 * 
 * YOLOv8N outputs a single tensor with format: [1, 84, 8400]
 * where 84 = 4 (bbox) + 80 (classes)
 */
std::vector<BoundingBox> parse_single_output(const dxs::DXTensor& output,
                                             const YoloConfig& config) {
    std::vector<BoundingBox> boxes;
    const auto* data = static_cast<const float*>(output._data);

    // YOLOv8 output shape: [1, 84, 8400] where 84 = 4 (bbox) + 80 (classes)
    auto dimensions = static_cast<int>(output._shape[1]);  // 84
    auto rows = static_cast<int>(output._shape[2]);        // 8400
    
    // Transpose data (84, 8400) -> (8400, 84)
    std::vector<float> data_transposed(static_cast<size_t>(rows) * dimensions);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < rows; j++) {
            data_transposed[static_cast<size_t>(j) * dimensions + i] =
                data[static_cast<size_t>(i) * rows + j];
        }
    }
    
    for (int i = 0; i < rows; i++) {
        size_t base_idx = static_cast<size_t>(i) * dimensions;
        
        // YOLOv8 format: [x, y, w, h, class_scores...] (no objectness)
        float center_x = data_transposed[base_idx + 0];
        float center_y = data_transposed[base_idx + 1];
        float width = data_transposed[base_idx + 2];
        float height = data_transposed[base_idx + 3];
        
        // Find the class with highest score
        int best_class = 0;
        float max_class_score = data_transposed[base_idx + 4];  // First class score
        
        for (int cls = 0; cls < config.num_classes; cls++) {
            if (data_transposed[base_idx + 4 + cls] > max_class_score) {
                max_class_score = data_transposed[base_idx + 4 + cls];
                best_class = cls;
            }
        }
        
        // YOLOv8: confidence is just the class score (no objectness multiplication)
        float confidence = max_class_score;
        if (confidence <= config.conf_threshold) continue;
        
        // Convert center coordinates to corner coordinates
        // Note: Coordinates are already normalized (0.0 to 1.0)
        float x1 = center_x - width / 2.0f;
        float y1 = center_y - height / 2.0f;
        float x2 = center_x + width / 2.0f;
        float y2 = center_y + height / 2.0f;
        
        boxes.emplace_back(x1, y1, x2, y2, confidence, best_class, config.class_names[best_class]);
    }
    
    return boxes;
}

std::vector<BoundingBox> parse_multi_output(
    const std::vector<dxs::DXTensor>& outputs,
    const YoloConfig& config) {
    
    std::vector<BoundingBox> detections;
    
    // Define scale dimensions: 80x80, 40x40, 20x20
    const int scales[] = {80, 40, 20};
    
    // Process 3 scales
    for (size_t scale_idx = 0; scale_idx < 3; ++scale_idx) {
        int spatial_dim = scales[scale_idx];
        
        // Find regression tensor (channel=64) and classification tensor (channel=80) for this scale
        int reg_idx = find_tensor_index_by_shape(outputs, 64, spatial_dim);
        int cls_idx = find_tensor_index_by_shape(outputs, config.num_classes, spatial_dim);
        
        if (reg_idx < 0 || cls_idx < 0) {
            GST_ERROR("Failed to find tensors for scale %dx%d\n", spatial_dim, spatial_dim);
            continue;
        }
        
        const float* reg_data = static_cast<const float*>(outputs[reg_idx]._data);
        const float* cls_data = static_cast<const float*>(outputs[cls_idx]._data);

        // Tensor shapes: reg=[1, 64, H, W], cls=[1, num_classes, H, W]
        int H = static_cast<int>(outputs[cls_idx]._shape[2]);
        int W = static_cast<int>(outputs[cls_idx]._shape[3]);
        int stride = config.input_width / W;
        int num_grid = H * W;
        
        // Process each grid cell
        for (int h = 0; h < H; ++h) {
            for (int w = 0; w < W; ++w) {
                int spatial_offset = h * W + w;
                
                // Find best class and its confidence
                int max_cls = 0;
                float max_cls_logit = cls_data[0 * num_grid + spatial_offset];
                
                for (int c = 1; c < config.num_classes; ++c) {
                    float logit = cls_data[c * num_grid + spatial_offset];
                    if (logit > max_cls_logit) {
                        max_cls_logit = logit;
                        max_cls = c;
                    }
                }
                
                // Apply sigmoid to convert logit to probability
                float max_cls_conf = 1.0f / (1.0f + std::exp(-max_cls_logit));
                
                // Skip if confidence too low
                if (max_cls_conf <= config.conf_threshold) continue;
                
                // DFL decoding: compute distance for 4 directions (left, top, right, bottom)
                float dist[4];
                for (int k = 0; k < 4; ++k) {
                    float exp_sum = 0.0f;
                    float weighted_sum = 0.0f;
                    float max_val = -std::numeric_limits<float>::infinity();
                    
                    // Find max value for numerical stability (softmax)
                    for (int d = 0; d < config.dfl_bins; ++d) {
                        float val = reg_data[(k * config.dfl_bins + d) * num_grid + spatial_offset];
                        if (val > max_val) max_val = val;
                    }
                    
                    // Compute softmax and weighted sum (expectation)
                    for (int d = 0; d < config.dfl_bins; ++d) {
                        float val = reg_data[(k * config.dfl_bins + d) * num_grid + spatial_offset];
                        float e = std::exp(val - max_val);
                        exp_sum += e;
                        weighted_sum += e * d;
                    }
                    
                    dist[k] = weighted_sum / exp_sum;
                }
                
                // Convert grid coordinates to bounding box
                float anchor_x = w + 0.5f;
                float anchor_y = h + 0.5f;
                
                float x1 = (anchor_x - dist[0]) * stride;
                float y1 = (anchor_y - dist[1]) * stride;
                float x2 = (anchor_x + dist[2]) * stride;
                float y2 = (anchor_y + dist[3]) * stride;
                
                // Create detection result
                detections.emplace_back(x1, y1, x2, y2, max_cls_conf, max_cls, config.class_names[max_cls]);
            }
        }
    }
    
    return detections;
}

std::vector<BoundingBox> parse_ppu_output(const dxs::DXTensor& output,
                                             const YoloConfig& config) {
    std::vector<BoundingBox> detections;

    auto num_detections = static_cast<int>(output._shape[1]);
    const auto* dataSrc = static_cast<dxs::DeviceBoundingBox_t*>(output._data);
    
    for (int i = 0; i < num_detections; i++) {
        auto *data = dataSrc + i;

        if (data->score < config.conf_threshold) continue;

        BoundingBox box(data->x - data->w / 2., data->y - data->h / 2., data->x + data->w / 2., data->y + data->h / 2., 
                       data->score, data->label, config.class_names[data->label]);
        detections.push_back(box);
    }
    
    return detections;
}

// ============================================================================
// Main Post-Processing Function
// ============================================================================

/**
 * @brief Main post-processing function for YOLOv8N object detection
 */
extern "C" void PostProcess(GstBuffer* buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    // Configuration setup
    YoloConfig config;
    
    // YOLOv11 uses 6 outputs (cv2/cv3 for 3 scales)
    std::vector<BoundingBox> all_boxes;
    if (network_output.size() == 6) {
        all_boxes = parse_multi_output(network_output, config);
    } else if (network_output.size() == 1) {
        if (network_output[0]._type == dxs::DataType::BBOX) {
            all_boxes = parse_ppu_output(network_output[0], config);
        } else {
            all_boxes = parse_single_output(network_output[0], config);
        }
    } else {
        GST_ERROR("Unexpected number of output tensors: %zu\n", network_output.size());
        return;
    }
    
    // Apply Non-Maximum Suppression
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
        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = box.confidence;
        obj_meta->_label = box.class_id;
        obj_meta->_label_name = box.class_name;
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
        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}
