#include "gst-dxscale.hpp"
#include "gst_frame_desc.hpp"
#include "video_transform_factory.hpp"
#include <gst/video/video.h>
#include <algorithm>
#include <array>

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

// ---------------------------------------------------------------------------
// GObject / GstElement boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxScale, gst_dxscale, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxscale_debug_category, "dxscale", 0,
                            "debug category for dxscale element"))

// GstVideoFormat → dxt::VideoFormat
static dxt::VideoFormat gst_to_dxt_format(GstVideoFormat fmt) {
    switch (fmt) {
        case GST_VIDEO_FORMAT_I420: return dxt::VideoFormat::I420;
        case GST_VIDEO_FORMAT_NV12: return dxt::VideoFormat::NV12;
        case GST_VIDEO_FORMAT_RGB:  return dxt::VideoFormat::RGB;
        case GST_VIDEO_FORMAT_BGR:  return dxt::VideoFormat::BGR;
        default: return dxt::VideoFormat::RGB;
    }
}

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
        "DeepX AI <support@deepx.ai>");

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
}

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxscale_init(GstDxScale *self) {
    self->_kernel     = nullptr;
    self->_width      = 0;
    self->_height     = 0;
    self->_negotiated = FALSE;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);
}

static void gst_dxscale_finalize(GObject *object) {
    auto *self = GST_DXSCALE(object);
    delete self->_kernel;
    self->_kernel = nullptr;
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
            break;
        case static_cast<guint>(PropertyID::PROP_HEIGHT):
            self->_height = g_value_get_uint(value);
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
    self->_negotiated = FALSE;
    delete self->_kernel;
    self->_kernel = nullptr;
    return TRUE;
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

    // Destroy existing kernel on renegotiation
    delete self->_kernel;
    self->_kernel = nullptr;

    // Build dst template for factory
    dxt::VideoFormat fmt = gst_to_dxt_format(in_fmt);

    dxt::FrameDesc dst_template;
    dst_template.width       = out_w;
    dst_template.height      = out_h;
    dst_template.format      = fmt;
    dst_template.memory_type = dxt::MemoryType::CPU_VIRTUAL;
    dst_template.num_planes  = dxt::num_planes_for_format(fmt);

    if (fmt == dxt::VideoFormat::NV12) {
        dst_template.planes[0].stride = out_w;
        dst_template.planes[0].height = out_h;
        dst_template.planes[1].stride = out_w;
        dst_template.planes[1].height = out_h / 2;
    } else if (fmt == dxt::VideoFormat::I420) {
        dst_template.planes[0].stride = out_w;
        dst_template.planes[0].height = out_h;
        dst_template.planes[1].stride = out_w / 2;
        dst_template.planes[1].height = out_h / 2;
        dst_template.planes[2].stride = out_w / 2;
        dst_template.planes[2].height = out_h / 2;
    } else {
        dst_template.planes[0].stride = out_w * dxt::bytes_per_pixel(fmt);
        dst_template.planes[0].height = out_h;
    }

    // Scale-only: no crop, no aspect-ratio padding
    dxt::TransformOps ops;

    auto kernel = dxt::VideoTransformFactory::create(dst_template, ops);

    // dxscale is same-format: verify the auto-selected backend actually
    // supports our format as *source*.  E.g. RGA accepts dst=RGB in init()
    // but only handles NV12 input — that would fail at transform time.
    if (kernel) {
        auto caps = kernel->capabilities();
        bool src_ok = false;
        for (auto &f : caps.src_formats) {
            if (f == fmt) { src_ok = true; break; }
        }
        if (!src_ok) {
            GST_INFO_OBJECT(self,
                "Backend '%s' cannot take %s as source, falling back to libyuv",
                kernel->backend_name(),
                gst_video_format_to_string(in_fmt));
            kernel = dxt::VideoTransformFactory::create_backend(
                "libyuv", dst_template, ops);
        }
    }

    if (!kernel) {
        GST_ERROR_OBJECT(self,
            "No transform backend available for %s %dx%d -> %dx%d",
            gst_video_format_to_string(in_fmt), in_w, in_h, out_w, out_h);
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Scale %dx%d -> %dx%d [%s] via %s",
                    in_w, in_h, out_w, out_h,
                    gst_video_format_to_string(in_fmt),
                    kernel->backend_name());

    self->_kernel     = kernel.release();
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

    if (!self->_negotiated || !self->_kernel) {
        GST_ERROR_OBJECT(self, "Caps not negotiated or kernel missing");
        return GST_FLOW_NOT_NEGOTIATED;
    }

    GstMapInfo in_map  = GST_MAP_INFO_INIT;
    GstMapInfo out_map = GST_MAP_INFO_INIT;

    if (!gst_buffer_map(inbuf, &in_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        return GST_FLOW_ERROR;
    }
    if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
        gst_buffer_unmap(inbuf, &in_map);
        GST_ERROR_OBJECT(self, "Failed to map output buffer");
        return GST_FLOW_ERROR;
    }

    GstFlowReturn ret = GST_FLOW_OK;

    gint in_w  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    gint in_h  = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    gint out_w = GST_VIDEO_INFO_WIDTH(&self->_output_info);
    gint out_h = GST_VIDEO_INFO_HEIGHT(&self->_output_info);

    // Same size → fast copy (passthrough path)
    // For YUV formats, copy plane-by-plane to handle stride/offset
    // differences between padded HW decoder buffers and output buffers.
    if (in_w == out_w && in_h == out_h) {
        GstVideoFormat pt_fmt = GST_VIDEO_INFO_FORMAT(&self->_input_info);
        if (pt_fmt == GST_VIDEO_FORMAT_NV12 || pt_fmt == GST_VIDEO_FORMAT_I420) {
            GstVideoMeta *vmeta = gst_buffer_get_video_meta(inbuf);
            int n_planes = (pt_fmt == GST_VIDEO_FORMAT_NV12) ? 2 : 3;
            for (int p = 0; p < n_planes; ++p) {
                int src_stride = vmeta ? static_cast<int>(vmeta->stride[p])
                                       : GST_VIDEO_INFO_PLANE_STRIDE(&self->_input_info, p);
                size_t src_off = vmeta ? vmeta->offset[p]
                                       : GST_VIDEO_INFO_PLANE_OFFSET(&self->_input_info, p);
                int dst_stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, p);
                size_t dst_off = GST_VIDEO_INFO_PLANE_OFFSET(&self->_output_info, p);
                int plane_h = (p == 0) ? in_h : in_h / 2;
                int row_bytes = (pt_fmt == GST_VIDEO_FORMAT_NV12)
                    ? in_w  // Y and UV both have width bytes per row
                    : ((p == 0) ? in_w : (in_w + 1) / 2);
                for (int row = 0; row < plane_h; ++row) {
                    memcpy(out_map.data + dst_off + row * dst_stride,
                           in_map.data  + src_off + row * src_stride,
                           row_bytes);
                }
            }
        } else {
            memcpy(out_map.data, in_map.data,
                   std::min(in_map.size, out_map.size));
        }
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
        goto cleanup;
    }

    {
        GstVideoFormat gst_fmt = GST_VIDEO_INFO_FORMAT(&self->_input_info);
        dxt::FrameDesc src_desc;
        dxt::FrameDesc dst_desc;

        if (gst_fmt == GST_VIDEO_FORMAT_NV12) {
            // make_nv12_frame_desc handles layout priority:
            //   GstVideoMeta > GstVideoInfo > heuristic
            src_desc = dxt::make_nv12_frame_desc(inbuf, in_w, in_h, &self->_input_info);
            // dxscale always uses CPU-mapped buffers
            // src_desc.memory_type = dxt::MemoryType::CPU_VIRTUAL;
            // src_desc.dma_fd = -1;
            src_desc.planes[0].data = in_map.data + src_desc.planes[0].offset;
            src_desc.planes[1].data = in_map.data + src_desc.planes[1].offset;

            // Output: standard NV12 layout from GstVideoInfo
            dst_desc.width       = out_w;
            dst_desc.height      = out_h;
            dst_desc.format      = dxt::VideoFormat::NV12;
            dst_desc.memory_type = dxt::MemoryType::CPU_VIRTUAL;
            dst_desc.num_planes  = 2;
            dst_desc.dma_fd      = -1;
            dst_desc.planes[0].data   = out_map.data;
            dst_desc.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, 0);
            dst_desc.planes[0].height = out_h;
            dst_desc.planes[0].offset = 0;
            dst_desc.planes[1].data   = out_map.data + GST_VIDEO_INFO_PLANE_OFFSET(&self->_output_info, 1);
            dst_desc.planes[1].stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, 1);
            dst_desc.planes[1].height = out_h / 2;
            dst_desc.planes[1].offset = GST_VIDEO_INFO_PLANE_OFFSET(&self->_output_info, 1);
        } else if (gst_fmt == GST_VIDEO_FORMAT_I420) {
            // Build I420 descriptors with GstVideoInfo strides/offsets
            src_desc = dxt::make_i420_frame_desc(self->_input_info);
            for (int i = 0; i < src_desc.num_planes; ++i)
                src_desc.planes[i].data = in_map.data + src_desc.planes[i].offset;

            dst_desc = dxt::make_i420_frame_desc(self->_output_info);
            for (int i = 0; i < dst_desc.num_planes; ++i)
                dst_desc.planes[i].data = out_map.data + dst_desc.planes[i].offset;
        } else {
            // Packed RGB/BGR — use GstVideoInfo strides for actual buffer layout
            dxt::VideoFormat fmt = gst_to_dxt_format(gst_fmt);
            src_desc = dxt::make_packed_frame_desc(in_map.data, in_w, in_h, fmt);
            src_desc.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_input_info, 0);
            dst_desc = dxt::make_packed_frame_desc(out_map.data, out_w, out_h, fmt);
            dst_desc.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, 0);
        }

        auto result = self->_kernel->transform(src_desc, dst_desc);
        if (!result.success) {
            GST_ERROR_OBJECT(self, "Kernel transform failed");
            ret = GST_FLOW_ERROR;
        } else {
            gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
        }
    }

cleanup:
    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);
    return ret;
}
