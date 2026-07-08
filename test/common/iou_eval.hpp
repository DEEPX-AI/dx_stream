// IoU computation and detection evaluation helpers
#pragma once
#include <algorithm>
#include <map>
#include <vector>

namespace dxtest {

using Box = std::vector<float>;
using DetectionMap = std::map<int, std::vector<Box>>;

inline float compute_iou(const Box &a, const Box &b) {
    if (a.size() < 4 || b.size() < 4) return 0.0f;
    float x1 = std::max(a[0], b[0]);
    float y1 = std::max(a[1], b[1]);
    float x2 = std::min(a[2], b[2]);
    float y2 = std::min(a[3], b[3]);
    float inter = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float area_a = (a[2] - a[0]) * (a[3] - a[1]);
    float area_b = (b[2] - b[0]) * (b[3] - b[1]);
    float uni = area_a + area_b - inter;
    return (uni > 0.0f) ? inter / uni : 0.0f;
}

struct EvalResult {
    int tp;
    int fp;
    int fn;
    float precision;
    float recall;
};

inline EvalResult evaluate_detections(const DetectionMap &gt,
                                      const DetectionMap &pred,
                                      float iou_threshold = 0.5f) {
    int tp = 0, fp = 0, fn = 0;

    for (auto &kv : gt) {
        int class_id = kv.first;
        const auto &gt_boxes = kv.second;
        std::vector<bool> matched(gt_boxes.size(), false);

        auto it = pred.find(class_id);
        if (it == pred.end()) {
            fn += (int)gt_boxes.size();
            continue;
        }

        const auto &pred_boxes = it->second;
        for (auto &pb : pred_boxes) {
            float best_iou = 0.0f;
            int best_idx = -1;
            for (int i = 0; i < (int)gt_boxes.size(); i++) {
                if (matched[i]) continue;
                float iou = compute_iou(pb, gt_boxes[i]);
                if (iou > best_iou) {
                    best_iou = iou;
                    best_idx = i;
                }
            }
            if (best_idx >= 0 && best_iou >= iou_threshold) {
                matched[best_idx] = true;
                tp++;
            } else {
                fp++;
            }
        }
        for (bool m : matched) {
            if (!m) fn++;
        }
    }

    for (auto &kv : pred) {
        if (gt.find(kv.first) == gt.end())
            fp += (int)kv.second.size();
    }

    float precision = (tp + fp > 0) ? (float)tp / (float)(tp + fp) : 0.0f;
    float recall = (tp + fn > 0) ? (float)tp / (float)(tp + fn) : 0.0f;
    return {tp, fp, fn, precision, recall};
}

inline bool is_box_inside_roi(const Box &box, const Box &roi) {
    if (box.size() < 4 || roi.size() < 4) return false;
    return box[0] >= roi[0] && box[1] >= roi[1] &&
           box[2] <= roi[2] && box[3] <= roi[3];
}

inline bool all_detections_inside_roi(const DetectionMap &dets, const Box &roi) {
    for (auto &kv : dets) {
        for (auto &b : kv.second) {
            if (b.size() != 4 || !is_box_inside_roi(b, roi))
                return false;
        }
    }
    return true;
}

// GT for test.jpg (COCO format: x1,y1,x2,y2)
inline DetectionMap test_jpg_gt() {
    DetectionMap gt;
    gt[0].push_back({124.547783f, 62.005188f, 246.898804f, 329.931824f});
    gt[0].push_back({269.850861f, 73.818756f, 367.612885f, 332.555573f});
    gt[0].push_back({395.051575f, 100.663300f, 484.869934f, 332.248627f});
    gt[1].push_back({392.320831f, 81.118011f, 623.422607f, 279.040558f});
    gt[13].push_back({79.094894f, 151.225708f, 527.124817f, 324.877441f});
    gt[16].push_back({408.001617f, 165.888428f, 451.949982f, 227.317139f});
    gt[24].push_back({94.010223f, 89.505127f, 186.151764f, 230.614990f});
    gt[39].push_back({255.098633f, 199.996796f, 267.930298f, 243.062042f});
    gt[67].push_back({313.397675f, 141.667389f, 329.099213f, 150.474091f});
    return gt;
}

// GT for secondary face detection on test.jpg
inline Box test_jpg_face_gt_box() {
    return {127.633537f, 62.656158f, 246.538330f, 330.089935f};
}

inline Box test_jpg_face_gt_face() {
    return {198.318054f, 79.526176f, 225.280426f, 120.265518f};
}

}  // namespace dxtest
