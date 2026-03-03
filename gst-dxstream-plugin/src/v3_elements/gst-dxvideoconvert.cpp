#include "gst-dxvideoconvert.hpp"
#include <gst/video/video.h>
#include <opencv2/imgproc.hpp>
#include "dxcv/dxcvext.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxvideoconvert_debug_category);
#define GST_CAT_DEFAULT gst_dxvideoconvert_debug_category

// Function declarations
static void gst_dxvideoconvert_finalize(GObject *object);
static GstFlowReturn gst_dxvideoconvert_transform(GstBaseTransform *trans,
                                                  GstBuffer *inbuf,
                                                  GstBuffer *outbuf);
static gboolean gst_dxvideoconvert_set_caps(GstBaseTransform *trans,
                                            GstCaps *incaps,
                                            GstCaps *outcaps);
static GstCaps *gst_dxvideoconvert_transform_caps(GstBaseTransform *trans,
                                                   GstPadDirection direction,
                                                   GstCaps *caps,
                                                   GstCaps *filter);
static gboolean gst_dxvideoconvert_transform_size(GstBaseTransform *trans,
                                                   GstPadDirection direction,
                                                   GstCaps *caps,
                                                   gsize size,
                                                   GstCaps *othercaps,
                                                   gsize *othersize);
static gboolean gst_dxvideoconvert_start(GstBaseTransform *trans);
static gboolean gst_dxvideoconvert_stop(GstBaseTransform *trans);

G_DEFINE_TYPE_WITH_CODE(
    GstDxVideoConvert, gst_dxvideoconvert, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxvideoconvert_debug_category, "dxvideoconvert",
                            0, "debug category for dxvideoconvert element"))

static void gst_dxvideoconvert_class_init(GstDxVideoConvertClass *klass) {
    auto *gobject_class = G_OBJECT_CLASS(klass);
    auto *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->finalize = gst_dxvideoconvert_finalize;

    // Add pad templates
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
        element_class, "DXVideoConvert", "Filter/Converter/Video",
        "Converts video format using OpenCV (I420, RGB, BGR)",
        "DeepX AI <support@deepx.ai>");

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxvideoconvert_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxvideoconvert_stop);
    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_dxvideoconvert_transform);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_dxvideoconvert_set_caps);
    base_transform_class->transform_caps = 
        GST_DEBUG_FUNCPTR(gst_dxvideoconvert_transform_caps);
    base_transform_class->transform_size = 
        GST_DEBUG_FUNCPTR(gst_dxvideoconvert_transform_size);
}

static void gst_dxvideoconvert_init(GstDxVideoConvert *self) {
    self->_negotiated = FALSE;
    gst_video_info_init(&self->_input_info);
    gst_video_info_init(&self->_output_info);

    self->_input_buffer = nullptr;
    self->_output_buffer = nullptr;
}

static void gst_dxvideoconvert_finalize(GObject *object) {
    G_OBJECT_CLASS(gst_dxvideoconvert_parent_class)->finalize(object);
}

static gboolean gst_dxvideoconvert_start(GstBaseTransform *trans) {
    auto *self = GST_DXVIDEOCONVERT(trans);
    self->_input_buffer = dxcvext::allocDspBuffer();
    self->_output_buffer = self->_input_buffer + 0xC00000; // 12MB offset
    return TRUE;
}

static gboolean gst_dxvideoconvert_stop(GstBaseTransform *trans) {
    auto *self = GST_DXVIDEOCONVERT(trans);
    self->_negotiated = FALSE;
    
    if (self->_input_buffer) {
        dxcvext::freeDspBuffer(self->_input_buffer);
        self->_input_buffer = nullptr;
        self->_output_buffer = nullptr;
    }

    return TRUE;
}

static GstCaps *gst_dxvideoconvert_transform_caps(GstBaseTransform *trans,
                                                   GstPadDirection direction,
                                                   GstCaps *caps,
                                                   GstCaps *filter) {
    std::ignore = trans;
    std::ignore = direction;
    std::ignore = caps;
    std::ignore = filter;

    GstCaps *ret_caps = nullptr;
    
    // Create caps with resolution limits (2K max due to 12MB buffer limitation)
    ret_caps = gst_caps_new_simple("video/x-raw",
                                   "format", G_TYPE_STRING, "I420",
                                   "width", GST_TYPE_INT_RANGE, 1, 2560,
                                   "height", GST_TYPE_INT_RANGE, 1, 1440,
                                   NULL);
    gst_caps_append(ret_caps, gst_caps_new_simple("video/x-raw",
                                                   "format", G_TYPE_STRING, "RGB",
                                                   "width", GST_TYPE_INT_RANGE, 1, 2560,
                                                   "height", GST_TYPE_INT_RANGE, 1, 1440,
                                                   NULL));
    gst_caps_append(ret_caps, gst_caps_new_simple("video/x-raw",
                                                   "format", G_TYPE_STRING, "BGR",
                                                   "width", GST_TYPE_INT_RANGE, 1, 2560,
                                                   "height", GST_TYPE_INT_RANGE, 1, 1440,
                                                   NULL));
    
    // Copy framerate, width, height from input caps
    for (guint i = 0; i < gst_caps_get_size(caps); i++) {
        GstStructure *structure = gst_caps_get_structure(caps, i);
        for (guint j = 0; j < gst_caps_get_size(ret_caps); j++) {
            GstStructure *ret_structure = gst_caps_get_structure(ret_caps, j);
            
            const GValue *framerate = gst_structure_get_value(structure, "framerate");
            const GValue *width = gst_structure_get_value(structure, "width");
            const GValue *height = gst_structure_get_value(structure, "height");
            
            if (framerate)
                gst_structure_set_value(ret_structure, "framerate", framerate);
            if (width)
                gst_structure_set_value(ret_structure, "width", width);
            if (height)
                gst_structure_set_value(ret_structure, "height", height);
        }
    }
    
    if (filter) {
        GstCaps *tmp = gst_caps_intersect_full(ret_caps, filter,
                                               GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(ret_caps);
        ret_caps = tmp;
    }
    
    return ret_caps;
}

static gboolean gst_dxvideoconvert_set_caps(GstBaseTransform *trans,
                                            GstCaps *incaps,
                                            GstCaps *outcaps) {
    GstDxVideoConvert *self = GST_DXVIDEOCONVERT(trans);
    
    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }
    
    if (!gst_video_info_from_caps(&self->_output_info, outcaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse output caps");
        return FALSE;
    }
    
    // Check buffer size limitation (12MB per buffer)
    gint width = self->_input_info.width;
    gint height = self->_input_info.height;
    GstVideoFormat in_format = self->_input_info.finfo->format;
    GstVideoFormat out_format = self->_output_info.finfo->format;
    
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
    
    GST_INFO_OBJECT(self, "Converting %dx%d from %s to %s (in: %zu bytes, out: %zu bytes)",
                    width, height,
                    gst_video_format_to_string(in_format),
                    gst_video_format_to_string(out_format),
                    in_size, out_size);
    
    self->_negotiated = TRUE;
    
    return TRUE;
}

static gboolean gst_dxvideoconvert_transform_size(GstBaseTransform *trans,
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
    std::ignore = othersize;

    GstDxVideoConvert *self = GST_DXVIDEOCONVERT(trans);
    GstVideoInfo info;
    
    if (!gst_video_info_from_caps(&info, othercaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse caps for size calculation");
        return FALSE;
    }
    
    *othersize = GST_VIDEO_INFO_SIZE(&info);
    
    return TRUE;
}

static GstFlowReturn gst_dxvideoconvert_transform(GstBaseTransform *trans,
                                                  GstBuffer *inbuf,
                                                  GstBuffer *outbuf) {
    GstDxVideoConvert *self = GST_DXVIDEOCONVERT(trans);
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
    
    GstVideoFormat in_format = self->_input_info.finfo->format;
    GstVideoFormat out_format = self->_output_info.finfo->format;
    GstFlowReturn ret = GST_FLOW_OK;
    
    // If formats are the same, just copy
    if (in_format == out_format) {
        memcpy(out_map.data, in_map.data, in_map.size);
        gst_buffer_unmap(outbuf, &out_map);
        gst_buffer_unmap(inbuf, &in_map);
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
        return GST_FLOW_OK;
    }

    cv::Mat input_frame;
    cv::Mat output_frame;

    // Get dimensions from video info
    int width = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    int height = GST_VIDEO_INFO_HEIGHT(&self->_input_info);

    // Copy data to pre-allocated buffers
    memcpy(self->_input_buffer, in_map.data, in_map.size);
    
    // Create input Mat based on format
    if (in_format == GST_VIDEO_FORMAT_I420) {
        input_frame = cv::Mat(height + height / 2, width, CV_8UC1, self->_input_buffer);
    } else {
        input_frame = cv::Mat(height, width, CV_8UC3, self->_input_buffer);
    }
    
    // Create output Mat based on format
    if (out_format == GST_VIDEO_FORMAT_I420) {
        output_frame = cv::Mat(height + height / 2, width, CV_8UC1, self->_output_buffer);
    } else {
        output_frame = cv::Mat(height, width, CV_8UC3, self->_output_buffer);
    }

    // Perform conversion using OpenCV
    if (in_format == GST_VIDEO_FORMAT_I420 && (out_format == GST_VIDEO_FORMAT_RGB || out_format == GST_VIDEO_FORMAT_BGR)) {
        // memory pointer with offset from input_info
        uint8_t* src_y = self->_input_buffer;
        uint8_t* src_u = self->_input_buffer + self->_input_info.offset[1];
        uint8_t* src_v = self->_input_buffer + self->_input_info.offset[2];
        
        if (out_format == GST_VIDEO_FORMAT_RGB) {
            dxcvext::I420ToRGB24(src_y, self->_input_info.stride[0], src_u, self->_input_info.stride[1], src_v, self->_input_info.stride[2],
                              self->_output_buffer, self->_output_info.stride[0], self->_input_info.width, self->_input_info.height);
        } else {
            dxcvext::I420ToBGR24(src_y, self->_input_info.stride[0], src_u, self->_input_info.stride[1], src_v, self->_input_info.stride[2],
                              self->_output_buffer, self->_output_info.stride[0], self->_input_info.width, self->_input_info.height);
        }
    } else if ((in_format == GST_VIDEO_FORMAT_RGB || in_format == GST_VIDEO_FORMAT_BGR) && out_format == GST_VIDEO_FORMAT_I420) {
        uint8_t* dst_y = self->_output_buffer;
        uint8_t* dst_u = self->_output_buffer + self->_output_info.offset[1];
        uint8_t* dst_v = self->_output_buffer + self->_output_info.offset[2];
        
        if (in_format == GST_VIDEO_FORMAT_RGB) {
            dxcvext::RGB24ToI420(self->_input_buffer, self->_input_info.stride[0], dst_y, self->_output_info.stride[0], 
                              dst_u, self->_output_info.stride[1], dst_v, self->_output_info.stride[2], self->_input_info.width, self->_input_info.height);
        } else {
            dxcvext::BGR24ToI420(self->_input_buffer, self->_input_info.stride[0], dst_y, self->_output_info.stride[0], 
                              dst_u, self->_output_info.stride[1], dst_v, self->_output_info.stride[2], self->_input_info.width, self->_input_info.height);
        }
    } else {
        GST_ERROR_OBJECT(self, "Unsupported format conversion: %s to %s",
                         gst_video_format_to_string(in_format),
                         gst_video_format_to_string(out_format));
        ret = GST_FLOW_NOT_SUPPORTED;
    }

    // Copy converted data to output buffer
    memcpy(out_map.data, self->_output_buffer, out_map.size);

    // Unmap GStreamer buffers
    gst_buffer_unmap(inbuf, &in_map);
    gst_buffer_unmap(outbuf, &out_map);
    
    if (ret == GST_FLOW_OK) {
        gst_buffer_copy_into(outbuf, inbuf, GST_BUFFER_COPY_METADATA, 0, -1);
    }
    
    return ret;
}
