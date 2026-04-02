#include "gst-dxgather.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "./../metadata/gst-dxusermeta.hpp"
#include <array>

GST_DEBUG_CATEGORY_STATIC(gst_dxgather_debug_category);
#define GST_CAT_DEFAULT gst_dxgather_debug_category

// NOSONAR - GStreamer API requires non-const GstStaticPadTemplate* for gst_static_pad_template_get()
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink_%u", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxgather_chain(GstPad *pad, GstObject *parent,
                                        GstBuffer *buf);
static void gst_dxgather_release_pad(GstElement *element, GstPad *pad);
static GstPad *gst_dxgather_request_new_pad(GstElement *element,
                                            GstPadTemplate *templ,
                                            const gchar *req_name,
                                            const GstCaps *caps);
static GstStateChangeReturn
gst_dxgather_change_state(GstElement *element, GstStateChange transition);
static void gst_dxgather_finalize(GObject *object);
static void gst_dxgather_dispose(GObject *object);

static gpointer gather_push_thread_func(GstDxGather *self);

G_DEFINE_TYPE(GstDxGather, gst_dxgather, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;  // NOSONAR - GStreamer standard pattern with G_DEFINE_TYPE macro

static void gst_dxgather_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxgather_finalize(GObject *object) {
    GstDxGather *self = GST_DXGATHER(object);

    for (auto &entry : self->_sinkpads) {
        gst_object_unref(entry.second);
    }
    self->_sinkpads.clear();

    for (auto &entry : self->_buffers) {
        if (entry.second) {
            if (GST_IS_BUFFER(entry.second)) {
                gst_buffer_unref(entry.second);
            }
            entry.second = nullptr;
        }
    }
    self->_buffers.clear();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstStateChangeReturn
gst_dxgather_change_state(GstElement *element, GstStateChange transition) {
    GstDxGather *self = GST_DXGATHER(element);

    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        if (!self->_running) {
            self->_running = TRUE;
            self->_thread =
                g_thread_new("gather-push-thread",
                             (GThreadFunc)gather_push_thread_func, self);
        }
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        if (self->_running) {
            self->_running = FALSE;
        }
        self->_cv.notify_all();
        g_thread_join(self->_thread);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_NULL:
        break;
    default:
        break;
    }

    GstStateChangeReturn result =
        GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    GST_DEBUG_OBJECT(self, "State change completed: %d", result);
    return result;
}

static gboolean gst_dxgather_sink_event(GstPad *pad, GstObject *parent,
                                        GstEvent *event) {
    GstDxGather *self = GST_DXGATHER(parent);

    gint stream_id = 0;
    if (g_str_has_prefix(GST_PAD_NAME(pad), "sink_")) {
        stream_id = atoi(GST_PAD_NAME(pad) + 5);
    }

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS:
        self->_eos_list[stream_id] = true;
        self->_cv.notify_all();
        gst_event_unref(event);
        return TRUE;
    case GST_EVENT_CAPS: {
        gboolean result = gst_pad_push_event(self->_srcpad, event);
        if (!result) {
            GST_ERROR_OBJECT(
                self, "Failed to push caps event to src pad : %d\n", result);
            return FALSE;
        }
        return result;
    }
    case GST_EVENT_SEGMENT:
        break;
    case GST_EVENT_FLUSH_START:
        break;
    case GST_EVENT_FLUSH_STOP:
        break;
    default:
        break;
    }
    return gst_pad_event_default(pad, parent, event);
}

static void gst_dxgather_class_init(GstDxGatherClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxgather_debug_category, "dxgather", 0,
                            "DXGather plugin");

    auto *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = gst_dxgather_dispose;

    auto *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->finalize = gst_dxgather_finalize;

    gst_element_class_set_static_metadata(
        element_class, "DxGather", "Generic",
        "Gather Multiple Streams (from the Same Source)",
        "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxgather_request_new_pad);
    element_class->release_pad = GST_DEBUG_FUNCPTR(gst_dxgather_release_pad);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = gst_dxgather_change_state;
}

static void gst_dxgather_init(GstDxGather *self) {
    self->_srcpad = gst_pad_new("src", GST_PAD_SRC);
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_sinkpads.clear();
    self->_buffers.clear();

    self->_eos_list = std::map<int, bool>();

    self->_thread = nullptr;
    self->_running = FALSE;
}

static GstPad *gst_dxgather_request_new_pad(GstElement *element,
                                            GstPadTemplate *templ,
                                            const gchar *name,
                                            const GstCaps *caps) {

    std::ignore = caps;

    GstDxGather *self = GST_DXGATHER(element);
    const gchar *pad_name = name
                          ? g_strdup(name)
                          : g_strdup_printf("sink_%ld", self->_sinkpads.size());

    GstPad *sinkpad = gst_pad_new_from_template(templ, pad_name);

    gst_pad_set_chain_function(sinkpad, GST_DEBUG_FUNCPTR(gst_dxgather_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxgather_sink_event));

    gint stream_id = 0;
    if (g_str_has_prefix(pad_name, "sink_")) {
        stream_id = atoi(pad_name + 5);
    }
    gst_pad_set_active(sinkpad, TRUE);
    gst_element_add_pad(element, sinkpad);

    self->_sinkpads[stream_id] = GST_PAD(gst_object_ref(sinkpad));
    self->_buffers[stream_id] = nullptr;
    self->_eos_list[stream_id] = false;

    return sinkpad;
}

static void gst_dxgather_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static void copy_box(std::array<float, 4> &dst, const std::array<float, 4> &src) {
    dst = src;
}

template <typename Container>
static void copy_container(Container &dst, const Container &src) {
    dst = src;
}

static void copy_input_tensors(DXObjectMeta *dst, const DXObjectMeta *src) {
    for (const auto &input_tensors : src->_input_tensors) {
        const auto &key = input_tensors.first;
        if (dst->_input_tensors.find(key) != dst->_input_tensors.end())
            continue;

        // Shallow copy - shared_ptr will handle reference counting
        dst->_input_tensors[key] = input_tensors.second;
    }
}

static void copy_output_tensors(DXObjectMeta *dst, const DXObjectMeta *src) {
    for (const auto &output_tensors : src->_output_tensors) {
        const auto &key = output_tensors.first;

        if (dst->_output_tensors.find(key) != dst->_output_tensors.end()) {
            g_error("[dxgather] Output Tensor is Exist \n");
            continue;
        }

        // Shallow copy - shared_ptr will handle reference counting
        dst->_output_tensors[key] = output_tensors.second;
    }
}

// merge용 헬퍼 (조건부 병합)

static void merge_if_empty_int(int &dst, int src) {
    if (dst == -1 && src != -1) {
        dst = src;
    }
}

static void merge_if_empty_float(float &dst, float src) {
    if (dst == -1.0f && src != -1.0f) {
        dst = src;
    }
}

static bool is_box_empty(const std::array<float, 4> &box) {
    return box[0] == 0 && box[1] == 0 && box[2] == 0 && box[3] == 0;
}

static void merge_box_if_empty(std::array<float, 4> &dst, const std::array<float, 4> &src) {
    if (is_box_empty(dst) && !is_box_empty(src)) {
        copy_box(dst, src);
    }
}

template <typename Container>
static void merge_container_if_empty(Container &dst, const Container &src) {
    if (dst.empty() && !src.empty()) {
        dst = src;
    }
}

// 실제 함수 구현

void copy_object_meta(DXObjectMeta *dst, const DXObjectMeta *src) {
    if (!dst || !src)
        return;

    dst->_meta_id = src->_meta_id;

    dst->_track_id = src->_track_id;
    dst->_label = src->_label;
    dst->_label_name = src->_label_name;
    dst->_confidence = src->_confidence;
    copy_box(dst->_box, src->_box);
    copy_container(dst->_keypoints, src->_keypoints);
    copy_container(dst->_body_feature, src->_body_feature);

    copy_box(dst->_face_box, src->_face_box);
    dst->_face_confidence = src->_face_confidence;
    copy_container(dst->_face_landmarks, src->_face_landmarks);
    copy_container(dst->_face_feature, src->_face_feature);

    if (!src->_seg_data.empty()) {
        dst->_seg_data = src->_seg_data;
        dst->_seg_width = src->_seg_width;
        dst->_seg_height = src->_seg_height;
    }

    copy_input_tensors(dst, src);
    copy_output_tensors(dst, src);
}

void merge_object_meta(DXObjectMeta *dst, const DXObjectMeta *src) {
    if (!dst || !src)
        return;

    merge_if_empty_int(dst->_track_id, src->_track_id);
    merge_if_empty_int(dst->_label, src->_label);
    if (dst->_label_name.empty() && !src->_label_name.empty()) {
        dst->_label_name = src->_label_name;
    }
    merge_if_empty_float(dst->_confidence, src->_confidence);

    merge_box_if_empty(dst->_box, src->_box);
    merge_container_if_empty(dst->_keypoints, src->_keypoints);
    merge_container_if_empty(dst->_body_feature, src->_body_feature);

    merge_box_if_empty(dst->_face_box, src->_face_box);
    merge_if_empty_float(dst->_face_confidence, src->_face_confidence);
    merge_container_if_empty(dst->_face_landmarks, src->_face_landmarks);
    merge_container_if_empty(dst->_face_feature, src->_face_feature);

    if (dst->_seg_data.empty() &&
        !src->_seg_data.empty()) {
        dst->_seg_data = src->_seg_data;
        dst->_seg_width = src->_seg_width;
        dst->_seg_height = src->_seg_height;
    }

    copy_input_tensors(dst, src);
    copy_output_tensors(dst, src);
}

void frame_meta_merge(GstBuffer **buf0, GstBuffer *buf1) {
    auto *frame_meta0 = dx_get_frame_meta(*buf0);
    const auto *frame_meta1 = dx_get_frame_meta(buf1);

    if (!frame_meta1) {
        return;
    }
    if (!frame_meta0) {
        gst_buffer_unref(*buf0);
        *buf0 = gst_buffer_ref(buf1);
        return;
    }

    for (const auto *obj_meta1 : frame_meta1->_object_meta_list) {
        gboolean found = FALSE;

        for (auto *obj_meta0 : frame_meta0->_object_meta_list) {

            if (obj_meta0->_meta_id == obj_meta1->_meta_id) {
                merge_object_meta(obj_meta0, obj_meta1);
                found = TRUE;
                break;
            }
        }

        if (!found) {
            DXObjectMeta *obj_meta0 = dx_acquire_obj_meta_from_pool();
            copy_object_meta(obj_meta0, obj_meta1);

            dx_add_obj_meta_to_frame(frame_meta0, obj_meta0);
        }
    }
}

gboolean check_same_source(GstBuffer *buf0, GstBuffer *buf1) {
    const auto *frame_meta0 = dx_get_frame_meta(buf0);
    const auto *frame_meta1 = dx_get_frame_meta(buf1);

    if (!frame_meta0) {
        return TRUE;
    }
    if (!frame_meta1) {
        return TRUE;
    }
    if (frame_meta0->_stream_id == frame_meta1->_stream_id) {
        return TRUE;
    }
    return FALSE;
}

gboolean check_buffer_null(const GstDxGather *self) {
    for (const auto &entry : self->_buffers) {
        if (!entry.second) {
            return false;
        }
    }
    return true;
}

// get_latest_pts 함수
static GstClockTime get_latest_pts(const std::map<int, GstBuffer *> &buffers) {
    auto latest_pts = GST_CLOCK_TIME_NONE;
    for (const auto &entry : buffers) {
        if (entry.second) {
            guint64 pts = GST_BUFFER_PTS(entry.second);
            if (latest_pts == GST_CLOCK_TIME_NONE || pts > latest_pts) {
                latest_pts = pts;
            }
        }
    }
    return latest_pts;
}

// merge_buffers_with_pts 함수
static GstBuffer *merge_buffers_with_pts(std::map<int, GstBuffer *> &buffers,
                                         GstClockTime latest_pts) {
    GstBuffer *output_buffer = nullptr;
    for (auto &entry : buffers) {
        GstBuffer *input_buffer = entry.second;
        if (input_buffer && GST_BUFFER_PTS(input_buffer) == latest_pts) {
            if (!output_buffer) {
                output_buffer = gst_buffer_ref(input_buffer);
            } else if (output_buffer != input_buffer &&
                       check_same_source(output_buffer, input_buffer)) {
                frame_meta_merge(&output_buffer, input_buffer);
            }
            gst_buffer_unref(input_buffer);
            entry.second = nullptr;
        }
    }
    return output_buffer;
}

static gpointer gather_push_thread_func(GstDxGather *self) {
    while (self->_running) {
        self->_cv.notify_all();
        g_usleep(1000);

        GstBuffer *output_buffer = nullptr;

        { // NOSONAR - scope for lock
            std::unique_lock<std::mutex> lock(self->_mutex);

            if (!check_buffer_null(self)) {
                continue;
            }

            GstClockTime latest_pts = get_latest_pts(self->_buffers);

            output_buffer = merge_buffers_with_pts(self->_buffers, latest_pts);
        }

        self->_cv.notify_all();

        if (output_buffer) {
            GST_DEBUG_OBJECT(self, "Pushing merged buffer: pts=%" GST_TIME_FORMAT,
                             GST_TIME_ARGS(GST_BUFFER_PTS(output_buffer)));
            GstFlowReturn ret = gst_pad_push(self->_srcpad, output_buffer);

            if (ret != GST_FLOW_OK) {
                GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
            }
        }

        bool all_eos = true;
        for (std::map<int, bool>::const_iterator it = self->_eos_list.begin(); it != self->_eos_list.end(); ++it) {
            if (!it->second) {
                all_eos = false;
                break;
            }
        }
        if (all_eos) {
            GstEvent *eos_event = gst_event_new_eos();
            if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                break;
            }
        }
    }

    return nullptr;
}

static GstFlowReturn gst_dxgather_chain(GstPad *pad, GstObject *parent,
                                        GstBuffer *buf) {
    GstDxGather *self = GST_DXGATHER(parent);

    gint stream_id = 0;
    if (g_str_has_prefix(GST_PAD_NAME(pad), "sink_")) {
        stream_id = atoi(GST_PAD_NAME(pad) + 5);
    }
    { // NOSONAR - scope for lock
        std::unique_lock<std::mutex> lock(self->_mutex);
        self->_cv.wait(lock, [self, stream_id]() {
            return self->_buffers[stream_id] == nullptr || !self->_running;
        });
        self->_buffers[stream_id] = gst_buffer_ref(buf);
        gst_buffer_unref(buf);
    }
    return GST_FLOW_OK;
}
