#include "dxosd_common.hpp"

void draw_segmentation(cv::Mat &img, const DXObjectMeta *meta) {
    if (meta->_seg_cls_map.data.empty())
        return;

    cv::Mat seg_vis(meta->_seg_cls_map.height, meta->_seg_cls_map.width, CV_8UC3);
    uchar* seg_ptr = seg_vis.data;
    const int total_pixels = meta->_seg_cls_map.height * meta->_seg_cls_map.width;
    size_t color_count = COLORS.size();
    for (int i = 0; i < total_pixels; ++i) {
        int cls = meta->_seg_cls_map.data[i];
        const cv::Scalar& color = COLORS[cls % color_count];
        seg_ptr[i * 3 + 0] = static_cast<uchar>(color[0]);
        seg_ptr[i * 3 + 1] = static_cast<uchar>(color[1]);
        seg_ptr[i * 3 + 2] = static_cast<uchar>(color[2]);
    }
    cv::Mat resized;
    cv::resize(seg_vis, resized, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::addWeighted(img, 1.0, resized, 1.0, 0.0, img);
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
    draw_segmentation(img, meta);
    draw_keypoints(img, meta, scale_x, scale_y);
    draw_face(img, meta, scale_x, scale_y);
    draw_label_or_id(img, meta, scale_x, scale_y);
    draw_clip(img, meta, v3_clip_text);
}

const std::vector<cv::Scalar> COLORS = {
    cv::Scalar(106, 15, 95),   // Deep Magenta
    cv::Scalar(54, 68, 113),   // Muted Blue
    cv::Scalar(25, 102, 10),   // Forest Green
    cv::Scalar(109, 185, 90),  // Soft Green
    cv::Scalar(132, 110, 106), // Warm Gray
    cv::Scalar(85, 158, 169),  // Cool Cyan
    cv::Scalar(26, 185, 188),  // Soft Aqua
    cv::Scalar(17, 1, 103),    // Deep Navy
    cv::Scalar(81, 144, 82),   // Sage Green
    cv::Scalar(184, 7, 92),    // Deep Pink
    cv::Scalar(155, 81, 49),   // Warm Brown
    cv::Scalar(69, 177, 179),  // Light Turquoise
    cv::Scalar(158, 187, 93),  // Pale Green
    cv::Scalar(73, 39, 13),    // Chocolate Brown
    cv::Scalar(60, 50, 12),    // Dark Olive
    cv::Scalar(33, 179, 16),   // Bright Green
    cv::Scalar(113, 129, 39),  // Olive Green
    cv::Scalar(133, 80, 164),  // Soft Purple
    cv::Scalar(114, 122, 83),  // Pale Olive
    cv::Scalar(172, 81, 99),   // Muted Rose
    cv::Scalar(104, 56, 95),   // Soft Plum
    cv::Scalar(86, 84, 37),    // Faded Olive
    cv::Scalar(122, 89, 14),   // Dull Yellow
    cv::Scalar(65, 7, 80),     // Deep Purple
    cv::Scalar(42, 35, 21),    // Dark Brown
    cv::Scalar(121, 8, 13),    // Deep Red
    cv::Scalar(28, 92, 142),   // Blue Cyan
    cv::Scalar(33, 118, 45),   // Green Cyan
    cv::Scalar(30, 118, 105),  // Teal
    cv::Scalar(124, 185, 7),   // Soft Lime
    cv::Scalar(146, 34, 46),   // Warm Red
    cv::Scalar(169, 184, 105), // Pale Yellow
    cv::Scalar(5, 18, 22),     // Dark Cyan
    cv::Scalar(73, 71, 147),   // Muted Blue
    cv::Scalar(91, 64, 181),   // Soft Violet
    cv::Scalar(184, 39, 31),   // Soft Coral
    cv::Scalar(33, 179, 164),  // Aqua Green
    cv::Scalar(18, 50, 96),    // Deep Navy
    cv::Scalar(165, 69, 112),  // Soft Burgundy
    cv::Scalar(63, 139, 15),   // Moss Green
    cv::Scalar(159, 191, 33),  // Light Lime
    cv::Scalar(32, 173, 182),  // Soft Cyan
    cv::Scalar(133, 113, 34),  // Mustard Yellow
    cv::Scalar(34, 135, 90),   // Teal Green
    cv::Scalar(86, 34, 53),    // Deep Wine
    cv::Scalar(190, 35, 141),  // Magenta
    cv::Scalar(8, 171, 6),     // Vibrant Green
    cv::Scalar(112, 76, 118),  // Soft Purple
    cv::Scalar(55, 60, 89),    // Muted Navy
    cv::Scalar(88, 54, 15),    // Warm Brown
    cv::Scalar(181, 75, 112),  // Soft Rose
    cv::Scalar(38, 147, 42),   // Forest Green
    cv::Scalar(63, 52, 138),   // Muted Purple
    cv::Scalar(149, 65, 128),  // Lavender Pink
    cv::Scalar(24, 103, 106),  // Deep Teal
    cv::Scalar(45, 33, 168),   // Indigo
    cv::Scalar(135, 136, 28),  // Olive Green
    cv::Scalar(108, 91, 86),   // Warm Taupe
    cv::Scalar(76, 11, 52),    // Deep Plum
    cv::Scalar(189, 6, 142),   // Vibrant Pink
    cv::Scalar(168, 81, 57),   // Burnt Orange
    cv::Scalar(148, 19, 55),   // Crimson
    cv::Scalar(89, 101, 182),  // Soft Blue
    cv::Scalar(179, 65, 44),   // Warm Red
    cv::Scalar(26, 33, 1),     // Dark Olive
    cv::Scalar(26, 164, 122),  // Aqua Green
    cv::Scalar(134, 63, 70),   // Soft Maroon
    cv::Scalar(82, 106, 137),  // Cool Blue
    cv::Scalar(52, 118, 120),  // Soft Teal
    cv::Scalar(42, 74, 129),   // Soft Navy
    cv::Scalar(112, 147, 182), // Pale Blue
    cv::Scalar(50, 157, 22),   // Bright Green
    cv::Scalar(20, 50, 56),    // Dark Cyan
    cv::Scalar(177, 22, 2),    // Dark Red
    cv::Scalar(106, 100, 156), // Soft Purple
    cv::Scalar(112, 116, 136), // Soft Gray
    cv::Scalar(130, 139, 119), // Pale Olive
    cv::Scalar(34, 139, 31),   // Green
    cv::Scalar(127, 6, 66),    // Deep Rose
    cv::Scalar(2, 39, 62),     // Deep Blue
    cv::Scalar(180, 99, 49),   // Soft Orange
    cv::Scalar(155, 119, 49),  // Pale Brown
    cv::Scalar(183, 50, 153),  // Soft Magenta
    cv::Scalar(3, 38, 125),    // Dark Blue
    cv::Scalar(143, 87, 129),  // Soft Purple
    cv::Scalar(40, 87, 49),    // Forest Green
    cv::Scalar(120, 62, 128),  // Pale Magenta
    cv::Scalar(148, 85, 73),   // Warm Peach
    cv::Scalar(118, 144, 28),  // Soft Lime
    cv::Scalar(24, 9, 29),     // Deep Brown
    cv::Scalar(108, 45, 175),  // Vibrant Violet
    cv::Scalar(64, 175, 81),   // Soft Green
    cv::Scalar(157, 19, 178),  // Vibrant Magenta
    cv::Scalar(190, 188, 74),  // Soft Yellow
    cv::Scalar(2, 114, 18),    // Deep Green
    cv::Scalar(96, 128, 62),   // Olive Green
    cv::Scalar(150, 3, 21),    // Deep Red
    cv::Scalar(95, 6, 0),      // Dark Red
    cv::Scalar(184, 20, 2),    // Warm Red
    cv::Scalar(185, 37, 122),  // Soft Pink
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

void draw_segmentation_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta) {
    if (meta->_seg_cls_map.data.empty()) return;

    const int seg_w = meta->_seg_cls_map.width;
    const int seg_h = meta->_seg_cls_map.height;
    const int total = seg_w * seg_h;
    size_t color_count = COLORS.size();

    // Build Y/U/V maps from seg class colors
    cv::Mat seg_y(seg_h, seg_w, CV_8UC1);
    cv::Mat seg_u(seg_h / 2, seg_w / 2, CV_8UC1);
    cv::Mat seg_v(seg_h / 2, seg_w / 2, CV_8UC1);

    for (int i = 0; i < total; ++i) {
        int cls = meta->_seg_cls_map.data[i];
        const cv::Scalar &c = COLORS[cls % color_count];
        YUVColor yuv = bgr_to_yuv_bt601(
            static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
        seg_y.data[i] = yuv.y;
    }
    for (int row = 0; row < seg_h / 2; ++row) {
        for (int col = 0; col < seg_w / 2; ++col) {
            int cls = meta->_seg_cls_map.data[row * 2 * seg_w + col * 2];
            const cv::Scalar &c = COLORS[cls % color_count];
            YUVColor yuv = bgr_to_yuv_bt601(
                static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
            seg_u.at<uint8_t>(row, col) = yuv.u;
            seg_v.at<uint8_t>(row, col) = yuv.v;
        }
    }

    // Resize to frame size and blend
    cv::Mat seg_y_resized, seg_u_resized, seg_v_resized;
    cv::resize(seg_y, seg_y_resized, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    cv::resize(seg_u, seg_u_resized, cv::Size(width / 2, height / 2), 0, 0, cv::INTER_LINEAR);
    cv::resize(seg_v, seg_v_resized, cv::Size(width / 2, height / 2), 0, 0, cv::INTER_LINEAR);

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::addWeighted(y_mat, 1.0, seg_y_resized, 1.0, 0.0, y_mat);

    for (int row = 0; row < height / 2; ++row) {
        memcpy(u_plane + row * stride_uv, seg_u_resized.ptr(row), width / 2);
        memcpy(v_plane + row * stride_uv, seg_v_resized.ptr(row), width / 2);
    }
}

void draw_segmentation_nv12(uint8_t *y_plane, uint8_t *uv_plane,
                            int stride_y, int stride_uv, int width, int height,
                            const DXObjectMeta *meta) {
    if (meta->_seg_cls_map.data.empty()) return;

    const int seg_w = meta->_seg_cls_map.width;
    const int seg_h = meta->_seg_cls_map.height;
    const int total = seg_w * seg_h;
    size_t color_count = COLORS.size();

    cv::Mat seg_y(seg_h, seg_w, CV_8UC1);
    cv::Mat seg_uv(seg_h / 2, seg_w / 2, CV_16UC1); // packed UV as uint16

    for (int i = 0; i < total; ++i) {
        int cls = meta->_seg_cls_map.data[i];
        const cv::Scalar &c = COLORS[cls % color_count];
        YUVColor yuv = bgr_to_yuv_bt601(
            static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
        seg_y.data[i] = yuv.y;
    }
    for (int row = 0; row < seg_h / 2; ++row) {
        for (int col = 0; col < seg_w / 2; ++col) {
            int cls = meta->_seg_cls_map.data[row * 2 * seg_w + col * 2];
            const cv::Scalar &c = COLORS[cls % color_count];
            YUVColor yuv = bgr_to_yuv_bt601(
                static_cast<uint8_t>(c[0]), static_cast<uint8_t>(c[1]), static_cast<uint8_t>(c[2]));
            seg_uv.at<uint16_t>(row, col) =
                static_cast<uint16_t>(yuv.u) | (static_cast<uint16_t>(yuv.v) << 8);
        }
    }

    cv::Mat seg_y_resized;
    cv::resize(seg_y, seg_y_resized, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    cv::Mat seg_uv_resized;
    cv::resize(seg_uv, seg_uv_resized, cv::Size(width / 2, height / 2), 0, 0, cv::INTER_NEAREST);

    cv::Mat y_mat(height, width, CV_8UC1, y_plane, stride_y);
    cv::addWeighted(y_mat, 1.0, seg_y_resized, 1.0, 0.0, y_mat);

    for (int row = 0; row < height / 2; ++row)
        memcpy(uv_plane + row * stride_uv, seg_uv_resized.ptr(row), width);
}

void draw_object_meta_yuv_i420(uint8_t *y_plane, uint8_t *u_plane, uint8_t *v_plane,
                               int stride_y, int stride_uv, int width, int height,
                               const DXObjectMeta *meta, float scale_x, float scale_y) {
    // Draw segmentation, pose keypoints and face landmarks first — they don't need _box
    draw_segmentation_i420(y_plane, u_plane, v_plane, stride_y, stride_uv, width, height, meta);
    draw_keypoints_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);
    draw_face_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);

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
    // Draw segmentation, pose keypoints and face landmarks first — they don't need _box
    draw_segmentation_nv12(y_plane, uv_plane, stride_y, stride_uv, width, height, meta);
    draw_keypoints_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);
    draw_face_y_plane(y_plane, stride_y, width, height, meta, scale_x, scale_y);

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
