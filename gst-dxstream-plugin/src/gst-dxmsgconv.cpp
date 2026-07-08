#include "gst-dxmsgconv.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxmsgmeta.hpp"
#include "gst-dxmsgmeta.hpp"
#include "transforms/gst_frame_desc.hpp"
#include "transforms/video_transform_factory.hpp"
#include "utils.hpp"
#include "dx_dlfcn.h"
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
static gboolean gst_dxmsgconv_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query);
static gboolean gst_dxmsgconv_query(GstBaseTransform *trans,
                                    GstPadDirection direction,
                                    GstQuery *query);

G_DEFINE_TYPE(GstDxMsgConv, gst_dxmsgconv, GST_TYPE_BASE_TRANSFORM);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static gboolean string_is_empty(const gchar *value) {
    return value == nullptr || value[0] == '\0';
}

static void dxmsgconv_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxmsgconv_finalize(GObject *object) {
    auto *self = GST_DXMSGCONV(object);

    g_free(self->_config_file_path);
    g_free(self->_library_file_path);

    self->_kernel_pool.~unique_ptr();
    self->_rgb_buf.~vector();
    self->_jpeg_buf.~vector();
    self->_seq_ids.~map();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void parse_config(GstDxMsgConv *self) {

    if (string_is_empty(self->_config_file_path)) {
        return;
    }

    if (!g_file_test(self->_config_file_path, G_FILE_TEST_EXISTS)) {
        GST_ERROR_OBJECT(self, "Config file does not exist: %s", self->_config_file_path);
        return;
    }

    GST_INFO_OBJECT(self, "Loading msgconv config file: %s", self->_config_file_path);
    JsonParser *parser = json_parser_new();
    GError *error = nullptr;
    if (!json_parser_load_from_file(parser, self->_config_file_path, &error)) {
        GST_WARNING_OBJECT(self, "Failed to load config file: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *node = json_parser_get_root(parser);
    if (!node) {
        GST_WARNING_OBJECT(self, "Config file has no root node");
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
    gobject_class->finalize = gst_dxmsgconv_finalize;
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

    GstCaps *video_caps = gst_caps_from_string(
        DX_VIDEORAW_CAPS_STR "; "
        "video/x-raw, format=(string){ NV12, I420, RGB, BGR }");
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, video_caps));
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, video_caps));
    gst_caps_unref(video_caps);

    auto *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->set_caps =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_set_caps);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxmsgconv_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxmsgconv_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_transform_ip);
    base_transform_class->propose_allocation =
        GST_DEBUG_FUNCPTR(gst_dxmsgconv_propose_allocation);
    base_transform_class->query = GST_DEBUG_FUNCPTR(gst_dxmsgconv_query);

    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));

    gst_element_class_set_details_simple(element_class, "DXMsgConv", "Generic",
                                         "DX Message Converter",
                                         "Sangil Jo <sijo@deepx.ai>");
}

static void gst_dxmsgconv_init(GstDxMsgConv *self) {
    GST_TRACE_OBJECT(self, "init");

    new (&self->_kernel_pool) std::unique_ptr<dxt::TransformKernelPool>();
    new (&self->_rgb_buf) std::vector<uint8_t>();
    new (&self->_jpeg_buf) std::vector<unsigned char>();
    new (&self->_seq_ids) std::map<int, guint64>();

    self->_config_file_path = nullptr;
    self->_library_file_path = nullptr;
    self->_library_handle = nullptr;
    self->_message_interval = 1;
    self->_include_frame = FALSE;
    self->_cached_width = 0;
    self->_cached_height = 0;
    self->_cached_format = GST_VIDEO_FORMAT_UNKNOWN;

    gst_base_transform_set_qos_enabled(GST_BASE_TRANSFORM(self), TRUE);
}

static gboolean gst_dxmsgconv_start(GstBaseTransform *trans) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    GST_DEBUG_OBJECT(self, "start");


    if (string_is_empty(self->_library_file_path)) {
        GST_ELEMENT_ERROR(self, RESOURCE, SETTINGS,
                          ("Custom library path (library-file-path) is not set"),
                          (NULL));
        return FALSE;
    }

    self->_library_handle = dlopen(self->_library_file_path, RTLD_LAZY);
    if (!self->_library_handle) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                          ("Failed to open custom library '%s': %s", self->_library_file_path, dlerror()),
                          (NULL));
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
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
                          ("Failed to load required functions from '%s': %s", self->_library_file_path, dlerror()),
                          (NULL));
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

    self->_kernel_pool.reset();
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

static dxt::FrameDesc build_src_frame_desc(GstBuffer *buf,
                                           const GstVideoInfo *vinfo) {
    int w = GST_VIDEO_INFO_WIDTH(vinfo);
    int h = GST_VIDEO_INFO_HEIGHT(vinfo);
    GstVideoFormat gst_fmt = GST_VIDEO_INFO_FORMAT(vinfo);

    switch (gst_fmt) {
    case GST_VIDEO_FORMAT_NV12:
    case GST_VIDEO_FORMAT_I420:
    case GST_VIDEO_FORMAT_RGB:
    case GST_VIDEO_FORMAT_BGR: {
        dxt::VideoFormat fmt = dxt::video_format_from_gst(gst_fmt);
        dxt::FrameDesc desc;
        dxt::detail::build_frame_layout(desc, buf, w, h, fmt, vinfo);
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
    GstVideoFormat gst_fmt = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    dxt::VideoFormat src_fmt = dxt::video_format_from_gst(gst_fmt);

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
    dxt::FrameDesc dst_desc = dxt::make_output_frame_desc(
        self->_rgb_buf.data(), w, h, dxt::VideoFormat::RGB);

    // Get kernel from pool and transform to RGB
    dxt::InputConfig input_cfg{src_fmt, w, h};
    auto result = self->_kernel_pool->transform(input_cfg, src_desc, dst_desc);
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

static void ensure_kernel_pool(GstDxMsgConv *self) {
    if (self->_kernel_pool) return;

    int w = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    int h = GST_VIDEO_INFO_HEIGHT(&self->_input_info);

    dxt::FrameDesc dst_template = dxt::make_output_frame_desc(
        nullptr, w, h, dxt::VideoFormat::RGB);
    dxt::TransformOps ops;

    self->_kernel_pool = std::make_unique<dxt::TransformKernelPool>(dst_template, ops);
    GST_INFO_OBJECT(self, "include-frame: kernel pool created for RGB conversion");
}

void convert(GstDxMsgConv *self, DXFrameMeta *frame_meta, GstBuffer *buf) {
    int stream_id = (frame_meta && frame_meta->_stream_id >= 0)
                        ? frame_meta->_stream_id : 0;
    guint64 &seq = self->_seq_ids[stream_id];
    if (self->_message_interval == 0 ||
        (seq % self->_message_interval) == 0) {
        GstDxMsgMetaInfo meta_info;
        meta_info._frame_meta = frame_meta;
        meta_info._seq_id = seq;
        meta_info._input_info = &self->_input_info;
        meta_info._include_frame = self->_include_frame;
        meta_info._frame_base64 = nullptr;

        gchar *base64_str = nullptr;
        if (self->_include_frame && self->_kernel_pool) {
            base64_str = encode_frame_to_base64(self, buf);
            if (!base64_str) {
                GST_DEBUG_OBJECT(self, "Frame encoding failed, frameData will be null");
            }
            meta_info._frame_base64 = base64_str;
        }

        DxMsgPayload *payload = nullptr;
        try {
            payload = self->_convert_payload_function(self->_context, &meta_info);
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(self, "convert_payload_function threw exception: %s", e.what());
            g_free(base64_str);
            return;
        } catch (...) {
            GST_ERROR_OBJECT(self, "convert_payload_function threw unknown exception");
            g_free(base64_str);
            return;
        }

        if (!payload) {
            GST_WARNING_OBJECT(self, "convert_payload_function returned null");
            g_free(base64_str);
            return;
        }

        dx_add_payload_to_buffer(buf, payload);

        g_free(payload->_data);
        g_free(payload);
        g_free(base64_str);

    } else {
        GST_DEBUG_OBJECT(self, "skip seq:%lu, _message_interval: %d",
                         seq, self->_message_interval);
    }
}

// ---------------------------------------------------------------------------
// set_caps — configure video info and reset RGB kernel on caps change
// ---------------------------------------------------------------------------
static gboolean gst_dxmsgconv_set_caps(GstBaseTransform *trans,
                                       GstCaps *incaps, GstCaps *outcaps) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);
    std::ignore = outcaps;

    // Domain mode: dxvideoraw caps carries no concrete dim/format; per-buffer DXFrameMeta supplies them.
    if (dx_caps_is_videoraw(incaps)) {
        self->_cached_width = 0;
        self->_cached_height = 0;
        self->_cached_format = GST_VIDEO_FORMAT_UNKNOWN;
        self->_kernel_pool.reset();
        return TRUE;
    }

    if (!gst_video_info_from_caps(&self->_input_info, incaps)) {
        GST_ERROR_OBJECT(self, "Failed to parse input caps");
        return FALSE;
    }

    self->_cached_width  = GST_VIDEO_INFO_WIDTH(&self->_input_info);
    self->_cached_height = GST_VIDEO_INFO_HEIGHT(&self->_input_info);
    self->_cached_format = GST_VIDEO_INFO_FORMAT(&self->_input_info);
    self->_kernel_pool.reset();

    GST_INFO_OBJECT(self, "Caps set: %dx%d format=%s",
                    self->_cached_width, self->_cached_height,
                    gst_video_format_to_string(self->_cached_format));
    return TRUE;
}

static GstFlowReturn gst_dxmsgconv_transform_ip(GstBaseTransform *trans,
                                                GstBuffer *buf) {
    GstDxMsgConv *self = GST_DXMSGCONV(trans);

    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    int stream_id = (frame_meta && frame_meta->_stream_id >= 0)
                        ? frame_meta->_stream_id : 0;
    guint64 &seq = self->_seq_ids[stream_id];
    seq++;

    GST_LOG_OBJECT(self, "Processing buffer: pts=%" GST_TIME_FORMAT
                   " stream=%d seq=%" G_GUINT64_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(buf)), stream_id, seq);

    if (self->_include_frame && !self->_kernel_pool) {
        ensure_kernel_pool(self);
    }

    if (!frame_meta) {
        GST_LOG_OBJECT(self, "No DXFrameMeta, passing through");
        return GST_FLOW_OK;
    }
    convert(self, frame_meta, buf);

    return GST_FLOW_OK;
}

static gboolean gst_dxmsgconv_propose_allocation(GstBaseTransform *trans,
                                                 GstQuery *decide_query,
                                                 GstQuery *query) {
    GstBaseTransformClass *base_class =
        GST_BASE_TRANSFORM_CLASS(parent_class);
    gboolean ret = TRUE;
    if (base_class && base_class->propose_allocation)
        ret = base_class->propose_allocation(trans, decide_query, query);
    gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
    gst_query_add_allocation_meta(query, GST_DXMSG_META_API_TYPE, NULL);
    return ret;
}

static gboolean gst_dxmsgconv_query(GstBaseTransform *trans,
                                    GstPadDirection direction,
                                    GstQuery *query) {
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
