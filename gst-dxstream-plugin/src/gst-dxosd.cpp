#include "gst-dxosd.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "utils.hpp"

#include <array>
#include <new>
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
static gboolean gst_dxosd_sink_event(GstBaseTransform *trans,
                                     GstEvent *event);
static gboolean gst_dxosd_propose_allocation(GstBaseTransform *trans,
                                             GstQuery *decide_query,
                                             GstQuery *query);
static gboolean gst_dxosd_query(GstBaseTransform *trans,
                                GstPadDirection direction, GstQuery *query);
static GstStateChangeReturn gst_dxosd_change_state(GstElement *element,
                                                   GstStateChange transition);

G_DEFINE_TYPE(GstDxOsd, gst_dxosd, GST_TYPE_BASE_TRANSFORM);

static GstBaseTransformClass *parent_class = nullptr;

static void gst_dxosd_finalize(GObject *object) {
    auto *self = GST_DXOSD(object);
    self->_stream_info.~map();
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_dxosd_class_init(GstDxOsdClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "dxosd", 0,
                            "DXOsd plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->finalize = gst_dxosd_finalize;

    auto *basetransform_class = GST_BASE_TRANSFORM_CLASS(klass);
    basetransform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_dxosd_transform_ip);
    basetransform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_dxosd_transform_caps);
    basetransform_class->sink_event = GST_DEBUG_FUNCPTR(gst_dxosd_sink_event);
    basetransform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxosd_propose_allocation);
    basetransform_class->query = GST_DEBUG_FUNCPTR(gst_dxosd_query);

    auto *element_class = GST_ELEMENT_CLASS(klass);
    element_class->change_state = gst_dxosd_change_state;
    gst_element_class_set_static_metadata(element_class, "DXOsd", "Generic",
                                          "Draw inference results",
                                          "Sangil Jo <sijo@deepx.ai>");

    // NOSONAR - GStreamer API requires non-const for gst_static_pad_template_get()
    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
        GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; "
                        "video/x-raw, "
                        "format = (string){ RGB, BGR, I420, NV12 }, "
                        "width = [ 1, 16384 ], "
                        "height = [ 1, 16384 ], "
                        "framerate = [ 0/1, 16384/1 ]"));

    static GstStaticPadTemplate src_template =
        GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                GST_STATIC_CAPS(DX_VIDEORAW_CAPS_STR "; "
                                                "video/x-raw, "
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
    GST_DEBUG_OBJECT(self, "Initializing OSD element (passthrough transform)");
    new (&self->_stream_info) std::map<int, GstVideoInfo>();
    gst_base_transform_set_qos_enabled(GST_BASE_TRANSFORM(self), TRUE);
}

static GstStateChangeReturn gst_dxosd_change_state(GstElement *element,
                                                   GstStateChange transition) {
    GstDxOsd *self = GST_DXOSD(element);
    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        self->_stream_info.clear();
        break;
    default:
        break;
    }
    return GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
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

static void set_stream_info(GstDxOsd *self, GstEvent *event, int stream_id) {
    GstCaps *incaps = nullptr;
    gst_event_parse_caps(event, &incaps);
    // Only register on first caps event per stream_id.
    // Dynamic resolution change within a running stream is not supported.
    if (incaps && self->_stream_info.find(stream_id) == self->_stream_info.end()) {
        gst_video_info_init(&self->_stream_info[stream_id]);
        if (!gst_video_info_from_caps(&self->_stream_info[stream_id], incaps)) {
            GST_WARNING_OBJECT(self, "Failed to parse caps for stream %d", stream_id);
            self->_stream_info.erase(stream_id);
        }
    }
}

static gboolean gst_dxosd_sink_event(GstBaseTransform *trans,
                                     GstEvent *event) {
    GstDxOsd *self = GST_DXOSD(trans);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        const GstStructure *s = gst_event_get_structure(event);
        if (gst_structure_has_name(s, "application/x-dx-wrapped-event")) {
            int stream_id = -1;
            GstEvent *original_event = nullptr;
            gst_structure_get_int(s, "stream-id", &stream_id);
            gst_structure_get(s, "event", GST_TYPE_EVENT, &original_event, NULL);
            if (original_event && GST_EVENT_TYPE(original_event) == GST_EVENT_CAPS) {
                set_stream_info(self, original_event, stream_id);
            }
            if (original_event) {
                gst_event_unref(original_event);
            }
        }
    } break;
    case GST_EVENT_CAPS: {
        GstCaps *incaps = nullptr;
        gst_event_parse_caps(event, &incaps);
        if (incaps && !dx_caps_is_videoraw(incaps)) {
            set_stream_info(self, event, 0);
        }
    } break;
    case GST_EVENT_FLUSH_STOP:
        self->_stream_info.clear();
        break;
    default:
        break;
    }
    return GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
}

static gboolean gst_dxosd_query(GstBaseTransform *trans,
                                GstPadDirection direction, GstQuery *query) {
    if (direction == GST_PAD_SRC && GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        if (!GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query))
            return FALSE;
        gboolean live;
        GstClockTime min_lat, max_lat;
        gst_query_parse_latency(query, &live, &min_lat, &max_lat);
        const GstClockTime self_lat = 1 * GST_USECOND;
        min_lat += self_lat;
        if (max_lat != GST_CLOCK_TIME_NONE)
            max_lat += self_lat;
        gst_query_set_latency(query, live, min_lat, max_lat);
        return TRUE;
    }
    return GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query);
}

static GstFlowReturn gst_dxosd_transform_ip(GstBaseTransform *trans,
                                             GstBuffer *buf) {
    GstDxOsd *self = GST_DXOSD(trans);

    const auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No frame metadata, passing through");
        return GST_FLOW_OK;
    }

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT " stream=%d, %dx%d",
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)),
                     frame_meta->_stream_id, frame_meta->_width, frame_meta->_height);

    // Look up video info by stream_id
    auto it = self->_stream_info.find(frame_meta->_stream_id);
    if (it == self->_stream_info.end()) {
        GST_LOG_OBJECT(self, "No video info for stream %d, passing through",
                       frame_meta->_stream_id);
        return GST_FLOW_OK;
    }

    GstVideoInfo *info = &it->second;

    // Map buffer for read/write
    GstVideoFrame frame;
    if (!gst_video_frame_map(&frame, info, buf, GST_MAP_READWRITE)) {
        GST_ELEMENT_ERROR(self, RESOURCE, READ,
            ("Failed to map video frame for OSD rendering"), (NULL));
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

        // Frame-level segmentation (semantic seg)
        draw_semantic_segmentation(surface, frame_meta);

        // Frame-level dense depth map (colormap overlay)
        draw_depth(surface, frame_meta);

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

        // Frame-level segmentation (semantic seg)
        draw_semantic_segmentation_nv12(y_plane, uv_plane, stride_y, stride_uv, width, height, frame_meta);

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

        // Frame-level segmentation (semantic seg)
        draw_semantic_segmentation_i420(y_plane, u_plane, v_plane, stride_y, stride_uv, width, height, frame_meta);

        for (const auto *obj_meta : frame_meta->_object_meta_list) {
            draw_object_meta_yuv_i420(y_plane, u_plane, v_plane, stride_y, stride_uv,
                                     width, height, obj_meta, scale_x, scale_y);
        }
    }

    gst_video_frame_unmap(&frame);
    return GST_FLOW_OK;
}

static gboolean gst_dxosd_propose_allocation(GstBaseTransform *trans,
                                             GstQuery *decide_query,
                                             GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);

    GstCaps *qcaps = nullptr;
    gst_query_parse_allocation(query, &qcaps, nullptr);
    if (qcaps && dx_caps_is_videoraw(qcaps))
        gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}
