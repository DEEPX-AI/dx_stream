#include "gst-dxosd.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>
#include "dxosd_common.hpp"


enum class PropertyID { PROP_0, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxosd_debug_category);
#define GST_CAT_DEFAULT gst_dxosd_debug_category

static GstFlowReturn gst_dxosd_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf);
static GstCaps *gst_dxosd_transform_caps(GstBaseTransform *trans,
                                         GstPadDirection direction,
                                         GstCaps *caps, GstCaps *filter);

G_DEFINE_TYPE(GstDxOsd, gst_dxosd, GST_TYPE_BASE_TRANSFORM);

static GstBaseTransformClass *parent_class = nullptr;

static void gst_dxosd_class_init(GstDxOsdClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "dxosd", 0,
                            "DXOsd plugin");

    auto *basetransform_class = GST_BASE_TRANSFORM_CLASS(klass);
    basetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_dxosd_transform_ip);
    basetransform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_dxosd_transform_caps);

    auto *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "DXOsd", "Generic",
                                          "Draw inference results",
                                          "Jo Sangil <sijo@deepx.ai>");

    // NOSONAR - GStreamer API requires non-const for gst_static_pad_template_get()
    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS("video/x-raw, "
                        "format = (string){ RGB, BGR, I420, NV12 }, "
                        "width = [ 1, 16384 ], "
                        "height = [ 1, 16384 ], "
                        "framerate = [ 0/1, 16384/1 ]"));

    static GstStaticPadTemplate src_template =
        GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                GST_STATIC_CAPS("video/x-raw, "
                                                "format = (string){ RGB, BGR, I420, NV12 }, "
                                                "width = [ 1, 16384 ], "
                                                "height = [ 1, 16384 ], "
                                                "framerate = [ 0/1, 16384/1 ]"));

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&sink_template));
    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    parent_class = GST_BASE_TRANSFORM_CLASS(g_type_class_peek_parent(klass));
}

static void gst_dxosd_init(GstDxOsd *self) {
    GST_INFO_OBJECT(self, "Initializing OSD element (passthrough transform)");
    // BaseTransform handles pad creation automatically
}

static GstCaps *gst_dxosd_transform_caps(GstBaseTransform *trans,
                                         GstPadDirection direction,
                                         GstCaps *caps, GstCaps *filter) {
    std::ignore = trans;
    std::ignore = direction;
    std::ignore = filter;
    // Passthrough: input caps == output caps
    return gst_caps_ref(caps);
}

static GstFlowReturn gst_dxosd_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf) {
    GstDxOsd *self = GST_DXOSD(trans);

    const auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No frame metadata, passing through");
        return GST_FLOW_OK;
    }

    GST_DEBUG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT " stream=%d, %dx%d",
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)), 
                     frame_meta->_stream_id, frame_meta->_width, frame_meta->_height);

    // Get current caps to parse format info
    GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(trans);
    GstCaps *caps = gst_pad_get_current_caps(sinkpad);
    if (!caps) {
        GST_ERROR_OBJECT(self, "No caps negotiated yet");
        return GST_FLOW_ERROR;
    }

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, caps)) {
        GST_ERROR_OBJECT(self, "Failed to parse caps");
        gst_caps_unref(caps);
        return GST_FLOW_ERROR;
    }
    gst_caps_unref(caps);

    // Map buffer for read/write
    GstVideoFrame frame;
    if (!gst_video_frame_map(&frame, &info, buf, GST_MAP_READWRITE)) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return GST_FLOW_ERROR;
    }

    int width = GST_VIDEO_FRAME_WIDTH(&frame);
    int height = GST_VIDEO_FRAME_HEIGHT(&frame);
    GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(&frame);

    // Draw based on format
    if (format == GST_VIDEO_FORMAT_RGB || format == GST_VIDEO_FORMAT_BGR) {
        // RGB/BGR: Direct OpenCV drawing on existing buffer
        int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
        auto *data = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
        cv::Mat surface(height, width, CV_8UC3, data, stride);

        float scale_x = static_cast<float>(frame_meta->_width) / static_cast<float>(width);
        float scale_y = static_cast<float>(frame_meta->_height) / static_cast<float>(height);

        for (const auto *obj_meta : frame_meta->_object_meta_list) {
            draw_object_meta(surface, obj_meta, scale_x, scale_y);
        }
    } else if (format == GST_VIDEO_FORMAT_NV12) {
        // NV12: YUV drawing with color boxes and white text
        auto *y_plane = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
        auto *uv_plane = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 1));
        int stride_y = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
        int stride_uv = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 1);

        float scale_x = static_cast<float>(frame_meta->_width) / static_cast<float>(width);
        float scale_y = static_cast<float>(frame_meta->_height) / static_cast<float>(height);

        GST_DEBUG_OBJECT(self, "Drawing on NV12: %zu objects", 
                        frame_meta->_object_meta_list.size());

        for (const auto *obj_meta : frame_meta->_object_meta_list) {
            draw_object_meta_yuv_nv12(y_plane, uv_plane, stride_y, stride_uv,
                                     width, height, obj_meta, scale_x, scale_y);
        }
    } else if (format == GST_VIDEO_FORMAT_I420) {
        // I420: YUV drawing with color boxes and white text
        auto *y_plane = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
        auto *u_plane = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 1));
        auto *v_plane = static_cast<uint8_t *>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 2));
        int stride_y = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);
        int stride_uv = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 1);

        float scale_x = static_cast<float>(frame_meta->_width) / static_cast<float>(width);
        float scale_y = static_cast<float>(frame_meta->_height) / static_cast<float>(height);

        GST_DEBUG_OBJECT(self, "Drawing on I420: %zu objects",
                        frame_meta->_object_meta_list.size());

        for (const auto *obj_meta : frame_meta->_object_meta_list) {
            draw_object_meta_yuv_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                                     width, height, obj_meta, scale_x, scale_y);
        }
    }

    gst_video_frame_unmap(&frame);
    return GST_FLOW_OK;
}
