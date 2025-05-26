#include "gst-dxinputselector.hpp"
#include "gst-dxmeta.hpp"
#include "utils.hpp"
#include <algorithm>
#include <list>

GST_DEBUG_CATEGORY_STATIC(gst_dxinputselector_debug_category);
#define GST_CAT_DEFAULT gst_dxinputselector_debug_category

static GstFlowReturn gst_dxinputselector_chain(GstPad *pad, GstObject *parent,
                                               GstBuffer *buf);
static void gst_dxinputselector_release_pad(GstElement *element, GstPad *pad);
static GstPad *gst_dxinputselector_request_new_pad(GstElement *element,
                                                   GstPadTemplate *templ,
                                                   const gchar *req_name,
                                                   const GstCaps *caps);
static gpointer push_thread_func(GstDxInputSelector *self);
static gboolean gst_dxinputselector_sink_event(GstPad *pad, GstObject *parent,
                                               GstEvent *event);

G_DEFINE_TYPE(GstDxInputSelector, gst_dxinputselector, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = nullptr;

bool all_buffer_values_are_valid(const std::map<int, GstBuffer *> &buffer_map) {
    for (const auto &pair : buffer_map) {
        if (pair.second == nullptr) {
            return false;
        }
    }
    return true;
}

void clear_buffer(std::map<int, GstBuffer *> &buffer_map) {
    for (auto &pair : buffer_map) {
        if (pair.second != nullptr) {
            gst_buffer_unref(pair.second);
            pair.second = nullptr;
        }
    }
}

int get_key_of_buffer_with_smallest_pts(
    const std::map<int, GstBuffer *> &buffer_map) {
    for (const auto &pair : buffer_map) {
        if (pair.second == nullptr) {
            return -1;
        }
    }

    GstClockTime min_pts = GST_CLOCK_TIME_NONE;
    int key_with_min_pts = -1;
    bool found_valid_pts = false;

    for (const auto &pair : buffer_map) {
        GstBuffer *buffer = pair.second;
        int current_key = pair.first;

        if (GST_BUFFER_PTS_IS_VALID(buffer)) {
            GstClockTime current_pts = GST_BUFFER_PTS(buffer);
            if (!found_valid_pts || current_pts < min_pts) {
                min_pts = current_pts;
                key_with_min_pts = current_key;
                found_valid_pts = true;
            }
        }
    }

    return key_with_min_pts;
}

static void dxinputselector_dispose(GObject *object) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(object);

    self->_eos_list.clear();
    self->_sinkpads.clear();
    self->_buffer_queue.clear();
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static GstStateChangeReturn
dxinputselector_change_state(GstElement *element, GstStateChange transition) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(element);

    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED: {
        if (!self->_running) {
            self->_running = true;
            self->_thread = g_thread_new("push-thread",
                                         (GThreadFunc)push_thread_func, self);
        }
    } break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY: {
        if (self->_running) {
            self->_running = false;
            self->_push_cv.notify_all();
        }
        g_thread_join(self->_thread);
    } break;
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

static void gst_dxinputselector_class_init(GstDxInputSelectorClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxinputselector_debug_category,
                            "dxinputselector", 0, "DXInputSelector plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = dxinputselector_dispose;

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gst_element_class_set_static_metadata(
        element_class, "DXInputSelector", "Generic",
        "Input Selection from Multi Channel Streams (N:1)",
        "Jo Sangil <sijo@deepx.ai>");

    static GstStaticPadTemplate sink_template =
        GST_STATIC_PAD_TEMPLATE("sink_%u", GST_PAD_SINK, GST_PAD_REQUEST,
                                GST_STATIC_CAPS("video/x-raw"));

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw"));

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxinputselector_request_new_pad);
    element_class->release_pad =
        GST_DEBUG_FUNCPTR(gst_dxinputselector_release_pad);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxinputselector_change_state;
}

static void gst_dxinputselector_init(GstDxInputSelector *self) {
    self->_srcpad = gst_pad_new("src", GST_PAD_SRC);
    GST_PAD_SET_PROXY_CAPS(self->_srcpad);
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_buffer_queue = std::map<int, GstBuffer *>();
    self->_thread = nullptr;
    self->_running = false;
    self->_global_eos = false;

    self->_eos_list.clear();
    self->_sinkpads.clear();

    self->_events.clear();
}

static gboolean gst_dxinputselector_sink_event(GstPad *pad, GstObject *parent,
                                               GstEvent *event) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(parent);
    gint stream_id = get_sink_pad_index(pad);
    gboolean res = TRUE;
    GstStructure *s =
        gst_structure_new("application/x-dx-route-info", "stream-id",
                          G_TYPE_INT, stream_id, NULL);
    GstEvent *route_info = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
    res = gst_pad_push_event(self->_srcpad, route_info);
    if (!res) {
        gst_event_unref(route_info);
        return FALSE;
    }

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        GST_INFO_OBJECT(self, "Get EOS From Stream [%d] ", stream_id);
        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            self->_aquire_cv.wait(lock, [self, stream_id]() {
                return self->_buffer_queue[stream_id] == nullptr;
            });
            self->_buffer_queue.erase(stream_id);
            self->_eos_list[stream_id] = true;
            self->_global_eos =
                std::all_of(self->_eos_list.begin(), self->_eos_list.end(),
                            [](const std::pair<const int, gboolean> &entry) {
                                return entry.second;
                            });
            self->_push_cv.notify_one();
        }

        if (!self->_global_eos) {
            GstStructure *s =
                gst_structure_new("application/x-dx-logical-stream-eos",
                                  "stream-id", G_TYPE_INT, stream_id, NULL);
            GstEvent *logical_eos_event =
                gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
            gboolean res = gst_pad_push_event(self->_srcpad, logical_eos_event);
            if (!res) {
                gst_event_unref(logical_eos_event);
            }
        }
        gst_event_unref(event);
        res = TRUE;
    } break;
    default:
        res = gst_pad_push_event(self->_srcpad, event);
        break;
    }
    return res;
}

static GstPad *gst_dxinputselector_request_new_pad(GstElement *element,
                                                   GstPadTemplate *templ,
                                                   const gchar *name,
                                                   const GstCaps *caps) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(element);
    gchar *pad_name = name
                          ? g_strdup(name)
                          : g_strdup_printf("sink_%ld", self->_sinkpads.size());

    GstPad *sinkpad = gst_pad_new_from_template(templ, pad_name);

    gst_pad_set_chain_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxinputselector_chain));
    gst_pad_set_event_function(
        sinkpad, GST_DEBUG_FUNCPTR(gst_dxinputselector_sink_event));

    gint stream_id = get_sink_pad_index(sinkpad);
    gst_element_add_pad(element, sinkpad);

    self->_sinkpads[stream_id] = GST_PAD(gst_object_ref(sinkpad));
    self->_eos_list[stream_id] = false;

    self->_events[stream_id] = std::queue<GstEvent *>();
    self->_buffer_queue[stream_id] = nullptr;
    g_free(pad_name);
    return sinkpad;
}

static void gst_dxinputselector_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static gpointer push_thread_func(GstDxInputSelector *self) {
    while (self->_running) {
        GstBuffer *buf = nullptr;
        int smallest_stream_id = -1;
        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            self->_push_cv.wait(lock, [self]() {
                return !self->_running || self->_global_eos ||
                       all_buffer_values_are_valid(self->_buffer_queue);
            });

            if (!self->_running) {
                clear_buffer(self->_buffer_queue);
                self->_aquire_cv.notify_all();
                break;
            }

            if (self->_buffer_queue.empty() && self->_global_eos) {
                GstEvent *eos_event = gst_event_new_eos();
                if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                    GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                }
                break;
            }

            smallest_stream_id =
                get_key_of_buffer_with_smallest_pts(self->_buffer_queue);
            if (smallest_stream_id < 0) {
                GST_ERROR_OBJECT(self, "Invalid GstBuffer \n");
                continue;
            }
            buf = gst_buffer_ref(self->_buffer_queue[smallest_stream_id]);
            gst_buffer_unref(self->_buffer_queue[smallest_stream_id]);
            self->_buffer_queue[smallest_stream_id] = nullptr;
            self->_aquire_cv.notify_all();
        }
        if (!buf) {
            GST_ERROR_OBJECT(self, "Failed to read Buffer (nullptr)\n");
            continue;
        }

        if (self->_eos_list[smallest_stream_id]) {
            GST_ERROR_OBJECT(self, "Invalid GstBuffer (EOS)\n");
            gst_buffer_unref(buf);
            continue;
        }

        DXFrameMeta *frame_meta =
            (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
        if (!frame_meta) {
            GST_ERROR_OBJECT(self, "Failed to find DXFrameMeta in Buffer\n");
            gst_buffer_unref(buf);
            continue;
        }
        // g_print("[%d] PTS: %" GST_TIME_FORMAT " \n", frame_meta->_stream_id,
        //         GST_TIME_ARGS(GST_BUFFER_PTS(buf)));
        GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);

        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer:% d\n ", ret);
        }
    }
    return nullptr;
}

static GstFlowReturn gst_dxinputselector_chain(GstPad *pad, GstObject *parent,
                                               GstBuffer *buf) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(parent);

    gint stream_id = get_sink_pad_index(pad);

    GstClockTime pts = GST_BUFFER_TIMESTAMP(buf);
    if (!GST_CLOCK_TIME_IS_VALID(pts)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    // add frame_meta
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        if (!gst_buffer_is_writable(buf)) {
            buf = gst_buffer_make_writable(buf);
        }
        frame_meta = (DXFrameMeta *)gst_buffer_add_meta(buf, DX_FRAME_META_INFO,
                                                        nullptr);
        GstCaps *caps = gst_pad_get_current_caps(pad);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        frame_meta->_name = gst_structure_get_name(s);
        frame_meta->_format = gst_structure_get_string(s, "format");
        gst_structure_get_int(s, "width", &frame_meta->_width);
        gst_structure_get_int(s, "height", &frame_meta->_height);
        gint num, denom;
        gst_structure_get_fraction(s, "framerate", &num, &denom);
        frame_meta->_frame_rate = (gfloat)num / (gfloat)denom;
        frame_meta->_stream_id = stream_id;
        frame_meta->_buf = buf;
        gst_caps_unref(caps);
    }

    {
        std::unique_lock<std::mutex> lock(self->_buffer_lock);
        self->_aquire_cv.wait(lock, [self, stream_id]() {
            return self->_buffer_queue[stream_id] == nullptr;
        });
        self->_buffer_queue[stream_id] = buf;
        self->_push_cv.notify_one();
    }

    return GST_FLOW_OK;
}
