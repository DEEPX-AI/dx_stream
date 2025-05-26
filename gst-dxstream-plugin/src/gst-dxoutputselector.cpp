#include "gst-dxoutputselector.hpp"
#include "gst-dxmeta.hpp"
#include "utils.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_dxoutputselector_debug_category);
#define GST_CAT_DEFAULT gst_dxoutputselector_debug_category

static GstFlowReturn gst_dxoutputselector_chain_function(GstPad *pad,
                                                         GstObject *parent,
                                                         GstBuffer *buf);
static void gst_dxoutputselector_release_pad(GstElement *element, GstPad *pad);
static GstPad *gst_dxoutputselector_request_pad(GstElement *element,
                                                GstPadTemplate *templ,
                                                const gchar *req_name,
                                                const GstCaps *caps);
static gboolean gst_dxoutputselector_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event);

G_DEFINE_TYPE(GstDxOutputSelector, gst_dxoutputselector, GST_TYPE_ELEMENT);
static GstElementClass *parent_class = nullptr;

static void dxoutputselector_dispose(GObject *object) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(object);
    self->_srcpads.clear();
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static GstStateChangeReturn
dxoutputselector_change_state(GstElement *element, GstStateChange transition) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);

    GST_INFO_OBJECT(self, "Attempting to change state");

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
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

static void gst_dxoutputselector_class_init(GstDxOutputSelectorClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxoutputselector_debug_category,
                            "dxoutputselector", 0, "DXOutputSelector plugin");
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->dispose = dxoutputselector_dispose;

    gst_element_class_set_static_metadata(
        element_class, "DXOutputSelector", "Generic",
        "Routing N output stream from Input N Logical stream (1:N)",
        "Jo Sangil <sijo@deepx.ai>");

    static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
        "sink", GST_PAD_SINK, GST_PAD_ALWAYS, GST_STATIC_CAPS_ANY);

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src_%u", GST_PAD_SRC, GST_PAD_REQUEST, GST_STATIC_CAPS_ANY);

    gst_element_class_add_static_pad_template(element_class, &sink_template);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    element_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_dxoutputselector_request_pad);
    element_class->release_pad =
        GST_DEBUG_FUNCPTR(gst_dxoutputselector_release_pad);
    element_class->change_state = dxoutputselector_change_state;
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
}

static void gst_dxoutputselector_init(GstDxOutputSelector *self) {
    self->_sinkpad = gst_pad_new("sink", GST_PAD_SINK);
    gst_pad_set_chain_function(
        self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_chain_function));
    gst_pad_set_event_function(
        self->_sinkpad, GST_DEBUG_FUNCPTR(gst_dxoutputselector_sink_event));
    GST_PAD_SET_PROXY_CAPS(self->_sinkpad);
    gst_element_add_pad(GST_ELEMENT(self), self->_sinkpad);

    self->_srcpads.clear();
    self->_cached_caps_for_stream.clear();
    self->_last_stream_id = -1;
}

static gboolean gst_dxoutputselector_sink_event(GstPad *pad, GstObject *parent,
                                                GstEvent *event) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);
    gboolean res = TRUE;

    const GstEventType current_event_type = GST_EVENT_TYPE(event);
    const gchar *event_type_name_for_log =
        gst_event_type_get_name(current_event_type);

    int stream_id_for_event_processing = -1;
    gboolean was_route_info_event = FALSE;

    if (current_event_type == GST_EVENT_CUSTOM_DOWNSTREAM) {
        const GstStructure *s_check = gst_event_get_structure(event);
        if (s_check != NULL) {
            if (gst_structure_has_name(s_check,
                                       "application/x-dx-route-info")) {
                gst_structure_get_int(s_check, "stream-id",
                                      &stream_id_for_event_processing);
                was_route_info_event = TRUE;
            } else if (gst_structure_has_name(
                           s_check, "application/x-dx-logical-stream-eos")) {
                gst_structure_get_int(s_check, "stream-id",
                                      &stream_id_for_event_processing);
            } else {
                std::unique_lock<std::mutex> lock(self->_event_mutex);
                stream_id_for_event_processing = self->_last_stream_id;
                lock.unlock();
            }
        } else {
            std::unique_lock<std::mutex> lock(self->_event_mutex);
            stream_id_for_event_processing = self->_last_stream_id;
            lock.unlock();
        }
    } else {
        std::unique_lock<std::mutex> lock(self->_event_mutex);
        stream_id_for_event_processing = self->_last_stream_id;
        lock.unlock();
    }

    GstPad *target_srcpad = nullptr;
    if (stream_id_for_event_processing != -1) {
        auto it_pad = self->_srcpads.find(stream_id_for_event_processing);
        if (it_pad != self->_srcpads.end() && it_pad->second &&
            GST_PAD_IS_LINKED(it_pad->second)) {
            target_srcpad = it_pad->second;
        }
    }

    switch (current_event_type) {
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
    case GST_EVENT_EOS: {
        if (target_srcpad) {
            res = gst_pad_push_event(target_srcpad, event);
            if (!res)
                gst_event_unref(event);
        } else {
            GST_DEBUG_OBJECT(self,
                             "No target_srcpad for %s event (stream_id %d), "
                             "using default handler.",
                             event_type_name_for_log,
                             stream_id_for_event_processing);
            res = gst_pad_event_default(pad, parent, event);
        }
    } break;
    case GST_EVENT_CUSTOM_DOWNSTREAM: {
        const GstStructure *structure = gst_event_get_structure(event);
        if (was_route_info_event) {
            std::unique_lock<std::mutex> lock(self->_event_mutex);
            self->_last_stream_id = stream_id_for_event_processing;
            lock.unlock();
            GST_INFO_OBJECT(self,
                            "Updated _last_stream_id to %d from route-info",
                            self->_last_stream_id);
            gst_event_unref(event);
            res = TRUE;
        } else if (structure &&
                   gst_structure_has_name(
                       structure, "application/x-dx-logical-stream-eos")) {
            GstPad *eos_target_pad = target_srcpad;

            GstEvent *actual_eos_event = gst_event_new_eos();
            if (eos_target_pad) {
                res = gst_pad_push_event(eos_target_pad, actual_eos_event);
                if (!res)
                    gst_event_unref(actual_eos_event);
            } else {
                GST_WARNING_OBJECT(self,
                                   "No target_srcpad for logical EOS for "
                                   "stream %d. Dropping EOS.",
                                   stream_id_for_event_processing);
                gst_event_unref(actual_eos_event);
                res = FALSE;
            }
            gst_event_unref(event);
        } else {
            if (target_srcpad) {
                res = gst_pad_push_event(target_srcpad, event);
                if (!res)
                    gst_event_unref(event);
            } else {
                GST_WARNING_OBJECT(self,
                                   "No target_srcpad for unknown custom event "
                                   "(stream_id %d), using default handler.",
                                   stream_id_for_event_processing);
                res = gst_pad_event_default(pad, parent, event);
            }
        }
    } break;
    case GST_EVENT_CAPS: {
        if (!target_srcpad) {
            GST_WARNING_OBJECT(self,
                               "No valid/linked srcpad for stream_id %d to "
                               "handle CAPS event. Dropping.",
                               stream_id_for_event_processing);
            gst_event_unref(event);
            res = FALSE;
            break;
        }
        GstCaps *new_caps;
        gst_event_parse_caps(event, &new_caps);
        {
            std::lock_guard<std::mutex> cache_lock(self->_caps_cache_mutex);
            if (self->_cached_caps_for_stream.count(
                    stream_id_for_event_processing)) {
                if (self->_cached_caps_for_stream
                        [stream_id_for_event_processing]) {
                    gst_caps_unref(self->_cached_caps_for_stream
                                       [stream_id_for_event_processing]);
                }
            }
            self->_cached_caps_for_stream[stream_id_for_event_processing] =
                gst_caps_ref(new_caps);
        }

        res = gst_pad_push_event(target_srcpad, event);
        if (!res) {
            gst_event_unref(event);
        }
    } break;
    default: {
        if (!target_srcpad) {
            GST_WARNING_OBJECT(self,
                               "No valid/linked srcpad for stream_id %d to "
                               "push event %s. Dropping.",
                               stream_id_for_event_processing,
                               event_type_name_for_log);
            gst_event_unref(event);
            res = FALSE;
            break;
        }
        res = gst_pad_push_event(target_srcpad, event);
        if (!res) {
            gst_event_unref(event);
        }
    } break;
    }

    if (was_route_info_event) {
        GST_INFO_OBJECT(
            self, "Output [%d] : type: %s (route-info processed) \t [%d]\n",
            stream_id_for_event_processing, "application/x-dx-route-info", res);
    } else {
        GST_INFO_OBJECT(self, "Output [%d] : type: %s  \t [%d]\n",
                        stream_id_for_event_processing, event_type_name_for_log,
                        res);
    }

    return res;
}

static GstPad *gst_dxoutputselector_request_pad(GstElement *element,
                                                GstPadTemplate *templ,
                                                const gchar *name,
                                                const GstCaps *caps) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(element);

    gchar *pad_name = name ? g_strdup(name)
                           : g_strdup_printf("src_%ld", self->_srcpads.size());

    GstPad *srcpad = gst_pad_new_from_template(templ, pad_name);

    gint stream_id = get_src_pad_index(srcpad);
    gst_pad_set_active(srcpad, TRUE);
    gst_element_add_pad(element, srcpad);

    self->_srcpads[stream_id] = GST_PAD(gst_object_ref(srcpad));
    self->_cached_caps_for_stream[stream_id] = nullptr;
    g_free(pad_name);
    return srcpad;
}

static void gst_dxoutputselector_release_pad(GstElement *element, GstPad *pad) {
    gst_element_remove_pad(element, pad);
}

static GstFlowReturn gst_dxoutputselector_chain_function(GstPad *pad,
                                                         GstObject *parent,
                                                         GstBuffer *buffer) {
    GstDxOutputSelector *self = GST_DXOUTPUTSELECTOR(parent);

    if (self->_srcpads.empty()) {
        GST_ERROR_OBJECT(self, "No src pads available to push buffer");
        return GST_FLOW_ERROR;
    }

    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buffer, DX_FRAME_META_API_TYPE);
    if (!frame_meta) {
        GST_WARNING_OBJECT(self, "No DXFrameMeta in GstBuffer \n");
        return GST_FLOW_OK;
    }

    gint stream_id_from_buffer = frame_meta->_stream_id;

    GstPad *target_srcpad = nullptr;
    GstCaps *caps_for_this_stream = nullptr;

    auto it_pad = self->_srcpads.find(stream_id_from_buffer);
    if (it_pad == self->_srcpads.end()) {
        GST_ERROR_OBJECT(self,
                         "Invalid stream_id: %u — no matching src pad found\n",
                         stream_id_from_buffer);
        return GST_FLOW_ERROR;
    }

    {
        std::lock_guard<std::mutex> cache_lock(self->_caps_cache_mutex);
        auto it_caps =
            self->_cached_caps_for_stream.find(stream_id_from_buffer);

        if (it_pad == self->_srcpads.end() ||
            it_caps == self->_cached_caps_for_stream.end()) {
            GST_WARNING_OBJECT(self,
                               "No srcpad or cached CAPS for stream_id %d",
                               stream_id_from_buffer);
            gst_buffer_unref(buffer);
            return GST_FLOW_ERROR;
        }
        caps_for_this_stream = it_caps->second;
    }
    target_srcpad = it_pad->second;

    GstCaps *current_srcpad_caps = gst_pad_get_current_caps(target_srcpad);
    gboolean needs_caps_event = TRUE;

    if (current_srcpad_caps) {
        if (gst_caps_is_equal(current_srcpad_caps, caps_for_this_stream)) {
            needs_caps_event = FALSE;
        }
        gst_caps_unref(current_srcpad_caps);
    }

    if (needs_caps_event) {
        GstEvent *caps_event =
            gst_event_new_caps(gst_caps_ref(caps_for_this_stream));
        GST_INFO_OBJECT(
            self, "Pushing CAPS %" GST_PTR_FORMAT " to %s:%s for stream_id %d",
            caps_for_this_stream, GST_DEBUG_PAD_NAME(target_srcpad),
            stream_id_from_buffer);
        if (!gst_pad_push_event(target_srcpad, caps_event)) {
            GST_WARNING_OBJECT(self,
                               "Failed to push CAPS event to %s:%s for "
                               "stream_id %d. Dropping buffer.",
                               GST_DEBUG_PAD_NAME(target_srcpad),
                               stream_id_from_buffer);
            gst_buffer_unref(buffer);
            return GST_FLOW_ERROR;
        }
    }

    GST_DEBUG_OBJECT(self, "Pushing buffer for stream_id %d to %s:%s",
                     stream_id_from_buffer, GST_DEBUG_PAD_NAME(target_srcpad));

    GstFlowReturn res = gst_pad_push(target_srcpad, buffer);
    return res;
}
