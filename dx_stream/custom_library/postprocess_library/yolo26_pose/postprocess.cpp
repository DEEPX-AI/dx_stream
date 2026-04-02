#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
#include <glib.h>
#include <gst/gst.h>
#include <string>
#include <tuple>

static constexpr int NUM_KEYPOINTS = 17;
static constexpr int NUM_CLASSES = 1; // person only

struct PoseBox {
    float x1, y1, x2, y2;
    float confidence;
    std::vector<float> keypoints; // 17*3 (x, y, vis)
};

struct PoseConfig {
    int input_width = 640;
    int input_height = 640;
    float conf_threshold = 0.25f;
};

// USE_ORT=OFF: Parse 9 raw tensors
// cv2 (bbox): [1,4,80,80], [1,4,40,40], [1,4,20,20]
// cv4_kpts (keypoints): [1,51,80,80], [1,51,40,40], [1,51,20,20]
// cv3 (class): [1,1,80,80], [1,1,40,40], [1,1,20,20]
static std::vector<PoseBox> parse_multi_output(const std::vector<dxs::DXTensor>& outputs,
                                               const PoseConfig& config) {
    std::vector<PoseBox> detections;

    std::vector<dxs::DXTensor> bbox_tensors, kpt_tensors, cls_tensors;
    for (const auto& t : outputs) {
        const auto& s = t._shape;
        if (s.size() != 4) continue;
        int ch = static_cast<int>(s[1]);
        if (ch == 4) bbox_tensors.push_back(t);
        else if (ch == NUM_KEYPOINTS * 3) kpt_tensors.push_back(t); // 51
        else if (ch == NUM_CLASSES) cls_tensors.push_back(t);       // 1
    }
    if (bbox_tensors.size() != 3 || kpt_tensors.size() != 3 || cls_tensors.size() != 3) {
        GST_ERROR("Pose parse_multi_output: unexpected tensor counts (bbox=%zu, kpt=%zu, cls=%zu)",
                  bbox_tensors.size(), kpt_tensors.size(), cls_tensors.size());
        return detections;
    }

    auto cmp = [](const dxs::DXTensor& a, const dxs::DXTensor& b) {
        return (a._shape[2] * a._shape[3]) > (b._shape[2] * b._shape[3]);
    };
    std::sort(bbox_tensors.begin(), bbox_tensors.end(), cmp);
    std::sort(kpt_tensors.begin(), kpt_tensors.end(), cmp);
    std::sort(cls_tensors.begin(), cls_tensors.end(), cmp);

    std::vector<int> strides = {8, 16, 32};
    for (size_t si = 0; si < 3; ++si) {
        int h = static_cast<int>(bbox_tensors[si]._shape[2]);
        int w = static_cast<int>(bbox_tensors[si]._shape[3]);
        float stride = static_cast<float>(strides[si]);
        int spatial = h * w;

        const float* bbox_data = static_cast<const float*>(bbox_tensors[si]._data);
        const float* kpt_data  = static_cast<const float*>(kpt_tensors[si]._data);
        const float* cls_data  = static_cast<const float*>(cls_tensors[si]._data);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;

                // Single class (person): sigmoid of confidence
                float score = 1.0f / (1.0f + std::exp(-cls_data[idx]));
                if (score < config.conf_threshold) continue;

                // Bbox decoding
                float left  = bbox_data[0 * spatial + idx];
                float top   = bbox_data[1 * spatial + idx];
                float right = bbox_data[2 * spatial + idx];
                float bot   = bbox_data[3 * spatial + idx];

                float x1 = ((x + 0.5f) - left) * stride;
                float y1 = ((y + 0.5f) - top) * stride;
                float x2 = ((x + 0.5f) + right) * stride;
                float y2 = ((y + 0.5f) + bot) * stride;

                // Keypoint decoding: offset from anchor center
                std::vector<float> kpts(NUM_KEYPOINTS * 3);
                for (int k = 0; k < NUM_KEYPOINTS; ++k) {
                    float raw_kx = kpt_data[(k * 3 + 0) * spatial + idx];
                    float raw_ky = kpt_data[(k * 3 + 1) * spatial + idx];
                    float raw_kv = kpt_data[(k * 3 + 2) * spatial + idx];
                    kpts[k * 3 + 0] = (raw_kx + x + 0.5f) * stride;
                    kpts[k * 3 + 1] = (raw_ky + y + 0.5f) * stride;
                    kpts[k * 3 + 2] = 1.0f / (1.0f + std::exp(-raw_kv)); // sigmoid visibility
                }

                detections.push_back({x1, y1, x2, y2, score, std::move(kpts)});
            }
        }
    }
    return detections;
}

// USE_ORT=ON: Parse single tensor [1, N, 57]
// Layout: [x1, y1, x2, y2, score, class_id, 17*3 keypoints]
static std::vector<PoseBox> parse_single_output(const std::vector<dxs::DXTensor>& outputs,
                                                const PoseConfig& config) {
    std::vector<PoseBox> detections;
    const float* data = nullptr;
    int num_dets = 0, vec_size = 0;
    for (const auto& t : outputs) {
        const auto& s = t._shape;
        if (s.size() == 3 && s[0] == 1 && s[2] >= 57) {
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

        PoseBox pb;
        pb.x1 = det[0]; pb.y1 = det[1]; pb.x2 = det[2]; pb.y2 = det[3];
        pb.confidence = score;
        pb.keypoints.assign(det + 6, det + 6 + NUM_KEYPOINTS * 3);
        detections.push_back(std::move(pb));
    }
    return detections;
}

extern "C" void PostProcess(GstBuffer* buf,
                            std::vector<dxs::DXTensor> network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    PoseConfig config;

    std::vector<PoseBox> all_boxes;
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
        obj_meta->_label = 0; // person
        obj_meta->_label_name = "person";
        obj_meta->_box = {x1, y1, x2, y2};

        // Scale keypoints to original image coordinates
        std::vector<float> kpts(NUM_KEYPOINTS * 3);
        for (int k = 0; k < NUM_KEYPOINTS; ++k) {
            kpts[k * 3 + 0] = (box.keypoints[k * 3 + 0] - w_pad) / r;
            kpts[k * 3 + 1] = (box.keypoints[k * 3 + 1] - h_pad) / r;
            kpts[k * 3 + 2] = box.keypoints[k * 3 + 2]; // visibility
        }
        obj_meta->_keypoints = std::move(kpts);

        if (frame_meta->_roi[0] != -1 && frame_meta->_roi[1] != -1 &&
            frame_meta->_roi[2] != -1 && frame_meta->_roi[3] != -1) {
            float roi_x = static_cast<float>(frame_meta->_roi[0]);
            float roi_y = static_cast<float>(frame_meta->_roi[1]);
            obj_meta->_box[0] += roi_x;
            obj_meta->_box[1] += roi_y;
            obj_meta->_box[2] += roi_x;
            obj_meta->_box[3] += roi_y;
            for (int k = 0; k < NUM_KEYPOINTS; ++k) {
                obj_meta->_keypoints[k * 3 + 0] += roi_x;
                obj_meta->_keypoints[k * 3 + 1] += roi_y;
            }
        }

        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
}
