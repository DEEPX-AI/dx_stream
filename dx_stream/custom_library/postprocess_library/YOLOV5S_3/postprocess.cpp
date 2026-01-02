#include "dx_stream/gst-dxframemeta.hpp"
#include "dx_stream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// ============================================================================
// YOLO Post-Processing Library for DX Stream
// ============================================================================
// This is a template implementation for YOLO object detection post-processing.
// Users can modify this code to adapt to their own YOLO models.
// 
// Key Features:
// - Supports both single output (ONNX converted) and multi-output (original YOLO) formats
// - Handles NCHW tensor format (batch, channels, height, width)
// - Implements Non-Maximum Suppression (NMS)
// - Supports padding-aware coordinate scaling
// - Configurable thresholds and parameters

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Simple bounding box structure for detected objects
 * 
 * This structure holds the detection results including:
 * - Coordinates (x1, y1, x2, y2) in pixel space
 * - Confidence score (0.0 to 1.0)
 * - Class ID and class name
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
 * 
 * This structure contains all configurable parameters for YOLO detection.
 * Users should modify these values according to their model specifications.
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
 * @param x Input value
 * @return Sigmoid output (0.0 to 1.0)
 */
inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

/**
 * @brief Calculate Intersection over Union (IoU) between two bounding boxes
 * @param box1 First bounding box
 * @param box2 Second bounding box
 * @return IoU value (0.0 to 1.0)
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
 * @param boxes Vector of detected bounding boxes
 * @param threshold IoU threshold for suppression
 * @return Filtered bounding boxes after NMS
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
 * @brief Parse single output format (e.g., ONNX converted models with "515" blob)
 * 
 * This function handles models that output a single tensor with format:
 * [num_detections, 5 + num_classes] where each row contains:
 * [x_center, y_center, width, height, objectness, class_scores...]
 * 
 * @param output Single output tensor
 * @param config YOLO configuration
 * @return Vector of detected bounding boxes
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

/**
 * @brief Parse multi-output format (original YOLO format with multiple feature maps)
 * 
 * This function handles models that output multiple tensors (feature maps) with format:
 * [batch, channels, height, width] where channels = 3 * (5 + num_classes)
 * 
 * Each grid cell contains 3 anchor boxes, and each anchor has:
 * [x_offset, y_offset, width_scale, height_scale, objectness, class_scores...]
 * 
 * @param outputs Vector of output tensors (feature maps)
 * @param config YOLO configuration
 * @return Vector of detected bounding boxes
 */
std::vector<BoundingBox> parse_multi_output(const std::vector<dxs::DXTensor>& outputs,
                                            const YoloConfig& config) {
    std::vector<BoundingBox> boxes;

    // ============================================================================
    // ANCHOR BOX CONFIGURATION
    // ============================================================================
    // Modify these anchor boxes according to your model's training configuration
    // Each layer has different anchor sizes for different object scales

    // Tensor nsmes
    std::vector<std::string> tensor_names = {
        "378",
        "439",
        "500"
    };

    std::vector<std::vector<std::pair<float, float>>> anchors = {
        // Layer 1 (small objects): 64x64 grid  
        {{10.0f, 13.0f}, {16.0f, 30.0f}, {33.0f, 23.0f}},
        // Layer 2 (medium objects): 32x32 grid
        {{30.0f, 61.0f}, {62.0f, 45.0f}, {59.0f, 119.0f}},
        // Layer 3 (large objects): 16x16 grid
        {{116.0f, 90.0f}, {156.0f, 198.0f}, {373.0f, 326.0f}}
    };

    // Process each output layer
    for (int layer_idx = 0; layer_idx < tensor_names.size(); layer_idx++) {
        const auto& output = outputs[get_index_by_tensor_name(outputs, tensor_names[layer_idx])];
        const float* data = static_cast<const float*>(output._data);

        // ============================================================================
        // TENSOR DIMENSIONS (NCHW format)
        // ============================================================================
        // output._shape = [batch, channels, height, width]
        int channels = output._shape[1];      // Total channels (3 * (5 + num_classes))
        int height = output._shape[2];        // Grid height (64, 32, or 16)
        int width = output._shape[3];         // Grid width (64, 32, or 16)

        // Calculate stride for coordinate scaling
        int stride_x = config.input_width / width;
        int stride_y = config.input_height / height;

        // ============================================================================
        // ANCHOR CONFIGURATION
        // ============================================================================
        int num_anchors = 3;                          // 3 anchors per grid cell
        int anchor_channels = channels / num_anchors; // Features per anchor (5 + num_classes)

        // ============================================================================
        // NCHW MEMORY LAYOUT
        // ============================================================================
        // In NCHW format, data is organized as:
        // [channel0_all_positions][channel1_all_positions][channel2_all_positions]...
        // Each channel spans the entire spatial dimensions (height * width)
        const int channel_stride = height * width;  // Memory distance between channels

        // ============================================================================
        // GRID CELL PROCESSING
        // ============================================================================
        for (int gy = 0; gy < height; ++gy) {
            for (int gx = 0; gx < width; ++gx) {
                // Calculate spatial offset for current grid cell
                const int spatial_offset = gy * width + gx;

                // Process each anchor in the current grid cell
                for (int anchor = 0; anchor < num_anchors; ++anchor) {
                    // Calculate base channel index for current anchor
                    const int anchor_base_channel = anchor * anchor_channels;

                    // ============================================================================
                    // STEP 1: OBJECTNESS SCORE
                    // ============================================================================
                    // Get objectness score (probability that this anchor contains an object)
                    const int obj_channel = anchor_base_channel + 4;
                    float objectness = sigmoid(data[obj_channel * channel_stride + spatial_offset]);

                    if (objectness <= config.conf_threshold) continue;

                    // ============================================================================
                    // STEP 2: CLASS SCORES
                    // ============================================================================
                    // Find the class with highest score
                    int best_class = 0;
                    float max_class_score = -1.0f;

                    for (int cls = 0; cls < config.num_classes; ++cls) {
                        const int class_channel = anchor_base_channel + 5 + cls;
                        float class_score = data[class_channel * channel_stride + spatial_offset];
                        if (class_score > max_class_score) {
                            max_class_score = class_score;
                            best_class = cls;
                        }
                    }
                    
                    // Calculate final confidence
                    float confidence = objectness * sigmoid(max_class_score);
                    if (confidence <= config.conf_threshold) continue;

                    // ============================================================================
                    // STEP 3: BOUNDING BOX COORDINATES
                    // ============================================================================
                    // Get raw coordinate predictions
                    const int x_channel = anchor_base_channel + 0;
                    const int y_channel = anchor_base_channel + 1;
                    const int w_channel = anchor_base_channel + 2;
                    const int h_channel = anchor_base_channel + 3;

                    float tx = data[x_channel * channel_stride + spatial_offset];
                    float ty = data[y_channel * channel_stride + spatial_offset];
                    float tw = data[w_channel * channel_stride + spatial_offset];
                    float th = data[h_channel * channel_stride + spatial_offset];
                    
                    // ============================================================================
                    // STEP 4: COORDINATE DECODING (YOLOv5/v7 format)
                    // ============================================================================
                    // Decode coordinates from network predictions to pixel coordinates
                    // This formula may vary depending on your YOLO version
                    float x = (sigmoid(tx) * 2.0f - 0.5f + gx) * stride_x;
                    float y = (sigmoid(ty) * 2.0f - 0.5f + gy) * stride_y;
                    float w = std::pow(sigmoid(tw) * 2.0f, 2) * anchors[layer_idx][anchor].first;
                    float h = std::pow(sigmoid(th) * 2.0f, 2) * anchors[layer_idx][anchor].second;

                    // Convert center coordinates to corner coordinates
                    boxes.emplace_back(x - w / 2, y - h / 2, x + w / 2, y + h / 2,
                                       confidence, best_class, config.class_names[best_class]);
                }
            }
        }
    }
    return boxes;
}

// ============================================================================
// Coordinate Transformation
// ============================================================================

/**
 * @brief Scale bounding box coordinates from model input size to original image size
 * 
 * This function handles aspect ratio preserving scaling and padding removal.
 * When input images are resized to fit the model, padding may be added to maintain
 * aspect ratio. This function removes the padding and scales coordinates back.
 * 
 * @param box Bounding box in model input coordinates
 * @param orig_width Original image width
 * @param orig_height Original image height
 * @param model_width Model input width
 * @param model_height Model input height
 * @return Scaled bounding box in original image coordinates
 */
BoundingBox scale_box(const BoundingBox& box, int orig_width, int orig_height, 
                      int model_width, int model_height) {
    // Calculate scaling ratio (maintains aspect ratio)
    float r = std::min(static_cast<float>(model_width) / orig_width,
                       static_cast<float>(model_height) / orig_height);
    
    // Calculate padding that was added during preprocessing
    float w_pad = (model_width - orig_width * r) / 2.0f;
    float h_pad = (model_height - orig_height * r) / 2.0f;
    
    // Remove padding and scale to original image coordinates
    float x1 = (box.x1 - w_pad) / r;
    float y1 = (box.y1 - h_pad) / r;
    float x2 = (box.x2 - w_pad) / r;
    float y2 = (box.y2 - h_pad) / r;
    
    return BoundingBox(x1, y1, x2, y2, box.confidence, box.class_id, box.class_name);
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
 * 2. Multi Output: Processes multiple feature maps (original YOLO format)
 * 
 * The function then applies NMS and scales coordinates to original image space.
 * 
 * @param network_output Vector of network output tensors
 * @param frame_meta Frame metadata containing image dimensions and ROI
 * @param object_meta Object metadata (output parameter)
 */
extern "C" void PostProcess(GstBuffer *buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta,
                            DXObjectMeta *object_meta) {
    // ============================================================================
    // CONFIGURATION SETUP
    // ============================================================================
    // Create configuration object - modify these values for your model
    YoloConfig config;
    // ============================================================================
    // OUTPUT PARSING
    // ============================================================================
    std::vector<BoundingBox> all_boxes;
    
    // Check if this is a single output format (ONNX converted model)
    int ort_idx = get_index_by_tensor_name(network_output, "output");
    
    if (ort_idx != -1) {
        // Single output format (e.g., ONNX converted models)
        all_boxes = parse_single_output(network_output[ort_idx], config);
    } else {
        // Multi-output format (original YOLO format)
        all_boxes = parse_multi_output(network_output, config);
    }
    
    // ============================================================================
    // NON-MAXIMUM SUPPRESSION
    // ============================================================================
    // Remove overlapping detections
    auto final_boxes = nms(all_boxes, config.nms_threshold);
    
    // ============================================================================
    // COORDINATE SCALING
    // ============================================================================
    // Get original image dimensions
    int orig_width = frame_meta->_width;
    int orig_height = frame_meta->_height;
    
    // Handle ROI (Region of Interest) if specified
    if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
        frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
        orig_width = frame_meta->_roi[2] - frame_meta->_roi[0];
        orig_height = frame_meta->_roi[3] - frame_meta->_roi[1];
    }
    
    // ============================================================================
    // RESULT CONVERSION
    // ============================================================================
    // Convert bounding boxes to DX Stream format
    for (const auto& box : final_boxes) {
        // Scale coordinates to original image space
        auto scaled_box = scale_box(box, orig_width, orig_height, 
                                   config.input_width, config.input_height);
        
        // Clamp coordinates to image boundaries
        scaled_box.x1 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_box.x1));
        scaled_box.y1 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_box.y1));
        scaled_box.x2 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_box.x2));
        scaled_box.y2 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_box.y2));
        
        // Create DX Stream object metadata
        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = scaled_box.confidence;
        obj_meta->_label = scaled_box.class_id;
        obj_meta->_label_name = g_string_new(scaled_box.class_name.c_str());
        obj_meta->_box[0] = scaled_box.x1;
        obj_meta->_box[1] = scaled_box.y1;
        obj_meta->_box[2] = scaled_box.x2;
        obj_meta->_box[3] = scaled_box.y2;
        
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
