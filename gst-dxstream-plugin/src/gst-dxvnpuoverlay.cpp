#include "gst-dxvnpuoverlay.hpp"
#include <cstring>
#include <map>
#include <mutex>

GST_DEBUG_CATEGORY_STATIC(gst_dxvnpuoverlay_debug_category);
#define GST_CAT_DEFAULT gst_dxvnpuoverlay_debug_category

enum {
    PROP_0,
    PROP_MODEL_PATH,
    PROP_KEEP_RATIO,
    PROP_DEVICE_ID,
    PROP_GROUP_COUNT,
    N_PROPERTIES
};

#define DXVNPUOVERLAY_CAPS "application/x-dxtensor"

struct SharedOverlay {
    std::shared_ptr<dxvnpu::OverlayRenderer> renderer;
    int ref_count{0};
};

struct OverlayRegistry {
    std::mutex mutex;
    std::map<int, SharedOverlay> map;
};

static OverlayRegistry& get_overlay_registry() {
    static OverlayRegistry instance;
    return instance;
}

static std::shared_ptr<dxvnpu::OverlayRenderer>
acquire_overlay(int device_id, const dxvnpu::OverlayConfig &cfg) {
    auto &reg = get_overlay_registry();
    std::lock_guard<std::mutex> lock(reg.mutex);
    auto it = reg.map.find(device_id);
    if (it != reg.map.end()) {
        it->second.ref_count++;
        return it->second.renderer;
    }

    try {
        auto renderer = std::make_shared<dxvnpu::OverlayRenderer>(cfg, device_id);
        reg.map[device_id] = {renderer, 1};
        return renderer;
    } catch (const std::exception &e) {
        return nullptr;
    }
}

static void release_overlay(int device_id,
                             std::shared_ptr<dxvnpu::OverlayRenderer> &renderer) {
    if (!renderer) return;

    std::shared_ptr<dxvnpu::OverlayRenderer> dying_renderer;
    {
        auto &reg = get_overlay_registry();
        std::lock_guard<std::mutex> lock(reg.mutex);
        auto it = reg.map.find(device_id);
        if (it == reg.map.end()) {
            renderer.reset();
            return;
        }

        it->second.ref_count--;
        if (it->second.ref_count <= 0) {
            dying_renderer = std::move(it->second.renderer);
            reg.map.erase(it);
        }
    }
    renderer.reset();

    if (dying_renderer) {
        auto eos = std::make_shared<dxvnpu::DXTensors>();
        eos->isEOS = true;
        dying_renderer->PutData(eos, 1000);
    }
}

static dxvnpu::OverlayConfig compute_overlay_config(
    int origin_w, int origin_h, int model_w, int model_h,
    bool keep_ratio, int group_count) {
    dxvnpu::OverlayConfig cfg;
    cfg.group_count = group_count;

    if (keep_ratio) {
        float ratio_src = static_cast<float>(origin_w) / origin_h;
        float ratio_model = static_cast<float>(model_w) / model_h;
        int out_w, out_h;
        if (ratio_src >= ratio_model) {
            out_w = model_w;
            out_h = static_cast<int>(model_w / ratio_src);
        } else {
            out_h = model_h;
            out_w = static_cast<int>(model_h * ratio_src);
        }
        cfg.canvas_w = out_w;
        cfg.canvas_h = out_h;
        cfg.canvas_x = static_cast<int>(std::round((model_w - out_w) / 2.0));
        cfg.canvas_y = static_cast<int>(std::round((model_h - out_h) / 2.0));
    } else {
        cfg.canvas_w = model_w;
        cfg.canvas_h = model_h;
        cfg.canvas_x = 0;
        cfg.canvas_y = 0;
    }
    return cfg;
}

G_DEFINE_TYPE(GstDxVnpuOverlay, gst_dxvnpuoverlay, GST_TYPE_BASE_SINK);

static void gst_dxvnpuoverlay_set_property(GObject *object, guint property_id,
                                             const GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUOVERLAY(object);
    switch (property_id) {
        case PROP_MODEL_PATH:
            g_free(self->model_path);
            self->model_path = g_value_dup_string(value);
            break;
        case PROP_KEEP_RATIO:
            self->keep_ratio = g_value_get_boolean(value);
            break;
        case PROP_DEVICE_ID:
            self->device_id = g_value_get_int(value);
            break;
        case PROP_GROUP_COUNT:
            self->group_count = g_value_get_int(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpuoverlay_get_property(GObject *object, guint property_id,
                                             GValue *value, GParamSpec *pspec) {
    auto *self = GST_DXVNPUOVERLAY(object);
    switch (property_id) {
        case PROP_MODEL_PATH:
            g_value_set_string(value, self->model_path);
            break;
        case PROP_KEEP_RATIO:
            g_value_set_boolean(value, self->keep_ratio);
            break;
        case PROP_DEVICE_ID:
            g_value_set_int(value, self->device_id);
            break;
        case PROP_GROUP_COUNT:
            g_value_set_int(value, self->group_count);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
            break;
    }
}

static void gst_dxvnpuoverlay_finalize(GObject *object) {
    auto *self = GST_DXVNPUOVERLAY(object);

    release_overlay(self->device_id, self->overlay_renderer);
    self->overlay_renderer.~shared_ptr();
    g_free(self->model_path);
    self->model_path = nullptr;

    G_OBJECT_CLASS(gst_dxvnpuoverlay_parent_class)->finalize(object);
}

static gboolean gst_dxvnpuoverlay_start(GstBaseSink *sink) {
    auto *self = GST_DXVNPUOVERLAY(sink);

    if (!self->model_path || self->model_path[0] == '\0') {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
            ("model-path property must be set"), (nullptr));
        return FALSE;
    }

    if (!g_file_test(self->model_path, G_FILE_TEST_IS_REGULAR)) {
        GST_ELEMENT_ERROR(self, RESOURCE, NOT_FOUND,
            ("model-path file does not exist or is not a regular file: %s",
             self->model_path), (nullptr));
        return FALSE;
    }

    auto shapes = dxvnpu::GetModelInputShapes(self->model_path);
    if (!shapes.empty() && shapes[0].second.size() >= 4) {
        auto &dims = shapes[0].second;
        if (dims[1] <= 4) {
            self->model_h = static_cast<int>(dims[2]);
            self->model_w = static_cast<int>(dims[3]);
        } else {
            self->model_h = static_cast<int>(dims[1]);
            self->model_w = static_cast<int>(dims[2]);
        }
        GST_INFO_OBJECT(self, "Model input shape: %dx%d", self->model_w, self->model_h);
    } else {
        GST_WARNING_OBJECT(self, "Could not read model input shape, using default 640x640");
        self->model_w = 640;
        self->model_h = 640;
    }

    return TRUE;
}

static gboolean gst_dxvnpuoverlay_stop(GstBaseSink *sink) {
    auto *self = GST_DXVNPUOVERLAY(sink);
    if (self->overlay_renderer) {
        release_overlay(self->device_id, self->overlay_renderer);
    }
    return TRUE;
}

static gboolean gst_dxvnpuoverlay_event(GstBaseSink *sink, GstEvent *event) {
    auto *self = GST_DXVNPUOVERLAY(sink);

    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
        if (self->overlay_renderer) {
            GST_INFO_OBJECT(self, "EOS received — releasing OverlayRenderer ref "
                            "(device=%d)", self->device_id);
            release_overlay(self->device_id, self->overlay_renderer);
        }
    }

    return GST_BASE_SINK_CLASS(gst_dxvnpuoverlay_parent_class)->event(sink, event);
}

static GstFlowReturn gst_dxvnpuoverlay_render(GstBaseSink *sink, GstBuffer *buf) {
    auto *self = GST_DXVNPUOVERLAY(sink);

    GST_LOG_OBJECT(self, "render: pts=%" GST_TIME_FORMAT,
                   GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta || frame_meta->_object_meta_list.empty())
        return GST_FLOW_OK;

    if (!self->overlay_renderer) {
        int group_count;
        {
            auto &reg = get_overlay_registry();
            std::lock_guard<std::mutex> lock(reg.mutex);
            auto it = reg.map.find(self->device_id);
            if (it != reg.map.end()) {
                group_count = 0;
            } else {
                group_count = self->group_count;
            }
        }

        dxvnpu::OverlayConfig cfg = compute_overlay_config(
            frame_meta->_width, frame_meta->_height,
            self->model_w, self->model_h, self->keep_ratio, group_count);

        self->overlay_renderer = acquire_overlay(self->device_id, cfg);

        if (!self->overlay_renderer) {
            GST_ERROR_OBJECT(self, "Failed to create OverlayRenderer for device %d",
                             self->device_id);
            return GST_FLOW_OK;
        }

        GST_INFO_OBJECT(self, "Acquired shared OverlayRenderer: origin=%dx%d, "
                        "model=%dx%d, canvas=%ux%u+%d+%d, device=%d, group_count=%d",
                        frame_meta->_width, frame_meta->_height,
                        self->model_w, self->model_h,
                        cfg.canvas_w, cfg.canvas_h, cfg.canvas_x, cfg.canvas_y,
                        self->device_id, cfg.group_count);
    }

    float orig_w = static_cast<float>(frame_meta->_width);
    float orig_h = static_cast<float>(frame_meta->_height);
    float r, w_pad, h_pad;
    if (self->keep_ratio) {
        r = std::min(static_cast<float>(self->model_w) / orig_w,
                     static_cast<float>(self->model_h) / orig_h);
        w_pad = (self->model_w - orig_w * r) / 2.0f;
        h_pad = (self->model_h - orig_h * r) / 2.0f;
    } else {
        r = 1.0f;
        w_pad = 0.0f;
        h_pad = 0.0f;
    }

    std::vector<dxvnpu::DeviceBoundingBox_t> bbox_list;
    bbox_list.reserve(frame_meta->_object_meta_list.size());

    float scale_x = self->keep_ratio ? r : static_cast<float>(self->model_w) / orig_w;
    float scale_y = self->keep_ratio ? r : static_cast<float>(self->model_h) / orig_h;

    for (const auto *obj : frame_meta->_object_meta_list) {
        dxvnpu::DeviceBoundingBox_t bb;
        std::memset(&bb, 0, sizeof(bb));
        bb.x = obj->_box[0] * scale_x + w_pad;
        bb.y = obj->_box[1] * scale_y + h_pad;
        bb.w = obj->_box[2] * scale_x + w_pad;
        bb.h = obj->_box[3] * scale_y + h_pad;
        bb.score = obj->_confidence;
        bb.label = static_cast<uint32_t>(obj->_label);
        bbox_list.push_back(bb);
    }

    auto tensors = std::make_shared<dxvnpu::DXTensors>();
    size_t data_size = bbox_list.size() * sizeof(dxvnpu::DeviceBoundingBox_t);
    tensors->allocateData(data_size);
    std::memcpy(tensors->getRawData(), bbox_list.data(), data_size);

    dxvnpu::DXTensors::TensorMeta tmeta;
    tmeta.name = "bbox";
    tmeta.data_type = dxvnpu::BBOX;
    tmeta.shape.push_back(1);
    tmeta.shape.push_back(static_cast<int64_t>(bbox_list.size()));
    tmeta.data_offset = 0;
    tmeta.data_size = data_size;
    tmeta.elem_size = sizeof(dxvnpu::DeviceBoundingBox_t);
    tensors->addTensor(tmeta);

    tensors->pipeline_id = frame_meta->_stream_id;
    tensors->timestamp = GST_BUFFER_PTS(buf);

    size_t num_boxes = bbox_list.size();
    auto status = self->overlay_renderer->PutData(tensors, 100);
    if (status != dxvnpu::Status::OK) {
        GST_WARNING_OBJECT(self, "OverlayRenderer PutData failed (status=%d, boxes=%zu)",
                           static_cast<int>(status), num_boxes);
    }

    GST_TRACE_OBJECT(self, "Sent %zu boxes (stream_id=%d, pts=%" GST_TIME_FORMAT ")",
                     num_boxes, frame_meta->_stream_id, GST_TIME_ARGS(GST_BUFFER_PTS(buf)));

    return GST_FLOW_OK;
}

static void gst_dxvnpuoverlay_class_init(GstDxVnpuOverlayClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxvnpuoverlay_debug_category,
                            "dxvnpuoverlay", 0, "DXVnpuOverlay plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = gst_dxvnpuoverlay_set_property;
    gobject_class->get_property = gst_dxvnpuoverlay_get_property;
    gobject_class->finalize = gst_dxvnpuoverlay_finalize;

    g_object_class_install_property(gobject_class, PROP_MODEL_PATH,
        g_param_spec_string("model-path", "Model Path",
                            "Path to .dxnn model file (used to read input shape for overlay config)",
                            nullptr,
                            static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_KEEP_RATIO,
        g_param_spec_boolean("keep-ratio", "Keep Ratio",
                             "Maintain aspect ratio for overlay canvas calculation",
                             TRUE,
                             static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_DEVICE_ID,
        g_param_spec_int("device-id", "Device ID",
                         "VNPU device ID for OverlayRenderer (-1 for auto)",
                         -1, G_MAXINT, -1,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_GROUP_COUNT,
        g_param_spec_int("group-count", "Group Count",
                         "Number of overlay groups for HDMI display grid layout",
                         1, G_MAXINT, 1,
                         static_cast<GParamFlags>(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    auto *element_class = GST_ELEMENT_CLASS(klass);

    static GstStaticPadTemplate sink_template =
        GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                GST_STATIC_CAPS(DXVNPUOVERLAY_CAPS));

    gst_element_class_add_static_pad_template(element_class, &sink_template);

    gst_element_class_set_static_metadata(
        element_class,
        "DX-VNPU Overlay Renderer",
        "Sink/Video",
        "Sends bounding-box overlay data to VNPU device HDMI output via OverlayRenderer",
        "Sangil Jo <sijo@deepx.ai>");

    auto *sink_class = GST_BASE_SINK_CLASS(klass);
    sink_class->start = GST_DEBUG_FUNCPTR(gst_dxvnpuoverlay_start);
    sink_class->stop = GST_DEBUG_FUNCPTR(gst_dxvnpuoverlay_stop);
    sink_class->event = GST_DEBUG_FUNCPTR(gst_dxvnpuoverlay_event);
    sink_class->render = GST_DEBUG_FUNCPTR(gst_dxvnpuoverlay_render);
}

static void gst_dxvnpuoverlay_init(GstDxVnpuOverlay *self) {
    new (&self->overlay_renderer) std::shared_ptr<dxvnpu::OverlayRenderer>();

    self->model_path = nullptr;
    self->keep_ratio = TRUE;
    self->device_id = -1;
    self->group_count = 1;
    self->model_w = 0;
    self->model_h = 0;

    gst_base_sink_set_sync(GST_BASE_SINK(self), FALSE);
}
