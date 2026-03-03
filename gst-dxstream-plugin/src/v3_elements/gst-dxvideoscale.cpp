#include "gst-dxvideoscale.hpp"
#include <gst/video/video.h>
#include <opencv2/imgproc.hpp>
#include "dxcv/dxcvext.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxvideoscale_debug_category);
#define GST_CAT_DEFAULT gst_dxvideoscale_debug_category

// Function declarations
static void gst_dxvideoscale_finalize(GObject *object);
static GstFlowReturn gst_dxvideoscale_transform(GstBaseTransform *trans,
                                                GstBuffer *inbuf,
                                                GstBuffer *outbuf);
static gboolean gst_dxvideoscale_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps,
                                          GstCaps *outcaps);
static GstCaps *gst_dxvideoscale_transform_caps(GstBaseTransform *trans,
                                                 GstPadDirection direction,
                                                 GstCaps *caps,
                                                 GstCaps *filter);
static gboolean gst_dxvideoscale_transform_size(GstBaseTransform *trans,
                                                 GstPadDirection direction,
                                                 GstCaps *caps,
                                                 gsize size,
                                                 GstCaps *othercaps,
                                                 gsize *othersize);
static gboolean gst_dxvideoscale_start(GstBaseTransform *trans);
static gboolean gst_dxvideoscale_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxVideoScale, gst_dxvideoscale, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxvideoscale_debug_category, "dxvideoscale",
                            0, "debug category for dxvideoscale element"))

static void gst_dxvideoscale_class_init(GstDxVideoScaleClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->finalize = gst_dxvideoscale_finalize;

    // Add pad templates - support I420, RGB, BGR
    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new(
            "sink", GST_PAD_SINK, GST_PAD_ALWAYS,
            gst_caps_from_string(
                "video/x-raw, format=(string){ I420, RGB, BGR }")));

    gst_element_class_add_pad_template(
        element_class,
        gst_pad_template_new(
            "src", GST_PAD_SRC, GST_PAD_ALWAYS,
            gst_caps_from_string(
                "video/x-raw, format=(string){ I420, RGB, BGR }")));

    gst_element_class_set_static_metadata(
        element_class, "DXVideoScale", "Filter/Converter/Video/Scaler",
        "Scales video using OpenCV (I420, RGB, BGR)",
        "DeepX AI <support@deepx.ai>");

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxvideoscale_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxvideoscale_stop);
    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_dxvideoscale_transform);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_dxvideoscale_set_caps);
    base_transform_class->transform_caps = 
        GST_DEBUG_FUNCPTR(gst_dxvideoscale_transform_caps);
    base_transform_class->transform_size = 
        GST_DEBUG_FUNCPTR(gst_dxvideoscale_transform_size);
}

static void gst_dxvideoscale_init(GstDxVideoScale *self) {
    self->_negotiated = FALSE;
    self->_input_buffer = nullptr;
    self->_output_buffer = nullptr;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);
}

static void gst_dxvideoscale_finalize(GObject *object) {
    G_OBJECT_CLASS(gst_dxvideoscale_parent_class)->finalize(object);
}

static gboolean gst_dxvideoscale_start(GstBaseTransform *trans) {
    auto *self = GST_DXVIDEOSCALE(trans);
    self->_input_buffer = dxcvext::allocDspBuffer();
    self->_output_buffer = self->_input_buffer + 0xC00000; // 12MB offset
    return TRUE;
}

static gboolean gst_dxvideoscale_stop(GstBaseTransform *trans) {
    auto *self = GST_DXVIDEOSCALE(trans);
    self->_negotiated = FALSE;
    
    if (self->_input_buffer) {
        dxcvext::freeDspBuffer(self->_input_buffer);
        self->_input_buffer = nullptr;
        self->_output_buffer = nullptr;
    }
    
    return TRUE;
}

static GstCaps *gst_dxvideoscale_transform_caps(GstBaseTransform *trans,
                                                 GstPadDirection direction,
                                                 GstCaps *caps,
                                                 GstCaps *filter) {
    std::ignore = trans;
    std::ignore = direction;
    auto *ret_caps = gst_caps_copy(caps);
    
    // Allow width and height scaling, but limit to 2K (2560x1440) due to 12MB buffer limitation
    for (guint i = 0; i < gst_caps_get_size(ret_caps); i++) {
        GstStructure *structure = gst_caps_get_structure(ret_caps, i);
        gst_structure_set(structure,
                         "width", GST_TYPE_INT_RANGE, 1, 2560,
                         "height", GST_TYPE_INT_RANGE, 1, 1440,
                         NULL);
    }
    
    if (filter) {
        auto *tmp = gst_caps_intersect_full(ret_caps, filter,
                                               GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(ret_caps);
        ret_caps = tmp;
    }
    
    return ret_caps;
}

static gboolean gst_dxvideoscale_set_caps(GstBaseTransform *trans,
                                          GstCaps *incaps,
                                          GstCaps *outcaps) {

    auto *self = GST_DXVIDEOSCALE(trans);
    
    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }
    
    if (!gst_video_info_from_caps(&self->_output_info, outcaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse output caps");
        return FALSE;
    }
    
    // Check buffer size limitation (12MB per buffer)
    gint in_width = self->_input_info.width;
    gint in_height = self->_input_info.height;
    gint out_width = self->_output_info.width;
    gint out_height = self->_output_info.height;
    GstVideoFormat format = self->_input_info.finfo->format;
    
    // Calculate required buffer sizes
    gsize in_size = GST_VIDEO_INFO_SIZE(&self->_input_info);
    gsize out_size = GST_VIDEO_INFO_SIZE(&self->_output_info);
    
    const gsize MAX_BUFFER_SIZE = 0xC00000; // 12MB
    
    if (in_size > MAX_BUFFER_SIZE) {
        GST_ERROR_OBJECT(self, 
            "Input buffer size (%zu bytes) exceeds maximum (%zu bytes). "
            "Maximum supported resolution: 2560x1440 (2K) for RGB/BGR",
            in_size, MAX_BUFFER_SIZE);
        return FALSE;
    }
    
    if (out_size > MAX_BUFFER_SIZE) {
        GST_ERROR_OBJECT(self, 
            "Output buffer size (%zu bytes) exceeds maximum (%zu bytes). "
            "Maximum supported resolution: 2560x1440 (2K) for RGB/BGR",
            out_size, MAX_BUFFER_SIZE);
        return FALSE;
    }
    
    self->_negotiated = TRUE;
    
    GST_INFO_OBJECT(self, "Scaling %dx%d to %dx%d, format: %s (in: %zu bytes, out: %zu bytes)",
                    in_width, in_height, out_width, out_height,
                    gst_video_format_to_string(format),
                    in_size, out_size);
    
    return TRUE;
}

static gboolean gst_dxvideoscale_transform_size(GstBaseTransform *trans,
                                                 GstPadDirection direction,
                                                 GstCaps *caps,
                                                 gsize size,
                                                 GstCaps *othercaps,
                                                 gsize *othersize) {
    
    std::ignore = trans;
    std::ignore = direction;
    std::ignore = caps;
    std::ignore = size;
    std::ignore = othercaps;
    
    GstVideoInfo info;
    
    if (!gst_video_info_from_caps(&info, othercaps)) {
        return FALSE;
    }
    
    *othersize = GST_VIDEO_INFO_SIZE(&info);
    
    return TRUE;
}

static GstFlowReturn gst_dxvideoscale_transform(GstBaseTransform *trans,
                                                GstBuffer *inbuf,
                                                GstBuffer *outbuf) {
    GstDxVideoScale *self = GST_DXVIDEOSCALE(trans);
    GstMapInfo in_map = GST_MAP_INFO_INIT;
    GstMapInfo out_map = GST_MAP_INFO_INIT;
    
    if (!self->_negotiated) {
        GST_ERROR_OBJECT(self, "Caps not negotiated");
        return GST_FLOW_NOT_NEGOTIATED;
    }
    
    if (!gst_buffer_map(inbuf, &in_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map input buffer");
        return GST_FLOW_ERROR;
    }
    
    if (!gst_buffer_map(outbuf, &out_map, GST_MAP_WRITE)) {
        gst_buffer_unmap(inbuf, &in_map);
        GST_ERROR_OBJECT(self, "Failed to map output buffer");
        return GST_FLOW_ERROR;
    }
    
    gint in_width = self->_input_info.width;
    gint in_height = self->_input_info.height;
    gint out_width = self->_output_info.width;
    gint out_height = self->_output_info.height;
    GstVideoFormat format = self->_input_info.finfo->format;
    GstFlowReturn ret = GST_FLOW_OK;
    
    // If size is the same, just copy
    if (in_width == out_width && in_height == out_height) {
        memcpy(out_map.data, in_map.data, in_map.size);
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
        goto cleanup;
    }

    // Copy data to pre-allocated buffer
    memcpy(self->_input_buffer, in_map.data, in_map.size);

    // Create input Mat based on format
    if (format == GST_VIDEO_FORMAT_I420) {
        try {
            uint8_t* src_y = self->_input_buffer;
            uint8_t* src_u = self->_input_buffer + self->_input_info.offset[1];
            uint8_t* src_v = self->_input_buffer + self->_input_info.offset[2];

            uint8_t* dst_y = self->_output_buffer;
            uint8_t* dst_u = self->_output_buffer + self->_output_info.offset[1];
            uint8_t* dst_v = self->_output_buffer + self->_output_info.offset[2];
            
            dxcvext::I420Scale(src_y, self->_input_info.stride[0], 
                              src_u, self->_input_info.stride[1], 
                              src_v, self->_input_info.stride[2], 
                              in_width, in_height,
                              dst_y, self->_output_info.stride[0], 
                              dst_u, self->_output_info.stride[1], 
                              dst_v, self->_output_info.stride[2], 
                              out_width, out_height);
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(self, "Exception during I420 scaling: %s", e.what());
            ret = GST_FLOW_ERROR;
        }
    } else if (format == GST_VIDEO_FORMAT_RGB) {
        try {
            dxcvext::RGB24Scale(self->_input_buffer, self->_input_info.stride[0], in_width, in_height,
                                self->_output_buffer, self->_output_info.stride[0], out_width, out_height);
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(self, "Exception during RGB24 scaling: %s", e.what());
            ret = GST_FLOW_ERROR;
        }
    } else if (format == GST_VIDEO_FORMAT_BGR) {
        try {
            dxcvext::BGR24Scale(self->_input_buffer, self->_input_info.stride[0], in_width, in_height,
                                self->_output_buffer, self->_output_info.stride[0], out_width, out_height);
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(self, "Exception during BGR24 scaling: %s", e.what());
            ret = GST_FLOW_ERROR;
        }
    } else {
        GST_ERROR_OBJECT(self, "Unsupported format for scaling: %s",
                         gst_video_format_to_string(format));
        ret = GST_FLOW_ERROR;
    }

    if (ret == GST_FLOW_OK) {
        // Copy scaled data to output buffer
        memcpy(out_map.data, self->_output_buffer, out_map.size);
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
    }
    
cleanup:
    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);
    
    return ret;
}
