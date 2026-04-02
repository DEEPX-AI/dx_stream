#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <tuple>
#include <glib.h>
#include <gst/gst.h>
#include <string>

// ============================================================================
// YOLOV8 Pose Detection Post-Processing Library for DX Stream
// ============================================================================
// This implementation handles YOLOV8 Pose model outputs.
// 
// Key Features:
// - Single output format with 56 channels per detection [x, y, w, h, obj, keypoints(3*17)]
// - Person bounding box detection with 17 pose keypoints
// - Non-Maximum Suppression (NMS)
// - Configurable thresholds and parameters

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Pose detection result structure
 * 
 * This structure holds the pose detection results including:
 * - Bounding box coordinates (x1, y1, x2, y2) in pixel space
 * - Confidence score (0.0 to 1.0)
 * - 17 pose keypoints (conf, x, y for each keypoint)
 */
struct PoseDetection {
    float x1;
    float y1;
    float x2;
    float y2;
    float confidence;           // Detection confidence (0.0 to 1.0)
    std::vector<std::tuple<float, float, float>> keypoints;  // 17 pose keypoints (conf, x, y)
    
    PoseDetection(float x1, float y1, float x2, float y2, float conf)
        : x1(x1), y1(y1), x2(x2), y2(y2), confidence(conf) {
        keypoints.resize(17);  // 17 pose keypoints
    }
};

/**
 * @brief Configuration structure for YOLOV8 Pose post-processing
 */
struct PoseConfig {
    // Model input dimensions
    int input_width = 640;
    int input_height = 640;
    
    // Detection thresholds
    float conf_threshold = 0.25f;    // Minimum confidence for detection (Python 코드와 일치)
    float nms_threshold = 0.45f;    // IoU threshold for NMS
    float kpt_conf_threshold = 0.5f; // Minimum confidence for keypoints
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

/**
 * @brief Sigmoid activation function
 */
inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

/**
 * @brief Convert bounding box from [x_center, y_center, w, h] to [x1, y1, x2, y2]
 */
void xywh2xyxy(float x_center, float y_center, float w, float h, float& x1, float& y1, float& x2, float& y2) {
    x1 = x_center - w / 2.0f;
    y1 = y_center - h / 2.0f;
    x2 = x_center + w / 2.0f;
    y2 = y_center + h / 2.0f;
}

/**
 * @brief Calculate Intersection over Union (IoU) between two bounding boxes
 */
float calculate_iou(const PoseDetection& box1, const PoseDetection& box2) {
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
std::vector<PoseDetection> nms(std::vector<PoseDetection>& poses, float threshold) {
    if (poses.empty()) return {};
    
    // Sort poses by confidence (highest first)
    std::sort(poses.begin(), poses.end(), 
              [](const PoseDetection& a, const PoseDetection& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> suppressed(poses.size(), false);
    std::vector<PoseDetection> result;
    
    for (size_t i = 0; i < poses.size(); ++i) {
        if (suppressed[i]) continue;
        
        // Keep the current pose
        result.push_back(poses[i]);
        
        // Check overlap with remaining poses
        for (size_t j = i + 1; j < poses.size(); ++j) {
            if (suppressed[j]) continue;
            
            if (calculate_iou(poses[i], poses[j]) > threshold) {
                suppressed[j] = true;
            }
        }
    }
    
    return result;
}

// ============================================================================
// Output Parsing Function
// ============================================================================

/**
 * @brief Parse YOLOV8 Pose output
 * 
 * Output format: [1, 56, 8400] -> Transposed to [1, 8400, 56]
 * 56 channels: [x, y, w, h, obj, keypoints(x,y,conf) * 17]
 * 
 * @param output Network output tensor
 * @param config Configuration parameters
 * @return Vector of detected poses
 */
std::vector<PoseDetection> parse_pose_output(const dxs::DXTensor& output,
                                            const PoseConfig& config) {
    std::vector<PoseDetection> poses;
    const auto* data = static_cast<const float*>(output._data);

    // Original shape: [1, 56, 8400]
    auto channels = static_cast<int>(output._shape[1]);  // 56
    auto num_detections = static_cast<int>(output._shape[2]);  // 8400
    
    // Transpose to [1, 8400, 56] (simulate Python's transpose)
    std::vector<float> transposed_data(num_detections * channels);
    for (int i = 0; i < num_detections; ++i) {
        for (int c = 0; c < channels; ++c) {
            transposed_data[i * channels + c] = data[c * num_detections + i];
        }
    }
    
    for (int i = 0; i < num_detections; ++i) {
        const float* detection_data = transposed_data.data() + (channels * i);
        
        // Parse bbox and objectness
        float center_x = detection_data[0];  // x
        float center_y = detection_data[1];  // y
        float width = detection_data[2];     // w
        float height = detection_data[3];    // h
        float objectness = detection_data[4]; // obj
        
        if (objectness <= config.conf_threshold) continue;
        
        // Convert to corner coordinates
        float x1;
        float y1;
        float x2;
        float y2;
        xywh2xyxy(center_x, center_y, width, height, x1, y1, x2, y2);
        
        // Extract keypoints (17 keypoints: x, y, conf for each, starting at index 5)
        std::vector<std::tuple<float, float, float>> keypoints;
        for (int kp = 0; kp < 17; ++kp) {
            float kp_x = detection_data[5 + kp * 3];       // keypoint x
            float kp_y = detection_data[5 + kp * 3 + 1];   // keypoint y
            float kp_conf = detection_data[5 + kp * 3 + 2]; // keypoint conf
            
            kp_conf = sigmoid(kp_conf);
            keypoints.push_back(std::make_tuple(kp_conf, kp_x, kp_y));
        }
        
        PoseDetection pose(x1, y1, x2, y2, objectness);
        pose.keypoints = keypoints;
        poses.push_back(pose);
    }
    
    return poses;
}

// ============================================================================
// Coordinate Transformation
// ============================================================================

/**
 * @brief Scale pose detection coordinates from model input size to original image size
 */
PoseDetection scale_pose(const PoseDetection& pose, int orig_width, int orig_height, 
                        int model_width, int model_height) {
    // Calculate scaling ratio (maintains aspect ratio)
    float r = std::min(static_cast<float>(model_width) / static_cast<float>(orig_width),
                       static_cast<float>(model_height) / static_cast<float>(orig_height));
    
    // Calculate padding that was added during preprocessing
    float w_pad = (static_cast<float>(model_width) - static_cast<float>(orig_width) * r) / 2.0f;
    float h_pad = (static_cast<float>(model_height) - static_cast<float>(orig_height) * r) / 2.0f;
    
    // Remove padding and scale to original image coordinates
    float x1 = (pose.x1 - w_pad) / r;
    float y1 = (pose.y1 - h_pad) / r;
    float x2 = (pose.x2 - w_pad) / r;
    float y2 = (pose.y2 - h_pad) / r;
    
    PoseDetection scaled_pose(x1, y1, x2, y2, pose.confidence);
    scaled_pose.keypoints = pose.keypoints;
    
    // Scale keypoints
    for (auto& keypoint : scaled_pose.keypoints) {
        float conf = std::get<0>(keypoint);
        float kx = std::get<1>(keypoint);
        float ky = std::get<2>(keypoint);
        
        // Scale coordinates
        kx = (kx - w_pad) / r;
        ky = (ky - h_pad) / r;
        
        keypoint = std::make_tuple(conf, kx, ky);
    }
    
    return scaled_pose;
}

// ============================================================================
// Main Post-Processing Function
// ============================================================================

/**
 * @brief Main post-processing function for YOLOV8 Pose
 * 
 * @param network_output Vector of network output tensors
 * @param frame_meta Frame metadata containing image dimensions and ROI
 * @param object_meta Object metadata (output parameter)
 */
extern "C" void PostProcess(GstBuffer* buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    PoseConfig config;
    
    int ort_idx = get_index_by_tensor_name(network_output, "output0");
    
    std::vector<PoseDetection> poses;

    // Parse pose detections
    if (ort_idx != -1) {
        poses = parse_pose_output(network_output[ort_idx], config);
    } else {
        GST_ERROR("YOLOV8Pose support only single output\n");
    }
    
    // ============================================================================
    // NON-MAXIMUM SUPPRESSION
    // ============================================================================
    auto final_poses = nms(poses, config.nms_threshold);
    
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
    // Convert pose detections to DX Stream format
    for (const auto& pose : final_poses) {
        // Scale coordinates to original image space
        auto scaled_pose = scale_pose(pose, orig_width, orig_height, 
                                     config.input_width, config.input_height);
        
        // Clamp coordinates to image boundaries
        scaled_pose.x1 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_pose.x1));
        scaled_pose.y1 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_pose.y1));
        scaled_pose.x2 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_pose.x2));
        scaled_pose.y2 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_pose.y2));
        
        // Create DX Stream object metadata
        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = scaled_pose.confidence;
        obj_meta->_label = 0;  // Person class
        obj_meta->_label_name = "person";
        obj_meta->_box[0] = scaled_pose.x1;
        obj_meta->_box[1] = scaled_pose.y1;
        obj_meta->_box[2] = scaled_pose.x2;
        obj_meta->_box[3] = scaled_pose.y2;
        
        // Add keypoints as additional metadata
        obj_meta->_keypoints.clear();
        for (int k = 0; k < 17; k++) {  // 17 pose keypoints
            float conf = std::get<0>(scaled_pose.keypoints[k]);
            float kx = std::get<1>(scaled_pose.keypoints[k]);
            float ky = std::get<2>(scaled_pose.keypoints[k]);
            
            // Only add keypoints with sufficient confidence
            if (conf < config.kpt_conf_threshold) {
                continue;
            }
            // Clamp keypoint coordinates to image boundaries
            kx = std::max(0.0f, std::min(static_cast<float>(orig_width), kx));
            ky = std::max(0.0f, std::min(static_cast<float>(orig_height), ky));
            
            // Add keypoints in x, y, confidence order
            if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
                frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
                obj_meta->_keypoints.push_back(kx + static_cast<float>(frame_meta->_roi[0]));
                obj_meta->_keypoints.push_back(ky + static_cast<float>(frame_meta->_roi[1]));
            } else {
                obj_meta->_keypoints.push_back(kx);
                obj_meta->_keypoints.push_back(ky);
            }
            obj_meta->_keypoints.push_back(conf);
        }
        
        // Adjust coordinates if ROI is specified
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            obj_meta->_box[0] += static_cast<float>(frame_meta->_roi[0]);
            obj_meta->_box[1] += static_cast<float>(frame_meta->_roi[1]);
            obj_meta->_box[2] += static_cast<float>(frame_meta->_roi[0]);
            obj_meta->_box[3] += static_cast<float>(frame_meta->_roi[1]);
        }
        
        // Add object to frame metadata
        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}
