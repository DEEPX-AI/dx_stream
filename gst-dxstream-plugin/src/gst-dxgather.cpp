#include "gst-dxgather.hpp"
#include "gst-dxmeta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxgather_debug_category);
#define GST_CAT_DEFAULT gst_dxgather_debug_category

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

static GstElementClass *parent_class = NULL;

static void gst_dxgather_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void gst_dxgather_finalize(GObject *object) {
    GstDxGather *self = GST_DXGATHER(object);

    for (auto &entry : self->_sinkpads) {
        gst_object_unref(entry.second);
    }
    self->_sinkpads.clear();

    for (auto tmp = self->_buffers.begin(); tmp != self->_buffers.end();
         tmp++) {
        int stream_id = tmp->first;
        if (self->_buffers[stream_id]) {
            gst_buffer_unref(self->_buffers[stream_id]);
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
    GST_INFO_OBJECT(self, "State change return: %d", result);
    return result;
}

static gboolean gst_dxgather_sink_event(GstPad *pad, GstObject *parent,
                                        GstEvent *event) {
    GstDxGather *self = GST_DXGATHER(parent);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS:
        break;
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

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = gst_dxgather_dispose;

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

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

    self->_thread = NULL;
    self->_running = FALSE;
}

static GstPad *gst_dxgather_request_new_pad(GstElement *element,
                                            GstPadTemplate *templ,
                                            const gchar *name,
                                            const GstCaps *caps) {
    GstDxGather *self = GST_DXGATHER(element);
    gchar *pad_name = name
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
    self->_buffers[stream_id] = NULL;

    return sinkpad;
}

static void gst_dxgather_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

void merge_object_meta(DXObjectMeta *obj_meta0, DXObjectMeta *obj_meta1) {
    if (obj_meta0->_track_id == -1 && obj_meta1->_track_id != -1) {
        obj_meta0->_track_id = obj_meta1->_track_id;
    }

    if (obj_meta0->_label == -1 && obj_meta1->_label != -1) {
        obj_meta0->_label = obj_meta1->_label;
    }

    if (obj_meta0->_confidence == -1.0f && obj_meta1->_confidence != -1.0f) {
        obj_meta0->_confidence = obj_meta1->_confidence;
    }

    if (!obj_meta0->_label_name && obj_meta1->_label_name) {
        obj_meta0->_label_name = g_string_new_len(obj_meta1->_label_name->str,
                                                  obj_meta1->_label_name->len);
    }

    if ((obj_meta0->_box[0] == 0 && obj_meta0->_box[1] == 0 &&
         obj_meta0->_box[2] == 0 && obj_meta0->_box[3] == 0) &&
        (obj_meta1->_box[0] != 0 || obj_meta1->_box[1] != 0 ||
         obj_meta1->_box[2] != 0 || obj_meta1->_box[3] != 0)) {
        obj_meta0->_box[0] = obj_meta1->_box[0];
        obj_meta0->_box[1] = obj_meta1->_box[1];
        obj_meta0->_box[2] = obj_meta1->_box[2];
        obj_meta0->_box[3] = obj_meta1->_box[3];
    }

    if (obj_meta0->_keypoints.empty() && !obj_meta1->_keypoints.empty()) {
        obj_meta0->_keypoints = obj_meta1->_keypoints;
    }

    if (obj_meta0->_body_feature.empty() && !obj_meta1->_body_feature.empty()) {
        obj_meta0->_body_feature = obj_meta1->_body_feature;
    }

    if ((obj_meta0->_face_box[0] == 0 && obj_meta0->_face_box[1] == 0 &&
         obj_meta0->_face_box[2] == 0 && obj_meta0->_face_box[3] == 0) &&
        (obj_meta1->_face_box[0] != 0 || obj_meta1->_face_box[1] != 0 ||
         obj_meta1->_face_box[2] != 0 || obj_meta1->_face_box[3] != 0)) {
        obj_meta0->_face_box[0] = obj_meta1->_face_box[0];
        obj_meta0->_face_box[1] = obj_meta1->_face_box[1];
        obj_meta0->_face_box[2] = obj_meta1->_face_box[2];
        obj_meta0->_face_box[3] = obj_meta1->_face_box[3];
    }

    if (obj_meta0->_face_confidence == -1.0f &&
        obj_meta1->_face_confidence != -1.0f) {
        obj_meta0->_face_confidence = obj_meta1->_face_confidence;
    }

    if (obj_meta0->_face_landmarks.empty() &&
        !obj_meta1->_face_landmarks.empty()) {
        obj_meta0->_face_landmarks = obj_meta1->_face_landmarks;
    }

    if (obj_meta0->_face_feature.empty() && !obj_meta1->_face_feature.empty()) {
        obj_meta0->_face_feature = obj_meta1->_face_feature;
    }

    if (!obj_meta0->_seg_cls_map.data && obj_meta1->_seg_cls_map.data) {
        size_t seg_map_size =
            obj_meta1->_seg_cls_map.width * obj_meta1->_seg_cls_map.height;
        obj_meta0->_seg_cls_map.data = (unsigned char *)malloc(seg_map_size);
        memcpy(obj_meta0->_seg_cls_map.data, obj_meta1->_seg_cls_map.data,
               seg_map_size);
        obj_meta0->_seg_cls_map.width = obj_meta1->_seg_cls_map.width;
        obj_meta0->_seg_cls_map.height = obj_meta1->_seg_cls_map.height;
    }

    for (auto &pool : obj_meta1->_input_memory_pool) {
        if (obj_meta0->_input_memory_pool.find(pool.first) ==
            obj_meta0->_input_memory_pool.end()) {
            obj_meta0->_input_memory_pool[pool.first] = pool.second;
        }
    }
    for (auto &pool : obj_meta1->_output_memory_pool) {
        if (obj_meta0->_output_memory_pool.find(pool.first) ==
            obj_meta0->_output_memory_pool.end()) {
            obj_meta0->_output_memory_pool[pool.first] = pool.second;
        }
    }
    for (auto &input_tensor : obj_meta1->_input_tensor) {
        if (obj_meta0->_input_tensor.find(input_tensor.first) ==
            obj_meta0->_input_tensor.end()) {
            dxs::DXTensor new_tensor;

            new_tensor._name = input_tensor.second._name;
            new_tensor._shape = input_tensor.second._shape;
            new_tensor._type = input_tensor.second._type;
            new_tensor._data =
                obj_meta0->_input_memory_pool[input_tensor.first]->allocate();
            new_tensor._phyAddr = input_tensor.second._phyAddr;
            new_tensor._elemSize = input_tensor.second._elemSize;

            memcpy(new_tensor._data, input_tensor.second._data,
                   obj_meta0->_input_memory_pool[input_tensor.first]
                       ->get_block_size());
            obj_meta0->_input_tensor[input_tensor.first] = new_tensor;
        }
    }
    for (auto &output_tensor : obj_meta1->_output_tensor) {
        if (obj_meta0->_output_memory_pool.find(output_tensor.first) ==
            obj_meta0->_output_memory_pool.end()) {
            g_error("[dxgather] Can't not find Output memory pool \n");
        }

        if (obj_meta0->_output_tensor.find(output_tensor.first) !=
            obj_meta0->_output_tensor.end()) {
            g_error("[dxgather] Output Tensor is Exist \n");
        }

        void *data =
            obj_meta0->_output_memory_pool[output_tensor.first]->allocate();
        memcpy(data, obj_meta1->_output_tensor[output_tensor.first][0]._data,
               obj_meta0->_output_memory_pool[output_tensor.first]
                   ->get_block_size());
        obj_meta0->_output_tensor[output_tensor.first] =
            std::vector<dxs::DXTensor>();
        for (auto &tensor : output_tensor.second) {
            dxs::DXTensor new_tensor;
            new_tensor._name = tensor._name;
            new_tensor._shape = tensor._shape;
            new_tensor._type = tensor._type;
            new_tensor._data = static_cast<void *>(
                static_cast<uint8_t *>(data) + tensor._phyAddr);
            new_tensor._phyAddr = tensor._phyAddr;
            new_tensor._elemSize = tensor._elemSize;
            obj_meta0->_output_tensor[output_tensor.first].push_back(
                new_tensor);
        }
    }
}

void copy_object_meta(DXObjectMeta *dest_object_meta,
                      DXObjectMeta *src_object_meta) {
    if (!dest_object_meta || !src_object_meta) {
        return;
    }

    dest_object_meta->_meta_id = src_object_meta->_meta_id;

    dest_object_meta->_track_id = src_object_meta->_track_id;
    dest_object_meta->_label = src_object_meta->_label;
    if (src_object_meta->_label_name) {
        dest_object_meta->_label_name =
            g_string_new_len(src_object_meta->_label_name->str,
                             src_object_meta->_label_name->len);
    } else {
        dest_object_meta->_label_name = nullptr;
    }
    dest_object_meta->_confidence = src_object_meta->_confidence;
    dest_object_meta->_box[0] = src_object_meta->_box[0];
    dest_object_meta->_box[1] = src_object_meta->_box[1];
    dest_object_meta->_box[2] = src_object_meta->_box[2];
    dest_object_meta->_box[3] = src_object_meta->_box[3];
    if (!src_object_meta->_keypoints.empty()) {
        dest_object_meta->_keypoints = src_object_meta->_keypoints;
    }
    if (!src_object_meta->_body_feature.empty()) {
        dest_object_meta->_body_feature = src_object_meta->_body_feature;
    }

    dest_object_meta->_face_box[0] = src_object_meta->_face_box[0];
    dest_object_meta->_face_box[1] = src_object_meta->_face_box[1];
    dest_object_meta->_face_box[2] = src_object_meta->_face_box[2];
    dest_object_meta->_face_box[3] = src_object_meta->_face_box[3];
    dest_object_meta->_face_confidence = src_object_meta->_face_confidence;
    for (auto &point : src_object_meta->_face_landmarks) {
        dest_object_meta->_face_landmarks.push_back(point);
    }
    if (!src_object_meta->_face_feature.empty()) {
        dest_object_meta->_face_feature = src_object_meta->_face_feature;
    }

    if (src_object_meta->_seg_cls_map.data != nullptr) {
        size_t seg_map_size = src_object_meta->_seg_cls_map.width *
                              src_object_meta->_seg_cls_map.height;

        dest_object_meta->_seg_cls_map.data =
            (unsigned char *)malloc(seg_map_size);
        memcpy(dest_object_meta->_seg_cls_map.data,
               src_object_meta->_seg_cls_map.data, seg_map_size);

        dest_object_meta->_seg_cls_map.width =
            src_object_meta->_seg_cls_map.width;
        dest_object_meta->_seg_cls_map.height =
            src_object_meta->_seg_cls_map.height;
    } else {
        dest_object_meta->_seg_cls_map.data = nullptr;
        dest_object_meta->_seg_cls_map.width = 0;
        dest_object_meta->_seg_cls_map.height = 0;
    }

    // memory pool & tensors
    // clear pool & tensor
    for (auto &pool : src_object_meta->_input_memory_pool) {
        if (dest_object_meta->_input_memory_pool.find(pool.first) ==
            dest_object_meta->_input_memory_pool.end()) {
            dest_object_meta->_input_memory_pool[pool.first] = pool.second;
        }
    }
    for (auto &pool : src_object_meta->_output_memory_pool) {
        if (dest_object_meta->_output_memory_pool.find(pool.first) ==
            dest_object_meta->_output_memory_pool.end()) {
            dest_object_meta->_output_memory_pool[pool.first] = pool.second;
        }
    }
    // copy tensor
    for (auto &input_tensor : src_object_meta->_input_tensor) {
        if (dest_object_meta->_input_tensor.find(input_tensor.first) ==
            dest_object_meta->_input_tensor.end()) {
            dxs::DXTensor new_tensor;

            new_tensor._name = input_tensor.second._name;
            new_tensor._shape = input_tensor.second._shape;
            new_tensor._type = input_tensor.second._type;
            new_tensor._data =
                dest_object_meta->_input_memory_pool[input_tensor.first]
                    ->allocate();
            new_tensor._phyAddr = input_tensor.second._phyAddr;
            new_tensor._elemSize = input_tensor.second._elemSize;

            memcpy(new_tensor._data, input_tensor.second._data,
                   dest_object_meta->_input_memory_pool[input_tensor.first]
                       ->get_block_size());
        }
    }

    for (auto &output_tensor : src_object_meta->_output_tensor) {
        if (dest_object_meta->_output_memory_pool.find(output_tensor.first) ==
            dest_object_meta->_output_memory_pool.end()) {
            g_error("[dxgather] Can't not find Output memory pool \n");
        }

        if (dest_object_meta->_output_tensor.find(output_tensor.first) ==
            dest_object_meta->_output_tensor.end()) {
            g_error("[dxgather] Output Tensor is Exist \n");
        }

        void *data = dest_object_meta->_output_memory_pool[output_tensor.first]
                         ->allocate();
        memcpy(data,
               src_object_meta->_output_tensor[output_tensor.first][0]._data,
               dest_object_meta->_output_memory_pool[output_tensor.first]
                   ->get_block_size());
        dest_object_meta->_output_tensor[output_tensor.first] =
            std::vector<dxs::DXTensor>();
        for (auto &tensor : output_tensor.second) {
            dxs::DXTensor new_tensor;
            new_tensor._name = tensor._name;
            new_tensor._shape = tensor._shape;
            new_tensor._type = tensor._type;
            new_tensor._data = static_cast<void *>(
                static_cast<uint8_t *>(data) + tensor._phyAddr);
            new_tensor._phyAddr = tensor._phyAddr;
            new_tensor._elemSize = tensor._elemSize;
            dest_object_meta->_output_tensor[output_tensor.first].push_back(
                new_tensor);
        }
    }
}

void frame_meta_merge(GstBuffer **buf0, GstBuffer *buf1) {
    DXFrameMeta *frame_meta0 =
        (DXFrameMeta *)gst_buffer_get_meta(*buf0, DX_FRAME_META_API_TYPE);
    DXFrameMeta *frame_meta1 =
        (DXFrameMeta *)gst_buffer_get_meta(buf1, DX_FRAME_META_API_TYPE);

    if (!frame_meta1) {
        return;
    }
    if (!frame_meta0) {
        gst_buffer_unref(*buf0);
        *buf0 = gst_buffer_ref(buf1);
        return;
    }

    for (GList *l1 = frame_meta1->_object_meta_list; l1 != NULL;
         l1 = l1->next) {
        DXObjectMeta *obj_meta1 = (DXObjectMeta *)l1->data;
        gboolean found = FALSE;

        for (GList *l0 = frame_meta0->_object_meta_list; l0 != NULL;
             l0 = l0->next) {
            DXObjectMeta *obj_meta0 = (DXObjectMeta *)l0->data;

            if (obj_meta0->_meta_id == obj_meta1->_meta_id) {
                merge_object_meta(obj_meta0, obj_meta1);
                found = TRUE;
                break;
            }
        }

        if (!found) {
            DXObjectMeta *obj_meta0 = dx_create_object_meta(*buf0);
            copy_object_meta(obj_meta0, obj_meta1);

            frame_meta0->_object_meta_list =
                g_list_append(frame_meta0->_object_meta_list, obj_meta0);
        }
    }
}

gboolean check_same_source(GstBuffer *buf0, GstBuffer *buf1) {
    DXFrameMeta *frame_meta0 =
        (DXFrameMeta *)gst_buffer_get_meta(buf0, DX_FRAME_META_API_TYPE);
    DXFrameMeta *frame_meta1 =
        (DXFrameMeta *)gst_buffer_get_meta(buf1, DX_FRAME_META_API_TYPE);

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

gboolean check_buffer_null(GstDxGather *self) {
    for (auto tmp = self->_buffers.begin(); tmp != self->_buffers.end();
         tmp++) {
        int stream_id = tmp->first;
        if (!self->_buffers[stream_id]) {
            return false;
        }
    }
    return true;
}

static gpointer gather_push_thread_func(GstDxGather *self) {
    while (self->_running) {
        self->_cv.notify_all();
        g_usleep(1000);
        GstBuffer *output_buffer = NULL;

        {
            std::unique_lock<std::mutex> lock(self->_mutex);
            if (!check_buffer_null(self)) {
                continue;
            }
            GstClockTime latest_pts = GST_CLOCK_TIME_NONE;
            for (const auto &entry : self->_buffers) {
                if (entry.second) {
                    guint64 pts = GST_BUFFER_PTS(entry.second);
                    if (latest_pts == GST_CLOCK_TIME_NONE || pts > latest_pts) {
                        latest_pts = pts;
                    }
                }
            }

            for (auto &entry : self->_buffers) {
                GstBuffer *input_buffer = entry.second;

                if (input_buffer &&
                    GST_BUFFER_PTS(input_buffer) == latest_pts) {
                    if (!output_buffer) {
                        output_buffer = gst_buffer_ref(input_buffer);
                    } else {
                        if (check_same_source(output_buffer, input_buffer)) {
                            frame_meta_merge(&output_buffer, input_buffer);
                        } else {
                            continue;
                        }
                    }
                    gst_buffer_unref(input_buffer);
                    entry.second = NULL;
                }
            }
        }
        self->_cv.notify_all();
        if (output_buffer) {
            GstFlowReturn ret = gst_pad_push(self->_srcpad, output_buffer);

            if (ret != GST_FLOW_OK) {
                GST_ERROR_OBJECT(self, "Failed to push buffer: %d\n", ret);
            }
        }
    }
    return NULL;
}

static GstFlowReturn gst_dxgather_chain(GstPad *pad, GstObject *parent,
                                        GstBuffer *buf) {
    GstDxGather *self = GST_DXGATHER(parent);

    gint stream_id = 0;
    if (g_str_has_prefix(GST_PAD_NAME(pad), "sink_")) {
        stream_id = atoi(GST_PAD_NAME(pad) + 5);
    }
    {
        std::unique_lock<std::mutex> lock(self->_mutex);
        self->_cv.wait(lock, [self, stream_id]() {
            return self->_buffers[stream_id] == NULL || !self->_running;
        });
        self->_buffers[stream_id] = gst_buffer_ref(buf);
        gst_buffer_unref(buf);
    }
    return GST_FLOW_OK;
}
