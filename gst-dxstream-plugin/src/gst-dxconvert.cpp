#include "gst-dxconvert.hpp"
#include "transforms/gst_frame_desc.hpp"
#include "transforms/transform_kernel_pool.hpp"
#include "../metadata/gst-dxframemeta.hpp"
#include <gst/video/video.h>
#include <algorithm>
#include <cstring>
#include <new>

GST_DEBUG_CATEGORY_STATIC(gst_dxconvert_debug_category);
#define GST_CAT_DEFAULT gst_dxconvert_debug_category

// Sink: accept all supported input formats
// Src:  accept all supported output formats
// Conversion is handled by the kernel backend
#define DXCONVERT_SINK_CAPS \
    "video/x-raw, format=(string){ NV12, I420, RGB, BGR }"
#define DXCONVERT_SRC_CAPS \
    "video/x-raw, format=(string){ NV12, I420, RGB, BGR }"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void gst_dxconvert_finalize(GObject *object);
static gboolean gst_dxconvert_start(GstBaseTransform *trans);
static gboolean gst_dxconvert_stop(GstBaseTransform *trans);
static gboolean gst_dxconvert_sink_event(GstBaseTransform *trans, GstEvent *event);
static gboolean gst_dxconvert_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps);
static GstCaps *gst_dxconvert_transform_caps(GstBaseTransform *trans,
                                             GstPadDirection direction,
                                             GstCaps *caps, GstCaps *filter);
static gboolean gst_dxconvert_transform_size(GstBaseTransform *trans,
                                             GstPadDirection direction,
                                             GstCaps *caps, gsize size,
                                             GstCaps *othercaps, gsize *othersize);
static GstFlowReturn gst_dxconvert_transform(GstBaseTransform *trans,
                                             GstBuffer *inbuf, GstBuffer *outbuf);
static gboolean gst_dxconvert_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query);
static gboolean gst_dxconvert_query(GstBaseTransform *trans,
                                    GstPadDirection direction,
                                    GstQuery *query);

// ---------------------------------------------------------------------------
// GObject / GstElement boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxConvert, gst_dxconvert, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxconvert_debug_category, "dxconvert", 0,
                            "debug category for dxconvert element"))


// ---------------------------------------------------------------------------
// class_init
// ---------------------------------------------------------------------------
static void gst_dxconvert_class_init(GstDxConvertClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->finalize = gst_dxconvert_finalize;

    // Pad templates — different formats on sink vs src
    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXCONVERT_SINK_CAPS)));

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXCONVERT_SRC_CAPS)));

    gst_element_class_set_static_metadata(
        element_class, "DXConvert", "Filter/Converter/Video",
        "Hardware-accelerated video color converter using VideoTransformKernel",
        "Sangil Jo <sijo@deepx.ai>");

    base_transform_class->start =
        GST_DEBUG_FUNCPTR(gst_dxconvert_start);
    base_transform_class->stop =
        GST_DEBUG_FUNCPTR(gst_dxconvert_stop);
    base_transform_class->set_caps =
        GST_DEBUG_FUNCPTR(gst_dxconvert_set_caps);
    base_transform_class->transform_caps =
        GST_DEBUG_FUNCPTR(gst_dxconvert_transform_caps);
    base_transform_class->transform_size =
        GST_DEBUG_FUNCPTR(gst_dxconvert_transform_size);
    base_transform_class->transform =
        GST_DEBUG_FUNCPTR(gst_dxconvert_transform);
    base_transform_class->sink_event =
        GST_DEBUG_FUNCPTR(gst_dxconvert_sink_event);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxconvert_propose_allocation);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxconvert_query);
}

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxconvert_init(GstDxConvert *self) {
    self->_negotiated = FALSE;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);
    // GObject zero-fills instance memory but does not call C++ constructors.
    // MSVC std::unique_ptr requires proper construction.
    new (&self->_kernel_pool) std::unique_ptr<dxt::TransformKernelPool>();
}

static void gst_dxconvert_finalize(GObject *object) {
    auto *self = GST_DXCONVERT(object);
    self->_kernel_pool.~unique_ptr();
    G_OBJECT_CLASS(gst_dxconvert_parent_class)->finalize(object);
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
static gboolean gst_dxconvert_start(GstBaseTransform *trans) {
    auto *self = GST_DXCONVERT(trans);
    self->_negotiated = FALSE;
    return TRUE;
}

static gboolean gst_dxconvert_stop(GstBaseTransform *trans) {
    auto *self = GST_DXCONVERT(trans);
    GST_DEBUG_OBJECT(self, "stop");
    self->_negotiated = FALSE;
    self->_kernel_pool.reset();
    return TRUE;
}

static gboolean gst_dxconvert_sink_event(GstBaseTransform *trans, GstEvent *event) {
    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        GST_DEBUG_OBJECT(trans, "EOS event received on sink pad");
    }
    return GST_BASE_TRANSFORM_CLASS(gst_dxconvert_parent_class)->sink_event(trans, event);
}

// ---------------------------------------------------------------------------
// transform_caps — allow format change, preserve width/height
// ---------------------------------------------------------------------------
static GstCaps *gst_dxconvert_transform_caps(GstBaseTransform *trans,
                                             GstPadDirection direction,
                                             GstCaps *caps,
                                             GstCaps *filter) {
    std::ignore = trans;

    GstCaps *ret_caps = nullptr;

    if (direction == GST_PAD_SINK) {
        // Sink → src: offer all output formats with same dimensions
        ret_caps = gst_caps_from_string(DXCONVERT_SRC_CAPS);
    } else {
        // Src → sink: offer all input formats with same dimensions
        ret_caps = gst_caps_from_string(DXCONVERT_SINK_CAPS);
    }

    // Copy width/height/framerate from input caps to output caps
    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure *in_struct = gst_caps_get_structure(caps, i);
        const GValue *width     = gst_structure_get_value(in_struct, "width");
        const GValue *height    = gst_structure_get_value(in_struct, "height");
        const GValue *framerate = gst_structure_get_value(in_struct, "framerate");

        for (guint j = 0; j < gst_caps_get_size(ret_caps); j++) {
            GstStructure *out_struct = gst_caps_get_structure(ret_caps, j);
            if (width)
                gst_structure_set_value(out_struct, "width", width);
            if (height)
                gst_structure_set_value(out_struct, "height", height);
            if (framerate)
                gst_structure_set_value(out_struct, "framerate", framerate);
        }
    }

    if (filter) {
        auto *tmp = gst_caps_intersect_full(ret_caps, filter,
                                            GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(ret_caps);
        ret_caps = tmp;
    }

    return ret_caps;
}

// ---------------------------------------------------------------------------
// set_caps — create transform kernel for color conversion
// ---------------------------------------------------------------------------
static gboolean gst_dxconvert_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps) {
    auto *self = GST_DXCONVERT(trans);

    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }
    if (!gst_video_info_from_caps(&self->_output_info, outcaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse output caps");
        return FALSE;
    }

    GstVideoFormat in_fmt  = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    GstVideoFormat out_fmt = GST_VIDEO_INFO_FORMAT(&self->_output_info);
    gint width  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    gint height = GST_VIDEO_INFO_HEIGHT(&self->_input_info);

    // Destroy existing kernel pool on renegotiation
    self->_kernel_pool.reset();

    // Same format → passthrough, no kernel needed
    if (in_fmt == out_fmt) {
        GST_INFO_OBJECT(self, "Same format %s — passthrough",
                        gst_video_format_to_string(in_fmt));
        gst_base_transform_set_passthrough(trans, TRUE);
        self->_negotiated = TRUE;
        return TRUE;
    }

    gst_base_transform_set_passthrough(trans, FALSE);
    gst_base_transform_set_in_place(trans, FALSE);

    // Build dst template for factory
    dxt::VideoFormat dst_fmt = dxt::video_format_from_gst(out_fmt);
    auto dst_template = dxt::make_dst_template(width, height, dst_fmt);

    // Conversion only, no crop/scale/padding
    dxt::TransformOps ops;

    self->_kernel_pool = std::make_unique<dxt::TransformKernelPool>(dst_template, ops);

    GST_INFO_OBJECT(self, "Convert %dx%d [%s -> %s] kernel pool created",
                    width, height,
                    gst_video_format_to_string(in_fmt),
                    gst_video_format_to_string(out_fmt));

    self->_negotiated = TRUE;
    return TRUE;
}

// ---------------------------------------------------------------------------
// transform_size
// ---------------------------------------------------------------------------
static gboolean gst_dxconvert_transform_size(GstBaseTransform *trans,
                                             GstPadDirection direction,
                                             GstCaps *caps, gsize size,
                                             GstCaps *othercaps,
                                             gsize *othersize) {
    std::ignore = trans;
    std::ignore = direction;
    std::ignore = caps;
    std::ignore = size;

    GstVideoInfo info;
    if (!gst_video_info_from_caps(&info, othercaps))
        return FALSE;

    *othersize = GST_VIDEO_INFO_SIZE(&info);
    return TRUE;
}

// ---------------------------------------------------------------------------
// transform — per-frame color conversion via kernel
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxconvert_transform(GstBaseTransform *trans,
                                             GstBuffer *inbuf,
                                             GstBuffer *outbuf) {
    auto *self = GST_DXCONVERT(trans);

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(inbuf)));

    if (!self->_negotiated) {
        GST_ERROR_OBJECT(self, "Caps not negotiated");
        return GST_FLOW_NOT_NEGOTIATED;
    }

    // Passthrough mode — kernel pool is null, transform() won't be called
    if (!self->_kernel_pool)
        return GST_FLOW_OK;

    dxt::GstSrcFrame src(inbuf, self->_input_info);
    dxt::GstDstFrame dst(outbuf, self->_output_info);
    if (!src.ok() || !dst.ok()) {
        GST_ERROR_OBJECT(self, "Failed to map buffers");
        return GST_FLOW_ERROR;
    }

    dxt::InputConfig input_cfg{src.desc().format, src.desc().width, src.desc().height};
    auto result = self->_kernel_pool->transform(input_cfg, src.desc(), dst.desc());
    if (!result.success) {
        GST_ELEMENT_ERROR(self, STREAM, FAILED,
            ("Kernel color conversion failed (%s -> %s)",
             gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&self->_input_info)),
             gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&self->_output_info))),
            (NULL));
        return GST_FLOW_ERROR;
    }

    gst_buffer_copy_into(outbuf, inbuf, static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS), 0, -1);
    auto* src_meta = dx_get_frame_meta(inbuf);
    if (src_meta) {
        dx_create_frame_meta(outbuf);
        auto* dst_meta = dx_get_frame_meta(outbuf);
        dx_frame_meta_copy(inbuf, src_meta, outbuf, dst_meta);
        dst_meta->_format = gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&self->_output_info));
    }
    return GST_FLOW_OK;
}

static gboolean gst_dxconvert_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(gst_dxconvert_parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}

static gboolean gst_dxconvert_query(GstBaseTransform *trans,
                                    GstPadDirection direction,
                                    GstQuery *query) {
    if (direction == GST_PAD_SRC && GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        if (!GST_BASE_TRANSFORM_CLASS(gst_dxconvert_parent_class)->query(trans, direction, query))
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
    return GST_BASE_TRANSFORM_CLASS(gst_dxconvert_parent_class)->query(trans, direction, query);
}
