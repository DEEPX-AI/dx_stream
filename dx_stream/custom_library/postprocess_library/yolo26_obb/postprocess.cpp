#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glib.h>
#include <gst/gst.h>
#include <string>
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct ObbBox {
    float cx, cy, w, h, angle;
    float confidence;
    int class_id;
    std::string class_name;
};

struct ObbConfig {
    int input_width = 1024;
    int input_height = 1024;
    float conf_threshold = 0.25f;
    int num_classes = 15;
    std::vector<std::string> class_names = {
        "plane", "baseball-diamond", "bridge", "ground-track-field", "small-vehicle",
        "large-vehicle", "ship", "tennis-court", "basketball-court", "storage-tank",
        "soccer-ball-field", "roundabout", "harbor", "swimming-pool", "helicopter"
    };
};

// Compute axis-aligned bounding box from OBB
static void obb_to_aabb(float cx, float cy, float w, float h, float angle,
                        float &x1, float &y1, float &x2, float &y2) {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    float hw = w / 2.0f, hh = h / 2.0f;
    // 4 corner offsets
    float dx[4] = { -hw, hw, hw, -hw };
    float dy[4] = { -hh, -hh, hh, hh };
    x1 = x2 = cx;
    y1 = y2 = cy;
    for (int i = 0; i < 4; i++) {
        float rx = cx + dx[i] * cos_a - dy[i] * sin_a;
        float ry = cy + dx[i] * sin_a + dy[i] * cos_a;
        x1 = std::min(x1, rx); y1 = std::min(y1, ry);
        x2 = std::max(x2, rx); y2 = std::max(y2, ry);
    }
}

// USE_ORT=OFF: Parse 9 raw tensors
// cv2 (bbox): [1,4,128,128], [1,4,64,64], [1,4,32,32]
// cv4 (angle): [1,1,128,128], [1,1,64,64], [1,1,32,32]
// cv3 (class): [1,15,128,128], [1,15,64,64], [1,15,32,32]
static std::vector<ObbBox> parse_multi_output(const std::vector<dxs::DXTensor>& outputs,
                                              const ObbConfig& config) {
    std::vector<ObbBox> detections;

    std::vector<const dxs::DXTensor*> bbox_tensors, angle_tensors, class_tensors;
    for (const auto& t : outputs) {
        const auto& s = t._shape;
        if (s.size() != 4) continue;
        int ch = static_cast<int>(s[1]);
        if (ch == 4) bbox_tensors.push_back(&t);
        else if (ch == 1) angle_tensors.push_back(&t);
        else if (ch == config.num_classes) class_tensors.push_back(&t);
    }
    if (bbox_tensors.size() != 3 || angle_tensors.size() != 3 || class_tensors.size() != 3) {
        GST_ERROR("OBB parse_multi_output: unexpected tensor counts (bbox=%zu, angle=%zu, class=%zu)",
                  bbox_tensors.size(), angle_tensors.size(), class_tensors.size());
        return detections;
    }

    auto cmp = [](const dxs::DXTensor* a, const dxs::DXTensor* b) {
        return (a->_shape[2] * a->_shape[3]) > (b->_shape[2] * b->_shape[3]);
    };
    std::sort(bbox_tensors.begin(), bbox_tensors.end(), cmp);
    std::sort(angle_tensors.begin(), angle_tensors.end(), cmp);
    std::sort(class_tensors.begin(), class_tensors.end(), cmp);

    std::vector<int> strides = {8, 16, 32};
    for (size_t si = 0; si < 3; ++si) {
        int h = static_cast<int>(bbox_tensors[si]->_shape[2]);
        int w = static_cast<int>(bbox_tensors[si]->_shape[3]);
        float stride = static_cast<float>(strides[si]);
        int spatial = h * w;

        const float* bbox_data = static_cast<const float*>(bbox_tensors[si]->_data);
        const float* angle_data = static_cast<const float*>(angle_tensors[si]->_data);
        const float* class_data = static_cast<const float*>(class_tensors[si]->_data);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;

                // Best class (sigmoid)
                float max_score = 0.0f;
                int best_class = -1;
                for (int c = 0; c < config.num_classes; ++c) {
                    float prob = 1.0f / (1.0f + std::exp(-class_data[c * spatial + idx]));
                    if (prob > max_score) { max_score = prob; best_class = c; }
                }
                if (max_score < config.conf_threshold || best_class < 0) continue;

                // Bbox distances
                float left  = bbox_data[0 * spatial + idx];
                float top   = bbox_data[1 * spatial + idx];
                float right = bbox_data[2 * spatial + idx];
                float bot   = bbox_data[3 * spatial + idx];

                // Angle: raw value used directly (not sigmoid)
                // Ultralytics dist2rbox uses cos/sin on the raw angle logit
                float raw_angle = angle_data[idx];

                // dist2rbox: rotated center offset
                float cos_a = std::cos(raw_angle), sin_a = std::sin(raw_angle);
                float xf = (right - left) / 2.0f;
                float yf = (bot - top) / 2.0f;
                float cx = (xf * cos_a - yf * sin_a + x + 0.5f) * stride;
                float cy = (xf * sin_a + yf * cos_a + y + 0.5f) * stride;
                float bw = (left + right) * stride;
                float bh = (top + bot) * stride;

                // regularize_rboxes: ensure w >= h and adjust angle
                float final_angle = raw_angle;
                if (bh > bw) {
                    std::swap(bw, bh);
                    final_angle += static_cast<float>(M_PI) / 2.0f;
                }
                final_angle = std::fmod(final_angle, static_cast<float>(M_PI));
                if (final_angle < 0) final_angle += static_cast<float>(M_PI);

                std::string name = (best_class < static_cast<int>(config.class_names.size()))
                    ? config.class_names[best_class] : "unknown";
                detections.push_back({cx, cy, bw, bh, final_angle, max_score, best_class, name});
            }
        }
    }
    return detections;
}

// USE_ORT=ON: Parse single tensor [1, N, 7] = [cx, cy, w, h, score, class_id, angle]
static std::vector<ObbBox> parse_single_output(const std::vector<dxs::DXTensor>& outputs,
                                               const ObbConfig& config) {
    std::vector<ObbBox> detections;
    if (outputs.empty()) return detections;

    // Find the tensor with shape [1, N, 7]
    const float* data = nullptr;
    int num_dets = 0, vec_size = 0;
    for (const auto& t : outputs) {
        const auto& s = t._shape;
        if (s.size() == 3 && s[0] == 1 && s[2] == 7) {
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
        detections.push_back({det[0], det[1], det[2], det[3], det[6],
                              score, class_id, name});
    }
    return detections;
}

extern "C" void PostProcess(GstBuffer* buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    ObbConfig config;

    std::vector<ObbBox> all_boxes;
    if (network_output.size() == 9) {
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
        // Scale OBB center to original image coordinates
        float cx = (box.cx - w_pad) / r;
        float cy = (box.cy - h_pad) / r;
        float bw = box.w / r;
        float bh = box.h / r;

        // Compute axis-aligned bounding box for label positioning
        float x1, y1, x2, y2;
        obb_to_aabb(cx, cy, bw, bh, box.angle, x1, y1, x2, y2);
        x1 = std::max(0.0f, std::min(static_cast<float>(orig_w), x1));
        y1 = std::max(0.0f, std::min(static_cast<float>(orig_h), y1));
        x2 = std::max(0.0f, std::min(static_cast<float>(orig_w), x2));
        y2 = std::max(0.0f, std::min(static_cast<float>(orig_h), y2));

        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_confidence = box.confidence;
        obj_meta->_label = box.class_id;
        obj_meta->_label_name = box.class_name;
        obj_meta->_box = {x1, y1, x2, y2};

        // Store OBB parameters [cx, cy, w, h, angle]
        obj_meta->_obb = {cx, cy, bw, bh, box.angle};

        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            float roi_x = static_cast<float>(frame_meta->_roi[0]);
            float roi_y = static_cast<float>(frame_meta->_roi[1]);
            obj_meta->_box[0] += roi_x;
            obj_meta->_box[1] += roi_y;
            obj_meta->_box[2] += roi_x;
            obj_meta->_box[3] += roi_y;
            obj_meta->_obb[0] += roi_x;
            obj_meta->_obb[1] += roi_y;
        }

        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}
