#include "gst-dxosd.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>
#include "dxosd_common.hpp"

#ifdef HAVE_LIBRGA
#include <gst/allocators/gstdmabuf.h>
#endif


enum class PropertyID { PROP_0, PROP_WIDTH, PROP_HEIGHT, N_PROPERTIES };

GST_DEBUG_CATEGORY_STATIC(gst_dxosd_debug_category);
#define GST_CAT_DEFAULT gst_dxosd_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
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

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static void dxosd_set_property(GObject *object, guint property_id,
                               const GValue *value, GParamSpec *pspec) {
    GstDxOsd *self = GST_DXOSD(object);
    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_WIDTH:
        self->_width = g_value_get_int(value);
        break;
    case PropertyID::PROP_HEIGHT:
        self->_height = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxosd_get_property(GObject *object, guint property_id,
                               GValue *value, GParamSpec *pspec) {
    const GstDxOsd *self = GST_DXOSD(object);

    switch (static_cast<PropertyID>(property_id)) {
    case PropertyID::PROP_WIDTH:
        g_value_set_int(value, self->_width);
        break;
    case PropertyID::PROP_HEIGHT:
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
    const gchar *transition_name = gst_state_change_get_name(transition);
    GST_INFO_OBJECT(self, "State transition: %s", transition_name);
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_DEBUG_OBJECT(self, "State change completed: %d", result);
    return result;
}

static void dxosd_dispose(GObject *object) {
#ifdef HAVE_LIBRGA
#else
    GstDxOsd *self = GST_DXOSD(object);
    // Vectors automatically clean up, just clear the maps
    self->_resized_frame.~map();
    self->_convert_buffer.~map();
#endif
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxosd_class_init(GstDxOsdClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxosd_debug_category, "dxosd", 0,
                            "DXOsd plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxosd_set_property;
    gobject_class->get_property = dxosd_get_property;
    gobject_class->dispose = dxosd_dispose;

    static std::array<GParamSpec*, static_cast<int>(PropertyID::N_PROPERTIES)> obj_properties = {
        nullptr,
    };

    obj_properties[static_cast<int>(PropertyID::PROP_WIDTH)] = g_param_spec_int(
        "width", "Width", "Sets the width of each tile in the grid.", 0, 10000,
        640, G_PARAM_READWRITE);

    obj_properties[static_cast<int>(PropertyID::PROP_HEIGHT)] = g_param_spec_int(
        "height", "Height", "Sets the height of each tile in the grid.", 0,
        10000, 360, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, static_cast<int>(PropertyID::N_PROPERTIES),
                                      obj_properties.data());

    auto *element_class = GST_ELEMENT_CLASS(klass);
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

static gboolean parse_input_caps(GstDxOsd *self, GstCaps const *incaps) {
    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self,
                         "Failed to parse input caps or format not supported");
        return FALSE;
    }

    GST_INFO_OBJECT(self, "Input format: %s, %dx%d",
                    gst_video_format_to_string(self->_input_info.finfo->format),
                    GST_VIDEO_INFO_WIDTH(&self->_input_info),
                    GST_VIDEO_INFO_HEIGHT(&self->_input_info));
    return TRUE;
}

static GstCaps *create_and_fixate_output_caps(GstDxOsd *self, GstCaps *incaps) {
    if (self->_width <= 0 || self->_height <= 0) {
        GST_ERROR_OBJECT(self, "Output width/height property not set!");
        return nullptr;
    }

    auto *outcaps = gst_caps_copy(incaps);
    gst_caps_set_simple(outcaps, "format", G_TYPE_STRING, "BGR", "width",
                        G_TYPE_INT, self->_width, "height", G_TYPE_INT,
                        self->_height, nullptr);

    GstCaps *fixed = gst_caps_fixate(outcaps);
    if (fixed != outcaps)
        gst_caps_unref(outcaps);

    if (!fixed) {
        GST_ERROR_OBJECT(self, "Failed to fixate generated output caps!");
        return nullptr;
    }

    GST_INFO_OBJECT(self, "Fixated output CAPS %" GST_PTR_FORMAT, fixed);
    return fixed;
}

static gboolean parse_output_info(GstDxOsd *self) {
    if (!self->_output_caps || !gst_caps_is_fixed(self->_output_caps)) {
        GST_ERROR_OBJECT(
            self, "Output caps are null or not fixed before parsing info!");
        return FALSE;
    }

    if (!gst_video_info_from_caps(&self->_output_info, self->_output_caps)) {
        GST_ERROR_OBJECT(self, "Failed to parse generated output caps!");
        return FALSE;
    }

    GST_INFO_OBJECT(
        self, "Output info: %s, %dx%d, size: %" G_GSIZE_FORMAT,
        gst_video_format_to_string(self->_output_info.finfo->format),
        GST_VIDEO_INFO_WIDTH(&self->_output_info),
        GST_VIDEO_INFO_HEIGHT(&self->_output_info), self->_output_info.size);
    return TRUE;
}

static gboolean push_caps_event(GstDxOsd *self) {
    GST_INFO_OBJECT(
        self, "Pushing new CAPS %" GST_PTR_FORMAT " downstream via src pad",
        self->_output_caps);
    GstEvent *new_caps_event = gst_event_new_caps(self->_output_caps);
    if (!gst_pad_push_event(self->_srcpad, new_caps_event)) {
        GST_ERROR_OBJECT(self, "Failed to push new CAPS event downstream.");
        return FALSE;
    }
    GST_INFO_OBJECT(self, "Successfully pushed new CAPS event downstream.");
    return TRUE;
}

static gboolean gst_dxosd_sink_event(GstPad * pad, GstObject *parent,
                                     GstEvent *event) {
    std::ignore = pad;
    GstDxOsd *self = GST_DXOSD(parent);

    GST_DEBUG_OBJECT(self, "Received event [%s] on sink pad", GST_EVENT_TYPE_NAME(event));

    if (GST_EVENT_TYPE(event) == GST_EVENT_CAPS) {
        GstCaps *incaps = nullptr;
        GstCaps *fixed = nullptr; // <-- 선언 위치 이동
        gst_event_parse_caps(event, &incaps);
        GST_INFO_OBJECT(self, "Negotiating input caps: %" GST_PTR_FORMAT, incaps);

        if (!parse_input_caps(self, incaps))
            goto fail;

        fixed = create_and_fixate_output_caps(self, incaps);
        if (!fixed)
            goto fail;

        if (self->_output_caps)
            gst_caps_unref(self->_output_caps);
        self->_output_caps = fixed;

        if (!parse_output_info(self))
            goto fail;

        if (!push_caps_event(self))
            goto fail;

        gst_event_unref(event);
        return TRUE;

    fail:
        if (self->_output_caps) {
            gst_caps_unref(self->_output_caps);
            self->_output_caps = nullptr;
        }
        gst_event_unref(event);
        return FALSE;
    }
    return gst_pad_push_event(self->_srcpad, event);
}

static gboolean gst_dxosd_src_query(GstPad *pad, GstObject *parent,
                                    GstQuery *query) {
    GstDxOsd *self = GST_DXOSD(parent);
    GstCaps *filter_caps;
    GstCaps *result_caps;
    GstCaps *possible_caps;
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
    GST_INFO_OBJECT(self, "Initializing OSD element (width=%d, height=%d)", 640, 360);

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

    self->_output_caps = nullptr;
#ifdef HAVE_LIBRGA
#else
    new (&self->_resized_frame) std::map<int, std::vector<uint8_t>>();
    new (&self->_convert_buffer) std::map<int, std::vector<uint8_t>>();
#endif
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
void draw_rga(GstDxOsd *self, GstBuffer *buf, GstBuffer *outbuffer) {
    if (self->_width % 16 != 0) {
        GST_ERROR_OBJECT(self, "DXOSD output width must be 16 aligned ! \n");
        return;
    }

    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to get DXFrameMeta \n");
        return;
    }

    GST_DEBUG_OBJECT(self, "Rendering %zu objects with RGA (stream %d, %dx%d -> %dx%d)",
                     frame_meta->_object_meta_list.size(), frame_meta->_stream_id,
                     frame_meta->_width, frame_meta->_height, self->_width, self->_height);

    if (g_strcmp0(frame_meta->_format.c_str(), "NV12") != 0) {
        GST_ERROR_OBJECT(self, "not supported format (use NV12)! \n");
        return;
    }

    GstMapInfo output_map;
    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    if ((float)frame_meta->_width / self->_width <= 0.125 ||
        (float)frame_meta->_width / self->_width >= 8 ||
        (float)frame_meta->_height / self->_height <= 0.125 ||
        (float)frame_meta->_height / self->_height >= 8) {
        GST_ERROR_OBJECT(self, "DX OSD : scale check error, scale limit[1/8 ~ 8] \n");
        return;
    }

    if (frame_meta->_width < 68 || frame_meta->_height < 2 ||
        frame_meta->_width > 8176 || frame_meta->_height > 8176) {
        GST_ERROR_OBJECT(self, "DX OSD : resolution check error, input range[68x2 ~ "
                "8176x8176] \n");
        return;
    }

    if (self->_width < 68 || self->_height < 2 || self->_width > 8128 ||
        self->_height > 8128) {
        GST_ERROR_OBJECT(self, "DX OSD : resolution check error, output range[68x2 ~ "
                "8128x8128] \n");
        return;
    }

    // Calculate actual stride from buffer size (GstVideoInfo stride can be incorrect)
    GstMemory *mem = gst_buffer_peek_memory(buf, 0);
    gsize mem_size = gst_memory_get_sizes(mem, NULL, NULL);
    
    // NV12: Y plane (stride×hstride) + UV plane (stride×hstride/2)
    // mem_size = stride × hstride × 1.5
    int hstride = ((frame_meta->_height + 15) / 16) * 16;  // 16-aligned height
    int actual_stride = mem_size / (hstride * 3 / 2);
    rga_buffer_t src_img;
    GstMapInfo input_map;
    bool is_dmabuf = false;
    bool input_mapped = false;

    if (gst_is_dmabuf_memory(mem)) {
        gint fd = gst_dmabuf_memory_get_fd(mem);
        if (fd >= 0) {
            // DMA-Buffer path - zero-copy hardware acceleration
            src_img = wrapbuffer_fd(
                fd, frame_meta->_width, frame_meta->_height,
                RK_FORMAT_YCbCr_420_SP, actual_stride, hstride);
            is_dmabuf = true;
            GST_DEBUG_OBJECT(self, "Using DMA-Buffer zero-copy path (fd=%d)", fd);
        } else {
            GST_WARNING_OBJECT(self, "Failed to get DMA-Buffer fd, falling back to virtual memory");
        }
    }

    if (!is_dmabuf) {
        // Fallback - map to virtual memory
        if (!gst_buffer_map(buf, &input_map, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (input)\n");
            gst_buffer_unmap(outbuffer, &output_map);
            return;
        }
        input_mapped = true;
        src_img = wrapbuffer_virtualaddr(
            reinterpret_cast<void *>(input_map.data), frame_meta->_width,
            frame_meta->_height, RK_FORMAT_YCbCr_420_SP, actual_stride, hstride);
        GST_DEBUG_OBJECT(self, "Using virtual memory path");
    }

    rga_buffer_t dst_img =
        wrapbuffer_virtualaddr(reinterpret_cast<void *>(output_map.data),
                               self->_width, self->_height, RK_FORMAT_BGR_888);

    imconfig(IM_CONFIG_SCHEDULER_CORE,
             IM_SCHEDULER_RGA3_CORE0 | IM_SCHEDULER_RGA3_CORE1);
    int ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret) {
        GST_ERROR_OBJECT(self, "check error: %d - %s\n", ret,
                         imStrError((IM_STATUS)ret));
        return;
    }

    ret = improcess(src_img, dst_img, {}, {}, {}, {}, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS) {
        GST_ERROR_OBJECT(self, "RGA resize (imresize) failed: %d - %s\n", ret,
                         imStrError((IM_STATUS)ret));
        return;
    }

    auto surface =
        cv::Mat(self->_height, self->_width, CV_8UC3, output_map.data);

    float scale_factor_x = (float)frame_meta->_width / (float)self->_width;
    float scale_factor_y = (float)frame_meta->_height / (float)self->_height;

    size_t object_length = frame_meta->_object_meta_list.size();
    for (size_t i = 0; i < object_length; i++) {
        auto *obj_meta = frame_meta->_object_meta_list[i];
        draw_object_meta(surface, obj_meta, scale_factor_x, scale_factor_y);
    }

    if (input_mapped) gst_buffer_unmap(buf, &input_map);
    gst_buffer_unmap(outbuffer, &output_map);
}
#else
void draw(GstDxOsd *self, GstBuffer *buf, GstBuffer *outbuffer) {
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to get DXFrameMeta \n");
        return;
    }

    GST_DEBUG_OBJECT(self, "Rendering %zu objects with SW (stream %d, %dx%d -> %dx%d)",
                     frame_meta->_object_meta_list.size(), frame_meta->_stream_id,
                     frame_meta->_width, frame_meta->_height, self->_width, self->_height);

    GstMapInfo output_map;
    if (!gst_buffer_map(outbuffer, &output_map, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "DXOSD Failed to map GstBuffer (output)\n");
        return;
    }

    // Ensure buffers exist in cache
    int stream_id = frame_meta->_stream_id;
    if (self->_resized_frame.find(stream_id) == self->_resized_frame.end()) {
        self->_resized_frame[stream_id] = std::vector<uint8_t>();
    }
    if (self->_convert_buffer.find(stream_id) == self->_convert_buffer.end()) {
        self->_convert_buffer[stream_id] = std::vector<uint8_t>();
    }

    // Resize if needed
    bool resized = false;
    if (frame_meta->_width != self->_width || frame_meta->_height != self->_height) {
        Resize(buf, &self->_input_info, self->_resized_frame[stream_id],
               frame_meta->_width, frame_meta->_height, self->_width, self->_height,
               frame_meta->_format.c_str());
        resized = true;
    }

    // Convert color format with buffer reuse
    if (resized) {
        CvtColor(self->_resized_frame[stream_id], self->_convert_buffer[stream_id],
                 self->_width, self->_height, frame_meta->_format.c_str(), "BGR");
    } else {
        CvtColor(buf, &self->_input_info, self->_convert_buffer[stream_id],
                 self->_width, self->_height, frame_meta->_format.c_str(), "BGR");
    }

    // Copy to output buffer
    memcpy(output_map.data, self->_convert_buffer[stream_id].data(), 
           self->_convert_buffer[stream_id].size());

    // Draw on surface
    auto surface = cv::Mat(self->_height, self->_width, CV_8UC3, output_map.data);

    float scale_factor_x = (float)frame_meta->_width / (float)self->_width;
    float scale_factor_y = (float)frame_meta->_height / (float)self->_height;

    size_t object_length = frame_meta->_object_meta_list.size();
    for (size_t i = 0; i < object_length; i++) {
        const auto *obj_meta = frame_meta->_object_meta_list[i];
        draw_object_meta(surface, obj_meta, scale_factor_x, scale_factor_y);
    }

    gst_buffer_unmap(outbuffer, &output_map);
}
#endif

static GstFlowReturn gst_dxosd_chain(GstPad * pad, GstObject *parent,
                                     GstBuffer *buf) {
    std::ignore = pad;           
    GstDxOsd *self = GST_DXOSD(parent);

    GST_DEBUG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    GstBuffer *outbuf = nullptr;
    GstFlowReturn ret = GST_FLOW_OK;

    gsize out_size = self->_output_info.size;
    outbuf = gst_buffer_new_allocate(nullptr, out_size, nullptr);

    if (!gst_buffer_copy_into(outbuf, buf,
                              (GstBufferCopyFlags)(GST_BUFFER_COPY_FLAGS |
                                                   GST_BUFFER_COPY_TIMESTAMPS),
                              0, -1)) {
        GST_WARNING_OBJECT(self, "Failed to copy buffer metadata");
    }

    GST_BUFFER_OFFSET(outbuf) = GST_BUFFER_OFFSET(buf);
    GST_BUFFER_OFFSET_END(outbuf) = GST_BUFFER_OFFSET_END(buf);

#ifdef HAVE_LIBRGA
    draw_rga(self, buf, outbuf);
#else
    draw(self, buf, outbuf);
#endif

    gst_buffer_unref(buf);
    // GST_INFO_OBJECT(self, "[%d] Pushing buffer to src pad PTS: %" GST_TIME_FORMAT, frame_meta->_stream_id, GST_TIME_ARGS(GST_BUFFER_PTS(outbuf)));
    ret = gst_pad_push(self->_srcpad, outbuf);
    if (ret != GST_FLOW_OK) {
        GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
    }
    return ret;
}