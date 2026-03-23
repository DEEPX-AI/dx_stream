#include "gst-dxconvert.hpp"
#include "gst_frame_desc.hpp"
#include "video_transform_factory.hpp"
#include <gst/video/video.h>
#include <algorithm>
#include <cstring>

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

// ---------------------------------------------------------------------------
// GObject / GstElement boilerplate
// ---------------------------------------------------------------------------
G_DEFINE_TYPE_WITH_CODE(
    GstDxConvert, gst_dxconvert, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxconvert_debug_category, "dxconvert", 0,
                            "debug category for dxconvert element"))

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
        "DeepX AI <support@deepx.ai>");

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
}

// ---------------------------------------------------------------------------
// init / finalize
// ---------------------------------------------------------------------------
static void gst_dxconvert_init(GstDxConvert *self) {
    self->_kernel     = nullptr;
    self->_negotiated = FALSE;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);
}

static void gst_dxconvert_finalize(GObject *object) {
    auto *self = GST_DXCONVERT(object);
    delete self->_kernel;
    self->_kernel = nullptr;
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
    self->_negotiated = FALSE;
    delete self->_kernel;
    self->_kernel = nullptr;
    return TRUE;
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

    // Destroy existing kernel on renegotiation
    delete self->_kernel;
    self->_kernel = nullptr;

    // Same format → passthrough, no kernel needed
    if (in_fmt == out_fmt) {
        GST_INFO_OBJECT(self, "Same format %s — passthrough",
                        gst_video_format_to_string(in_fmt));
        gst_base_transform_set_in_place(trans, TRUE);
        self->_negotiated = TRUE;
        return TRUE;
    }

    gst_base_transform_set_in_place(trans, FALSE);

    // Build dst template for factory
    dxt::VideoFormat dst_fmt = gst_to_dxt_format(out_fmt);

    dxt::FrameDesc dst_template;
    dst_template.width       = width;
    dst_template.height      = height;
    dst_template.format      = dst_fmt;
    dst_template.memory_type = dxt::MemoryType::CPU_VIRTUAL;
    dst_template.num_planes  = dxt::num_planes_for_format(dst_fmt);

    if (dst_fmt == dxt::VideoFormat::I420) {
        dst_template.planes[0].stride = width;
        dst_template.planes[0].height = height;
        dst_template.planes[1].stride = width / 2;
        dst_template.planes[1].height = height / 2;
        dst_template.planes[2].stride = width / 2;
        dst_template.planes[2].height = height / 2;
    } else if (dst_fmt == dxt::VideoFormat::NV12) {
        dst_template.planes[0].stride = width;
        dst_template.planes[0].height = height;
        dst_template.planes[1].stride = width;
        dst_template.planes[1].height = height / 2;
    } else {
        dst_template.planes[0].stride = width * dxt::bytes_per_pixel(dst_fmt);
        dst_template.planes[0].height = height;
    }

    // Conversion only, no crop/scale/padding
    dxt::TransformOps ops;

    dxt::VideoFormat src_fmt_dxt = gst_to_dxt_format(in_fmt);

    auto kernel = dxt::VideoTransformFactory::create(dst_template, ops);

    // Verify the auto-selected backend supports our source format.
    // E.g. RGA accepts dst=RGB in init() but only handles NV12 input.
    if (kernel) {
        auto caps = kernel->capabilities();
        bool src_ok = false;
        for (auto &f : caps.src_formats) {
            if (f == src_fmt_dxt) { src_ok = true; break; }
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
            "No transform backend available for %s -> %s %dx%d",
            gst_video_format_to_string(in_fmt),
            gst_video_format_to_string(out_fmt), width, height);
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Convert %dx%d [%s -> %s] via %s",
                    width, height,
                    gst_video_format_to_string(in_fmt),
                    gst_video_format_to_string(out_fmt),
                    kernel->backend_name());

    self->_kernel     = kernel.release();
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

    if (!self->_negotiated) {
        GST_ERROR_OBJECT(self, "Caps not negotiated");
        return GST_FLOW_NOT_NEGOTIATED;
    }

    // Same format passthrough (kernel is null)
    if (!self->_kernel) {
        gst_buffer_copy_into(outbuf, inbuf,
                             static_cast<GstBufferCopyFlags>(
                                 GST_BUFFER_COPY_METADATA | GST_BUFFER_COPY_MEMORY),
                             0, -1);
        return GST_FLOW_OK;
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

    gint width  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    gint height = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    GstVideoFormat in_gst_fmt  = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    GstVideoFormat out_gst_fmt = GST_VIDEO_INFO_FORMAT(&self->_output_info);

    {
        dxt::FrameDesc src_desc;
        dxt::FrameDesc dst_desc;

        // Build src descriptor
        if (in_gst_fmt == GST_VIDEO_FORMAT_NV12) {
            src_desc = dxt::make_nv12_frame_desc(inbuf, width, height, &self->_input_info);
            src_desc.memory_type = dxt::MemoryType::CPU_VIRTUAL;
            src_desc.dma_fd = -1;
            src_desc.planes[0].data = in_map.data + src_desc.planes[0].offset;
            src_desc.planes[1].data = in_map.data + src_desc.planes[1].offset;
        } else if (in_gst_fmt == GST_VIDEO_FORMAT_I420) {
            src_desc = dxt::make_i420_frame_desc(self->_input_info);
            for (int i = 0; i < src_desc.num_planes; ++i)
                src_desc.planes[i].data = in_map.data + src_desc.planes[i].offset;
        } else {
            dxt::VideoFormat fmt = gst_to_dxt_format(in_gst_fmt);
            src_desc = dxt::make_packed_frame_desc(in_map.data, width, height, fmt);
        }

        // Build dst descriptor
        if (out_gst_fmt == GST_VIDEO_FORMAT_I420) {
            dst_desc = dxt::make_i420_frame_desc(self->_output_info);
            for (int i = 0; i < dst_desc.num_planes; ++i)
                dst_desc.planes[i].data = out_map.data + dst_desc.planes[i].offset;
        } else if (out_gst_fmt == GST_VIDEO_FORMAT_NV12) {
            // NV12 output: 2 planes (Y + interleaved UV)
            dst_desc.width       = width;
            dst_desc.height      = height;
            dst_desc.format      = dxt::VideoFormat::NV12;
            dst_desc.memory_type = dxt::MemoryType::CPU_VIRTUAL;
            dst_desc.num_planes  = 2;
            dst_desc.dma_fd      = -1;
            int y_stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, 0);
            dst_desc.planes[0].data   = out_map.data + GST_VIDEO_INFO_PLANE_OFFSET(&self->_output_info, 0);
            dst_desc.planes[0].stride = y_stride;
            dst_desc.planes[0].height = height;
            dst_desc.planes[1].data   = out_map.data + GST_VIDEO_INFO_PLANE_OFFSET(&self->_output_info, 1);
            dst_desc.planes[1].stride = GST_VIDEO_INFO_PLANE_STRIDE(&self->_output_info, 1);
            dst_desc.planes[1].height = height / 2;
        } else {
            dxt::VideoFormat fmt = gst_to_dxt_format(out_gst_fmt);
            dst_desc = dxt::make_packed_frame_desc(out_map.data, width, height, fmt);
        }

        auto result = self->_kernel->transform(src_desc, dst_desc);
        if (!result.success) {
            GST_ERROR_OBJECT(self, "Kernel color conversion failed (%s -> %s)",
                             gst_video_format_to_string(in_gst_fmt),
                             gst_video_format_to_string(out_gst_fmt));
            ret = GST_FLOW_ERROR;
        } else {
            gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
        }
    }

    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);
    return ret;
}
