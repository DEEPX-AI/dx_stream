#include "dxcommon.hpp"
#include "gst-dxmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

// ============================================================================
// YOLOV5S Face Detection Post-Processing Library for DX Stream
// ============================================================================
// This implementation handles YOLOV5S_Face-1 model outputs.
// 
// Key Features:
// - Single output format with 16 channels per detection
// - Face bounding box detection with 5 facial keypoints
// - Non-Maximum Suppression (NMS)
// - Configurable thresholds and parameters

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Face detection result structure
 * 
 * This structure holds the face detection results including:
 * - Bounding box coordinates (x1, y1, x2, y2) in pixel space
 * - Confidence score (0.0 to 1.0)
 * - 5 facial keypoints (left eye, right eye, nose, left mouth, right mouth)
 */
struct FaceDetection {
    float x1, y1, x2, y2;      // Bounding box coordinates (left, top, right, bottom)
    float confidence;           // Detection confidence (0.0 to 1.0)
    std::vector<std::pair<float, float>> landmarks;  // 5 facial keypoints
    
    FaceDetection(float x1, float y1, float x2, float y2, float conf)
        : x1(x1), y1(y1), x2(x2), y2(y2), confidence(conf) {
        landmarks.resize(5);  // 5 facial keypoints
    }
};

/**
 * @brief Configuration structure for YOLOV5S_Face-1 post-processing
 */
struct FaceConfig {
    // Model input dimensions
    int input_width = 640;
    int input_height = 640;
    
    // Detection thresholds
    float conf_threshold = 0.5f;    // Minimum confidence for detection
    float nms_threshold = 0.45f;      // IoU threshold for NMS
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
float calculate_iou(const FaceDetection& box1, const FaceDetection& box2) {
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
std::vector<FaceDetection> nms(std::vector<FaceDetection>& faces, float threshold) {
    if (faces.empty()) return {};
    
    // Sort faces by confidence (highest first)
    std::sort(faces.begin(), faces.end(), 
              [](const FaceDetection& a, const FaceDetection& b) {
                  return a.confidence > b.confidence;
              });
    
    std::vector<bool> suppressed(faces.size(), false);
    std::vector<FaceDetection> result;
    
    for (size_t i = 0; i < faces.size(); ++i) {
        if (suppressed[i]) continue;
        
        // Keep the current face
        result.push_back(faces[i]);
        
        // Check overlap with remaining faces
        for (size_t j = i + 1; j < faces.size(); ++j) {
            if (suppressed[j]) continue;
            
            if (calculate_iou(faces[i], faces[j]) > threshold) {
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
 * @brief Parse YOLOV5S_Face-1 output
 * 
 * Output format: [batch, num_detections, 16]
 * 16 channels: [x, y, w, h, obj, landmarks(x,y) * 5 keypoints]
 * 
 * @param output Network output tensor
 * @param config Configuration parameters
 * @return Vector of detected faces
 */
std::vector<FaceDetection> parse_face_output(const dxs::DXTensor& output, 
                                            const FaceConfig& config) {
    std::vector<FaceDetection> faces;
    const float* data = static_cast<const float*>(output._data);
    
    // Tensor shape: [batch, num_detections, 16]
    int num_detections = output._shape[1];
    int features_per_detection = output._shape[2];  // 16
    
    for (int i = 0; i < num_detections; i++) {
        // Get data for current detection
        const float* detection_data = data + (features_per_detection * i);
        
        // Parse coordinates and objectness
        float center_x = detection_data[0];  // x
        float center_y = detection_data[1];  // y
        float width = detection_data[2];     // w
        float height = detection_data[3];    // h
        float objectness = detection_data[4]; // obj
        float class_conf = detection_data[15]; // class_conf

        // Apply sigmoid to objectness score
        if (objectness * class_conf <= config.conf_threshold) continue;
        
        // Convert center coordinates to corner coordinates
        // Note: Coordinates are already normalized (0.0 to 1.0)
        float x1 = center_x - width / 2.0f;
        float y1 = center_y - height / 2.0f;
        float x2 = center_x + width / 2.0f;
        float y2 = center_y + height / 2.0f;
        
        // Extract landmarks (5 keypoints: left eye, right eye, nose, left mouth, right mouth)
        // landmarks start at index 5: [x1, y1, x2, y2, x3, y3, x4, y4, x5, y5]
        std::vector<std::pair<float, float>> landmarks;
        for (int kp = 0; kp < 5; ++kp) {
            float kp_x = detection_data[5 + kp * 2];     // landmark x
            float kp_y = detection_data[5 + kp * 2 + 1]; // landmark y
            landmarks.push_back(std::make_pair(kp_x, kp_y));
        }
        
        FaceDetection face(x1, y1, x2, y2, objectness * class_conf);
        face.landmarks = landmarks;
        faces.push_back(face);
    }
    
    return faces;
}

// ============================================================================
// Coordinate Transformation
// ============================================================================

/**
 * @brief Scale face detection coordinates from model input size to original image size
 */
FaceDetection scale_face(const FaceDetection& face, int orig_width, int orig_height, 
                        int model_width, int model_height) {
    // Calculate scaling ratio (maintains aspect ratio)
    float r = std::min(static_cast<float>(model_width) / orig_width,
                       static_cast<float>(model_height) / orig_height);
    
    // Calculate padding that was added during preprocessing
    float w_pad = (model_width - orig_width * r) / 2.0f;
    float h_pad = (model_height - orig_height * r) / 2.0f;
    
    // Remove padding and scale to original image coordinates
    float x1 = (face.x1 - w_pad) / r;
    float y1 = (face.y1 - h_pad) / r;
    float x2 = (face.x2 - w_pad) / r;
    float y2 = (face.y2 - h_pad) / r;
    
    FaceDetection scaled_face(x1, y1, x2, y2, face.confidence);
    scaled_face.landmarks = face.landmarks;
    
    // Scale landmarks
    for (auto& landmark : scaled_face.landmarks) {
        landmark.first = (landmark.first - w_pad) / r;
        landmark.second = (landmark.second - h_pad) / r;
    }
    
    return scaled_face;
}

// ============================================================================
// Main Post-Processing Function
// ============================================================================

/**
 * @brief Main post-processing function for YOLOV5S_Face-1
 * 
 * @param network_output Vector of network output tensors
 * @param frame_meta Frame metadata containing image dimensions and ROI
 * @param object_meta Object metadata (output parameter)
 */
extern "C" void PostProcess(std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {
    
    // ============================================================================
    // CONFIGURATION SETUP
    // ============================================================================
    FaceConfig config;
    
    // ============================================================================
    // OUTPUT PARSING
    // ============================================================================
    
    // Output parsing
    std::vector<FaceDetection> faces;
    
    int ort_idx = get_index_by_tensor_name(network_output, "704");

    if (ort_idx != -1) {
        faces = parse_face_output(network_output[ort_idx], config);
    } else {
        GST_ERROR("YOLOV5S_Face-1 support only single output\n");
    }
    
    // ============================================================================
    // NON-MAXIMUM SUPPRESSION
    // ============================================================================
    auto final_faces = nms(faces, config.nms_threshold);
    
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
    // Convert face detections to DX Stream format
    for (const auto& face : final_faces) {
        // Scale coordinates to original image space
        auto scaled_face = scale_face(face, orig_width, orig_height, 
                                     config.input_width, config.input_height);
        
        // Clamp coordinates to image boundaries
        scaled_face.x1 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_face.x1));
        scaled_face.y1 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_face.y1));
        scaled_face.x2 = std::max(0.0f, std::min(static_cast<float>(orig_width), scaled_face.x2));
        scaled_face.y2 = std::max(0.0f, std::min(static_cast<float>(orig_height), scaled_face.y2));
        
        // Create DX Stream object metadata
        DXObjectMeta *obj_meta = dx_create_object_meta(frame_meta->_buf);
        obj_meta->_confidence = scaled_face.confidence;
        obj_meta->_label = 0;  // Face class
        obj_meta->_label_name = g_string_new("face");
        obj_meta->_face_box[0] = scaled_face.x1;
        obj_meta->_face_box[1] = scaled_face.y1;
        obj_meta->_face_box[2] = scaled_face.x2;
        obj_meta->_face_box[3] = scaled_face.y2;
        
        // Add landmarks as additional metadata
        obj_meta->_face_landmarks.clear();
        for (int k = 0; k < 5; k++) {  // 5 facial keypoints
            float kx = scaled_face.landmarks[k].first;
            float ky = scaled_face.landmarks[k].second;
            float ks = 1.0f;  // Default confidence for keypoint
            
            // Add to face landmarks
            obj_meta->_face_landmarks.push_back(dxs::Point_f(kx, ky, ks));
        }
        
        // Adjust coordinates if ROI is specified
        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            obj_meta->_face_box[0] += frame_meta->_roi[0];
            obj_meta->_face_box[1] += frame_meta->_roi[1];
            obj_meta->_face_box[2] += frame_meta->_roi[0];
            obj_meta->_face_box[3] += frame_meta->_roi[1];
        }
        
        // Add object to frame metadata
        dx_add_object_meta_to_frame_meta(obj_meta, frame_meta);
    }
}
