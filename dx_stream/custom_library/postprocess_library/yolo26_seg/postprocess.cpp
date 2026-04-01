#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include <dxrt/dxrt_api.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <glib.h>
#include <gst/gst.h>

static constexpr int NUM_CLASSES = 80;
static constexpr int NUM_MASK_COEFS = 32;
static constexpr unsigned char SEG_MASK_FG = 255;

struct SegBox {
    float x1, y1, x2, y2;
    float confidence;
    int class_id;
    std::string class_name;
    std::vector<float> mask_coefs; // 32 coefficients
};

struct SegConfig {
    int input_width = 640;
    int input_height = 640;
    float conf_threshold = 0.25f;
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

// Generate an ROI-local binary instance mask.
// The mask is stored in object-local coordinates, and obj_meta->_box is used
// later to place it back onto the frame during OSD.
static void generate_instance_mask(const SegBox& det, const float* proto_data,
                                   int proto_h, int proto_w, int input_w, int input_h,
                                   float r, float w_pad, float h_pad,
                                   int roi_x1, int roi_y1, int roi_x2, int roi_y2,
                                   std::vector<unsigned char>& seg_data_out,
                                   int& seg_w_out, int& seg_h_out) {
    int mask_area = proto_h * proto_w;
    float scale_w = static_cast<float>(proto_w) / input_w;
    float scale_h = static_cast<float>(proto_h) / input_h;

    // ROI in input image (from raw detection box, still in input space)
    int bx1 = std::max(0, static_cast<int>(det.x1));
    int by1 = std::max(0, static_cast<int>(det.y1));
    int bx2 = std::min(input_w, static_cast<int>(det.x2));
    int by2 = std::min(input_h, static_cast<int>(det.y2));
    if (bx1 >= bx2 || by1 >= by2) return;

    // ROI in prototype space
    int mx1 = std::max(0, static_cast<int>(std::floor(bx1 * scale_w)));
    int my1 = std::max(0, static_cast<int>(std::floor(by1 * scale_h)));
    int mx2 = std::min(proto_w, static_cast<int>(std::ceil(bx2 * scale_w)));
    int my2 = std::min(proto_h, static_cast<int>(std::ceil(by2 * scale_h)));
    int roi_w = mx2 - mx1, roi_h = my2 - my1;
    if (roi_w <= 0 || roi_h <= 0) return;

    // Compute mask in ROI
    std::vector<float> roi_mask(roi_w * roi_h, 0.0f);
    for (int c = 0; c < NUM_MASK_COEFS; ++c) {
        float coef = det.mask_coefs[c];
        const float* proto_plane = proto_data + c * mask_area;
        for (int h = 0; h < roi_h; ++h) {
            const float* row = proto_plane + (my1 + h) * proto_w;
            float* out_row = roi_mask.data() + h * roi_w;
            for (int w = 0; w < roi_w; ++w)
                out_row[w] += coef * row[mx1 + w];
        }
    }
    // Sigmoid
    for (float& v : roi_mask)
        v = 1.0f / (1.0f + std::exp(-v));

    if (roi_x1 >= roi_x2 || roi_y1 >= roi_y2) return;

    seg_w_out = roi_x2 - roi_x1;
    seg_h_out = roi_y2 - roi_y1;
    seg_data_out.assign(static_cast<size_t>(seg_w_out) * seg_h_out, 0);

    for (int out_y = 0; out_y < seg_h_out; ++out_y) {
        int frame_y = roi_y1 + out_y;
        float inp_y = frame_y * r + h_pad;
        float src_y = inp_y * scale_h - my1;
        int y0 = std::max(0, std::min(static_cast<int>(src_y), roi_h - 1));
        int y1 = std::min(y0 + 1, roi_h - 1);
        float dy = src_y - y0;
        for (int out_x = 0; out_x < seg_w_out; ++out_x) {
            int frame_x = roi_x1 + out_x;
            float inp_x = frame_x * r + w_pad;
            float src_x = inp_x * scale_w - mx1;
            int x0 = std::max(0, std::min(static_cast<int>(src_x), roi_w - 1));
            int x1 = std::min(x0 + 1, roi_w - 1);
            float dx = src_x - x0;
            float val = roi_mask[y0 * roi_w + x0] * (1 - dx) * (1 - dy)
                      + roi_mask[y0 * roi_w + x1] * dx * (1 - dy)
                      + roi_mask[y1 * roi_w + x0] * (1 - dx) * dy
                      + roi_mask[y1 * roi_w + x1] * dx * dy;
            if (val > 0.5f)
                seg_data_out[out_y * seg_w_out + out_x] = SEG_MASK_FG;
        }
    }
}

// USE_ORT=OFF: Parse 10 raw tensors
// cv2 (bbox): [1,4,80,80], [1,4,40,40], [1,4,20,20]
// cv4 (mask coef): [1,32,80,80], [1,32,40,40], [1,32,20,20]
// cv3 (class): [1,80,80,80], [1,80,40,40], [1,80,20,20]
// output1 (proto): [1,32,160,160]
static std::vector<SegBox> parse_multi_output(const dxrt::TensorPtrs& outputs,
                                              const SegConfig& config) {
    std::vector<SegBox> detections;

    std::vector<std::shared_ptr<dxrt::Tensor>> bbox_tensors, coef_tensors, class_tensors;
    std::shared_ptr<dxrt::Tensor> proto_tensor;

    for (const auto& t : outputs) {
        const auto& s = t->shape();
        if (s.size() == 4 && s[0] == 1) {
            int ch = static_cast<int>(s[1]);
            int h = static_cast<int>(s[2]), w = static_cast<int>(s[3]);
            if (ch == 4) bbox_tensors.push_back(t);
            else if (ch == NUM_MASK_COEFS && h == w && h >= 160) proto_tensor = t; // 160x160
            else if (ch == NUM_MASK_COEFS) coef_tensors.push_back(t);
            else if (ch == NUM_CLASSES) class_tensors.push_back(t);
        }
    }

    if (bbox_tensors.size() != 3 || coef_tensors.size() != 3 || class_tensors.size() != 3 || !proto_tensor) {
        GST_ERROR("Seg parse_multi_output: unexpected tensor counts");
        return detections;
    }

    auto cmp = [](const std::shared_ptr<dxrt::Tensor>& a, const std::shared_ptr<dxrt::Tensor>& b) {
        return (a->shape()[2] * a->shape()[3]) > (b->shape()[2] * b->shape()[3]);
    };
    std::sort(bbox_tensors.begin(), bbox_tensors.end(), cmp);
    std::sort(coef_tensors.begin(), coef_tensors.end(), cmp);
    std::sort(class_tensors.begin(), class_tensors.end(), cmp);

    std::vector<int> strides = {8, 16, 32};
    for (size_t si = 0; si < 3; ++si) {
        int h = static_cast<int>(bbox_tensors[si]->shape()[2]);
        int w = static_cast<int>(bbox_tensors[si]->shape()[3]);
        float stride = static_cast<float>(strides[si]);
        int spatial = h * w;

        const float* bbox_data = static_cast<const float*>(bbox_tensors[si]->data());
        const float* coef_data = static_cast<const float*>(coef_tensors[si]->data());
        const float* cls_data  = static_cast<const float*>(class_tensors[si]->data());

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = y * w + x;

                // Best class
                float max_score = 0.0f;
                int best_class = -1;
                for (int c = 0; c < NUM_CLASSES; ++c) {
                    float prob = 1.0f / (1.0f + std::exp(-cls_data[c * spatial + idx]));
                    if (prob > max_score) { max_score = prob; best_class = c; }
                }
                if (max_score < config.conf_threshold || best_class < 0) continue;

                float left  = bbox_data[0 * spatial + idx];
                float top   = bbox_data[1 * spatial + idx];
                float right = bbox_data[2 * spatial + idx];
                float bot   = bbox_data[3 * spatial + idx];

                float bx1 = ((x + 0.5f) - left) * stride;
                float by1 = ((y + 0.5f) - top) * stride;
                float bx2 = ((x + 0.5f) + right) * stride;
                float by2 = ((y + 0.5f) + bot) * stride;

                // Extract 32 mask coefficients
                std::vector<float> coefs(NUM_MASK_COEFS);
                for (int c = 0; c < NUM_MASK_COEFS; ++c)
                    coefs[c] = coef_data[c * spatial + idx];

                std::string name = (best_class < static_cast<int>(config.class_names.size()))
                    ? config.class_names[best_class] : "unknown";
                detections.push_back({bx1, by1, bx2, by2, max_score, best_class, name,
                                      std::move(coefs)});
            }
        }
    }
    return detections;
}

// USE_ORT=ON: Parse output0 [1, 300, 38] + output1 [1, 32, 160, 160]
static std::vector<SegBox> parse_single_output(const dxrt::TensorPtrs& outputs,
                                               const SegConfig& config) {
    std::vector<SegBox> detections;

    // Find detection tensor [1, N, 38] and proto tensor [1, 32, H, W]
    const float* det_data = nullptr;
    int num_dets = 0, vec_size = 0;
    for (const auto& t : outputs) {
        const auto& s = t->shape();
        if (s.size() == 3 && s[0] == 1 && s[2] >= 38) {
            det_data = static_cast<const float*>(t->data());
            num_dets = static_cast<int>(s[1]);
            vec_size = static_cast<int>(s[2]);
            break;
        }
    }
    if (!det_data) return detections;

    for (int i = 0; i < num_dets; ++i) {
        const float* det = det_data + i * vec_size;
        float score = det[4];
        if (score < config.conf_threshold) continue;
        int class_id = static_cast<int>(det[5]);
        if (class_id < 0 || class_id >= NUM_CLASSES) continue;

        SegBox sb;
        sb.x1 = det[0]; sb.y1 = det[1]; sb.x2 = det[2]; sb.y2 = det[3];
        sb.confidence = score;
        sb.class_id = class_id;
        sb.class_name = (class_id < static_cast<int>(config.class_names.size()))
            ? config.class_names[class_id] : "unknown";
        sb.mask_coefs.assign(det + 6, det + 6 + NUM_MASK_COEFS);
        detections.push_back(std::move(sb));
    }
    return detections;
}

// Find prototype tensor [1, 32, H, W]
static std::pair<const float*, std::pair<int,int>> find_proto(const dxrt::TensorPtrs& outputs) {
    for (const auto& t : outputs) {
        const auto& s = t->shape();
        if (s.size() == 4 && s[0] == 1 && s[1] == NUM_MASK_COEFS) {
            int h = static_cast<int>(s[2]), w = static_cast<int>(s[3]);
            if (h >= 128) // prototype is the large spatial tensor
                return {static_cast<const float*>(t->data()), {h, w}};
        }
    }
    return {nullptr, {0, 0}};
}

extern "C" void PostProcess(GstBuffer* buf,
                            const dxrt::TensorPtrs& network_output,
                            DXFrameMeta* frame_meta,
                            DXObjectMeta* object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;
    SegConfig config;

    std::vector<SegBox> all_boxes;
    if (network_output.size() >= 10) {
        all_boxes = parse_multi_output(network_output, config);
    } else {
        all_boxes = parse_single_output(network_output, config);
    }

    // Find prototype tensor
    auto [proto_data, proto_dim] = find_proto(network_output);
    int proto_h = proto_dim.first, proto_w = proto_dim.second;

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

        int mask_x1 = std::max(0, std::min(orig_w, static_cast<int>(std::floor(x1))));
        int mask_y1 = std::max(0, std::min(orig_h, static_cast<int>(std::floor(y1))));
        int mask_x2 = std::max(mask_x1, std::min(orig_w, static_cast<int>(std::ceil(x2))));
        int mask_y2 = std::max(mask_y1, std::min(orig_h, static_cast<int>(std::ceil(y2))));

        // Generate an ROI-local binary mask and place it using obj_meta->_box.
        if (proto_data && !box.mask_coefs.empty() && mask_x2 > mask_x1 && mask_y2 > mask_y1) {
            generate_instance_mask(box, proto_data, proto_h, proto_w,
                                   config.input_width, config.input_height,
                                   r, w_pad, h_pad, mask_x1, mask_y1, mask_x2, mask_y2,
                                   obj_meta->_seg_data, obj_meta->_seg_width, obj_meta->_seg_height);
        }

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
