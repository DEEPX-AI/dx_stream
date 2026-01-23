#include "dx_stream/gst-dxframemeta.hpp"
#include "dx_stream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

// ============================================================================
// SCRFD Face Detection Post-Processing Library for DX Stream
// ============================================================================
// This implementation handles SCRFD face detection model outputs.
// 
// Key Features:
// - Multi-scale face detection (8x8, 16x16, 32x32 grids)
// - Face bounding box detection
// - Facial keypoint detection (5 points: eyes, nose, mouth corners)
// - Non-Maximum Suppression (NMS)
// - Configurable thresholds and parameters

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Face detection result structure
 */
struct FaceDetection {
    float x1, y1, x2, y2;      // Bounding box coordinates (left, top, right, bottom)
    float confidence;           // Detection confidence (0.0 to 1.0)
    std::vector<std::pair<float, float>> keypoints;  // 5 facial keypoints
    
    FaceDetection(float x1, float y1, float x2, float y2, float conf)
        : x1(x1), y1(y1), x2(x2), y2(y2), confidence(conf) {
        keypoints.resize(5);  // 5 facial keypoints
    }
};

/**
 * @brief Configuration structure for SCRFD post-processing
 */
struct SCRFDConfig {
    // Model input dimensions
    int input_width = 640;
    int input_height = 640;
    
    // Detection thresholds
    float conf_threshold = 0.5f;    // Minimum confidence for detection
    float nms_threshold = 0.45f;    // IoU threshold for NMS
    
    // Anchor scales for different grid sizes
    std::vector<float> anchor_scales = {8.0f, 16.0f, 32.0f};
    
    // Strides for different grid sizes
    std::vector<int> strides = {8, 16, 32};
    
    // Number of anchors per grid cell
    int num_anchors = 2;
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
 * @brief Parse SCRFD outputs
 * 
 * SCRFD outputs multiple tensors for different scales:
 * - score_8: [B, N, 1] - confidence scores for 8x8 grid
 * - bbox_8: [B, N, 4] - bounding boxes for 8x8 grid
 * - kps_8: [B, N, 10] - keypoints for 8x8 grid
 * - Similar for 16x16 and 32x32 grids
 * 
 * @param network_output Vector of network output tensors
 * @param config Configuration parameters
 * @return Vector of detected faces
 */
std::vector<FaceDetection> parse_scrfd_outputs(const std::vector<dxs::DXTensor>& network_output,
                                              const SCRFDConfig& config) {
    std::vector<FaceDetection> faces;
    
    // Process each scale (8, 16, 32)
    for (size_t scale_idx = 0; scale_idx < config.anchor_scales.size(); ++scale_idx) {
        float anchor_scale = config.anchor_scales[scale_idx];
        int stride = config.strides[scale_idx];
        
        // Find tensors for current scale
        const dxs::DXTensor* score_tensor = nullptr;
        const dxs::DXTensor* bbox_tensor = nullptr;
        const dxs::DXTensor* kps_tensor = nullptr;

        int score_idx = get_index_by_tensor_name(network_output, "score_" + std::to_string(static_cast<int>(anchor_scale)));
        int bbox_idx = get_index_by_tensor_name(network_output, "bbox_" + std::to_string(static_cast<int>(anchor_scale)));
        int kps_idx = get_index_by_tensor_name(network_output, "kps_" + std::to_string(static_cast<int>(anchor_scale)));

        if (score_idx == -1 || bbox_idx == -1 || kps_idx == -1) {
            g_warning("Missing tensors for scale %d in SCRFD output\n", static_cast<int>(anchor_scale));
            continue;
        }
        
        score_tensor = &network_output[score_idx];
        bbox_tensor = &network_output[bbox_idx];
        kps_tensor = &network_output[kps_idx];
        
        const float* score_data = static_cast<const float*>(score_tensor->_data);
        const float* bbox_data = static_cast<const float*>(bbox_tensor->_data);
        const float* kps_data = static_cast<const float*>(kps_tensor->_data);
        
        // Tensor shape: [B, N, features]
        int num_detections = score_tensor->_shape[1];

        // Feature map size inferred from stride (H, W)
        const int feature_map_width = config.input_width / stride;
        const int feature_map_height = config.input_height / stride;
        const int expected_num_detections = feature_map_width * feature_map_height * config.num_anchors;
        if (num_detections != expected_num_detections) {
            g_warning("SCRFD: num_detections(%d) != H(%d)*W(%d)*anchors(%d)=%d for stride %d\n",
                      num_detections, feature_map_height, feature_map_width, config.num_anchors,
                      expected_num_detections, stride);
        }
        
        for (int det = 0; det < num_detections; ++det) {
            // Get confidence score
            if (score_data[det] <= config.conf_threshold) continue;

            // --- 1. 중심점(Center Point) 계산 ---
            // 1D 인덱스(det)에서 앵커 차원 분리 후 2D 그리드 좌표로 변환
            const int loc_idx = det / config.num_anchors; // 위치 인덱스 (앵커 제거)
            const int grid_x = loc_idx % feature_map_width;
            const int grid_y = loc_idx / feature_map_width;
            
            // 그리드 기준점(cx, cy) 계산: 셀 중심이 아닌 좌상단 격자 기준
            const float cx = static_cast<float>(grid_x * stride);
            const float cy = static_cast<float>(grid_y * stride);

            // --- 2. Bounding Box 디코딩 ---
            // bbox_data는 (l, t, r, b) 거리 값을 의미
            float l = bbox_data[det * 4 + 0];
            float t = bbox_data[det * 4 + 1];
            float r = bbox_data[det * 4 + 2];
            float b = bbox_data[det * 4 + 3];

            // 중심점과 거리를 이용해 실제 좌표(x1, y1, x2, y2) 계산
            float x1 = cx - (l * stride);
            float y1 = cy - (t * stride);
            float x2 = cx + (r * stride);
            float y2 = cy + (b * stride);

            // Create face detection
            FaceDetection face(x1, y1, x2, y2, score_data[det]);

            // --- 3. Keypoints 디코딩 ---
            for (int kp = 0; kp < 5; ++kp) {
                // kps_data는 중심점에서의 (x, y) 오프셋을 의미
                float kp_offset_x = kps_data[det * 10 + kp * 2];
                float kp_offset_y = kps_data[det * 10 + kp * 2 + 1];

                // 중심점과 오프셋을 이용해 실제 키포인트 좌표 계산
                face.keypoints[kp].first = cx + (kp_offset_x * stride);
                face.keypoints[kp].second = cy + (kp_offset_y * stride);
            }
            
            faces.push_back(face);
        }
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
    scaled_face.keypoints = face.keypoints;
    
    // Scale keypoints
    for (auto& keypoint : scaled_face.keypoints) {
        keypoint.first = (keypoint.first - w_pad) / r;
        keypoint.second = (keypoint.second - h_pad) / r;
    }
    
    return scaled_face;
}

// ============================================================================
// Main Post-Processing Function
// ============================================================================

/**
 * @brief Main post-processing function for SCRFD
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
    SCRFDConfig config;
    
    // ============================================================================
    // OUTPUT PARSING
    // ============================================================================
    if (network_output.empty()) {
        g_error("No output tensors found for SCRFD\n");
        return;
    }
    
    // Parse face detections
    auto faces = parse_scrfd_outputs(network_output, config);
    
    // ============================================================================
    // NON-MAXIMUM SUPPRESSION
    // ============================================================================
    auto final_faces = nms(faces, config.nms_threshold);
    
    // ============================================================================
    // COORDINATE SCALING
    // ============================================================================
    // Get original image dimensions
    int orig_width = object_meta->_box[2] - object_meta->_box[0];
    int orig_height = object_meta->_box[3] - object_meta->_box[1];
    
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
        object_meta->_confidence = scaled_face.confidence;
        object_meta->_label = 0;  // Face class
        object_meta->_label_name = g_string_new("face");
        object_meta->_face_box[0] = scaled_face.x1 + object_meta->_box[0];
        object_meta->_face_box[1] = scaled_face.y1 + object_meta->_box[1];
        object_meta->_face_box[2] = scaled_face.x2 + object_meta->_box[0];
        object_meta->_face_box[3] = scaled_face.y2 + object_meta->_box[1];
        
        // Add keypoints as additional metadata
        object_meta->_face_landmarks.clear();
        for (int k = 0; k < 5; k++) {  // 5 facial keypoints
            float kx = scaled_face.keypoints[k].first;
            float ky = scaled_face.keypoints[k].second;
            float ks = 1.0f;  // Default confidence for keypoint
            
            // Clamp keypoint coordinates to image boundaries
            kx = std::max(0.0f, std::min(static_cast<float>(orig_width), kx));
            ky = std::max(0.0f, std::min(static_cast<float>(orig_height), ky));
            
            // Add to face landmarks
            object_meta->_face_landmarks.push_back(dxs::Point_f(kx + object_meta->_box[0], ky + object_meta->_box[1], ks));
        }
    }
}
