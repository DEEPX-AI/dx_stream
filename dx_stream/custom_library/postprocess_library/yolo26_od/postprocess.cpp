#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glib.h>
#include <gst/gst.h>
#include <string>
#include <tuple>

struct BoundingBox {
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
    std::string class_name;
};

struct YoloConfig {
    int input_width = 640;
    int input_height = 640;
    float conf_threshold = 0.25f;
    int num_classes = 80;
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

// USE_ORT=OFF: Parse 6 raw tensors
// cv2 (bbox): [1,4,80,80], [1,4,40,40], [1,4,20,20]
// cv3 (class): [1,80,80,80], [1,80,40,40], [1,80,20,20]
static std::vector<BoundingBox> parse_multi_output(const std::vector<dxs::DXTensor>& outputs,
                                            const YoloConfig& config) {
    std::vector<BoundingBox> detections;

    if (outputs.size() != 6) {
        GST_ERROR("parse_multi_output: Expected 6 tensors, got %zu", outputs.size());
        return detections;
    }

    std::vector<dxs::DXTensor> bbox_tensors, class_tensors;
    for (const auto& tensor : outputs) {
        const auto& shape = tensor._shape;
        if (shape.size() == 4 && shape[1] == 4)
            bbox_tensors.push_back(tensor);
        else if (shape.size() == 4 && static_cast<int>(shape[1]) == config.num_classes)
            class_tensors.push_back(tensor);
    }

    if (bbox_tensors.size() != 3 || class_tensors.size() != 3) {
        GST_ERROR("parse_multi_output: Invalid tensor configuration (bbox=%zu, class=%zu)",
                  bbox_tensors.size(), class_tensors.size());
        return detections;
    }
    
    // Sort by spatial size descending (80x80, 40x40, 20x20)
    auto size_comparator = [](const dxs::DXTensor& a, const dxs::DXTensor& b) {
        return (a._shape[2] * a._shape[3]) > (b._shape[2] * b._shape[3]);
    };
    std::sort(bbox_tensors.begin(), bbox_tensors.end(), size_comparator);
    std::sort(class_tensors.begin(), class_tensors.end(), size_comparator);

    std::vector<int> strides = {8, 16, 32};
    for (size_t scale_idx = 0; scale_idx < 3; ++scale_idx) {
        int height = static_cast<int>(bbox_tensors[scale_idx]._shape[2]);
        int width = static_cast<int>(bbox_tensors[scale_idx]._shape[3]);
        float stride = static_cast<float>(strides[scale_idx]);
        int spatial_size = height * width;

        const float* bbox_data = static_cast<const float*>(bbox_tensors[scale_idx]._data);
        const float* class_data = static_cast<const float*>(class_tensors[scale_idx]._data);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;

                // Find best class (sigmoid)
                float max_score = 0.0f;
                int best_class = -1;
                for (int c = 0; c < config.num_classes; ++c) {
                    float prob = 1.0f / (1.0f + std::exp(-class_data[c * spatial_size + idx]));
                    if (prob > max_score) { max_score = prob; best_class = c; }
                }
                if (max_score < config.conf_threshold || best_class < 0) continue;

                float left  = bbox_data[0 * spatial_size + idx];
                float top   = bbox_data[1 * spatial_size + idx];
                float right = bbox_data[2 * spatial_size + idx];
                float bot   = bbox_data[3 * spatial_size + idx];

                float x1 = ((x + 0.5f) - left) * stride;
                float y1 = ((y + 0.5f) - top) * stride;
                float x2 = ((x + 0.5f) + right) * stride;
                float y2 = ((y + 0.5f) + bot) * stride;

                std::string name = (best_class < static_cast<int>(config.class_names.size()))
                    ? config.class_names[best_class] : "unknown";
                detections.push_back({x1, y1, x2, y2, max_score, best_class, std::move(name)});
            }
        }
    }
    
    return detections;
}

// USE_ORT=ON: Parse single tensor [1, N, 6]
static std::vector<BoundingBox> parse_single_output(const std::vector<dxs::DXTensor>& outputs,
                                                    const YoloConfig& config) {
    std::vector<BoundingBox> detections;
    const float* data = nullptr;
    int num_dets = 0, vec_size = 0;

    for (const auto& t : outputs) {
        const auto& s = t._shape;
        if (s.size() == 3 && s[0] == 1 && s[2] == 6) {
            data = static_cast<const float*>(t._data);
            num_dets = static_cast<int>(s[1]);
            vec_size = static_cast<int>(s[2]);
            break;
        }
    }
    if (!data) return detections;

    for (int i = 0; i < num_dets; ++i) {
        const float* det = data + i * vec_size;
        float score = det[4];
        if (score < config.conf_threshold) continue;
        int class_id = static_cast<int>(det[5]);
        if (class_id < 0 || class_id >= config.num_classes) continue;

        std::string name = (class_id < static_cast<int>(config.class_names.size()))
            ? config.class_names[class_id] : "unknown";
        detections.push_back({det[0], det[1], det[2], det[3], score, class_id, std::move(name)});
    }
    return detections;
}

DX_CUSTOM_EXPORT void PostProcess(GstBuffer* buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    YoloConfig config;

    std::vector<BoundingBox> all_boxes;
    if (network_output.size() == 6) {
        all_boxes = parse_multi_output(network_output, config);
    } else {
        all_boxes = parse_single_output(network_output, config);
    }

    int orig_w = frame_meta->_width;
    int orig_h = frame_meta->_height;
    if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
        frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
        orig_w = frame_meta->_roi[2] - frame_meta->_roi[0];
        orig_h = frame_meta->_roi[3] - frame_meta->_roi[1];
    }

    float r = std::min(static_cast<float>(config.input_width) / orig_w,
                       static_cast<float>(config.input_height) / orig_h);
    float w_pad = (config.input_width - orig_w * r) / 2.0f;
    float h_pad = (config.input_height - orig_h * r) / 2.0f;

    for (const auto& box : all_boxes) {
        float x1 = (box.x1 - w_pad) / r;
        float y1 = (box.y1 - h_pad) / r;
        float x2 = (box.x2 - w_pad) / r;
        float y2 = (box.y2 - h_pad) / r;
        x1 = std::max(0.0f, std::min(static_cast<float>(orig_w), x1));
        y1 = std::max(0.0f, std::min(static_cast<float>(orig_h), y1));
        x2 = std::max(0.0f, std::min(static_cast<float>(orig_w), x2));
        y2 = std::max(0.0f, std::min(static_cast<float>(orig_h), y2));

        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = box.confidence;
        obj_meta->_label = box.class_id;
        obj_meta->_label_name = box.class_name;
        obj_meta->_box = {x1, y1, x2, y2};

        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            obj_meta->_box[0] += frame_meta->_roi[0];
            obj_meta->_box[1] += frame_meta->_roi[1];
            obj_meta->_box[2] += frame_meta->_roi[0];
            obj_meta->_box[3] += frame_meta->_roi[1];
        }

        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}
