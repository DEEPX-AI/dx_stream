#include "gst-dxgather.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "./../metadata/gst-dxusermeta.hpp"
#include <array>
#include <vector>

GST_DEBUG_CATEGORY_STATIC(gst_dxgather_debug_category);
#define GST_CAT_DEFAULT gst_dxgather_debug_category

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink_%u", GST_PAD_SINK, GST_PAD_REQUEST, GST_STATIC_CAPS("video/x-raw"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
    "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("video/x-raw"));

static GstFlowReturn gst_dxgather_aggregate(GstAggregator *agg, gboolean timeout);
static GstFlowReturn gst_dxgather_update_src_caps(GstAggregator *agg,
                                                  GstCaps *downstream_caps,
                                                  GstCaps **ret);
static gboolean gst_dxgather_src_query(GstAggregator *agg, GstQuery *query);
static gboolean gst_dxgather_sink_query(GstAggregator *agg,
                                        GstAggregatorPad *pad,
                                        GstQuery *query);

G_DEFINE_TYPE(GstDxGather, gst_dxgather, GST_TYPE_AGGREGATOR);

static GstAggregatorClass *parent_class = nullptr;  // NOSONAR

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
        dst->_input_tensors[key] = input_tensors.second;
    }
}

static void copy_output_tensors(DXObjectMeta *dst, const DXObjectMeta *src) {
    for (const auto &output_tensors : src->_output_tensors) {
        const auto &key = output_tensors.first;
        if (dst->_output_tensors.find(key) != dst->_output_tensors.end()) {
            GST_WARNING("Output tensor key '%d' already exists, skipping duplicate", key);
            continue;
        }
        dst->_output_tensors[key] = output_tensors.second;
    }
}

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

static GstFlowReturn
gst_dxgather_aggregate(GstAggregator *agg, gboolean /*timeout*/) {
    GstDxGather *self = GST_DXGATHER(agg);

    GST_LOG_OBJECT(self, "aggregate called");

    GstClockTime latest_pts = GST_CLOCK_TIME_NONE;
    std::vector<std::pair<GstAggregatorPad *, GstBuffer *>> peeked;
    gboolean all_eos = TRUE;

    GST_OBJECT_LOCK(agg);
    for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
        GstAggregatorPad *pad = GST_AGGREGATOR_PAD(l->data);
        if (gst_aggregator_pad_is_eos(pad)) {
            continue;
        }
        all_eos = FALSE;
        GstBuffer *buf = gst_aggregator_pad_peek_buffer(pad);
        if (!buf) {
            GST_OBJECT_UNLOCK(agg);
            for (auto &p : peeked) gst_buffer_unref(p.second);
            return GST_AGGREGATOR_FLOW_NEED_DATA;
        }
        GstClockTime pts = GST_BUFFER_PTS(buf);
        if (latest_pts == GST_CLOCK_TIME_NONE || (GST_CLOCK_TIME_IS_VALID(pts) && pts > latest_pts)) {
            latest_pts = pts;
        }
        peeked.emplace_back(pad, buf);
    }
    GST_OBJECT_UNLOCK(agg);

    if (all_eos) {
        GST_DEBUG_OBJECT(self, "All sinkpads EOS, returning GST_FLOW_EOS");
        return GST_FLOW_EOS;
    }

    GstBuffer *merged = nullptr;
    for (auto &p : peeked) {
        GstAggregatorPad *pad = p.first;
        GstBuffer *peek_buf = p.second;
        GstClockTime pts = GST_BUFFER_PTS(peek_buf);
        gst_buffer_unref(peek_buf);

        if (pts != latest_pts) {
            GstBuffer *stale = gst_aggregator_pad_pop_buffer(pad);
            if (stale) {
                GST_WARNING_OBJECT(self,
                    "PTS mismatch on pad %s: expected %" GST_TIME_FORMAT
                    ", got %" GST_TIME_FORMAT " — dropping stale buffer",
                    GST_PAD_NAME(pad), GST_TIME_ARGS(latest_pts),
                    GST_TIME_ARGS(pts));
                gst_buffer_unref(stale);
            }
            continue;
        }
        GstBuffer *buf = gst_aggregator_pad_pop_buffer(pad);
        if (!buf) {
            continue;
        }
        if (!merged) {
            merged = buf;
        } else if (check_same_source(merged, buf)) {
            frame_meta_merge(&merged, buf);
            gst_buffer_unref(buf);
        } else {
            GST_WARNING_OBJECT(self,
                "dxgather requires all sink pads fed from the same source; "
                "dropping buffer at pts=%" GST_TIME_FORMAT
                " from pad %s (different source than merged buffer)",
                GST_TIME_ARGS(GST_BUFFER_PTS(buf)),
                GST_PAD_NAME(pad));
            gst_buffer_unref(buf);
        }
    }

    if (!merged) {
        return GST_AGGREGATOR_FLOW_NEED_DATA;
    }

    GST_LOG_OBJECT(self, "Pushing merged buffer: pts=%" GST_TIME_FORMAT,
                     GST_TIME_ARGS(GST_BUFFER_PTS(merged)));
    return gst_aggregator_finish_buffer(agg, merged);
}

static GstFlowReturn
gst_dxgather_update_src_caps(GstAggregator *agg,
                             GstCaps *downstream_caps, GstCaps **ret) {
    GstCaps *sink_caps = nullptr;

    GST_OBJECT_LOCK(agg);
    for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
        GstAggregatorPad *pad = GST_AGGREGATOR_PAD(l->data);
        sink_caps = gst_pad_get_current_caps(GST_PAD(pad));
        if (sink_caps)
            break;
    }
    GST_OBJECT_UNLOCK(agg);

    if (sink_caps) {
        *ret = gst_caps_intersect(sink_caps, downstream_caps);
        gst_caps_unref(sink_caps);
    } else {
        *ret = gst_caps_ref(downstream_caps);
    }

    return GST_FLOW_OK;
}

static void gst_dxgather_class_init(GstDxGatherClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxgather_debug_category, "dxgather", 0,
                            "DXGather plugin");

    auto *element_class = GST_ELEMENT_CLASS(klass);
    auto *agg_class = GST_AGGREGATOR_CLASS(klass);

    gst_element_class_set_static_metadata(
        element_class, "DxGather", "Generic",
        "Gather Multiple Streams (from the Same Source)",
        "Sangil Jo <sijo@deepx.ai>");

    gst_element_class_add_static_pad_template_with_gtype(
        element_class, &sink_template, GST_TYPE_AGGREGATOR_PAD);
    gst_element_class_add_static_pad_template(element_class, &src_template);

    parent_class = GST_AGGREGATOR_CLASS(g_type_class_peek_parent(klass));
    agg_class->aggregate = GST_DEBUG_FUNCPTR(gst_dxgather_aggregate);
    agg_class->update_src_caps = gst_dxgather_update_src_caps;
    agg_class->src_query = GST_DEBUG_FUNCPTR(gst_dxgather_src_query);
    agg_class->sink_query = GST_DEBUG_FUNCPTR(gst_dxgather_sink_query);
}

static gboolean gst_dxgather_src_query(GstAggregator *agg, GstQuery *query) {
    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_LATENCY: {
        if (!GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query))
            return FALSE;
        // self_buffering: gather waits for matching PTS across N sinks.
        // Approximate self = one frame at the slowest sink framerate.
        gboolean live;
        GstClockTime min_lat, max_lat;
        gst_query_parse_latency(query, &live, &min_lat, &max_lat);
        GstClockTime worst_frame = 0;
        GST_OBJECT_LOCK(agg);
        for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
            GstPad *pad = GST_PAD(l->data);
            GstCaps *caps = gst_pad_get_current_caps(pad);
            if (!caps) continue;
            const GstStructure *s = gst_caps_get_structure(caps, 0);
            gint num = 0, denom = 1;
            if (s && gst_structure_get_fraction(s, "framerate", &num, &denom) &&
                num > 0 && denom > 0) {
                GstClockTime d = gst_util_uint64_scale_int(GST_SECOND, denom, num);
                if (d > worst_frame) worst_frame = d;
            }
            gst_caps_unref(caps);
        }
        GST_OBJECT_UNLOCK(agg);
        if (worst_frame > 0) {
            min_lat += worst_frame;
            if (GST_CLOCK_TIME_IS_VALID(max_lat)) max_lat += worst_frame;
        }
        gst_query_set_latency(query, live, min_lat, max_lat);
        return TRUE;
    }
    case GST_QUERY_ALLOCATION: {
        std::vector<GstPad *> pads;
        GST_OBJECT_LOCK(agg);
        for (GList *l = GST_ELEMENT(agg)->sinkpads; l; l = l->next) {
            pads.push_back(GST_PAD(gst_object_ref(l->data)));
        }
        GST_OBJECT_UNLOCK(agg);
        for (GstPad *p : pads) {
            if (gst_pad_peer_query(p, query)) {
                for (GstPad *pp : pads) gst_object_unref(pp);
                return TRUE;
            }
        }
        for (GstPad *p : pads) gst_object_unref(p);
        return GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query);
    }
    default:
        return GST_AGGREGATOR_CLASS(parent_class)->src_query(agg, query);
    }
}

static gboolean gst_dxgather_sink_query(GstAggregator *agg,
                                        GstAggregatorPad *pad,
                                        GstQuery *query) {
    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_ALLOCATION: {
        gboolean ret =
            GST_AGGREGATOR_CLASS(parent_class)->sink_query(agg, pad, query);
        gst_query_add_allocation_meta(query, DX_FRAME_META_API_TYPE, NULL);
        return ret;
    }
    default:
        return GST_AGGREGATOR_CLASS(parent_class)->sink_query(agg, pad, query);
    }
}

static void gst_dxgather_init(GstDxGather * /*self*/) {
    /* Aggregator handles all pad management */
}
