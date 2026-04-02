#include "dxosd_common.hpp"

#include <cmath>

// Internal helper: draw segmentation from raw data buffer
// Resizes the class-index map with INTER_NEAREST first, then colorizes at target resolution
// to avoid mosaic artifacts from interpolating color values.
static void draw_seg_bgr(cv::Mat &img, const unsigned char* data, int seg_w, int seg_h, bool skip_bg) {
    // Resize the class-index map to target resolution with nearest-neighbor
    cv::Mat seg_map(seg_h, seg_w, CV_8UC1, const_cast<unsigned char*>(data));
    cv::Mat resized_map;
    cv::resize(seg_map, resized_map, img.size(), 0, 0, cv::INTER_NEAREST);

    // Colorize at target resolution
    int total_pixels = img.rows * img.cols;
    cv::Mat seg_vis(img.rows, img.cols, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::Mat seg_mask(img.rows, img.cols, CV_8UC1, cv::Scalar(0));
    uchar* vis_ptr = seg_vis.data;
    uchar* map_ptr = resized_map.data;
    uchar* mask_ptr = seg_mask.data;
    size_t color_count = COLORS.size();
    for (int i = 0; i < total_pixels; ++i) {
        int cls = map_ptr[i];
        if (skip_bg && cls == 255) continue;
        const cv::Scalar& color = COLORS[cls % color_count];
        vis_ptr[i * 3 + 0] = static_cast<uchar>(color[0]);
        vis_ptr[i * 3 + 1] = static_cast<uchar>(color[1]);
        vis_ptr[i * 3 + 2] = static_cast<uchar>(color[2]);
        mask_ptr[i] = 255;
    }
    cv::Mat blend;
    cv::addWeighted(img, 0.6, seg_vis, 0.4, 0.0, blend);
    blend.copyTo(img, seg_mask);
}

void draw_semantic_segmentation(cv::Mat &img, const DXFrameMeta *meta) {
    if (meta->_seg_data.empty()) return;
    draw_seg_bgr(img, meta->_seg_data.data(), meta->_seg_width, meta->_seg_height, false);
}

static cv::Scalar get_instance_color_bgr(const DXObjectMeta *meta) {
    int label = (meta->_label >= 0) ? meta->_label : 0;
    return COLORS[label % COLORS.size()];
}

static bool get_instance_mask_roi(const DXObjectMeta *meta,
                                  int width, int height,
                                  float sx, float sy,
                                  cv::Rect &roi) {
    if (meta->_seg_data.empty() || meta->_seg_width <= 0 || meta->_seg_height <= 0)
        return false;
    if (meta->_box[2] <= meta->_box[0] || meta->_box[3] <= meta->_box[1])
        return false;

    int x1 = std::max(0, std::min(width,
        static_cast<int>(std::floor(meta->_box[0] / sx))));
    int y1 = std::max(0, std::min(height,
        static_cast<int>(std::floor(meta->_box[1] / sy))));
    int x2 = std::max(0, std::min(width,
        static_cast<int>(std::ceil(meta->_box[2] / sx))));
    int y2 = std::max(0, std::min(height,
        static_cast<int>(std::ceil(meta->_box[3] / sy))));

    if (x2 <= x1 || y2 <= y1)
        return false;

    roi = cv::Rect(x1, y1, x2 - x1, y2 - y1);
    return true;
}

static cv::Rect get_uv_roi(const cv::Rect &roi, int width, int height) {
    int uv_width = width / 2;
    int uv_height = height / 2;
    int x1 = std::max(0, std::min(uv_width, roi.x / 2));
    int y1 = std::max(0, std::min(uv_height, roi.y / 2));
    int x2 = std::max(0, std::min(uv_width, (roi.x + roi.width + 1) / 2));
    int y2 = std::max(0, std::min(uv_height, (roi.y + roi.height + 1) / 2));
    return cv::Rect(x1, y1, std::max(0, x2 - x1), std::max(0, y2 - y1));
}

static cv::Mat resize_instance_mask(const DXObjectMeta *meta, const cv::Size &size) {
    cv::Mat mask(meta->_seg_height, meta->_seg_width, CV_8UC1,
                 const_cast<unsigned char *>(meta->_seg_data.data()));
    cv::Mat resized_mask;
    cv::resize(mask, resized_mask, size, 0, 0, cv::INTER_NEAREST);
    return resized_mask;
}

void draw_instance_segmentation(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    cv::Rect roi;
    if (!get_instance_mask_roi(meta, img.cols, img.rows, sx, sy, roi))
        return;

    cv::Mat mask = resize_instance_mask(meta, roi.size());
    cv::Mat img_roi = img(roi);
    cv::Mat seg_vis(roi.height, roi.width, CV_8UC3, get_instance_color_bgr(meta));
    cv::Mat blend;
    cv::addWeighted(img_roi, 0.6, seg_vis, 0.4, 0.0, blend);
    blend.copyTo(img_roi, mask);
}

void draw_keypoints(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_keypoints.empty())
        return;
    std::vector<cv::Point> pts;
    for (int i = 0; i < 17; ++i) {
        float x = meta->_keypoints[i * 3] / sx;
        float y = meta->_keypoints[i * 3 + 1] / sy;
        float s = meta->_keypoints[i * 3 + 2];
        pts.emplace_back((s > 0.5f) ? cv::Point(int(x), int(y)) : cv::Point(-1, -1));
    }
    for (size_t i = 0; i < skeleton.size(); ++i) {
        auto &p = skeleton[i];
        if (pts[p[0]].x >= 0 && pts[p[1]].x >= 0)
            cv::line(img, pts[p[0]], pts[p[1]], pose_limb_color[i], 2, cv::LINE_AA);
    }
    for (size_t i = 0; i < pts.size(); ++i)
        cv::circle(img, pts[i], 3, pose_kpt_color[i], -1, cv::LINE_AA);
}

void draw_obb(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_obb.size() != 5) return;
    float cx = meta->_obb[0] / sx;
    float cy = meta->_obb[1] / sy;
    float w = meta->_obb[2] / sx;
    float h = meta->_obb[3] / sy;
    float angle_rad = meta->_obb[4];
    cv::RotatedRect rrect(cv::Point2f(cx, cy), cv::Size2f(w, h),
                          angle_rad * 180.0f / static_cast<float>(CV_PI));
    cv::Point2f pts[4];
    rrect.points(pts);
    int id = meta->_track_id;
    int label = meta->_label;
    int color_idx = (id != -1) ? id : (label >= 0 ? label : 0);
    cv::Scalar color = COLORS[color_idx % COLORS.size()];
    for (int i = 0; i < 4; i++)
        cv::line(img, pts[i], pts[(i + 1) % 4], color, 2, cv::LINE_AA);

    // Draw label text near the top of the OBB
    std::string text;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s=%.2f", meta->_label_name.c_str(), meta->_confidence);
    }
    if (!text.empty()) {
        // Find top-most point of OBB for label placement
        float min_y = pts[0].y;
        int min_idx = 0;
        for (int i = 1; i < 4; i++) {
            if (pts[i].y < min_y) { min_y = pts[i].y; min_idx = i; }
        }
        int tx = static_cast<int>(pts[min_idx].x);
        int ty = static_cast<int>(pts[min_idx].y);
        double font_scale = 0.00075 * std::min(img.cols, img.rows);
        int baseline = 0;
        auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
        cv::rectangle(img,
                      cv::Rect(cv::Point(tx, ty - text_size.height),
                               cv::Point(tx + text_size.width, ty)),
                      color, cv::FILLED);
        cv::putText(img, text, cv::Point(tx, ty), cv::FONT_HERSHEY_SIMPLEX,
                    font_scale, cv::Scalar(255, 255, 255));
    }
}

void draw_face(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    // face_landmarks is now std::vector<float> with pairs of x,y,conf coordinates
    for (size_t i = 0; i + 2 < meta->_face_landmarks.size(); i += 3) {
        float x = meta->_face_landmarks[i];
        float y = meta->_face_landmarks[i + 1];
        cv::circle(img, cv::Point(int(x / sx), int(y / sy)), 3, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
    }
    if (meta->_face_box[2] > meta->_face_box[0] && meta->_face_box[3] > meta->_face_box[1]) {
        cv::rectangle(
            img,
            cv::Rect(
                cv::Point(int(meta->_face_box[0] / sx), int(meta->_face_box[1] / sy)),
                cv::Point(int(meta->_face_box[2] / sx), int(meta->_face_box[3] / sy))),
            cv::Scalar(255, 0, 0), 2);
    }
}

void draw_label_or_id(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    // OBB objects are drawn by draw_obb — skip AABB here
    if (meta->_obb.size() == 5) return;
    if (meta->_box[2] - meta->_box[0] <= 0 || meta->_box[3] - meta->_box[1] <= 0)
        return;
    int id = meta->_track_id;
    bool has_id = (id != -1);
    int label = meta->_label;
    bool has_label = (!has_id && label != -1);
    if (!has_id && !has_label)
        return;
    std::string text;
    cv::Scalar color;
    if (has_id) {
        text = std::to_string(id);
        color = COLORS[id % COLORS.size()];
    } else {
        text = cv::format("%s=%.2f", meta->_label_name.c_str(), meta->_confidence);
        color = COLORS[label % COLORS.size()];
    }
    auto x = int(meta->_box[0] / sx);
    auto y = int(meta->_box[1] / sy);
    auto x2 = int(meta->_box[2] / sx);
    auto y2 = int(meta->_box[3] / sy);
    double font_scale = 0.00075 * std::min(img.cols, img.rows);
    int baseline = 0;
    auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
    cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Point(x2, y2)), color, 2);
    cv::rectangle(img,
                  cv::Rect(cv::Point(x, y - text_size.height),
                           cv::Point(x + text_size.width, y)),
                  color, cv::FILLED);
    cv::putText(img, text, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX,
                font_scale, cv::Scalar(255, 255, 255));
}

void draw_clip(cv::Mat &img, const DXObjectMeta *meta, bool v3_clip_text) {
    if (meta->_confidence > 0.24 && meta->_label == -1 && meta->_box[0] == -1 && meta->_box[1] == -1 && meta->_box[2] == -1 && meta->_box[3] == -1) {
        std::string text;
        if (v3_clip_text) {
            text = cv::format("%s", meta->_label_name.c_str());
        } else {
            text = cv::format("%s=%.2f", meta->_label_name.c_str(), meta->_confidence);
        }
        auto text_area_height = int(img.rows * 0.15);
        auto text_area_width = int(img.cols * 0.9);
        auto margin_x = int(img.cols * 0.05);
        auto margin_y = int(img.rows * 0.02);
        double font_scale = 0.002 * std::min(img.cols, img.rows);
        int baseline = 0;
        cv::Size text_size;
        do {
            text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 2, &baseline);
            if (text_size.width <= text_area_width && text_size.height <= text_area_height - margin_y) {
                break;
            }
            font_scale *= 0.9;
        } while (font_scale > 0.3);
        int box_y_start = img.rows - text_size.height - margin_y * 2;
        int box_width = text_size.width + margin_x * 2;
        cv::rectangle(img, 
                     cv::Rect(cv::Point(0, box_y_start), 
                             cv::Point(box_width, img.rows)), 
                     cv::Scalar(39, 129, 113), cv::FILLED);
        int text_x = margin_x;
        int text_y = img.rows - margin_y;
        cv::putText(img, text, cv::Point(text_x, text_y), 
                   cv::FONT_HERSHEY_SIMPLEX, font_scale, 
                   cv::Scalar(255, 255, 255), 2);
    }
}

void draw_object_meta(cv::Mat &img, const DXObjectMeta *meta, float scale_x, float scale_y, bool v3_clip_text) {
    draw_instance_segmentation(img, meta, scale_x, scale_y);
    draw_keypoints(img, meta, scale_x, scale_y);
    draw_obb(img, meta, scale_x, scale_y);
    draw_face(img, meta, scale_x, scale_y);
    draw_label_or_id(img, meta, scale_x, scale_y);
    draw_clip(img, meta, v3_clip_text);
}

const std::vector<cv::Scalar> COLORS = {
    // Ultralytics-style bright palette (BGR format)
    cv::Scalar(56, 56, 255),     // Red
    cv::Scalar(31, 112, 255),    // Orange
    cv::Scalar(29, 178, 255),    // Amber
    cv::Scalar(49, 210, 207),    // Yellow-Green
    cv::Scalar(10, 249, 72),     // Lime
    cv::Scalar(23, 204, 146),    // Green
    cv::Scalar(134, 219, 61),    // Sea Green
    cv::Scalar(187, 212, 0),     // Turquoise
    cv::Scalar(255, 255, 0),     // Cyan
    cv::Scalar(168, 153, 44),    // Teal
    cv::Scalar(255, 194, 0),     // Sky Blue
    cv::Scalar(255, 115, 100),   // Periwinkle
    cv::Scalar(236, 24, 0),      // Navy Blue
    cv::Scalar(255, 56, 132),    // Purple
    cv::Scalar(255, 56, 203),    // Violet
    cv::Scalar(200, 149, 255),   // Pink
    cv::Scalar(199, 55, 255),    // Hot Pink
    cv::Scalar(151, 157, 255),   // Salmon
    cv::Scalar(0, 127, 255),     // Dark Orange
    cv::Scalar(0, 255, 0),       // Pure Green
    // Extended bright colors
    cv::Scalar(0, 255, 255),     // Yellow
    cv::Scalar(255, 0, 0),       // Blue
    cv::Scalar(255, 0, 255),     // Magenta
    cv::Scalar(0, 200, 255),     // Gold
    cv::Scalar(52, 147, 26),     // Forest Green
    cv::Scalar(0, 69, 255),      // Orange-Red
    cv::Scalar(71, 99, 255),     // Tomato
    cv::Scalar(50, 205, 50),     // Lime Green
    cv::Scalar(255, 144, 30),    // Dodger Blue
    cv::Scalar(0, 215, 255),     // Gold 2
    cv::Scalar(128, 0, 255),     // Rose
    cv::Scalar(0, 255, 128),     // Spring Green
    cv::Scalar(128, 255, 0),     // Chartreuse
    cv::Scalar(255, 128, 0),     // Azure
    cv::Scalar(80, 200, 120),    // Emerald
    cv::Scalar(208, 224, 64),    // Turquoise 2
    cv::Scalar(133, 0, 82),      // Dark Purple
    cv::Scalar(0, 165, 255),     // Orange 3
    cv::Scalar(47, 255, 173),    // Green Yellow
    cv::Scalar(0, 140, 255),     // Dark Orange 2
    cv::Scalar(205, 0, 0),       // Medium Blue
    cv::Scalar(147, 20, 255),    // Deep Pink
    cv::Scalar(180, 105, 255),   // Hot Pink 2
    cv::Scalar(238, 130, 238),   // Violet 2
    cv::Scalar(250, 206, 135),   // Light Sky Blue
    cv::Scalar(0, 250, 154),     // Med Spring Green
    cv::Scalar(144, 238, 144),   // Light Green
    cv::Scalar(130, 0, 75),      // Indigo
    cv::Scalar(60, 20, 220),     // Crimson
    cv::Scalar(180, 130, 70),    // Steel Blue
    // Bright variations
    cv::Scalar(100, 100, 255),   // Coral
    cv::Scalar(100, 200, 255),   // Peach
    cv::Scalar(100, 255, 100),   // Bright Lime
    cv::Scalar(255, 255, 100),   // Aqua
    cv::Scalar(255, 100, 100),   // Azure 2
    cv::Scalar(255, 100, 255),   // Fuchsia
    cv::Scalar(50, 150, 255),    // Tangerine
    cv::Scalar(50, 255, 150),    // Chartreuse 2
    cv::Scalar(150, 50, 255),    // Violet 3
    cv::Scalar(255, 50, 150),    // Cerulean
    cv::Scalar(150, 255, 50),    // Yellow-Green 2
    cv::Scalar(255, 150, 50),    // Sky 2
    cv::Scalar(100, 200, 200),   // Warm Amber
    cv::Scalar(200, 100, 200),   // Orchid
    cv::Scalar(200, 200, 100),   // Teal 2
    cv::Scalar(100, 200, 100),   // Fern
    cv::Scalar(200, 100, 100),   // Slate Blue
    cv::Scalar(139, 139, 0),     // Dark Cyan
    cv::Scalar(212, 255, 127),   // Aquamarine
    cv::Scalar(0, 252, 124),     // Lawn Green
    cv::Scalar(180, 0, 0),       // Dark Blue 2
    cv::Scalar(203, 192, 255),   // Pink 2
    cv::Scalar(0, 100, 200),     // Chocolate
    cv::Scalar(80, 127, 255),    // Light Coral
    cv::Scalar(113, 179, 60),    // Olive Drab
    cv::Scalar(230, 216, 173),   // Light Steel Blue
    cv::Scalar(0, 0, 255),       // Pure Red
    cv::Scalar(127, 255, 212),   // Aquamarine 2
    cv::Scalar(180, 238, 180),   // Honeydew
    cv::Scalar(105, 105, 255),   // Indian Red
};

const std::vector<std::vector<int>> skeleton = {
    {15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
    {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
    {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6},
};

const std::vector<cv::Scalar> pose_limb_color = {
    cv::Scalar(51, 153, 255), cv::Scalar(51, 153, 255),
    cv::Scalar(51, 153, 255), cv::Scalar(51, 153, 255),
    cv::Scalar(255, 51, 255), cv::Scalar(255, 51, 255),
    cv::Scalar(255, 51, 255), cv::Scalar(255, 128, 0),
    cv::Scalar(255, 128, 0),  cv::Scalar(255, 128, 0),
    cv::Scalar(255, 128, 0),  cv::Scalar(255, 128, 0),
    cv::Scalar(0, 255, 0),    cv::Scalar(0, 255, 0),
    cv::Scalar(0, 255, 0),    cv::Scalar(0, 255, 0),
    cv::Scalar(0, 255, 0),    cv::Scalar(0, 255, 0),
    cv::Scalar(0, 255, 0),
};

const std::vector<cv::Scalar> pose_kpt_color = {
    cv::Scalar(0, 255, 0),    cv::Scalar(0, 255, 0),
    cv::Scalar(0, 255, 0),    cv::Scalar(0, 255, 0),
    cv::Scalar(0, 255, 0),    cv::Scalar(255, 128, 0),
    cv::Scalar(255, 128, 0),  cv::Scalar(255, 128, 0),
    cv::Scalar(255, 128, 0),  cv::Scalar(255, 128, 0),
    cv::Scalar(255, 128, 0),  cv::Scalar(51, 153, 255),
    cv::Scalar(51, 153, 255), cv::Scalar(51, 153, 255),
    cv::Scalar(51, 153, 255), cv::Scalar(51, 153, 255),
    cv::Scalar(51, 153, 255),
};

// ==================== YUV Drawing Functions ====================

YUVColor bgr_to_yuv_bt601(uint8_t b, uint8_t g, uint8_t r) {
    // BT.601 standard (studio range)
    int y = (66 * r + 129 * g + 25 * b + 128) / 256 + 16;
    int u = (-38 * r - 74 * g + 112 * b + 128) / 256 + 128;
    int v = (112 * r - 94 * g - 18 * b + 128) / 256 + 128;
    
    // Clamp to valid range
    y = std::min(std::max(y, 16), 235);
    u = std::min(std::max(u, 16), 240);
    v = std::min(std::max(v, 16), 240);
    
    return YUVColor{static_cast<uint8_t>(y), static_cast<uint8_t>(u), static_cast<uint8_t>(v)};
}

void draw_rectangle_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                         int stride_y, int stride_uv, int width, int height,
                         int x1, int y1, int x2, int y2, YUVColor color, int thickness) {
    // Clamp coordinates
    x1 = std::max(0, std::min(x1, width - 1));
    y1 = std::max(0, std::min(y1, height - 1));
    x2 = std::max(0, std::min(x2, width - 1));
    y2 = std::max(0, std::min(y2, height - 1));
    
    if (x1 >= x2 || y1 >= y2) return;
    
    // Draw horizontal lines on Y plane
    for (int t = 0; t < thickness; t++) {
        // Top line
        if (y1 + t < height) {
            for (int x = x1; x <= x2; x++) {
                y_plane[(y1 + t) * stride_y + x] = color.y;
            }
        }
        // Bottom line
        if (y2 - t >= 0) {
            for (int x = x1; x <= x2; x++) {
                y_plane[(y2 - t) * stride_y + x] = color.y;
            }
        }
    }
    
    // Draw vertical lines on Y plane
    for (int t = 0; t < thickness; t++) {
        // Left line
        if (x1 + t < width) {
            for (int y = y1; y <= y2; y++) {
                y_plane[y * stride_y + (x1 + t)] = color.y;
            }
        }
        // Right line
        if (x2 - t >= 0) {
            for (int y = y1; y <= y2; y++) {
                y_plane[y * stride_y + (x2 - t)] = color.y;
            }
        }
    }
    
    // Draw on U/V planes (subsampled 4:2:0 - 2x2 blocks share one UV value)
    int uv_x1 = x1 / 2;
    int uv_y1 = y1 / 2;
    int uv_x2 = x2 / 2;
    int uv_y2 = y2 / 2;
    int uv_thickness = std::max(1, thickness / 2);
    int uv_width = width / 2;
    int uv_height = height / 2;
    
    // Horizontal UV lines
    for (int t = 0; t < uv_thickness; t++) {
        if (uv_y1 + t < uv_height) {
            for (int x = uv_x1; x <= uv_x2; x++) {
                u_plane[(uv_y1 + t) * stride_uv + x] = color.u;
                v_plane[(uv_y1 + t) * stride_uv + x] = color.v;
            }
        }
        if (uv_y2 - t >= 0 && uv_y2 - t < uv_height) {
            for (int x = uv_x1; x <= uv_x2; x++) {
                u_plane[(uv_y2 - t) * stride_uv + x] = color.u;
                v_plane[(uv_y2 - t) * stride_uv + x] = color.v;
            }
        }
    }
    
    // Vertical UV lines
    for (int t = 0; t < uv_thickness; t++) {
        if (uv_x1 + t < uv_width) {
            for (int y = uv_y1; y <= uv_y2; y++) {
                u_plane[y * stride_uv + (uv_x1 + t)] = color.u;
                v_plane[y * stride_uv + (uv_x1 + t)] = color.v;
            }
        }
        if (uv_x2 - t >= 0 && uv_x2 - t < uv_width) {
            for (int y = uv_y1; y <= uv_y2; y++) {
                u_plane[y * stride_uv + (uv_x2 - t)] = color.u;
                v_plane[y * stride_uv + (uv_x2 - t)] = color.v;
            }
        }
    }
}

void draw_filled_rect_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                           int stride_y, int stride_uv, int width, int height,
                           int x1, int y1, int x2, int y2, YUVColor color) {
    x1 = std::max(0, std::min(x1, width - 1));
    y1 = std::max(0, std::min(y1, height - 1));
    x2 = std::max(0, std::min(x2, width));
    y2 = std::max(0, std::min(y2, height));
    if (x1 >= x2 || y1 >= y2) return;

    int fill_w = x2 - x1;
    for (int row = y1; row < y2; row++)
        memset(y_plane + row * stride_y + x1, color.y, fill_w);

    int uv_x1 = x1 / 2, uv_y1 = y1 / 2;
    int uv_x2 = x2 / 2, uv_y2 = y2 / 2;
    int uv_fill_w = uv_x2 - uv_x1;
    for (int row = uv_y1; row < uv_y2; row++) {
        memset(u_plane + row * stride_uv + uv_x1, color.u, uv_fill_w);
        memset(v_plane + row * stride_uv + uv_x1, color.v, uv_fill_w);
    }
}

void draw_rectangle_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                         int stride_y, int stride_uv, int width, int height,
                         int x1, int y1, int x2, int y2, YUVColor color, int thickness) {
    // Clamp coordinates
    x1 = std::max(0, std::min(x1, width - 1));
    y1 = std::max(0, std::min(y1, height - 1));
    x2 = std::max(0, std::min(x2, width - 1));
    y2 = std::max(0, std::min(y2, height - 1));
    
    if (x1 >= x2 || y1 >= y2) return;
    
    // Draw Y plane (same as I420)
    for (int t = 0; t < thickness; t++) {
        if (y1 + t < height) {
            for (int x = x1; x <= x2; x++) {
                y_plane[(y1 + t) * stride_y + x] = color.y;
            }
        }
        if (y2 - t >= 0) {
            for (int x = x1; x <= x2; x++) {
                y_plane[(y2 - t) * stride_y + x] = color.y;
            }
        }
    }
    
    for (int t = 0; t < thickness; t++) {
        if (x1 + t < width) {
            for (int y = y1; y <= y2; y++) {
                y_plane[y * stride_y + (x1 + t)] = color.y;
            }
        }
        if (x2 - t >= 0) {
            for (int y = y1; y <= y2; y++) {
                y_plane[y * stride_y + (x2 - t)] = color.y;
            }
        }
    }
    
    // Draw UV plane (interleaved UVUV...)
    int uv_x1 = x1 / 2;
    int uv_y1 = y1 / 2;
    int uv_x2 = x2 / 2;
    int uv_y2 = y2 / 2;
    int uv_thickness = std::max(1, thickness / 2);
    int uv_width = width / 2;
    int uv_height = height / 2;
    
    // Horizontal UV lines
    for (int t = 0; t < uv_thickness; t++) {
        if (uv_y1 + t < uv_height) {
            for (int x = uv_x1; x <= uv_x2; x++) {
                int offset = (uv_y1 + t) * stride_uv + x * 2;
                uv_plane[offset] = color.u;
                uv_plane[offset + 1] = color.v;
            }
        }
        if (uv_y2 - t >= 0 && uv_y2 - t < uv_height) {
            for (int x = uv_x1; x <= uv_x2; x++) {
                int offset = (uv_y2 - t) * stride_uv + x * 2;
                uv_plane[offset] = color.u;
                uv_plane[offset + 1] = color.v;
            }
        }
    }
    
    // Vertical UV lines
    for (int t = 0; t < uv_thickness; t++) {
        if (uv_x1 + t < uv_width) {
            for (int y = uv_y1; y <= uv_y2; y++) {
                int offset = y * stride_uv + (uv_x1 + t) * 2;
                uv_plane[offset] = color.u;
                uv_plane[offset + 1] = color.v;
            }
        }
        if (uv_x2 - t >= 0 && uv_x2 - t < uv_width) {
            for (int y = uv_y1; y <= uv_y2; y++) {
                int offset = y * stride_uv + (uv_x2 - t) * 2;
                uv_plane[offset] = color.u;
                uv_plane[offset + 1] = color.v;
            }
        }
    }
}

void draw_filled_rect_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                           int stride_y, int stride_uv, int width, int height,
                           int x1, int y1, int x2, int y2, YUVColor color) {
    x1 = std::max(0, std::min(x1, width - 1));
    y1 = std::max(0, std::min(y1, height - 1));
    x2 = std::max(0, std::min(x2, width));
    y2 = std::max(0, std::min(y2, height));
    if (x1 >= x2 || y1 >= y2) return;

    int fill_w = x2 - x1;
    for (int row = y1; row < y2; row++)
        memset(y_plane + row * stride_y + x1, color.y, fill_w);

    int uv_x1 = x1 / 2, uv_y1 = y1 / 2;
    int uv_x2 = x2 / 2, uv_y2 = y2 / 2;
    uint16_t uv_val = (static_cast<uint16_t>(color.v) << 8) | color.u;
    for (int row = uv_y1; row < uv_y2; row++) {
        auto *uv_row = reinterpret_cast<uint16_t *>(uv_plane + row * stride_uv) + uv_x1;
        for (int col = uv_x1; col < uv_x2; col++)
            *uv_row++ = uv_val;
    }
}

void draw_text_y_plane(uint8_t *y_plane, int stride, int width, int height,
                       const char *text, int x, int y, double scale) {
    // Create temporary Y-only cv::Mat (grayscale)
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride);
    
    // Draw white text (255) on Y plane only
    cv::putText(y_mat, text, cv::Point(x, y), 
                cv::FONT_HERSHEY_SIMPLEX, scale, 
                cv::Scalar(255), 1, cv::LINE_8);
}

void draw_keypoints_y_plane(uint8_t *y_plane, int stride, int width, int height,
                            const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_keypoints.empty()) return;
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride);
    std::vector<cv::Point> pts;
    for (int i = 0; i < 17; ++i) {
        float kx = meta->_keypoints[i * 3] / sx;
        float ky = meta->_keypoints[i * 3 + 1] / sy;
        float ks = meta->_keypoints[i * 3 + 2];
        pts.emplace_back((ks > 0.5f) ? cv::Point(int(kx), int(ky)) : cv::Point(-1, -1));
    }
    for (size_t i = 0; i < skeleton.size(); ++i) {
        auto &p = skeleton[i];
        if (pts[p[0]].x >= 0 && pts[p[1]].x >= 0)
            cv::line(y_mat, pts[p[0]], pts[p[1]], cv::Scalar(200), 2, cv::LINE_AA);
    }
    for (auto &pt : pts) {
        if (pt.x >= 0)
            cv::circle(y_mat, pt, 3, cv::Scalar(235), -1, cv::LINE_AA);
    }
}

void draw_obb_y_plane(uint8_t *y_plane, int stride, int width, int height,
                      const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_obb.size() != 5) return;
    float cx = meta->_obb[0] / sx;
    float cy = meta->_obb[1] / sy;
    float w = meta->_obb[2] / sx;
    float h = meta->_obb[3] / sy;
    float angle_rad = meta->_obb[4];
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride);
    cv::RotatedRect rrect(cv::Point2f(cx, cy), cv::Size2f(w, h),
                          angle_rad * 180.0f / static_cast<float>(CV_PI));
    cv::Point2f pts[4];
    rrect.points(pts);
    int label = meta->_label;
    cv::Scalar bgr_color = COLORS[(label >= 0 ? label : 0) % COLORS.size()];
    YUVColor yuv_color = bgr_to_yuv_bt601(
        static_cast<uint8_t>(bgr_color[0]),
        static_cast<uint8_t>(bgr_color[1]),
        static_cast<uint8_t>(bgr_color[2]));
    for (int i = 0; i < 4; i++)
        cv::line(y_mat, pts[i], pts[(i + 1) % 4], cv::Scalar(yuv_color.y), 2, cv::LINE_AA);

    // Draw label text
    std::string text;
    int id = meta->_track_id;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s %.2f", meta->_label_name.c_str(), meta->_confidence);
    }
    if (!text.empty()) {
        float min_y = pts[0].y;
        int min_idx = 0;
        for (int i = 1; i < 4; i++) {
            if (pts[i].y < min_y) { min_y = pts[i].y; min_idx = i; }
        }
        double font_scale = 0.00075 * std::min(width, height);
        draw_text_y_plane(y_plane, stride, width, height,
                          text.c_str(), static_cast<int>(pts[min_idx].x),
                          static_cast<int>(pts[min_idx].y) - 2, font_scale);
    }
}


void draw_obb_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                   int stride_y, int stride_uv, int width, int height,
                   const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_obb.size() != 5) return;
    float cx = meta->_obb[0] / sx;
    float cy = meta->_obb[1] / sy;
    float w = meta->_obb[2] / sx;
    float h = meta->_obb[3] / sy;
    float angle_rad = meta->_obb[4];

    cv::RotatedRect rrect(cv::Point2f(cx, cy), cv::Size2f(w, h),
                          angle_rad * 180.0f / static_cast<float>(CV_PI));
    cv::Point2f pts[4];
    rrect.points(pts);

    int id = meta->_track_id;
    int label = meta->_label;
    int color_idx = (id != -1) ? id : (label >= 0 ? label : 0);
    cv::Scalar bgr_color = COLORS[color_idx % COLORS.size()];
    YUVColor yuv_color = bgr_to_yuv_bt601(
        static_cast<uint8_t>(bgr_color[0]),
        static_cast<uint8_t>(bgr_color[1]),
        static_cast<uint8_t>(bgr_color[2]));

    // Draw on Y plane
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    for (int i = 0; i < 4; i++)
        cv::line(y_mat, pts[i], pts[(i + 1) % 4], cv::Scalar(yuv_color.y), 2, cv::LINE_AA);

    // Draw on U and V planes (half resolution)
    cv::Mat u_mat(height / 2, width / 2, CV_8UC1, u_plane, stride_uv);
    cv::Mat v_mat(height / 2, width / 2, CV_8UC1, v_plane, stride_uv);
    cv::Point2f uv_pts[4];
    for (int i = 0; i < 4; i++) {
        uv_pts[i].x = pts[i].x / 2.0f;
        uv_pts[i].y = pts[i].y / 2.0f;
    }
    for (int i = 0; i < 4; i++) {
        cv::line(u_mat, uv_pts[i], uv_pts[(i + 1) % 4], cv::Scalar(yuv_color.u), 1, cv::LINE_AA);
        cv::line(v_mat, uv_pts[i], uv_pts[(i + 1) % 4], cv::Scalar(yuv_color.v), 1, cv::LINE_AA);
    }

    // Draw label text
    std::string text;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s %.2f", meta->_label_name.c_str(), meta->_confidence);
    }
    if (!text.empty()) {
        float min_y = pts[0].y;
        int min_idx = 0;
        for (int i = 1; i < 4; i++) {
            if (pts[i].y < min_y) { min_y = pts[i].y; min_idx = i; }
        }
        int tx = static_cast<int>(pts[min_idx].x);
        int ty = static_cast<int>(pts[min_idx].y) - 2;
        double font_scale = 0.00075 * std::min(width, height);
        int baseline = 0;
        auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
        int bg_x2 = std::min(width, tx + text_size.width);
        int bg_y1 = std::max(0, ty - text_size.height);
        draw_filled_rect_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                              width, height, tx, bg_y1, bg_x2, ty + 2, yuv_color);
        draw_text_y_plane(y_plane, stride_y, width, height,
                         text.c_str(), tx, ty, font_scale);
    }
}

void draw_obb_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                   int stride_y, int stride_uv, int width, int height,
                   const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_obb.size() != 5) return;
    float cx = meta->_obb[0] / sx;
    float cy = meta->_obb[1] / sy;
    float w = meta->_obb[2] / sx;
    float h = meta->_obb[3] / sy;
    float angle_rad = meta->_obb[4];

    cv::RotatedRect rrect(cv::Point2f(cx, cy), cv::Size2f(w, h),
                          angle_rad * 180.0f / static_cast<float>(CV_PI));
    cv::Point2f pts[4];
    rrect.points(pts);

    int id = meta->_track_id;
    int label = meta->_label;
    int color_idx = (id != -1) ? id : (label >= 0 ? label : 0);
    cv::Scalar bgr_color = COLORS[color_idx % COLORS.size()];
    YUVColor yuv_color = bgr_to_yuv_bt601(
        static_cast<uint8_t>(bgr_color[0]),
        static_cast<uint8_t>(bgr_color[1]),
        static_cast<uint8_t>(bgr_color[2]));

    // Draw on Y plane
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    for (int i = 0; i < 4; i++)
        cv::line(y_mat, pts[i], pts[(i + 1) % 4], cv::Scalar(yuv_color.y), 2, cv::LINE_AA);

    // Draw on interleaved UV plane (CV_8UC2)
    cv::Mat uv_mat(height / 2, width / 2, CV_8UC2, uv_plane, stride_uv);
    cv::Point2f uv_pts[4];
    for (int i = 0; i < 4; i++) {
        uv_pts[i].x = pts[i].x / 2.0f;
        uv_pts[i].y = pts[i].y / 2.0f;
    }
    for (int i = 0; i < 4; i++)
        cv::line(uv_mat, uv_pts[i], uv_pts[(i + 1) % 4], cv::Scalar(yuv_color.u, yuv_color.v), 1, cv::LINE_AA);

    // Draw label text
    std::string text;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s %.2f", meta->_label_name.c_str(), meta->_confidence);
    }
    if (!text.empty()) {
        float min_y = pts[0].y;
        int min_idx = 0;
        for (int i = 1; i < 4; i++) {
            if (pts[i].y < min_y) { min_y = pts[i].y; min_idx = i; }
        }
        int tx = static_cast<int>(pts[min_idx].x);
        int ty = static_cast<int>(pts[min_idx].y) - 2;
        double font_scale = 0.00075 * std::min(width, height);
        int baseline = 0;
        auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
        int bg_x2 = std::min(width, tx + text_size.width);
        int bg_y1 = std::max(0, ty - text_size.height);
        draw_filled_rect_nv12(y_plane, uv_plane, stride_y, stride_uv,
                              width, height, tx, bg_y1, bg_x2, ty + 2, yuv_color);
        draw_text_y_plane(y_plane, stride_y, width, height,
                         text.c_str(), tx, ty, font_scale);
    }
}

void draw_face_y_plane(uint8_t *y_plane, int stride, int width, int height,
                       const DXObjectMeta *meta, float sx, float sy) {
    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride);
    for (size_t i = 0; i + 2 < meta->_face_landmarks.size(); i += 3) {
        float fx = meta->_face_landmarks[i];
        float fy = meta->_face_landmarks[i + 1];
        cv::circle(y_mat, cv::Point(int(fx / sx), int(fy / sy)), 3, cv::Scalar(235), -1, cv::LINE_AA);
    }
    if (meta->_face_box[2] > meta->_face_box[0] && meta->_face_box[3] > meta->_face_box[1]) {
        cv::rectangle(y_mat,
            cv::Rect(cv::Point(int(meta->_face_box[0] / sx), int(meta->_face_box[1] / sy)),
                     cv::Point(int(meta->_face_box[2] / sx), int(meta->_face_box[3] / sy))),
            cv::Scalar(235), 2);
    }
}

void draw_instance_segmentation_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta, float sx, float sy) {
    cv::Rect roi;
    if (!get_instance_mask_roi(meta, width, height, sx, sy, roi))
        return;

    cv::Scalar color = get_instance_color_bgr(meta);
    YUVColor yuv = bgr_to_yuv_bt601(
        static_cast<uint8_t>(color[0]),
        static_cast<uint8_t>(color[1]),
        static_cast<uint8_t>(color[2]));

    cv::Mat mask = resize_instance_mask(meta, roi.size());

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::Mat y_roi = y_mat(roi);
    cv::Mat seg_y(roi.height, roi.width, CV_8UC1, cv::Scalar(yuv.y));
    cv::Mat blend_y;
    cv::addWeighted(y_roi, 0.6, seg_y, 0.4, 0.0, blend_y);
    blend_y.copyTo(y_roi, mask);

    cv::Rect uv_roi = get_uv_roi(roi, width, height);
    if (uv_roi.width <= 0 || uv_roi.height <= 0)
        return;

    cv::Mat mask_uv;
    cv::resize(mask, mask_uv, uv_roi.size(), 0, 0, cv::INTER_NEAREST);

    cv::Mat u_mat(height / 2, width / 2, CV_8UC1, u_plane, stride_uv);
    cv::Mat v_mat(height / 2, width / 2, CV_8UC1, v_plane, stride_uv);
    cv::Mat u_roi = u_mat(uv_roi);
    cv::Mat v_roi = v_mat(uv_roi);
    cv::Mat seg_u(uv_roi.height, uv_roi.width, CV_8UC1, cv::Scalar(yuv.u));
    cv::Mat seg_v(uv_roi.height, uv_roi.width, CV_8UC1, cv::Scalar(yuv.v));
    cv::Mat blend_u, blend_v;
    cv::addWeighted(u_roi, 0.6, seg_u, 0.4, 0.0, blend_u);
    cv::addWeighted(v_roi, 0.6, seg_v, 0.4, 0.0, blend_v);
    blend_u.copyTo(u_roi, mask_uv);
    blend_v.copyTo(v_roi, mask_uv);
}

void draw_instance_segmentation_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta, float sx, float sy) {
    cv::Rect roi;
    if (!get_instance_mask_roi(meta, width, height, sx, sy, roi))
        return;

    cv::Scalar color = get_instance_color_bgr(meta);
    YUVColor yuv = bgr_to_yuv_bt601(
        static_cast<uint8_t>(color[0]),
        static_cast<uint8_t>(color[1]),
        static_cast<uint8_t>(color[2]));

    cv::Mat mask = resize_instance_mask(meta, roi.size());

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::Mat y_roi = y_mat(roi);
    cv::Mat seg_y(roi.height, roi.width, CV_8UC1, cv::Scalar(yuv.y));
    cv::Mat blend_y;
    cv::addWeighted(y_roi, 0.6, seg_y, 0.4, 0.0, blend_y);
    blend_y.copyTo(y_roi, mask);

    cv::Rect uv_roi = get_uv_roi(roi, width, height);
    if (uv_roi.width <= 0 || uv_roi.height <= 0)
        return;

    cv::Mat mask_uv;
    cv::resize(mask, mask_uv, uv_roi.size(), 0, 0, cv::INTER_NEAREST);

    cv::Mat uv_mat(height / 2, width / 2, CV_8UC2, uv_plane, stride_uv);
    cv::Mat uv_roi_view = uv_mat(uv_roi);
    cv::Mat seg_uv(uv_roi.height, uv_roi.width, CV_8UC2, cv::Scalar(yuv.u, yuv.v));
    cv::Mat blend_uv;
    cv::addWeighted(uv_roi_view, 0.6, seg_uv, 0.4, 0.0, blend_uv);
    blend_uv.copyTo(uv_roi_view, mask_uv);
}

// Internal helper: draw YUV I420 segmentation from raw data
// Resize class-index map with INTER_NEAREST, then colorize at target resolution
static void draw_seg_i420_impl(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const unsigned char* data, int seg_w, int seg_h, bool skip_bg) {
    size_t color_count = COLORS.size();

    cv::Mat seg_map(seg_h, seg_w, CV_8UC1, const_cast<unsigned char*>(data));
    cv::Mat resized_map;
    cv::resize(seg_map, resized_map, cv::Size(width, height), 0, 0, cv::INTER_NEAREST);

    cv::Mat seg_y(height, width, CV_8UC1, cv::Scalar(0));
    cv::Mat seg_mask(height, width, CV_8UC1, cv::Scalar(0));
    for (int i = 0; i < height * width; ++i) {
        int cls = resized_map.data[i];
        if (skip_bg && cls == 255) continue;
        const cv::Scalar &c = COLORS[cls % color_count];
        YUVColor yuv = bgr_to_yuv_bt601(
            static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
        seg_y.data[i] = yuv.y;
        seg_mask.data[i] = 255;
    }

    cv::Mat resized_map_uv;
    cv::resize(resized_map, resized_map_uv, cv::Size(width / 2, height / 2), 0, 0, cv::INTER_NEAREST);
    cv::Mat seg_u(height / 2, width / 2, CV_8UC1, cv::Scalar(128));
    cv::Mat seg_v(height / 2, width / 2, CV_8UC1, cv::Scalar(128));
    cv::Mat mask_uv(height / 2, width / 2, CV_8UC1, cv::Scalar(0));
    for (int i = 0; i < (height / 2) * (width / 2); ++i) {
        int cls = resized_map_uv.data[i];
        if (skip_bg && cls == 255) continue;
        const cv::Scalar &c = COLORS[cls % color_count];
        YUVColor yuv = bgr_to_yuv_bt601(
            static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
        seg_u.data[i] = yuv.u;
        seg_v.data[i] = yuv.v;
        mask_uv.data[i] = 255;
    }

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::Mat blend_y;
    cv::addWeighted(y_mat, 0.6, seg_y, 0.4, 0.0, blend_y);
    blend_y.copyTo(y_mat, seg_mask);

    cv::Mat u_mat(height / 2, width / 2, CV_8UC1, u_plane, stride_uv);
    cv::Mat v_mat(height / 2, width / 2, CV_8UC1, v_plane, stride_uv);
    cv::Mat blend_u, blend_v;
    cv::addWeighted(u_mat, 0.6, seg_u, 0.4, 0.0, blend_u);
    cv::addWeighted(v_mat, 0.6, seg_v, 0.4, 0.0, blend_v);
    blend_u.copyTo(u_mat, mask_uv);
    blend_v.copyTo(v_mat, mask_uv);
}

// Internal helper: draw YUV NV12 segmentation from raw data
static void draw_seg_nv12_impl(uint8_t *y_plane, uint8_t *uv_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const unsigned char* data, int seg_w, int seg_h, bool skip_bg) {
    size_t color_count = COLORS.size();

    cv::Mat seg_map(seg_h, seg_w, CV_8UC1, const_cast<unsigned char*>(data));
    cv::Mat resized_map;
    cv::resize(seg_map, resized_map, cv::Size(width, height), 0, 0, cv::INTER_NEAREST);

    cv::Mat seg_y(height, width, CV_8UC1, cv::Scalar(0));
    cv::Mat seg_mask(height, width, CV_8UC1, cv::Scalar(0));
    for (int i = 0; i < height * width; ++i) {
        int cls = resized_map.data[i];
        if (skip_bg && cls == 255) continue;
        const cv::Scalar &c = COLORS[cls % color_count];
        YUVColor yuv = bgr_to_yuv_bt601(
            static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
        seg_y.data[i] = yuv.y;
        seg_mask.data[i] = 255;
    }

    cv::Mat resized_map_uv;
    cv::resize(resized_map, resized_map_uv, cv::Size(width / 2, height / 2), 0, 0, cv::INTER_NEAREST);
    cv::Mat seg_uv(height / 2, width / 2, CV_8UC2, cv::Scalar(128, 128));
    cv::Mat mask_uv(height / 2, width / 2, CV_8UC1, cv::Scalar(0));
    for (int row = 0; row < height / 2; ++row) {
        for (int col = 0; col < width / 2; ++col) {
            int cls = resized_map_uv.at<uint8_t>(row, col);
            if (skip_bg && cls == 255) continue;
            const cv::Scalar &c = COLORS[cls % color_count];
            YUVColor yuv = bgr_to_yuv_bt601(
                static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
            seg_uv.at<cv::Vec2b>(row, col) = cv::Vec2b(yuv.u, yuv.v);
            mask_uv.at<uint8_t>(row, col) = 255;
        }
    }

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::Mat blend_y;
    cv::addWeighted(y_mat, 0.6, seg_y, 0.4, 0.0, blend_y);
    blend_y.copyTo(y_mat, seg_mask);

    cv::Mat uv_mat(height / 2, width / 2, CV_8UC2, uv_plane, stride_uv);
    cv::Mat blend_uv;
    cv::addWeighted(uv_mat, 0.6, seg_uv, 0.4, 0.0, blend_uv);
    blend_uv.copyTo(uv_mat, mask_uv);
}

void draw_semantic_segmentation_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                                  int stride_y, int stride_uv, int width, int height,
                                  const DXFrameMeta *meta) {
    if (meta->_seg_data.empty()) return;
    draw_seg_i420_impl(y_plane, u_plane, v_plane, stride_y, stride_uv,
                       width, height, meta->_seg_data.data(), meta->_seg_width, meta->_seg_height, false);
}

void draw_semantic_segmentation_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                                  int stride_y, int stride_uv, int width, int height,
                                  const DXFrameMeta *meta) {
    if (meta->_seg_data.empty()) return;
    draw_seg_nv12_impl(y_plane, uv_plane, stride_y, stride_uv,
                       width, height, meta->_seg_data.data(), meta->_seg_width, meta->_seg_height, false);
}

void draw_object_meta_yuv_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const DXObjectMeta *meta, float scale_x, float scale_y) {
    // Draw segmentation, pose keypoints, OBB and face landmarks first — they don't need _box
    draw_instance_segmentation_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                                    width, height, meta, scale_x, scale_y);
    draw_keypoints_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);
    draw_obb_i420(y_plane, u_plane, v_plane, stride_y, stride_uv, width, height, meta, scale_x, scale_y);
    draw_face_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);

    // OBB objects have their own drawing — skip AABB for them
    if (meta->_obb.size() == 5) return;

    // Bounding box and text label require a valid _box
    bool has_box = (meta->_box[2] - meta->_box[0] > 0 && meta->_box[3] - meta->_box[1] > 0);
    if (!has_box)
        return;

    auto x1 = static_cast<int>(meta->_box[0] / scale_x);
    auto y1 = static_cast<int>(meta->_box[1] / scale_y);
    auto x2 = static_cast<int>(meta->_box[2] / scale_x);
    auto y2 = static_cast<int>(meta->_box[3] / scale_y);

    // Choose color based on track_id or label
    int id = meta->_track_id;
    int label = meta->_label;
    int color_idx = (id != -1) ? id : label;
    if (color_idx < 0) color_idx = 0;

    cv::Scalar bgr_color = COLORS[color_idx % COLORS.size()];
    YUVColor yuv_color = bgr_to_yuv_bt601(
        static_cast<uint8_t>(bgr_color[0]),
        static_cast<uint8_t>(bgr_color[1]),
        static_cast<uint8_t>(bgr_color[2])
    );

    // Draw colored bounding box
    draw_rectangle_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                        width, height, x1, y1, x2, y2, yuv_color, 2);

    // Draw white text label
    std::string text;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s %.2f", meta->_label_name.c_str(), meta->_confidence);
    }

    if (!text.empty()) {
        double font_scale = 0.00075 * std::min(width, height);
        int baseline = 0;
        auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
        int bg_x2 = std::min(width, x1 + text_size.width);
        int bg_y1 = std::max(0, y1 - text_size.height - 2);
        draw_filled_rect_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                              width, height, x1, bg_y1, bg_x2, y1, yuv_color);
        draw_text_y_plane(y_plane, stride_y, width, height,
                         text.c_str(), x1, y1 - 2, font_scale);
    }
}

void draw_object_meta_yuv_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const DXObjectMeta *meta, float scale_x, float scale_y) {
    // Draw segmentation, pose keypoints, OBB and face landmarks first — they don't need _box
    draw_instance_segmentation_nv12(y_plane, uv_plane, stride_y, stride_uv,
                                    width, height, meta, scale_x, scale_y);
    draw_keypoints_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);
    draw_obb_nv12(y_plane, uv_plane, stride_y, stride_uv, width, height, meta, scale_x, scale_y);
    draw_face_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);

    // OBB objects have their own drawing — skip AABB for them
    if (meta->_obb.size() == 5) return;

    // Bounding box and text label require a valid _box
    bool has_box = (meta->_box[2] - meta->_box[0] > 0 && meta->_box[3] - meta->_box[1] > 0);
    if (!has_box)
        return;

    auto x1 = static_cast<int>(meta->_box[0] / scale_x);
    auto y1 = static_cast<int>(meta->_box[1] / scale_y);
    auto x2 = static_cast<int>(meta->_box[2] / scale_x);
    auto y2 = static_cast<int>(meta->_box[3] / scale_y);

    // Choose color
    int id = meta->_track_id;
    int label = meta->_label;
    int color_idx = (id != -1) ? id : label;
    if (color_idx < 0) color_idx = 0;

    cv::Scalar bgr_color = COLORS[color_idx % COLORS.size()];
    YUVColor yuv_color = bgr_to_yuv_bt601(
        static_cast<uint8_t>(bgr_color[0]),
        static_cast<uint8_t>(bgr_color[1]),
        static_cast<uint8_t>(bgr_color[2])
    );

    // Draw colored bounding box
    draw_rectangle_nv12(y_plane, uv_plane, stride_y, stride_uv,
                        width, height, x1, y1, x2, y2, yuv_color, 2);

    // Draw white text label
    std::string text;
    if (id != -1) {
        text = std::to_string(id);
    } else if (label != -1) {
        text = cv::format("%s %.2f", meta->_label_name.c_str(), meta->_confidence);
    }

    if (!text.empty()) {
        double font_scale = 0.00075 * std::min(width, height);
        int baseline = 0;
        auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
        int bg_x2 = std::min(width, x1 + text_size.width);
        int bg_y1 = std::max(0, y1 - text_size.height - 2);
        draw_filled_rect_nv12(y_plane, uv_plane, stride_y, stride_uv,
                              width, height, x1, bg_y1, bg_x2, y1, yuv_color);
        draw_text_y_plane(y_plane, stride_y, width, height,
                         text.c_str(), x1, y1 - 2, font_scale);
    }
}
