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
    double font_scale = 0.00075 * std::min(img.cols, img.rows);
    int baseline = 0;
    auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale, 1, &baseline);
    auto x = int(meta->_box[0] / sx);
    auto y = int(meta->_box[1] / sy);
    auto x2 = int(meta->_box[2] / sx);
    auto y2 = int(meta->_box[3] / sy);
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
