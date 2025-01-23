#include "gst-dxmuxer.hpp"
#include "gst-dxmeta.hpp"

enum { PROP_0, PROP_LIVE_SOURCE, N_PROPERTIES };

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxmuxer_debug_category);
#define GST_CAT_DEFAULT gst_dxmuxer_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink_%u", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

static GstFlowReturn gst_dxmuxer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf);
static void gst_dxmuxer_release_pad(GstElement *element, GstPad *pad);
static GstPad *gst_dxmuxer_request_new_pad(GstElement *element,
                                           GstPadTemplate *templ,
                                           const gchar *req_name,
                                           const GstCaps *caps);
static void gst_dxmuxer_finalize(GObject *object);

G_DEFINE_TYPE(GstDxMuxer, gst_dxmuxer, GST_TYPE_ELEMENT);

static GstElementClass *parent_class = NULL;

gint get_pad_index(GstPad *sinkpad) {
    const gchar *pad_name = gst_pad_get_name(sinkpad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "sink_")) {
        pad_index = atoi(pad_name + 5);
    }
    return pad_index;
}

static gboolean forward_events(GstPad *pad, GstEvent **event,
                               gpointer user_data) {
    gboolean res = TRUE;
    GstPad *srcpad = GST_PAD_CAST(user_data);

    if (GST_EVENT_TYPE(*event) != GST_EVENT_EOS) {
        res = gst_pad_push_event(srcpad, gst_event_ref(*event));
    }
    return res;
}

static void dxmuxer_set_property(GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec) {
    GstDxMuxer *self = GST_DXMUXER(object);
    switch (property_id) {
    case PROP_LIVE_SOURCE:
        self->_live_source = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxmuxer_get_property(GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec) {
    GstDxMuxer *self = GST_DXMUXER(object);

    switch (property_id) {
    case PROP_LIVE_SOURCE:
        g_value_set_boolean(value, self->_live_source);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void dxmuxer_dispose(GObject *object) {
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static gboolean gst_dxmuxer_sink_event(GstPad *pad, GstObject *parent,
                                       GstEvent *event) {
    GstDxMuxer *self = GST_DXMUXER(parent);

    gint stream_id = get_pad_index(pad);

    gboolean res = TRUE;
    gboolean forward_event = TRUE;

    if (GST_EVENT_IS_STICKY(event)) {
        // GST_PAD_STREAM_LOCK(self->_srcpad);
        if (GST_EVENT_TYPE(event) == GST_EVENT_STREAM_START) {
            forward_event = TRUE;
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_SEGMENT &&
                   !self->_live_source) {
            forward_event = FALSE;
            gst_event_copy_segment(event, &self->_segments[stream_id]);
            if (self->_segments.size() == self->_sinkpads.size()) {
                GstClockTime min_start = GST_CLOCK_TIME_NONE;
                GstClockTime max_stop = 0;

                for (const auto &entry : self->_segments) {
                    const GstSegment &segment = entry.second;

                    if (segment.start < min_start) {
                        min_start = segment.start;
                    }
                    if (segment.stop > max_stop) {
                        max_stop = segment.stop;
                    }
                }

                GstSegment new_segment;
                gst_segment_init(&new_segment, GST_FORMAT_TIME);
                new_segment.start = min_start;
                new_segment.stop = max_stop;
                new_segment.position = min_start;
                new_segment.time = 0;

                GST_DEBUG_OBJECT(
                    self,
                    "Sending merged segment event: start=%" GST_TIME_FORMAT
                    ", stop=%" GST_TIME_FORMAT,
                    GST_TIME_ARGS(new_segment.start),
                    GST_TIME_ARGS(new_segment.stop));

                GstEvent *segment_event = gst_event_new_segment(&new_segment);

                // Push the merged segment event to the src pad
                res = gst_pad_push_event(self->_srcpad, segment_event);
                if (!res) {
                    GST_ERROR_OBJECT(self,
                                     "Failed to push merged segment event");
                }
            }
        } else if (GST_EVENT_TYPE(event) == GST_EVENT_EOS) {
            GST_PAD_STREAM_LOCK(self->_srcpad);
            GST_OBJECT_LOCK(self);
            self->_eos_list[stream_id] = true;
            for (auto tmp = self->_eos_list.begin();
                 tmp != self->_eos_list.end(); tmp++) {
                int stream_id = tmp->first;
                if (!tmp->second) {
                    forward_event = FALSE;
                }
            }
            GST_OBJECT_UNLOCK(self);
            GST_PAD_STREAM_UNLOCK(self->_srcpad);
        }
        // GST_PAD_STREAM_UNLOCK(self->_srcpad);
    } else if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP) {
        GST_PAD_STREAM_LOCK(self->_srcpad);
        GST_OBJECT_LOCK(self);
        self->_eos_list[stream_id] = false;
        GST_OBJECT_UNLOCK(self);
        GST_PAD_STREAM_UNLOCK(self->_srcpad);
    }

    if (forward_event)
        res = gst_pad_push_event(self->_srcpad, event);
    else
        gst_event_unref(event);

    return res;
}

static gboolean gst_dxmuxer_src_event(GstPad *pad, GstObject *parent,
                                      GstEvent *event) {
    GstDxMuxer *self = GST_DXMUXER(parent);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_QOS: {

        GstQOSType type;
        GstClockTime timestamp;
        GstClockTimeDiff diff;
        gst_event_parse_qos(event, &type, NULL, &diff, &timestamp);

        if (type == GST_QOS_TYPE_THROTTLE && diff > 0) {
            GST_OBJECT_LOCK(parent);
            // self->_throttling_delay = diff;
            GST_OBJECT_UNLOCK(parent);
            // gst_event_unref(event);
        }

        if (type == GST_QOS_TYPE_UNDERFLOW && diff > 0) {
            GST_OBJECT_LOCK(parent);

            self->_qos_timediff = diff;
            self->_qos_timestamp = timestamp;

            GST_OBJECT_UNLOCK(parent);
            // gst_event_unref(event);
        }
    }
        /* fall-through */
    default:
        break;
    }

    /** other events are handled in the default event handler */
    return gst_pad_event_default(pad, parent, event);
}

GstClockTime get_max_timestamp(std::map<int, _GstBuffer *> buffers) {
    GstClockTime max_timestamp = 0;

    for (const auto &[stream_id, buffer] : buffers) {
        GstClockTime pts = GST_BUFFER_TIMESTAMP(buffer);

        if (GST_CLOCK_TIME_IS_VALID(pts) && pts > max_timestamp) {
            max_timestamp = pts;
        }
    }
    return max_timestamp;
}

GstClockTime get_min_timestamp(std::map<int, _GstBuffer *> buffers) {
    GstClockTime min_pts = std::numeric_limits<GstClockTime>::max();

    for (const auto &[stream_id, buffer] : buffers) {
        GstClockTime pts = GST_BUFFER_TIMESTAMP(buffer);

        if (GST_CLOCK_TIME_IS_VALID(pts) && pts < min_pts) {
            min_pts = pts;
        }
    }
    return min_pts;
}

bool check_all_buffer_updated(std::map<int, _GstBuffer *> buffers) {
    for (auto tmp = buffers.begin(); tmp != buffers.end(); tmp++) {
        int stream_id = tmp->first;
        GstBuffer *buf = tmp->second;
        if (buf == NULL) {
            return false;
        }
    }
    return true;
}

void clear_pts(GstDxMuxer *self) {
    if (self->_pts != NULL) {
        delete self->_pts;
        self->_pts = NULL;
    }
}

bool update_buffers_for_sync(GstDxMuxer *self) {
    bool push = true;
    for (auto &buffer_pair : self->_buffers) {
        int stream_id = buffer_pair.first;
        GstBuffer *&buffer = buffer_pair.second;
        GstClockTime pts = GST_BUFFER_TIMESTAMP(buffer);
        if (!GST_CLOCK_TIME_IS_VALID(pts) && pts < *self->_pts) {
            gst_buffer_unref(buffer);
            buffer = NULL;
            push = false;
        }
    }
    return push;
}

void push_buffers(GstDxMuxer *self) {

    GstClockTime in_ts = *self->_pts;

    if (self->_qos_timediff > 0 && self->_pts) {

        GstClockTimeDiff earliest_time;

        if (self->_throttling_delay > 0) {
            earliest_time = self->_qos_timestamp + 2 * self->_qos_timediff +
                            self->_throttling_delay;
        } else {
            earliest_time = self->_qos_timestamp + self->_qos_timediff;
        }

        if (earliest_time > in_ts) {
            for (auto &tmp : self->_buffers) {
                int stream_id = tmp.first;
                {
                    std::unique_lock<std::mutex> lock(
                        self->_mutexes[stream_id]);
                    gst_buffer_unref(self->_buffers[stream_id]);
                    self->_buffers[stream_id] = NULL;
                }
            }
            return; // Drop (We will not push buf)
        }
    }

    for (auto &tmp : self->_buffers) {
        int stream_id = tmp.first;
        {
            std::unique_lock<std::mutex> lock(self->_mutexes[stream_id]);

            if (!self->_buffers[stream_id]) {
                g_printerr("Failed to create GstBuffers\n");
                return;
            }

            if (!self->_caps[stream_id]) {
                self->_caps[stream_id] =
                    gst_pad_get_current_caps(self->_sinkpads[stream_id]);
            }
            GstBuffer *outbuf = gst_buffer_copy_deep(self->_buffers[stream_id]);
            DXFrameMeta *frame_meta = (DXFrameMeta *)gst_buffer_get_meta(
                outbuf, DX_FRAME_META_API_TYPE);
            if (!frame_meta) {
                DXFrameMeta *frame_meta = dx_create_frame_meta(outbuf);

                GstStructure *s =
                    gst_caps_get_structure(self->_caps[stream_id], 0);
                frame_meta->_name = gst_structure_get_name(s);
                frame_meta->_format = gst_structure_get_string(s, "format");
                gst_structure_get_int(s, "width", &frame_meta->_width);
                gst_structure_get_int(s, "height", &frame_meta->_height);
                gint num, denom;
                gst_structure_get_fraction(s, "framerate", &num, &denom);
                frame_meta->_frame_rate = (gfloat)num / (gfloat)denom;
                frame_meta->_stream_id = stream_id;
                frame_meta->_buf = outbuf;
            } else {
                frame_meta->_stream_id = stream_id;
                frame_meta->_buf = outbuf;
            }

            GST_BUFFER_PTS(outbuf) = *self->_pts;
            GstFlowReturn ret = gst_pad_push(self->_srcpad, outbuf);

            if (ret != GST_FLOW_OK) {
                GST_ERROR_OBJECT(self, "[DXMuxer] Failed to push buffer:% d\n ",
                                 ret);
            }
            gst_buffer_unref(self->_buffers[stream_id]);
            self->_buffers[stream_id] = NULL;
        }
        self->_cv.notify_all();
    }
}

void handle_sync_case(GstDxMuxer *self) {
    if (self->_pts == NULL) {
        self->_pts = new GstClockTime;
        *self->_pts = get_max_timestamp(self->_buffers);
    }
    if (!update_buffers_for_sync(self)) {
        return;
    }
    push_buffers(self);
    clear_pts(self);
}

static gpointer push_thread_func(GstDxMuxer *self) {
    while (self->_running) {
        if (!check_all_buffer_updated(self->_buffers)) {
            std::vector<int> remove_index;
            for (auto tmp = self->_buffers.begin(); tmp != self->_buffers.end();
                 tmp++) {
                int stream_id = tmp->first;
                if (self->_eos_list[stream_id]) {
                    remove_index.push_back(stream_id);
                }
            }
            for (auto &index : remove_index) {
                if (self->_buffers[index] != NULL) {
                    gst_buffer_unref(self->_buffers[index]);
                }
                self->_buffers.erase(index);
            }

            self->_cv.notify_all();
            g_usleep(1000);
            continue;
        }
        handle_sync_case(self);

        self->_cv.notify_all();
        g_usleep(1000);
    }
    return NULL;
}

static GstStateChangeReturn dxmuxer_change_state(GstElement *element,
                                                 GstStateChange transition) {
    GstDxMuxer *self = GST_DXMUXER(element);

    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        if (!self->_running) {
            self->_running = TRUE;
            self->_thread = g_thread_new("push-thread",
                                         (GThreadFunc)push_thread_func, self);
        }
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        if (!self->_running) {
            self->_running = TRUE;
            self->_thread = g_thread_new("push-thread",
                                         (GThreadFunc)push_thread_func, self);
        }
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED: {
        break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY: {
        if (self->_running) {
            self->_running = FALSE;
        }
        self->_cv.notify_all();
        g_thread_join(self->_thread);
        break;
    }
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

static void gst_dxmuxer_class_init(GstDxMuxerClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxmuxer_debug_category, "dxmuxer", 0,
                            "DXMuxer plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = dxmuxer_set_property;
    gobject_class->get_property = dxmuxer_get_property;
    gobject_class->dispose = dxmuxer_dispose;

    obj_properties[PROP_LIVE_SOURCE] = g_param_spec_boolean(
        "live-source", "Muxer Live source Mode",
        "Determines whether to allow only realtime video streams as input.",
        FALSE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

    gobject_class->finalize = gst_dxmuxer_finalize;

    gst_element_class_set_static_metadata(element_class, "DxMuxer", "Generic",
                                          "Mux Multi Channel Streams",
                                          "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxmuxer_request_new_pad);
    element_class->release_pad = GST_DEBUG_FUNCPTR(gst_dxmuxer_release_pad);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
    element_class->change_state = dxmuxer_change_state;
}

static void gst_dxmuxer_init(GstDxMuxer *self) {
    self->_srcpad = gst_pad_new("src", GST_PAD_SRC);
    gst_pad_set_event_function(self->_srcpad,
                               GST_DEBUG_FUNCPTR(gst_dxmuxer_src_event));
    gst_element_add_pad(GST_ELEMENT(self), self->_srcpad);

    self->_live_source = false;

    self->_thread = NULL;
    self->_running = FALSE;

    self->_buffers.clear();
    self->_caps.clear();
    self->_mutexes.clear();
    self->_eos_list.clear();
    self->_segments.clear();
    self->_sinkpads.clear();

    self->_pts = NULL;

    self->_qos_timestamp = 0;
    self->_qos_timediff = 0;
    self->_throttling_delay = 0;
}

static void gst_dxmuxer_finalize(GObject *object) {
    GstDxMuxer *self = GST_DXMUXER(object);

    for (auto &pair : self->_buffers) {
        if (pair.second != nullptr) {
            gst_buffer_unref(pair.second);
        }
    }
    self->_buffers.clear();
    for (auto &pair : self->_caps) {
        if (pair.second != nullptr) {
            gst_caps_unref(pair.second);
        }
    }
    self->_caps.clear();
    self->_buffers.clear();
    self->_mutexes.clear();
    self->_eos_list.clear();
    self->_segments.clear();
    self->_sinkpads.clear();

    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static GstPad *gst_dxmuxer_request_new_pad(GstElement *element,
                                           GstPadTemplate *templ,
                                           const gchar *name,
                                           const GstCaps *caps) {
    GstDxMuxer *self = GST_DXMUXER(element);
    gchar *pad_name = name ? g_strdup(name)
                           : g_strdup_printf("sink_%d", self->_sinkpads.size());

    GstPad *sinkpad = gst_pad_new_from_template(templ, pad_name);

    gst_pad_set_chain_function(sinkpad, GST_DEBUG_FUNCPTR(gst_dxmuxer_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_dxmuxer_sink_event));

    gint stream_id = get_pad_index(sinkpad);
    gst_pad_set_active(sinkpad, TRUE);
    gst_element_add_pad(element, sinkpad);

    self->_sinkpads[stream_id] = GST_PAD(gst_object_ref(sinkpad));

    self->_buffers[stream_id] = NULL;
    self->_caps[stream_id] = NULL;
    self->_mutexes[stream_id];
    self->_eos_list[stream_id] = false;

    return sinkpad;
}

static void gst_dxmuxer_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static GstFlowReturn gst_dxmuxer_chain(GstPad *pad, GstObject *parent,
                                       GstBuffer *buf) {
    GstDxMuxer *self = GST_DXMUXER(parent);

    gint stream_id = get_pad_index(pad);
    if (self->_eos_list[stream_id]) {
        gst_buffer_unref(buf);

        auto it = self->_buffers.find(stream_id);
        if (it != self->_buffers.end()) {
            self->_buffers.erase(stream_id);
        }
        return GST_FLOW_OK;
    }
    if (self->_caps[stream_id] == NULL) {
        self->_caps[stream_id] = gst_pad_get_current_caps(pad);
    }

    GstClockTime pts = GST_BUFFER_TIMESTAMP(buf);
    if (!GST_CLOCK_TIME_IS_VALID(pts)) {
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    if (self->_live_source) {
        GstClock *clock = gst_element_get_clock(GST_ELEMENT(self));
        if (!clock) {
            return GST_FLOW_OK;
        }
        GstClockTime now = gst_clock_get_time(clock);

        GstClockTime base_time = gst_element_get_base_time(GST_ELEMENT(self));
        GstClockTime running_time = now - base_time;

        GstClockTimeDiff threshold = 1000 * GST_MSECOND;

        if (pts + threshold < running_time) {
            gst_buffer_unref(buf);
            return GST_FLOW_OK;
        }
    }

    {
        std::unique_lock<std::mutex> lock(self->_mutexes[stream_id]);
        auto it = self->_buffers.find(stream_id);
        if (it != self->_buffers.end()) {
            self->_buffers[stream_id] = buf;
        }
        self->_cv.wait(lock, [self, stream_id]() {
            return self->_buffers[stream_id] == NULL || !self->_running;
        });
    }

    return GST_FLOW_OK;
}