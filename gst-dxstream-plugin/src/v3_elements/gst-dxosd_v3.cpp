#include "gst-dxosd_v3.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"

#include <cmath>
#include <cstdio>
#include <opencv2/opencv.hpp>
#include <gst/video/video.h>
#include "../dxosd_common.hpp"

enum class PropertyID { PROP_0, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxosd_v3_debug_category);
#define GST_CAT_DEFAULT gst_dxosd_v3_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, "
                    "format = (string){ RGB }, "
                    "width = [ 1, 16384 ], "
                    "height = [ 1, 16384 ], "
                    "framerate = [ 0/1, 16384/1 ]"));

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("video/x-raw, "
                                            "format = (string){ RGB }, "
                                            "width = [ 1, 16384 ], "
                                            "height = [ 1, 16384 ], "
                                            "framerate = [ 0/1, 16384/1 ]"));

// Forward declarations
static gboolean gst_dxosd_v3_set_caps(GstBaseTransform *base, GstCaps *incaps, GstCaps *outcaps);
static GstFlowReturn gst_dxosd_v3_transform_ip(GstBaseTransform *base, GstBuffer *buf);

G_DEFINE_TYPE(GstDxOsdV3, gst_dxosd_v3, GST_TYPE_BASE_TRANSFORM);

static void gst_dxosd_v3_class_init(GstDxOsdV3Class *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_v3_debug_category, "dxosdv3", 0,
                            "DXOsd V3 plugin");

    auto *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(
        element_class, "DXOsd V3", "Filter/Video",
        "Draw inference results on RGB video frames using OpenCV",
        "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    auto *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_dxosd_v3_set_caps);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_dxosd_v3_transform_ip);
    base_transform_class->passthrough_on_same_caps = FALSE;
}

static void gst_dxosd_v3_init(GstDxOsdV3 *self) {
    GST_DEBUG_OBJECT(self, "Initializing dxosdv3");
    self->width = 0;
    self->height = 0;
    
    // Initialize FPS tracking
    self->enable_fps = FALSE;
    self->total_frames = 0;
    self->fps_update_time = 0;
    self->measured_fps = 0.0;
}

static gboolean gst_dxosd_v3_set_caps(GstBaseTransform *base, GstCaps *incaps, GstCaps *outcaps) {
    std::ignore = outcaps;
    GstDxOsdV3 *self = GST_DXOSD_V3(base);
    GstVideoInfo info;

    if (!gst_video_info_from_caps(&info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }

    self->width = GST_VIDEO_INFO_WIDTH(&info);
    self->height = GST_VIDEO_INFO_HEIGHT(&info);

    GST_DEBUG_OBJECT(self, "Set caps: %dx%d", self->width, self->height);
    return TRUE;
}



static void draw_fps(cv::Mat &img, gdouble fps) {
    if (fps <= 0.0)
        return;
    
    std::string fps_str = std::to_string(fps);
    std::string fps_text = "FPS: " + fps_str.substr(0, fps_str.find('.') + 2);
    
    double font_scale = 0.001 * std::min(img.cols, img.rows);
    int thickness = 2;
    int baseline = 0;
    
    cv::Size text_size = cv::getTextSize(fps_text, cv::FONT_HERSHEY_SIMPLEX, 
                                          font_scale, thickness, &baseline);
    
    int margin = 10;
    int box_x = img.cols - text_size.width - margin * 2;
    
    // Draw background box
    cv::rectangle(img, 
                 cv::Rect(box_x - margin, margin, 
                         text_size.width + margin * 2, 
                         text_size.height + margin * 2),
                 cv::Scalar(0, 0, 0), cv::FILLED);
    
    // Draw FPS text
    cv::putText(img, fps_text, 
               cv::Point(box_x, margin + text_size.height + margin),
               cv::FONT_HERSHEY_SIMPLEX, font_scale, 
               cv::Scalar(0, 255, 0), thickness, cv::LINE_AA);
}

static void draw_objects(GstDxOsdV3 *self, GstBuffer *buf) {
    const auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta found in buffer");
        return;
    }

    if (frame_meta->_object_meta_list.empty()) {
        GST_DEBUG_OBJECT(self, "No objects to draw");
        return;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
        GST_ERROR_OBJECT(self, "Failed to map buffer for drawing");
        return;
    }

    // Create OpenCV matrix from buffer (RGB format)
    cv::Mat surface(self->height, self->width, CV_8UC3, map.data);

    // Calculate scale factors for coordinate transformation
    float scale_factor_x = (float)frame_meta->_width / self->width;
    float scale_factor_y = (float)frame_meta->_height / self->height;

    // Draw each object in the metadata list
    guint object_count = frame_meta->_object_meta_list.size();
    for (guint i = 0; i < object_count; i++) {
        auto *obj_meta = frame_meta->_object_meta_list[i];
        if (obj_meta) {
            draw_object_meta(surface, obj_meta, scale_factor_x, scale_factor_y, true);
        }
    }

    gst_buffer_unmap(buf, &map);
}

static GstFlowReturn gst_dxosd_v3_transform_ip(GstBaseTransform *base,
                                                GstBuffer *buf) {
    GstDxOsdV3 *self = GST_DXOSD_V3(base);

    if (self->width <= 0 || self->height <= 0) {
        GST_ERROR_OBJECT(self, "Invalid dimensions: %dx%d", self->width, self->height);
        return GST_FLOW_ERROR;
    }

    // Update FPS calculation
    if (self->enable_fps) {
        GstClockTime current_time = gst_util_get_timestamp();
        self->total_frames++;
        
        if (self->fps_update_time == 0) {
            self->fps_update_time = current_time;
        } else {
            GstClockTime elapsed = current_time - self->fps_update_time;
            // Update FPS every 500ms
            if (elapsed >= 500 * GST_MSECOND) {
                self->measured_fps = (gdouble)self->total_frames * GST_SECOND / elapsed;
                self->total_frames = 0;
                self->fps_update_time = current_time;
            }
        }
    }

    draw_objects(self, buf);
    
    // Draw FPS overlay
    if (self->enable_fps && self->measured_fps > 0.0) {
        GstMapInfo map;
        if (gst_buffer_map(buf, &map, GST_MAP_READWRITE)) {
            cv::Mat surface(self->height, self->width, CV_8UC3, map.data);
            draw_fps(surface, self->measured_fps);
            gst_buffer_unmap(buf, &map);
        }
    }

    return GST_FLOW_OK;
}
