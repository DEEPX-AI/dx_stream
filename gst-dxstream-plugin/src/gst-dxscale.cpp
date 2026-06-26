#include "gst-dxscale.hpp"
#include "transforms/gst_frame_desc.hpp"
#include "transforms/transform_kernel_pool.hpp"
#include "../metadata/gst-dxframemeta.hpp"
#include <gst/video/video.h>
#include <algorithm>
#include <array>
#include <new>

GST_DEBUG_CATEGORY_STATIC(gst_dxscale_debug_category);
#define GST_CAT_DEFAULT gst_dxscale_debug_category

// ---------------------------------------------------------------------------
// Property IDs
// ---------------------------------------------------------------------------
enum class PropertyID { PROP_0, PROP_WIDTH, PROP_HEIGHT, N_PROPERTIES };

// Supported formats (same format on both pads — scale only, no conversion)
#define DXSCALE_CAPS \
    "video/x-raw, format=(string){ NV12, I420, RGB, BGR }"

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void gst_dxscale_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec);
static void gst_dxscale_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec);
static void gst_dxscale_finalize(GObject *object);
static gboolean gst_dxscale_start(GstBaseTransform *trans);
static gboolean gst_dxscale_stop(GstBaseTransform *trans);
static gboolean gst_dxscale_sink_event(GstBaseTransform *trans, GstEvent *event);
static gboolean gst_dxscale_set_caps(GstBaseTransform *trans,
                                     GstCaps *incaps, GstCaps *outcaps);
static GstCaps *gst_dxscale_transform_caps(GstBaseTransform *trans,
                                           GstPadDirection direction,
                                           GstCaps *caps, GstCaps *filter);
static gboolean gst_dxscale_transform_size(GstBaseTransform *trans,
                                           GstPadDirection direction,
                                           GstCaps *caps, gsize size,
                                           GstCaps *othercaps, gsize *othersize);
static GstFlowReturn gst_dxscale_transform(GstBaseTransform *trans,
                                           GstBuffer *inbuf, GstBuffer *outbuf);
static gboolean gst_dxscale_propose_allocation(GstBaseTransform *trans,
                                               GstQuery *decide_query,
                                               GstQuery *query);
static gboolean gst_dxscale_query(GstBaseTransform *trans,
                                  GstPadDirection direction,
                                  GstQuery *query);

// ---------------------------------------------------------------------------
// GObject / GstElement boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxScale, gst_dxscale, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxscale_debug_category, "dxscale", 0,
                            "debug category for dxscale element"))


// ---------------------------------------------------------------------------
// class_init
// ---------------------------------------------------------------------------
static void gst_dxscale_class_init(GstDxScaleClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_dxscale_set_property;
    gobject_class->get_property = gst_dxscale_get_property;
    gobject_class->finalize = gst_dxscale_finalize;

    // Properties
    static std::array<GParamSpec*,
                      static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {nullptr};

    obj_properties[static_cast<guint>(PropertyID::PROP_WIDTH)] =
        g_param_spec_uint("width", "Width",
                          "Target output width (0 = passthrough)",
                          0, 8192, 0, G_PARAM_READWRITE);

    obj_properties[static_cast<guint>(PropertyID::PROP_HEIGHT)] =
        g_param_spec_uint("height", "Height",
                          "Target output height (0 = passthrough)",
                          0, 8192, 0, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class,
                                      static_cast<guint>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    // Pad templates
    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXSCALE_CAPS)));

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                             gst_caps_from_string(DXSCALE_CAPS)));

    gst_element_class_set_static_metadata(
        element_class, "DXScale", "Filter/Converter/Video/Scaler",
        "Hardware-accelerated video scaler using VideoTransformKernel",
        "Sangil Jo <sijo@deepx.ai>");

    base_transform_class->start =
        GST_DEBUG_FUNCPTR(gst_dxscale_start);
    base_transform_class->stop =
        GST_DEBUG_FUNCPTR(gst_dxscale_stop);
    base_transform_class->set_caps =
        GST_DEBUG_FUNCPTR(gst_dxscale_set_caps);
    base_transform_class->transform_caps =
        GST_DEBUG_FUNCPTR(gst_dxscale_transform_caps);
    base_transform_class->transform_size =
        GST_DEBUG_FUNCPTR(gst_dxscale_transform_size);
    base_transform_class->transform =
        GST_DEBUG_FUNCPTR(gst_dxscale_transform);
    base_transform_class->sink_event =
        GST_DEBUG_FUNCPTR(gst_dxscale_sink_event);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxscale_propose_allocation);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxscale_query);
}

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxscale_init(GstDxScale *self) {
    self->_width      = 0;
    self->_height     = 0;
    self->_negotiated = FALSE;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);
    // GObject zero-fills instance memory but does not call C++ constructors.
    // MSVC std::unique_ptr requires proper construction.
    new (&self->_kernel_pool) std::unique_ptr<dxt::TransformKernelPool>();
}

static void gst_dxscale_finalize(GObject *object) {
    auto *self = GST_DXSCALE(object);
    self->_kernel_pool.~unique_ptr();
    G_OBJECT_CLASS(gst_dxscale_parent_class)->finalize(object);
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------
static void gst_dxscale_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXSCALE(object);
    switch (property_id) {
        case static_cast<guint>(PropertyID::PROP_WIDTH):
            self->_width = g_value_get_uint(value);
            gst_base_transform_reconfigure_src(GST_BASE_TRANSFORM(self));
            break;
        case static_cast<guint>(PropertyID::PROP_HEIGHT):
            self->_height = g_value_get_uint(value);
            gst_base_transform_reconfigure_src(GST_BASE_TRANSFORM(self));
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxscale_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXSCALE(object);
    switch (property_id) {
        case static_cast<guint>(PropertyID::PROP_WIDTH):
            g_value_set_uint(value, self->_width);
            break;
        case static_cast<guint>(PropertyID::PROP_HEIGHT):
            g_value_set_uint(value, self->_height);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

// ---------------------------------------------------------------------------
// start / stop
// ---------------------------------------------------------------------------
static gboolean gst_dxscale_start(GstBaseTransform *trans) {
    auto *self = GST_DXSCALE(trans);
    self->_negotiated = FALSE;
    return TRUE;
}

static gboolean gst_dxscale_stop(GstBaseTransform *trans) {
    auto *self = GST_DXSCALE(trans);
    GST_DEBUG_OBJECT(self, "stop");
    self->_negotiated = FALSE;
    self->_kernel_pool.reset();
    return TRUE;
}

static gboolean gst_dxscale_sink_event(GstBaseTransform *trans, GstEvent *event) {
    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        GST_DEBUG_OBJECT(trans, "EOS event received on sink pad");
    }
    return GST_BASE_TRANSFORM_CLASS(gst_dxscale_parent_class)->sink_event(trans, event);
}

// ---------------------------------------------------------------------------
// transform_caps — negotiate output size from properties
// ---------------------------------------------------------------------------
static GstCaps *gst_dxscale_transform_caps(GstBaseTransform *trans,
                                           GstPadDirection direction,
                                           GstCaps *caps,
                                           GstCaps *filter) {
    auto *self = GST_DXSCALE(trans);
    auto *ret_caps = gst_caps_copy(caps);

    for (guint i = 0; i < gst_caps_get_size(ret_caps); i++) {
        GstStructure *structure = gst_caps_get_structure(ret_caps, i);

        if (direction == GST_PAD_SINK && self->_width > 0 && self->_height > 0) {
            // Sink → src: fix output to target dimensions
            gst_structure_set(structure,
                              "width", G_TYPE_INT, static_cast<gint>(self->_width),
                              "height", G_TYPE_INT, static_cast<gint>(self->_height),
                              NULL);
        } else if (direction == GST_PAD_SRC) {
            // Src → sink: accept any input dimensions
            gst_structure_set(structure,
                              "width", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                              "height", GST_TYPE_INT_RANGE, 1, G_MAXINT,
                              NULL);
        }
        // width/height == 0 and SINK direction: caps pass through unchanged (passthrough)
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
// set_caps — create transform kernel
// ---------------------------------------------------------------------------
static gboolean gst_dxscale_set_caps(GstBaseTransform *trans,
                                     GstCaps *incaps, GstCaps *outcaps) {
    auto *self = GST_DXSCALE(trans);

    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }
    if (!gst_video_info_from_caps(&self->_output_info, outcaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse output caps");
        return FALSE;
    }

    // Verify same format on both pads (dxscale is scale-only)
    GstVideoFormat in_fmt  = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    GstVideoFormat out_fmt = GST_VIDEO_INFO_FORMAT(&self->_output_info);
    if (in_fmt != out_fmt) {
        GST_ERROR_OBJECT(self,
            "Input format %s != output format %s (dxscale is scale-only)",
            gst_video_format_to_string(in_fmt),
            gst_video_format_to_string(out_fmt));
        return FALSE;
    }

    gint in_w  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    gint in_h  = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    gint out_w = GST_VIDEO_INFO_WIDTH(&self->_output_info);
    gint out_h = GST_VIDEO_INFO_HEIGHT(&self->_output_info);

    // Destroy existing kernel pool on renegotiation
    self->_kernel_pool.reset();

    // Build dst template for factory
    dxt::VideoFormat fmt = dxt::video_format_from_gst(in_fmt);
    auto dst_template = dxt::make_dst_template(out_w, out_h, fmt);

    // Scale-only: no crop, no aspect-ratio padding
    dxt::TransformOps ops;

    self->_kernel_pool = std::make_unique<dxt::TransformKernelPool>(dst_template, ops);

    GST_INFO_OBJECT(self, "Scale %dx%d -> %dx%d [%s] kernel pool created",
                    in_w, in_h, out_w, out_h,
                    gst_video_format_to_string(in_fmt));

    self->_negotiated = TRUE;
    return TRUE;
}

// ---------------------------------------------------------------------------
// transform_size
// ---------------------------------------------------------------------------
static gboolean gst_dxscale_transform_size(GstBaseTransform *trans,
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
// transform — per-frame scale via kernel
// ---------------------------------------------------------------------------
static GstFlowReturn gst_dxscale_transform(GstBaseTransform *trans,
                                           GstBuffer *inbuf,
                                           GstBuffer *outbuf) {
    auto *self = GST_DXSCALE(trans);

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(inbuf)));

    if (!self->_negotiated || !self->_kernel_pool) {
        GST_ERROR_OBJECT(self, "Caps not negotiated or kernel pool missing");
        return GST_FLOW_NOT_NEGOTIATED;
    }

    gint in_w  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    gint in_h  = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    gint out_w = GST_VIDEO_INFO_WIDTH(&self->_output_info);
    gint out_h = GST_VIDEO_INFO_HEIGHT(&self->_output_info);

    // Same size → plane-aware passthrough copy
    if (in_w == out_w && in_h == out_h)
        return dxt::gst_copy_video_frame(inbuf, outbuf, self->_input_info, self->_output_info);

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
            ("Kernel transform failed"), (NULL));
        return GST_FLOW_ERROR;
    }

    gst_buffer_copy_into(outbuf, inbuf, static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS), 0, -1);
    auto* src_meta = dx_get_frame_meta(inbuf);
    if (src_meta) {
        dx_create_frame_meta(outbuf);
        auto* dst_meta = dx_get_frame_meta(outbuf);
        dx_frame_meta_copy(inbuf, src_meta, outbuf, dst_meta);
        dst_meta->_width = GST_VIDEO_INFO_WIDTH(&self->_output_info);
        dst_meta->_height = GST_VIDEO_INFO_HEIGHT(&self->_output_info);
    }
    return GST_FLOW_OK;
}

static gboolean gst_dxscale_propose_allocation(GstBaseTransform *trans,
                                               GstQuery *decide_query,
                                               GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(gst_dxscale_parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    return ret;
}

static gboolean gst_dxscale_query(GstBaseTransform *trans,
                                  GstPadDirection direction,
                                  GstQuery *query) {
    if (direction == GST_PAD_SRC && GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        if (!GST_BASE_TRANSFORM_CLASS(gst_dxscale_parent_class)->query(trans, direction, query))
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
    return GST_BASE_TRANSFORM_CLASS(gst_dxscale_parent_class)->query(trans, direction, query);
}
