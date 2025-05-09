#include "gst-dxosd.hpp"
#include "gst-dxmeta.hpp"

#include <cmath>
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

enum { PROP_0, PROP_WIDTH, PROP_HEIGHT, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

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

static GstElementClass *parent_class = NULL;

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

static gboolean gst_dxosd_sink_event(GstPad *pad, GstObject *parent,
                                     GstEvent *event) {
    GstDxOsd *self = GST_DXOSD(parent);
    gboolean ret = TRUE;

    GST_DEBUG_OBJECT(self, "Received event %s on sink pad",
                     GST_EVENT_TYPE_NAME(event));

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CAPS: {
        GstCaps *incaps = NULL;
        GstCaps *outcaps = NULL;
        GstCaps *fixed_outcaps = NULL;
        gboolean success = TRUE;

        gst_event_parse_caps(event, &incaps);
        GST_INFO_OBJECT(self, "Received CAPS %" GST_PTR_FORMAT, incaps);

        if (success && !gst_video_info_from_caps(&self->_input_info, incaps)) {
            GST_ERROR_OBJECT(
                self, "Failed to parse input caps or format not supported");
            success = FALSE;
        }
        if (success) {
            GST_INFO_OBJECT(
                self, "Input format: %s, %dx%d",
                gst_video_format_to_string(self->_input_info.finfo->format),
                GST_VIDEO_INFO_WIDTH(&self->_input_info),
                GST_VIDEO_INFO_HEIGHT(&self->_input_info));
        }
        if (success && (self->_width <= 0 || self->_height <= 0)) {
            GST_ERROR_OBJECT(self, "Output width/height property not set!");
            success = FALSE;
        }
        if (success) {
            outcaps = gst_caps_copy(incaps);
            gst_caps_set_simple(outcaps, "format", G_TYPE_STRING, "BGR",
                                "width", G_TYPE_INT, self->_width, "height",
                                G_TYPE_INT, self->_height, NULL);
        }
        if (success) {
            fixed_outcaps = gst_caps_fixate(outcaps);
            if (fixed_outcaps != outcaps) {
                gst_caps_unref(outcaps);
                outcaps = NULL;
            }
            if (!fixed_outcaps) {
                GST_ERROR_OBJECT(self,
                                 "Failed to fixate generated output caps!");
                success = FALSE;
            } else {
                GST_INFO_OBJECT(self, "Fixated output CAPS %" GST_PTR_FORMAT,
                                fixed_outcaps);
                if (self->_output_caps) {
                    gst_caps_unref(self->_output_caps);
                }
                self->_output_caps = fixed_outcaps;
            }
        } else if (outcaps) {
            gst_caps_unref(outcaps);
            outcaps = NULL;
        }
        if (success) {
            if (!self->_output_caps || !gst_caps_is_fixed(self->_output_caps)) {
                GST_ERROR_OBJECT(
                    self,
                    "Output caps are null or not fixed before parsing info!");
                success = FALSE;
            } else if (!gst_video_info_from_caps(&self->_output_info,
                                                 self->_output_caps)) {
                GST_ERROR_OBJECT(self,
                                 "Failed to parse generated output caps!");
                success = FALSE;
            }

            if (success) {
                GST_INFO_OBJECT(
                    self, "Output info: %s, %dx%d, size: %" G_GSIZE_FORMAT,
                    gst_video_format_to_string(
                        self->_output_info.finfo->format),
                    GST_VIDEO_INFO_WIDTH(&self->_output_info),
                    GST_VIDEO_INFO_HEIGHT(&self->_output_info),
                    self->_output_info.size);
            } else {
                if (self->_output_caps) {
                    gst_caps_unref(self->_output_caps);
                    self->_output_caps = NULL;
                }
            }
        }
        if (success) {
            GST_INFO_OBJECT(self,
                            "Pushing new CAPS %" GST_PTR_FORMAT
                            " downstream via src pad",
                            self->_output_caps);
            GstEvent *new_caps_event = gst_event_new_caps(self->_output_caps);
            if (!gst_pad_push_event(self->_srcpad, new_caps_event)) {
                GST_ERROR_OBJECT(self,
                                 "Failed to push new CAPS event downstream.");
                success = FALSE;
            } else {
                GST_INFO_OBJECT(
                    self, "Successfully pushed new CAPS event downstream.");
            }
        }
        gst_event_unref(event);
        ret = success;
        break;
    }
    default:
        GST_DEBUG_OBJECT(self, "Forwarding event %s to default handler",
                         GST_EVENT_TYPE_NAME(event));
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }

    return ret;
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

    self->_output_caps = NULL;
#ifdef HAVE_LIBRGA
#else
    self->_resized_frame = std::map<int, uint8_t *>();
#endif
}

void draw_object_meta(cv::Mat &img, DXObjectMeta *obj_meta,
                      float scale_factor_x, float scale_factor_y) {
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
            float kx = obj_meta->_keypoints[k * 3 + 0] / scale_factor_x;
            float ky = obj_meta->_keypoints[k * 3 + 1] / scale_factor_y;
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
        cv::circle(img,
                   cv::Point(int(landmark._x / scale_factor_x),
                             int(landmark._y / scale_factor_y)),
                   3, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
    }

    if (obj_meta->_face_box[0] || obj_meta->_face_box[1] ||
        obj_meta->_face_box[2] || obj_meta->_face_box[3]) {
        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_face_box[0] / scale_factor_x),
                               int(obj_meta->_face_box[1] / scale_factor_y)),
                     cv::Point(int(obj_meta->_face_box[2] / scale_factor_x),
                               int(obj_meta->_face_box[3] / scale_factor_y))),
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
            cv::Rect(cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                               int(obj_meta->_box[1] / scale_factor_y)),
                     cv::Point(int(obj_meta->_box[2] / scale_factor_x),
                               int(obj_meta->_box[3] / scale_factor_y))),
            COLORS[(obj_meta->_track_id) % COLORS.size()], 2);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                               int((obj_meta->_box[1] / scale_factor_y) -
                                   textSize.height)),
                     cv::Point(int((obj_meta->_box[0] / scale_factor_x) +
                                   textSize.width),
                               int(obj_meta->_box[1] / scale_factor_y))),
            COLORS[(obj_meta->_track_id) % COLORS.size()], cv::FILLED);

        cv::putText(img, text,
                    cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                              int(obj_meta->_box[1] / scale_factor_y)),
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
            cv::Rect(cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                               int(obj_meta->_box[1] / scale_factor_y)),
                     cv::Point(int(obj_meta->_box[2] / scale_factor_x),
                               int(obj_meta->_box[3] / scale_factor_y))),
            COLORS[obj_meta->_label % COLORS.size()], 2);

        cv::rectangle(
            img,
            cv::Rect(cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                               int((obj_meta->_box[1] / scale_factor_y) -
                                   textSize.height)),
                     cv::Point(int((obj_meta->_box[0] / scale_factor_x) +
                                   textSize.width),
                               int(obj_meta->_box[1] / scale_factor_y))),
            COLORS[obj_meta->_label % COLORS.size()], cv::FILLED);

        cv::putText(img, text,
                    cv::Point(int(obj_meta->_box[0] / scale_factor_x),
                              int(obj_meta->_box[1] / scale_factor_y)),
                    cv::FONT_HERSHEY_SIMPLEX, font_scale,
                    cv::Scalar(255, 255, 255));
    }
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
void draw_rga(GstDxOsd *self, DXFrameMeta *frame_meta, GstBuffer *outbuffer) {
    GstVideoMeta *meta = gst_buffer_get_video_meta(frame_meta->_buf);
    if (!meta) {
        g_error("ERROR : video meta is nullptr! \n");
        return;
    }

    if (self->_width % 16 != 0) {
        g_error("ERROR : DXOSD output W stride must be 16 aligned ! \n");
        return;
    }

    if (g_strcmp0(frame_meta->_format, "NV12") != 0) {
        g_error("ERROR : not supported format (use NV12)! \n");
        return;
    }

    GstMapInfo input_map, output_map;
    if (!gst_buffer_map(frame_meta->_buf, &input_map, GST_MAP_READ)) {
        g_error("ERROR : DXOSD Failed to map GstBuffer (input)\n");
        return;
    }

    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        g_error("ERROR : DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    if ((float)frame_meta->_width / self->_width <= 0.125 ||
        (float)frame_meta->_width / self->_width >= 8 ||
        (float)frame_meta->_height / self->_height <= 0.125 ||
        (float)frame_meta->_height / self->_height >= 8) {
        g_error("DX OSD : scale check error, scale limit[1/8 ~ 8] \n");
        return;
    }

    if (frame_meta->_width < 68 || frame_meta->_height < 2 ||
        frame_meta->_width > 8176 || frame_meta->_height > 8176) {
        g_error("DX OSD : resolution check error, input range[68x2 ~ "
                "8176x8176] \n");
        return;
    }

    if (self->_width < 68 || self->_height < 2 || self->_width > 8128 ||
        self->_height > 8128) {
        g_error("DX OSD : resolution check error, output range[68x2 ~ "
                "8128x8128] \n");
        return;
    }

    int wstride, hstride;
    calculate_strides(frame_meta->_width, frame_meta->_height, 16, 16, &wstride,
                      &hstride);
    rga_buffer_t src_img = wrapbuffer_virtualaddr(
        reinterpret_cast<void *>(input_map.data), frame_meta->_width,
        frame_meta->_height, RK_FORMAT_YCbCr_420_SP, meta->stride[0], hstride);
    rga_buffer_t dst_img =
        wrapbuffer_virtualaddr(reinterpret_cast<void *>(output_map.data),
                               self->_width, self->_height, RK_FORMAT_BGR_888);

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret) {
        std::cerr << "check error: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
        return;
    }

    ret = improcess(src_img, dst_img, {}, {}, {}, {}, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        std::cerr << "RGA resize (imresize) failed: " << ret << " - "
                  << imStrError((IM_STATUS)ret) << std::endl;
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

    gst_buffer_unmap(frame_meta->_buf, &input_map);
    gst_buffer_unmap(outbuffer, &output_map);
}
#else
void draw(GstDxOsd *self, DXFrameMeta *frame_meta, GstBuffer *outbuffer) {
    GstMapInfo output_map;

    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        g_error("ERROR : DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    auto iter = self->_resized_frame.find(frame_meta->_stream_id);
    if (iter == self->_resized_frame.end()) {
        self->_resized_frame[frame_meta->_stream_id] = nullptr;
    }

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);

    Resize(frame_meta->_buf, &self->_resized_frame[frame_meta->_stream_id],
           frame_meta->_width, frame_meta->_height, self->_width, self->_height,
           frame_meta->_format);

    CvtColor(self->_resized_frame[frame_meta->_stream_id], &output_map.data,
             self->_width, self->_height, frame_meta->_format, "BGR");

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

    GstBuffer *outbuf = NULL;
    GstFlowReturn ret = GST_FLOW_OK;

    gsize out_size = self->_output_info.size;
    outbuf = gst_buffer_new_allocate(NULL, out_size, NULL);

    if (!gst_buffer_copy_into(outbuf, buf,
                              (GstBufferCopyFlags)(GST_BUFFER_COPY_FLAGS |
                                                   GST_BUFFER_COPY_TIMESTAMPS),
                              0, -1)) {
        GST_WARNING_OBJECT(self, "Failed to copy buffer metadata");
    }

    GST_BUFFER_OFFSET(outbuf) = GST_BUFFER_OFFSET(buf);
    GST_BUFFER_OFFSET_END(outbuf) = GST_BUFFER_OFFSET_END(buf);

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
    } else {
#ifdef HAVE_LIBRGA
        draw_rga(self, frame_meta, outbuf);
#else
        draw(self, frame_meta, outbuf);
#endif
    }

    gst_buffer_unref(buf);
    ret = gst_pad_push(self->_srcpad, outbuf);
    if (ret != GST_FLOW_OK) {
        GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
    }
    return ret;
}