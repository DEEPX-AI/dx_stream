#include "gst-dxmsgconv.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxmsgmeta.hpp"
#include "gst-dxmsgmeta.hpp"
#include "transforms/gst_frame_desc.hpp"
#include "transforms/video_transform_factory.hpp"
#include <dlfcn.h>
#include <json-glib/json-glib.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

enum class PropertyID {
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_LIBRARY_FILE_PATH,
    PROP_MESSAGE_INTERVAL,
    PROP_INCLUDE_FRAME
};

GST_DEBUG_CATEGORY_STATIC(gst_dxmsgconv_debug_category);
#define GST_CAT_DEFAULT gst_dxmsgconv_debug_category

static GstFlowReturn gst_dxmsgconv_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf);
static gboolean gst_dxmsgconv_start(GstBaseTransform *trans);
static gboolean gst_dxmsgconv_stop(GstBaseTransform *trans);
static gboolean gst_dxmsgconv_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps);

G_DEFINE_TYPE(GstDxMsgConv, gst_dxmsgconv, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static GstStateChangeReturn dxmsgconv_change_state(GstElement *element,
                                                   GstStateChange transition) {
    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    return result;
}

static void dxmsgconv_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void parse_config(GstDxMsgConv *self) {
    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        g_print("Config file does not exist: %s\n", self->_config_file_path);
        return;
    }

    JsonParser *parser = json_parser_new();
    GError *error = nullptr;
    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        g_warning("Failed to load config file: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    if (!node) {
        g_warning("Config file has no root node");
        g_object_unref(parser);
        return;
    }

    JsonObject *object = json_node_get_object(node);

    if (json_object_has_member(object, "library_file_path")) {
        const gchar *path =
            json_object_get_string_member(object, "library_file_path");
        g_object_set(self, "library-file-path", path, nullptr);
    }

    if (json_object_has_member(object, "message_interval")) {
        gint64 interval = json_object_get_int_member(object, "message_interval");
        g_object_set(self, "message-interval", (gint)interval, nullptr);
    }

    if (json_object_has_member(object, "include_frame")) {
        gboolean include_frame =
            json_object_get_boolean_member(object, "include_frame");
        g_object_set(self, "include-frame", include_frame, nullptr);
    }

    g_object_unref(parser);
}

static void gst_dxmsgconv_set_property(GObject *object, guint prop_id,
                                       const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXMSGCONV(object);

    switch (static_cast<PropertyID>(prop_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        g_free(self->_config_file_path);
        self->_config_file_path = g_value_dup_string(value);
        parse_config(self);
        break;
    case PropertyID::PROP_LIBRARY_FILE_PATH:
        g_free(self->_library_file_path);
        self->_library_file_path = g_value_dup_string(value);
        break;
    case PropertyID::PROP_MESSAGE_INTERVAL:
        self->_message_interval = g_value_get_int(value);
        break;
    case PropertyID::PROP_INCLUDE_FRAME:
        self->_include_frame = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgconv_get_property(GObject *object, guint prop_id,
                                       GValue *value, GParamSpec *pspec) {

    const auto *self = GST_DXMSGCONV(object);

    switch (static_cast<PropertyID>(prop_id)) {
    case PropertyID::PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, self->_config_file_path);
        break;
    case PropertyID::PROP_LIBRARY_FILE_PATH:
        g_value_set_string(value, self->_library_file_path);
        break;
    case PropertyID::PROP_MESSAGE_INTERVAL:
        g_value_set_int(value, self->_message_interval);
        break;
    case PropertyID::PROP_INCLUDE_FRAME:
        g_value_set_boolean(value, self->_include_frame);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxmsgconv_class_init(GstDxMsgConvClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxmsgconv_debug_category, "dxmsgconv", 0,
                            "debug category for dxmsgconv element");

    auto *gobject_class = (GObjectClass *)klass;
    auto *element_class = (GstElementClass *)klass;

    gobject_class->dispose = dxmsgconv_dispose;
    gobject_class->set_property = gst_dxmsgconv_set_property;
    gobject_class->get_property = gst_dxmsgconv_get_property;

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_CONFIG_FILE_PATH),
        g_param_spec_string("config-file-path", "Config File Path",
                            "Path to the configuration file containing private "
                            "properties for message formats. (optional).",
                            nullptr, G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_LIBRARY_FILE_PATH),
        g_param_spec_string(
            "library-file-path", "Library File Path",
            "Path to the custom message converter library. Required.", nullptr,
            G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_MESSAGE_INTERVAL),
        g_param_spec_int(
            "message-interval", "Message Interval",
            "Frame interval at which message is converted (optional).", 1,
            10000, 1, G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, static_cast<guint>(PropertyID::PROP_INCLUDE_FRAME),
        g_param_spec_boolean(
            "include-frame", "Include Frame",
            "Flag whether to include frame data as base64 JPEG in the message. (optional).",
            FALSE, G_PARAM_READWRITE));

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->set_caps =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_set_caps);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxmsgconv_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxmsgconv_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_transform_ip);

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxmsgconv_change_state;

    gst_element_class_set_details_simple(element_class, "DXMsgConv", "Generic",
                                         "DX Message Converter",
                                         "JB Lim <jblim@dxsolution.kr>");
}

static void gst_dxmsgconv_init(GstDxMsgConv *self) {
    GST_TRACE_OBJECT(self, "init");

    self->_seq_id = 0;
    self->_config_file_path = nullptr;
    self->_library_file_path = nullptr;
    self->_library_handle = nullptr;
    self->_message_interval = 1;
    self->_include_frame = FALSE;
    self->_cached_width = 0;
    self->_cached_height = 0;
    self->_cached_format = GST_VIDEO_FORMAT_UNKNOWN;
}

static gboolean gst_dxmsgconv_start(GstBaseTransform *trans) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    GST_DEBUG_OBJECT(self, "start");

    if (self->_library_file_path == nullptr) {
        GST_ERROR_OBJECT(self, "dxmsgconv custom library is not set\n");
        return FALSE;
    }

    self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
    if (!self->_library_handle) {
        GST_ERROR_OBJECT(self, "dxmsgconv custom library: %s\n", dlerror());
        return FALSE;
    }
    self->_create_context_function = (DXMsg_CreateContextFptr)dlsym(
        self->_library_handle, "dxmsg_create_context");
    self->_delete_context_function = (DXMsg_DeleteContextFptr)dlsym(
        self->_library_handle, "dxmsg_delete_context");
    self->_convert_payload_function = (DXMsg_ConvertPayloadFptr)dlsym(
        self->_library_handle, "dxmsg_convert_payload");

    if (!self->_create_context_function ||
        !self->_delete_context_function ||
        !self->_convert_payload_function) {
        GST_ERROR_OBJECT(self, "dxmsgconv loading functions: %s\n", dlerror());
        if (self->_library_handle) {
            dlclose(self->_library_handle);
            self->_library_handle = nullptr;
        }
        return FALSE;
    }

    self->_context = self->_create_context_function();

    return TRUE;
}

static gboolean gst_dxmsgconv_stop(GstBaseTransform *trans) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    GST_DEBUG_OBJECT(trans, "stop");

    self->_rgb_kernel.reset();
    self->_rgb_buf.clear();
    self->_rgb_buf.shrink_to_fit();
    self->_jpeg_buf.clear();
    self->_jpeg_buf.shrink_to_fit();

    if (self->_context) {
        self->_delete_context_function(self->_context);
        self->_context = nullptr;
    }

    if (self->_library_handle) {
        dlclose(self->_library_handle);
        self->_library_handle = nullptr;
    }
    return TRUE;
}

static dxt::VideoFormat gst_to_dxt_format(GstVideoFormat fmt) {
    switch (fmt) {
    case GST_VIDEO_FORMAT_I420: return dxt::VideoFormat::I420;
    case GST_VIDEO_FORMAT_NV12: return dxt::VideoFormat::NV12;
    case GST_VIDEO_FORMAT_RGB:  return dxt::VideoFormat::RGB;
    case GST_VIDEO_FORMAT_BGR:  return dxt::VideoFormat::BGR;
    default:
        GST_WARNING("gst_to_dxt_format: unsupported format %d", fmt);
        return dxt::VideoFormat::NV12;
    }
}

static dxt::FrameDesc build_src_frame_desc(GstBuffer *buf,
                                           const GstVideoInfo *vinfo) {
    int w = GST_VIDEO_INFO_WIDTH(vinfo);
    int h = GST_VIDEO_INFO_HEIGHT(vinfo);
    GstVideoFormat gst_fmt = GST_VIDEO_INFO_FORMAT(vinfo);

    switch (gst_fmt) {
    case GST_VIDEO_FORMAT_NV12:
        return dxt::make_nv12_frame_desc(buf, w, h, vinfo);
    case GST_VIDEO_FORMAT_I420:
        return dxt::make_i420_frame_desc(*vinfo);
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR: {
        dxt::FrameDesc desc;
        desc.width  = w;
        desc.height = h;
        desc.format = gst_to_dxt_format(gst_fmt);
        desc.num_planes = 1;
        desc.planes[0].data   = nullptr;
        desc.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 0);
        desc.planes[0].height = h;
        desc.planes[0].offset = 0;
        return desc;
    }
    default:
        GST_WARNING("build_src_frame_desc: unsupported format %d", gst_fmt);
        return {};
    }
}

static gchar *encode_frame_to_base64(GstDxMsgConv *self, GstBuffer *buf) {
    int w = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    int h = GST_VIDEO_INFO_HEIGHT(&self->_input_info);

    // Build src FrameDesc
    dxt::FrameDesc src_desc = build_src_frame_desc(buf, &self->_input_info);

    // Map buffer and fill plane data pointers
    GstMapInfo map;
    if (!gst_buffer_map(buf, &map, GST_MAP_READ)) {
        GST_WARNING_OBJECT(self, "Failed to map buffer for frame encoding");
        return nullptr;
    }

    for (int i = 0; i < src_desc.num_planes; ++i) {
        src_desc.planes[i].data = map.data + src_desc.planes[i].offset;
    }

    // Build dst FrameDesc (RGB) — reuse member buffer
    self->_rgb_buf.resize(w * h * 3);
    dxt::FrameDesc dst_desc = dxt::make_packed_frame_desc(
        self->_rgb_buf.data(), w, h, dxt::VideoFormat::RGB);

    // Transform to RGB
    auto result = self->_rgb_kernel->transform(src_desc, dst_desc);
    gst_buffer_unmap(buf, &map);

    if (!result.success) {
        GST_WARNING_OBJECT(self, "Frame color conversion failed");
        return nullptr;
    }

    // JPEG encode — OpenCV expects BGR channel order
    cv::Mat rgb_mat(h, w, CV_8UC3, self->_rgb_buf.data(), w * 3);
    cv::cvtColor(rgb_mat, rgb_mat, cv::COLOR_RGB2BGR);
    self->_jpeg_buf.clear();
    if (!cv::imencode(".jpg", rgb_mat, self->_jpeg_buf) || self->_jpeg_buf.empty()) {
        GST_WARNING_OBJECT(self, "JPEG encoding failed");
        return nullptr;
    }

    return g_base64_encode(self->_jpeg_buf.data(), self->_jpeg_buf.size());
}

static void ensure_rgb_kernel(GstDxMsgConv *self) {
    if (self->_rgb_kernel) return;

    int w = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    int h = GST_VIDEO_INFO_HEIGHT(&self->_input_info);

    dxt::FrameDesc dst_template;
    dst_template.width  = w;
    dst_template.height = h;
    dst_template.format = dxt::VideoFormat::RGB;
    dst_template.num_planes = 1;
    dst_template.planes[0] = { nullptr, w * 3, h, 0 };

    dxt::TransformOps ops;
    self->_rgb_kernel = dxt::VideoTransformFactory::create(dst_template, ops);

    if (self->_rgb_kernel) {
        GST_INFO_OBJECT(self, "include-frame: using %s backend for RGB conversion",
                        self->_rgb_kernel->backend_name());
    } else {
        GST_ERROR_OBJECT(self, "include-frame: failed to create transform kernel, disabling include-frame");
        self->_include_frame = FALSE;
    }
}

void convert(GstDxMsgConv *self, DXFrameMeta *frame_meta, GstBuffer *buf) {
    if (self->_message_interval == 0 ||
        (self->_seq_id % self->_message_interval) == 0) {
        GstDxMsgMetaInfo meta_info;
        meta_info._frame_meta = frame_meta;
        meta_info._seq_id = self->_seq_id;
        meta_info._input_info = &self->_input_info;
        meta_info._include_frame = self->_include_frame;
        meta_info._frame_base64 = nullptr;

        gchar *base64_str = nullptr;
        if (self->_include_frame && self->_rgb_kernel) {
            base64_str = encode_frame_to_base64(self, buf);
            if (!base64_str) {
                GST_WARNING_OBJECT(self, "Frame encoding failed, frameData will be null");
            }
            meta_info._frame_base64 = base64_str;
        }

        auto *payload =
            self->_convert_payload_function(self->_context, &meta_info);

        if (!payload) {
            GST_ERROR_OBJECT(self, "convert_payload_function returned null");
            g_free(base64_str);
            return;
        }

        dx_add_payload_to_buffer(buf, payload);

        g_free(payload->_data);
        g_free(payload);
        g_free(base64_str);

    } else {
        GST_DEBUG_OBJECT(self, "skip seq:%lu, _message_interval: %d",
                         self->_seq_id, self->_message_interval);
    }
}

// ---------------------------------------------------------------------------
// set_caps — configure video info and reset RGB kernel on caps change
// ---------------------------------------------------------------------------
static gboolean gst_dxmsgconv_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    std::ignore = outcaps;

    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }

    self->_cached_width  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    self->_cached_height = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    self->_cached_format = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    self->_rgb_kernel.reset();

    GST_INFO_OBJECT(self, "Caps set: %dx%d format=%s",
                    self->_cached_width, self->_cached_height,
                    gst_video_format_to_string(self->_cached_format));
    return TRUE;
}

static GstFlowReturn gst_dxmsgconv_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);

    GST_DEBUG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT " seq=%" G_GUINT64_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(buf)), self->_seq_id + 1);

    self->_seq_id++;

    if (self->_include_frame && !self->_rgb_kernel) {
        ensure_rgb_kernel(self);
    }

    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }
    convert(self, frame_meta, buf);

    return GST_FLOW_OK;
}
