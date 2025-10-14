#include "gst-dxinputselector.hpp"
#include "gst-dxmeta.hpp"
#include "utils.hpp"
#include <algorithm>
#include <list>
#include <utility>

GST_DEBUG_CATEGORY_STATIC(gst_dxinputselector_debug_category);
#define GST_CAT_DEFAULT gst_dxinputselector_debug_category

enum { PROP_0, PROP_MAX_QUEUE_SIZE };

static void gst_dxinputselector_set_property(GObject *object, guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec);
static void gst_dxinputselector_get_property(GObject *object, guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec);

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

void clear_queues(std::map<int, std::queue<GstBuffer *>> &buffer_map) {
    for (auto &pair : buffer_map) {
        while (!pair.second.empty()) {
            gst_buffer_unref(pair.second.front());
            pair.second.pop();
        }
    }
}

static void dxinputselector_dispose(GObject *object) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(object);

    clear_queues(self->_buffer_queue);
    while (!self->_pts_heap.empty()) {
        self->_pts_heap.pop();
    }

    self->_stream_eos_arrived.clear();
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
            self->_aquire_cv.notify_all();
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
    gobject_class->set_property = gst_dxinputselector_set_property;
    gobject_class->get_property = gst_dxinputselector_get_property;

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    g_object_class_install_property(
        gobject_class, PROP_MAX_QUEUE_SIZE,
        g_param_spec_uint(
            "max-queue-size", "Max Queue Size",
            "Maximum number of buffers per stream queue", 1, G_MAXUINT, 10,
            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
                          GST_PARAM_MUTABLE_READY)));

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

    self->_buffer_queue = std::map<int, std::queue<GstBuffer *>>();
    self->_pts_heap =
        std::priority_queue<std::pair<GstClockTime, int>,
                            std::vector<std::pair<GstClockTime, int>>,
                            std::greater<std::pair<GstClockTime, int>>>();
    self->_thread = nullptr;
    self->_running = false;
    self->_max_queue_size = 2;

    self->_stream_eos_arrived.clear();
    self->_stream_eos_sent.clear();

    self->_sinkpads.clear();
}

static void gst_dxinputselector_set_property(GObject *object, guint prop_id,
                                             const GValue *value,
                                             GParamSpec *pspec) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(object);
    switch (prop_id) {
    case PROP_MAX_QUEUE_SIZE:
        self->_max_queue_size = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxinputselector_get_property(GObject *object, guint prop_id,
                                             GValue *value,
                                             GParamSpec *pspec) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(object);
    switch (prop_id) {
    case PROP_MAX_QUEUE_SIZE:
        g_value_set_uint(value, self->_max_queue_size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

gboolean send_wrapped_event(GstDxInputSelector *self, GstEvent *event, gint stream_id) {
    gboolean res = TRUE;
    GST_INFO_OBJECT(self, "Received event [%s] from stream [%d]", GST_EVENT_TYPE_NAME(event), stream_id);
    GstStructure *s = gst_structure_new("application/x-dx-wrapped-event", 
                                        "stream-id", G_TYPE_INT, stream_id,
                                        "event", GST_TYPE_EVENT, event,
                                        NULL);
    
    GstEvent *wrapped_event = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
    res = gst_pad_push_event(self->_srcpad, wrapped_event);
    if (!res) {
        gst_event_unref(event);
        return FALSE;
    }
    return res;
}

static gboolean gst_dxinputselector_sink_event(GstPad *pad, GstObject *parent,
                                               GstEvent *event) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(parent);
    gint stream_id = get_sink_pad_index(pad);
    gboolean res = TRUE;

    std::unique_lock<std::mutex> lock(self->_event_mutex);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        GST_INFO_OBJECT(self, "Get EOS From Stream [%d] ", stream_id);
        int buffer_size = 0;
        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            buffer_size = self->_buffer_queue[stream_id].size();
        }

        if (buffer_size > 0) {
            GST_INFO_OBJECT(self, "EOS Arrived From Stream [%d] ", stream_id);
            self->_stream_eos_arrived.insert(stream_id);
            return TRUE;
        }
        GstEvent *eos_event = gst_event_new_eos();
        GstStructure *s = gst_structure_new("application/x-dx-wrapped-event",
                                            "stream-id", G_TYPE_INT, stream_id,
                                            "event", GST_TYPE_EVENT, eos_event,
                                            NULL);

        GstEvent *wrapped_event = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
        GST_INFO_OBJECT(self, "Push EOS From Stream [%d] ", stream_id);
        gboolean res = gst_pad_push_event(self->_srcpad, wrapped_event);
        if (!res) {
            gst_event_unref(eos_event);
            return FALSE;
        }

        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            while (self->_buffer_queue[stream_id].size() > 0) {
                gst_buffer_unref(self->_buffer_queue[stream_id].front());
                self->_buffer_queue[stream_id].pop();
            }
            self->_buffer_queue.erase(stream_id);
        }

        self->_stream_eos_sent.insert(stream_id);

        if (self->_stream_eos_sent.size() == self->_sinkpads.size()) {
            GST_INFO_OBJECT(self, "All streams reached EOS, pushing global EOS");
            self->_running = false;
            self->_push_cv.notify_all();
            GstEvent *eos_event = gst_event_new_eos();
            if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                return FALSE;
            }
        }
    } break;
    case GST_EVENT_STREAM_GROUP_DONE: {
        gst_event_unref(event);
        return TRUE;
    }
    case GST_EVENT_STREAM_START:
    case GST_EVENT_SEGMENT:
    case GST_EVENT_CAPS: {
        res = gst_pad_push_event(self->_srcpad, event);
        if (!res) {
            GST_ERROR_OBJECT(self, "Failed to push Event\n");
            return FALSE;
        }
        res = send_wrapped_event(self, event, stream_id);
    } break;
    default: {
        res = send_wrapped_event(self, event, stream_id);
    } break;
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

    self->_buffer_queue[stream_id] = std::queue<GstBuffer *>();
    g_free(pad_name);
    return sinkpad;
}

static void gst_dxinputselector_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static gpointer push_thread_func(GstDxInputSelector *self) {
    // Prerolling
    bool prerolling = false;
    while (self->_running && !prerolling) {
        prerolling = true;
        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            for (auto &pair : self->_buffer_queue) {
                if (pair.second.size() == 0) {
                    prerolling = false;
                    break;
                }
            }
            self->_aquire_cv.notify_all();
        }
        g_usleep(1000);
    }

    while (self->_running) {
        GstBuffer *buf = nullptr;
        int smallest_stream_id = -1;
        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            self->_push_cv.wait(lock, [self]() { return !self->_running || self->_buffer_queue.size() > 0; });

            if (!self->_running) {
                clear_queues(self->_buffer_queue);
                while (!self->_pts_heap.empty())
                    self->_pts_heap.pop();
                self->_aquire_cv.notify_all();
                break;
            }

            

            if (self->_pts_heap.empty()) {
                continue;
            }

            smallest_stream_id = self->_pts_heap.top().second;
            self->_pts_heap.pop();

            buf = self->_buffer_queue[smallest_stream_id].front();
            self->_buffer_queue[smallest_stream_id].pop();

            if (!self->_buffer_queue[smallest_stream_id].empty()) {
                GstBuffer *next_buf =
                    self->_buffer_queue[smallest_stream_id].front();
                self->_pts_heap.push(
                    {GST_BUFFER_PTS(next_buf), smallest_stream_id});
            }

            self->_aquire_cv.notify_all();
        }
        if (!buf) {
            GST_ERROR_OBJECT(self, "Failed to read Buffer (nullptr)\n");
            continue;
        }

        // GST_INFO_OBJECT(self, "Push Buffer From Stream [%d] PTS: %" GST_TIME_FORMAT, smallest_stream_id, GST_TIME_ARGS(GST_BUFFER_PTS(buf)));
        GstFlowReturn ret = gst_pad_push(self->_srcpad, buf);

        if (ret != GST_FLOW_OK) {
            GST_ERROR_OBJECT(self, "Failed to push buffer:% d\n ", ret);
        }

        int buffer_size = 0;

        {
            std::unique_lock<std::mutex> lock(self->_buffer_lock);
            buffer_size = self->_buffer_queue[smallest_stream_id].size();
        }

        {
            std::unique_lock<std::mutex> lock(self->_event_mutex);
            if (self->_stream_eos_arrived.count(smallest_stream_id) > 0 && buffer_size == 0) {
                GstEvent *eos_event = gst_event_new_eos();
                GstStructure *s = gst_structure_new("application/x-dx-wrapped-event",
                                                    "stream-id", G_TYPE_INT, smallest_stream_id,
                                                    "event", GST_TYPE_EVENT, eos_event,
                                                    NULL);

                GstEvent *wrapped_event = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
                GST_INFO_OBJECT(self, "Push EOS From Stream [%d] ", smallest_stream_id);
                gboolean res = gst_pad_push_event(self->_srcpad, wrapped_event);
                if (!res) {
                    gst_event_unref(eos_event);
                    return FALSE;
                }
                self->_stream_eos_sent.insert(smallest_stream_id);
                self->_stream_eos_arrived.erase(smallest_stream_id);
                
                {
                    std::unique_lock<std::mutex> lock(self->_buffer_lock);
                    while (self->_buffer_queue[smallest_stream_id].size() > 0) {
                        gst_buffer_unref(self->_buffer_queue[smallest_stream_id].front());
                        self->_buffer_queue[smallest_stream_id].pop();
                    }
                    self->_buffer_queue.erase(smallest_stream_id);
                }

                if (self->_stream_eos_sent.size() == self->_sinkpads.size()) {
                    GST_INFO_OBJECT(self, "All streams reached EOS, pushing global EOS");
                    self->_running = false;
                    self->_push_cv.notify_all();
                    self->_aquire_cv.notify_all();
                    GstEvent *eos_event = gst_event_new_eos();
                    if (!gst_pad_push_event(self->_srcpad, eos_event)) {
                        GST_ERROR_OBJECT(self, "Failed to push EOS Event\n");
                    }
                    self->_stream_eos_sent.clear();
                    self->_stream_eos_arrived.clear();
                }
            }
        }
    }
    return nullptr;
}

static GstFlowReturn gst_dxinputselector_chain(GstPad *pad, GstObject *parent,
                                               GstBuffer *buf) {
    GstDxInputSelector *self = GST_DXINPUTSELECTOR(parent);

    gint stream_id = get_sink_pad_index(pad);

    // GST_INFO_OBJECT(self, "Chain Buffer From Stream [%d] ", stream_id);

    GstClockTime pts = GST_BUFFER_TIMESTAMP(buf);
    if (!GST_CLOCK_TIME_IS_VALID(pts)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

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
        gst_caps_unref(caps);
    }

    {
        std::unique_lock<std::mutex> lock(self->_buffer_lock);

        self->_aquire_cv.wait(lock, [self, stream_id]() {
            return !self->_running ||
                   self->_buffer_queue[stream_id].size() < self->_max_queue_size ||
                   self->_stream_eos_arrived.count(stream_id) > 0;
        });

        if (!self->_running) {
            gst_buffer_unref(buf);
            self->_push_cv.notify_all();
            return GST_FLOW_OK;
        }

        bool was_empty = self->_buffer_queue[stream_id].empty();
        self->_buffer_queue[stream_id].push(buf);

        if (was_empty) {
            self->_pts_heap.push({GST_BUFFER_PTS(buf), stream_id});
        }
        self->_push_cv.notify_all();
    }

    return GST_FLOW_OK;
}