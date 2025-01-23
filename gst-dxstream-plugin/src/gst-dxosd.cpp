#include "gst-dxosd.hpp"
#include "format_convert.hpp"
#include "gst-dxmeta.hpp"
#include "utils.hpp"
#include <cmath>
#include <gst/video/video.h>
#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>

static std::vector<std::vector<int>> skeleton = {
    {15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
    {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
    {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6},
};

static std::vector<cv::Scalar> pose_limb_color = {
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

static std::vector<cv::Scalar> pose_kpt_color = {
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

std::vector<cv::Scalar> COLORS = {
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

GST_DEBUG_CATEGORY_STATIC(gst_dxosd_debug_category);
#define GST_CAT_DEFAULT gst_dxosd_debug_category

static GstFlowReturn gst_dxosd_transform_ip(GstBaseTransform *trans,
                                            GstBuffer *buf);
static gboolean gst_dxosd_start(GstBaseTransform *trans);
static gboolean gst_dxosd_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxOsd, gst_dxosd, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "gst-dxosd", 0,
                            "debug category for gst-dxosd element"))

static GstElementClass *parent_class = NULL;

static GstStateChangeReturn dxosd_change_state(GstElement *element,
                                               GstStateChange transition) {
    GstDxOsd *self = GST_DXOSD(element);
    GST_INFO_OBJECT(self, "Attempting to change state");
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static void dxosd_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxosd_class_init(GstDxOsdClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "dxosd", 0,
                            "DXOsd plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = dxosd_dispose;

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "DXOsd", "Generic",
                                          "Draw inference results",
                                          "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxosd_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxosd_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxosd_transform_ip);

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxosd_change_state;
}

static void gst_dxosd_init(GstDxOsd *self) { GST_DEBUG_OBJECT(self, "init"); }

static gboolean gst_dxosd_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    return TRUE;
}

static gboolean gst_dxosd_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

void draw_object_meta(cv::Mat &img, DXObjectMeta *obj_meta) {

    if (obj_meta->_seg_cls_map.data != nullptr) {
        cv::Mat seg_vis_map(obj_meta->_seg_cls_map.height,
                            obj_meta->_seg_cls_map.width, CV_8UC3);
        for (int h = 0; h < obj_meta->_seg_cls_map.height; h++) {
            for (int w = 0; w < obj_meta->_seg_cls_map.width; w++) {
                int cls = obj_meta->_seg_cls_map
                              .data[obj_meta->_seg_cls_map.width * h + w];
                cv::Scalar cls_color = COLORS[cls % COLORS.size()];
                seg_vis_map.at<cv::Vec3b>(h, w) =
                    cv::Vec3b(cls_color[0], cls_color[1], cls_color[2]);
            }
        }

        cv::Mat add;
        cv::resize(seg_vis_map, add, cv::Size(img.cols, img.rows), 0, 0,
                   cv::INTER_LINEAR);
        cv::addWeighted(img, 1.0, add, 1.0, 0.0, img);
    }

    if (obj_meta->_keypoints.size() > 0) {

        std::vector<cv::Point> points;
        for (int k = 0; k < 17; k++) {
            float kx = obj_meta->_keypoints[k * 3 + 0];
            float ky = obj_meta->_keypoints[k * 3 + 1];
            float ks = obj_meta->_keypoints[k * 3 + 2];
            if (ks > 0.5) {
                points.emplace_back(cv::Point(kx, ky));
            } else {
                points.emplace_back(cv::Point(-1, -1));
            }
        }

        for (int index = 0; index < (int)skeleton.size(); index++) {
            auto pp = skeleton[index];
            auto kp0 = points[pp[0]];
            auto kp1 = points[pp[1]];
            if (kp0.x >= 0 && kp1.x >= 0)
                cv::line(img, kp0, kp1, pose_limb_color[index], 2, cv::LINE_AA);
        }

        for (int index = 0; index < (int)points.size(); index++) {
            cv::circle(img, points[index], 3, pose_kpt_color[index], -1,
                       cv::LINE_AA);
        }
    }

    for (auto &landmark : obj_meta->_face_landmarks) {
        cv::circle(img, cv::Point(landmark._x, landmark._y), 3,
                   cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
    }

    if (obj_meta->_face_box[0] || obj_meta->_face_box[1] ||
        obj_meta->_face_box[2] || obj_meta->_face_box[3]) {
        cv::rectangle(img,
                      cv::Rect(cv::Point(int(obj_meta->_face_box[0]),
                                         int(obj_meta->_face_box[1])),
                               cv::Point(int(obj_meta->_face_box[2]),
                                         int(obj_meta->_face_box[3]))),
                      cv::Scalar(255, 0, 0), 2);
    }

    // visualize track id
    if (obj_meta->_box[2] - obj_meta->_box[0] <= 0 ||
        obj_meta->_box[3] - obj_meta->_box[1] <= 0) {
        return;
    }
    if (obj_meta->_track_id != -1) {
        int baseline = 0;
        char text[50];
        double font_scale = 0.00075 * std::min(img.cols, img.rows);
        sprintf(text, "%d", obj_meta->_track_id);
        auto textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX,
                                        font_scale, 1, &baseline);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0]), int(obj_meta->_box[1])),
                     cv::Point(int(obj_meta->_box[2]), int(obj_meta->_box[3]))),
            COLORS[(obj_meta->_track_id) % COLORS.size()], 2);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0]),
                               int(obj_meta->_box[1] - textSize.height)),
                     cv::Point(int(obj_meta->_box[0] + textSize.width),
                               int(obj_meta->_box[1]))),
            COLORS[(obj_meta->_track_id) % COLORS.size()], cv::FILLED);

        cv::putText(img, text,
                    cv::Point(int(obj_meta->_box[0]), int(obj_meta->_box[1])),
                    cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(255, 255, 255));
    } else if (obj_meta->_label != -1) {
        int baseline = 0;
        char text[50];
        double font_scale = 0.00075 * std::min(img.cols, img.rows);
        sprintf(text, "%s=%.2f", obj_meta->_label_name->str,
                (double)obj_meta->_confidence);
        auto textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX,
                                        font_scale, 1, &baseline);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0]), int(obj_meta->_box[1])),
                     cv::Point(int(obj_meta->_box[2]), int(obj_meta->_box[3]))),
            COLORS[obj_meta->_label % COLORS.size()], 2);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0]),
                               int(obj_meta->_box[1] - textSize.height)),
                     cv::Point(int(obj_meta->_box[0] + textSize.width),
                               int(obj_meta->_box[1]))),
            COLORS[obj_meta->_label % COLORS.size()], cv::FILLED);

        cv::putText(img, text,
                    cv::Point(int(obj_meta->_box[0]), int(obj_meta->_box[1])),
                    cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(255, 255, 255));
    }
}

void draw(DXFrameMeta *frame_meta) {
    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);

    uint8_t *convert_frame =
        CvtColor(frame_meta->_buf, frame_meta->_width, frame_meta->_height,
                 frame_meta->_format, "RGB");
    cv::Mat surface = cv::Mat(frame_meta->_height, frame_meta->_width, CV_8UC3,
                              convert_frame);
    for (int i = 0; i < object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        draw_object_meta(surface, obj_meta);
    }
    SurfaceToOrigin(frame_meta, convert_frame);
    surface.release();
    free(convert_frame);
}

static GstFlowReturn gst_dxosd_transform_ip(GstBaseTransform *trans,
                                            GstBuffer *buf) {
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        g_error("No DXFrameMeta in GstBuffer \n");
    }
    draw(frame_meta);

    return GST_FLOW_OK;
}
