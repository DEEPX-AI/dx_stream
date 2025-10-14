#include "gst-dxosd.hpp"
#include "gst-dxmeta.hpp"

#include <cmath>
#include <cstdio>
#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>

static const std::vector<std::vector<int>> skeleton = {
    {15, 13}, {13, 11}, {16, 14}, {14, 12}, {11, 12}, {5, 11}, {6, 12},
    {5, 6},   {5, 7},   {6, 8},   {7, 9},   {8, 10},  {1, 2},  {0, 1},
    {0, 2},   {1, 3},   {2, 4},   {3, 5},   {4, 6},
};

static const std::vector<cv::Scalar> pose_limb_color = {
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

static const std::vector<cv::Scalar> pose_kpt_color = {
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

static const std::vector<cv::Scalar> COLORS = {
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

enum { PROP_0, PROP_WIDTH, PROP_HEIGHT, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxosd_debug_category);
#define GST_CAT_DEFAULT gst_dxosd_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, "
                    "format = (string){ RGB, I420, NV12 }, "
                    "width = [ 1, 16384 ], "
                    "height = [ 1, 16384 ], "
                    "framerate = [ 0/1, 16384/1 ]"));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw, "
                                            "format = (string){ BGR }, "
                                            "width = [ 1, 16384 ], "
                                            "height = [ 1, 16384 ], "
                                            "framerate = [ 0/1, 16384/1 ]"));

static GstFlowReturn gst_dxosd_chain(GstPad *pad, GstObject *parent,
                                     GstBuffer *buf);

G_DEFINE_TYPE(GstDxOsd, gst_dxosd, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;

static void dxosd_set_property(GObject *object, guint property_id,
                               const GValue *value, GParamSpec *pspec) {
    GstDxOsd *self = GST_DXOSD(object);
    switch (property_id) {
    case PROP_WIDTH:
        self->_width = g_value_get_int(value);
        break;
    case PROP_HEIGHT:
        self->_height = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxosd_get_property(GObject *object, guint property_id,
                               GValue *value, GParamSpec *pspec) {
    GstDxOsd *self = GST_DXOSD(object);

    switch (property_id) {
    case PROP_WIDTH:
        g_value_set_int(value, self->_width);
        break;
    case PROP_HEIGHT:
        g_value_set_int(value, self->_height);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

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
#ifdef HAVE_LIBRGA
#else
    GstDxOsd *self = GST_DXOSD(object);
    for (std::map<int, uint8_t *>::iterator it = self->_resized_frame.begin();
         it != self->_resized_frame.end(); ++it) {
        if (it->second != nullptr) {
            free(it->second);
        }
    }
    self->_resized_frame.clear();
#endif
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxosd_class_init(GstDxOsdClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "dxosd", 0,
                            "DXOsd plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxosd_set_property;
    gobject_class->get_property = dxosd_get_property;
    gobject_class->dispose = dxosd_dispose;

    static GParamSpec *obj_properties[N_PROPERTIES] = {
        nullptr,
    };

    obj_properties[PROP_WIDTH] = g_param_spec_int(
        "width", "Width", "Sets the width of each tile in the grid.", 0, 10000,
        640, G_PARAM_READWRITE);

    obj_properties[PROP_HEIGHT] = g_param_spec_int(
        "height", "Height", "Sets the height of each tile in the grid.", 0,
        10000, 360, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "DXOsd", "Generic",
                                          "Draw inference results",
                                          "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxosd_change_state;
}

static gboolean parse_input_caps(GstDxOsd *self, GstCaps *incaps) {
    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self,
                         "Failed to parse input caps or format not supported");
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Input format: %s, %dx%d",
                    gst_video_format_to_string(self->_input_info.finfo->format),
                    GST_VIDEO_INFO_WIDTH(&self->_input_info),
                    GST_VIDEO_INFO_HEIGHT(&self->_input_info));
    return TRUE;
}

static GstCaps *create_and_fixate_output_caps(GstDxOsd *self, GstCaps *incaps) {
    if (self->_width <= 0 || self->_height <= 0) {
        GST_ERROR_OBJECT(self, "Output width/height property not set!");
        return nullptr;
    }

    GstCaps *outcaps = gst_caps_copy(incaps);
    gst_caps_set_simple(outcaps, "format", G_TYPE_STRING, "BGR", "width",
                        G_TYPE_INT, self->_width, "height", G_TYPE_INT,
                        self->_height, nullptr);

    GstCaps *fixed = gst_caps_fixate(outcaps);
    if (fixed != outcaps)
        gst_caps_unref(outcaps);

    if (!fixed) {
        GST_ERROR_OBJECT(self, "Failed to fixate generated output caps!");
        return nullptr;
    }

    GST_INFO_OBJECT(self, "Fixated output CAPS %" GST_PTR_FORMAT, fixed);
    return fixed;
}

static gboolean parse_output_info(GstDxOsd *self) {
    if (!self->_output_caps || !gst_caps_is_fixed(self->_output_caps)) {
        GST_ERROR_OBJECT(
            self, "Output caps are null or not fixed before parsing info!");
        return FALSE;
    }

    if (!gst_video_info_from_caps(&self->_output_info, self->_output_caps)) {
        GST_ERROR_OBJECT(self, "Failed to parse generated output caps!");
        return FALSE;
    }

    GST_INFO_OBJECT(
        self, "Output info: %s, %dx%d, size: %" G_GSIZE_FORMAT,
        gst_video_format_to_string(self->_output_info.finfo->format),
        GST_VIDEO_INFO_WIDTH(&self->_output_info),
        GST_VIDEO_INFO_HEIGHT(&self->_output_info), self->_output_info.size);
    return TRUE;
}

static gboolean push_caps_event(GstDxOsd *self) {
    GST_INFO_OBJECT(
        self, "Pushing new CAPS %" GST_PTR_FORMAT " downstream via src pad",
        self->_output_caps);
    GstEvent *new_caps_event = gst_event_new_caps(self->_output_caps);
    if (!gst_pad_push_event(self->_srcpad, new_caps_event)) {
        GST_ERROR_OBJECT(self, "Failed to push new CAPS event downstream.");
        return FALSE;
    }
    GST_INFO_OBJECT(self, "Successfully pushed new CAPS event downstream.");
    return TRUE;
}

static gboolean gst_dxosd_sink_event(GstPad *pad, GstObject *parent,
                                     GstEvent *event) {
    GstDxOsd *self = GST_DXOSD(parent);

    GST_INFO_OBJECT(self, "Received event [%s] on sink pad", GST_EVENT_TYPE_NAME(event));

    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        GstCaps *incaps = nullptr;
        GstCaps *fixed = nullptr; // <-- 선언 위치 이동
        gst_event_parse_caps(event, &incaps);
        GST_INFO_OBJECT(self, "Received CAPS %" GST_PTR_FORMAT, incaps);

        if (!parse_input_caps(self, incaps))
            goto fail;

        fixed = create_and_fixate_output_caps(self, incaps);
        if (!fixed)
            goto fail;

        if (self->_output_caps)
            gst_caps_unref(self->_output_caps);
        self->_output_caps = fixed;

        if (!parse_output_info(self))
            goto fail;

        if (!push_caps_event(self))
            goto fail;

        gst_event_unref(event);
        return TRUE;

    fail:
        if (self->_output_caps) {
            gst_caps_unref(self->_output_caps);
            self->_output_caps = nullptr;
        }
        gst_event_unref(event);
        return FALSE;
    }
    return gst_pad_push_event(self->_srcpad, event);
}

static gboolean gst_dxosd_src_query(GstPad *pad, GstObject *parent,
                                    GstQuery *query) {
    GstDxOsd *self = GST_DXOSD(parent);
    GstCaps *filter_caps, *result_caps, *possible_caps;
    gboolean ret = TRUE;

    GST_DEBUG_OBJECT(self, "Handling query %s", GST_QUERY_TYPE_NAME(query));

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_CAPS:
        if (!self->_output_caps) {
            GST_WARNING_OBJECT(self, "Output caps not negotiated yet, cannot "
                                     "answer caps query precisely.");
            return gst_pad_query_default(pad, parent, query);
        }

        gst_query_parse_caps(query, &filter_caps);
        GST_INFO_OBJECT(self,
                        "Answering CAPS query, filter is %" GST_PTR_FORMAT,
                        filter_caps);
        GST_INFO_OBJECT(self, "My output caps are %" GST_PTR_FORMAT,
                        self->_output_caps);

        possible_caps = gst_caps_copy(self->_output_caps);

        if (filter_caps) {
            result_caps = gst_caps_intersect_full(filter_caps, possible_caps,
                                                  GST_CAPS_INTERSECT_FIRST);
            gst_caps_unref(possible_caps);
            GST_INFO_OBJECT(self, "Intersection result: %" GST_PTR_FORMAT,
                            result_caps);
        } else {
            result_caps = possible_caps;
        }

        gst_query_set_caps_result(query, result_caps);
        gst_caps_unref(result_caps);
        ret = TRUE;
        break;

    case GST_QUERY_ALLOCATION:
        ret = gst_pad_query_default(pad, parent, query);
        break;

    default:
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    return ret;
}

static gboolean gst_dxosd_src_event(GstPad *pad, GstObject *parent,
                                    GstEvent *event) {
    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxosd_init(GstDxOsd *self) {
    GST_DEBUG_OBJECT(self, "init");

    self->_sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    gst_pad_set_chain_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxosd_chain));
    gst_pad_set_event_function(self->_sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxosd_sink_event));
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    self->_srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxosd_src_event));
    gst_pad_set_query_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxosd_src_query));
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_width = 640;
    self->_height = 360;

    self->_output_caps = nullptr;
#ifdef HAVE_LIBRGA
#else
    self->_resized_frame = std::map<int, uint8_t *>();
#endif
}

void draw_segmentation(cv::Mat &img, const DXObjectMeta *meta) {
    if (meta->_seg_cls_map.data.empty())
        return;

    cv::Mat seg_vis(meta->_seg_cls_map.height, meta->_seg_cls_map.width,
                    CV_8UC3);
    for (int h = 0; h < meta->_seg_cls_map.height; ++h) {
        for (int w = 0; w < meta->_seg_cls_map.width; ++w) {
            int cls = meta->_seg_cls_map.data[meta->_seg_cls_map.width * h + w];
            cv::Scalar color = COLORS[cls % COLORS.size()];
            seg_vis.at<cv::Vec3b>(h, w) =
                cv::Vec3b(color[0], color[1], color[2]);
        }
    }

    cv::Mat resized;
    cv::resize(seg_vis, resized, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::addWeighted(img, 1.0, resized, 1.0, 0.0, img);
}

void draw_keypoints(cv::Mat &img, const DXObjectMeta *meta, float sx,
                    float sy) {
    if (meta->_keypoints.empty())
        return;

    std::vector<cv::Point> pts;
    for (int i = 0; i < 17; ++i) {
        float x = meta->_keypoints[i * 3] / sx;
        float y = meta->_keypoints[i * 3 + 1] / sy;
        float s = meta->_keypoints[i * 3 + 2];
        pts.emplace_back((s > 0.5f) ? cv::Point(x, y) : cv::Point(-1, -1));
    }

    for (size_t i = 0; i < skeleton.size(); ++i) {
        auto &p = skeleton[i];
        if (pts[p[0]].x >= 0 && pts[p[1]].x >= 0)
            cv::line(img, pts[p[0]], pts[p[1]], pose_limb_color[i], 2,
                     cv::LINE_AA);
    }

    for (size_t i = 0; i < pts.size(); ++i)
        cv::circle(img, pts[i], 3, pose_kpt_color[i], -1, cv::LINE_AA);
}

void draw_face(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    for (const auto &lm : meta->_face_landmarks) {
        cv::circle(img, cv::Point(int(lm._x / sx), int(lm._y / sy)), 3,
                   cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
    }

    if (meta->_face_box[2] > meta->_face_box[0] &&
        meta->_face_box[3] > meta->_face_box[1]) {
        cv::rectangle(
            img,
            cv::Rect(
                cv::Point(meta->_face_box[0] / sx, meta->_face_box[1] / sy),
                cv::Point(meta->_face_box[2] / sx, meta->_face_box[3] / sy)),
            cv::Scalar(255, 0, 0), 2);
    }
}

void draw_label_or_id(cv::Mat &img, const DXObjectMeta *meta, float sx,
                      float sy) {
    if (meta->_box[2] - meta->_box[0] <= 0 ||
        meta->_box[3] - meta->_box[1] <= 0)
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
        text = cv::format("%s=%.2f", meta->_label_name->str, meta->_confidence);
        color = COLORS[label % COLORS.size()];
    }

    double font_scale = 0.00075 * std::min(img.cols, img.rows);
    int baseline = 0;
    auto text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, font_scale,
                                     1, &baseline);

    int x = int(meta->_box[0] / sx);
    int y = int(meta->_box[1] / sy);
    int x2 = int(meta->_box[2] / sx);
    int y2 = int(meta->_box[3] / sy);

    cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Point(x2, y2)), color, 2);
    cv::rectangle(img,
                  cv::Rect(cv::Point(x, y - text_size.height),
                           cv::Point(x + text_size.width, y)),
                  color, cv::FILLED);
    cv::putText(img, text, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX,
                font_scale, cv::Scalar(255, 255, 255));
}

void draw_clip(cv::Mat &img, const DXObjectMeta *meta, float sx, float sy) {
    if (meta->_confidence > 0.24 && meta->_label == -1 && meta->_box[0] == -1 && meta->_box[1] == -1 && meta->_box[2] == -1 && meta->_box[3] == -1) {
        std::string text = cv::format("%s=%.2f", meta->_label_name->str, meta->_confidence);
        
        int text_area_height = img.rows * 0.15;
        int text_area_width = img.cols * 0.9;
        int margin_x = img.cols * 0.05;
        int margin_y = img.rows * 0.02;
        
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

void draw_object_meta(cv::Mat &img, DXObjectMeta *meta, float scale_x,
                      float scale_y) {
    draw_segmentation(img, meta);
    draw_keypoints(img, meta, scale_x, scale_y);
    draw_face(img, meta, scale_x, scale_y);
    draw_label_or_id(img, meta, scale_x, scale_y);
    draw_clip(img, meta, scale_x, scale_y);
}

bool calculate_strides(int w, int h, int wa, int ha, int *ws, int *hs) {
    if (!ws || !hs || w <= 0 || h <= 0 || (h % 2 != 0) || wa < 0 || ha < 0) {
        return false;
    }
    *ws = (wa <= 1) ? w : (((w + wa - 1) / wa) * wa);
    *hs = (ha <= 1) ? h : (((h + ha - 1) / ha) * ha);

    return true;
}

#ifdef HAVE_LIBRGA
void draw_rga(GstDxOsd *self, GstBuffer *buf, GstBuffer *outbuffer) {
    if (self->_width % 16 != 0) {
        GST_ERROR_OBJECT(self, "DXOSD output width must be 16 aligned ! \n");
        return;
    }

    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to get DXFrameMeta \n");
        return;
    }

    if (g_strcmp0(frame_meta->_format, "NV12") != 0) {
        GST_ERROR_OBJECT(self, "not supported format (use NV12)! \n");
        return;
    }

    GstMapInfo input_map, output_map;
    if (!gst_buffer_map(buf, &input_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (input)\n");
        return;
    }

    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    if ((float)frame_meta->_width / self->_width <= 0.125 ||
        (float)frame_meta->_width / self->_width >= 8 ||
        (float)frame_meta->_height / self->_height <= 0.125 ||
        (float)frame_meta->_height / self->_height >= 8) {
        GST_ERROR_OBJECT(self, "DX OSD : scale check error, scale limit[1/8 ~ 8] \n");
        return;
    }

    if (frame_meta->_width < 68 || frame_meta->_height < 2 ||
        frame_meta->_width > 8176 || frame_meta->_height > 8176) {
        GST_ERROR_OBJECT(self, "DX OSD : resolution check error, input range[68x2 ~ "
                "8176x8176] \n");
        return;
    }

    if (self->_width < 68 || self->_height < 2 || self->_width > 8128 ||
        self->_height > 8128) {
        GST_ERROR_OBJECT(self, "DX OSD : resolution check error, output range[68x2 ~ "
                "8128x8128] \n");
        return;
    }

    int wstride, hstride;
    calculate_strides(frame_meta->_width, frame_meta->_height, 16, 16, &wstride,
                      &hstride);
    rga_buffer_t src_img = wrapbuffer_virtualaddr(
        reinterpret_cast<void *>(input_map.data), frame_meta->_width,
        frame_meta->_height, RK_FORMAT_YCbCr_420_SP, self->_input_info.stride[0], hstride);
    rga_buffer_t dst_img =
        wrapbuffer_virtualaddr(reinterpret_cast<void *>(output_map.data),
                               self->_width, self->_height, RK_FORMAT_BGR_888);

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret) {
        GST_ERROR_OBJECT(self, "check error: %d - %s\n", ret,
                         imStrError((IM_STATUS)ret));
        return;
    }

    ret = improcess(src_img, dst_img, {}, {}, {}, {}, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        GST_ERROR_OBJECT(self, "RGA resize (imresize) failed: %d - %s\n", ret,
                         imStrError((IM_STATUS)ret));
        return;
    }

    cv::Mat surface =
        cv::Mat(self->_height, self->_width, CV_8UC3, output_map.data);

    float scale_factor_x = (float)frame_meta->_width / self->_width;
    float scale_factor_y = (float)frame_meta->_height / self->_height;

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);
    for (size_t i = 0; i < (size_t)object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        draw_object_meta(surface, obj_meta, scale_factor_x, scale_factor_y);
    }

    gst_buffer_unmap(buf, &input_map);
    gst_buffer_unmap(outbuffer, &output_map);
}
#else
void draw(GstDxOsd *self, GstBuffer *buf, GstBuffer *outbuffer) {
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to get DXFrameMeta \n");
        return;
    }

    GstMapInfo output_map;

    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    auto iter = self->_resized_frame.find(frame_meta->_stream_id);
    if (iter == self->_resized_frame.end()) {
        self->_resized_frame[frame_meta->_stream_id] = nullptr;
    }

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);

    bool resized = false;
    if (frame_meta->_width != self->_width || frame_meta->_height != self->_height) {
        Resize(buf, &self->_input_info, &self->_resized_frame[frame_meta->_stream_id],
           frame_meta->_width, frame_meta->_height, self->_width, self->_height,
           frame_meta->_format);
        resized = true;
    }

    if (resized) {
        CvtColor(self->_resized_frame[frame_meta->_stream_id], &output_map.data,
             self->_width, self->_height, frame_meta->_format, "BGR");
    } else {
        CvtColor(buf, &self->_input_info, &output_map.data,
             self->_width, self->_height, frame_meta->_format, "BGR");
    }

    cv::Mat surface =
        cv::Mat(self->_height, self->_width, CV_8UC3, output_map.data);

    float scale_factor_x = (float)frame_meta->_width / self->_width;
    float scale_factor_y = (float)frame_meta->_height / self->_height;

    for (size_t i = 0; i < (size_t)object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        draw_object_meta(surface, obj_meta, scale_factor_x, scale_factor_y);
    }

    gst_buffer_unmap(outbuffer, &output_map);
}
#endif

static GstFlowReturn gst_dxosd_chain(GstPad *pad, GstObject *parent,
                                     GstBuffer *buf) {
    GstDxOsd *self = GST_DXOSD(parent);

    GstBuffer *outbuf = nullptr;
    GstFlowReturn ret = GST_FLOW_OK;

    gsize out_size = self->_output_info.size;
    outbuf = gst_buffer_new_allocate(nullptr, out_size, nullptr);

    if (!gst_buffer_copy_into(outbuf, buf,
                              (GstBufferCopyFlags)(GST_BUFFER_COPY_FLAGS |
                                                   GST_BUFFER_COPY_TIMESTAMPS),
                              0, -1)) {
        GST_WARNING_OBJECT(self, "Failed to copy buffer metadata");
    }

    GST_BUFFER_OFFSET(outbuf) = GST_BUFFER_OFFSET(buf);
    GST_BUFFER_OFFSET_END(outbuf) = GST_BUFFER_OFFSET_END(buf);

#ifdef HAVE_LIBRGA
    draw_rga(self, buf, outbuf);
#else
    draw(self, buf, outbuf);
#endif

    gst_buffer_unref(buf);
    // GST_INFO_OBJECT(self, "[%d] Pushing buffer to src pad PTS: %" GST_TIME_FORMAT, frame_meta->_stream_id, GST_TIME_ARGS(GST_BUFFER_PTS(outbuf)));
    ret = gst_pad_push(self->_srcpad, outbuf);
    if (ret != GST_FLOW_OK) {
        GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
    }
    return ret;
}