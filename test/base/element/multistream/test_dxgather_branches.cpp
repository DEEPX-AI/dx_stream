// Phase 5 — dxgather 3-branch merge correctness
// A11: box/face_box/label/confidence/track_id from different branches merge via meta_id
// Tests frame_meta_merge + merge_object_meta field-level correctness

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";
static const gsize BUF_SIZE = 4 * 4 * 3;

static GstBuffer *make_buf(GstClockTime pts, int stream_id) {
    GstBuffer *b = gst_buffer_new_allocate(nullptr, BUF_SIZE, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, BUF_SIZE);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, 4, 4);
    return b;
}

struct GatherPipeline {
    GstElement *pipe;
    GstElement *src0, *src1, *src2;
    GstElement *gather;
    GstElement *sink;

    GatherPipeline(int n_branches) {
        pipe = gst_pipeline_new("gather-test");
        gather = gst_element_factory_make("dxgather", "gather");
        sink = gst_element_factory_make("appsink", "sink");
        g_object_set(sink, "sync", FALSE, "emit-signals", FALSE, nullptr);

        gst_bin_add_many(GST_BIN(pipe), gather, sink, nullptr);
        gst_element_link(gather, sink);

        src0 = make_appsrc("src0");
        link_appsrc(src0, "sink_0");
        if (n_branches >= 2) {
            src1 = make_appsrc("src1");
            link_appsrc(src1, "sink_1");
        }
        if (n_branches >= 3) {
            src2 = make_appsrc("src2");
            link_appsrc(src2, "sink_2");
        }

        gst_element_set_state(pipe, GST_STATE_PLAYING);
    }

    ~GatherPipeline() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }

    GstElement *make_appsrc(const char *name) {
        GstElement *s = gst_element_factory_make("appsrc", name);
        g_object_set(s, "is-live", TRUE, "format", GST_FORMAT_TIME, nullptr);
        GstCaps *caps = gst_caps_from_string(CAPS);
        g_object_set(s, "caps", caps, nullptr);
        gst_caps_unref(caps);
        gst_bin_add(GST_BIN(pipe), s);
        return s;
    }

    void link_appsrc(GstElement *s, const char *pad_name) {
        GstPad *req = gst_element_get_request_pad(gather, pad_name);
        GstPad *srcpad = gst_element_get_static_pad(s, "src");
        gst_pad_link(srcpad, req);
        gst_object_unref(srcpad);
        gst_object_unref(req);
    }

    void push(GstElement *src, GstBuffer *buf) {
        gst_app_src_push_buffer(GST_APP_SRC(src), buf);
    }

    void push_eos(GstElement *src) {
        gst_app_src_end_of_stream(GST_APP_SRC(src));
    }

    GstBuffer *pull(GstClockTime timeout = 2 * GST_SECOND) {
        GstSample *sample = gst_app_sink_try_pull_sample(
            GST_APP_SINK(sink), timeout);
        if (!sample) return nullptr;
        GstBuffer *buf = gst_buffer_ref(gst_sample_get_buffer(sample));
        gst_sample_unref(sample);
        return buf;
    }
};

// CE_gather_3branch_merge_by_meta_id: 3 branches, same meta_id → fields merged
// Target: merge_object_meta L113-141 (merge_if_empty for each field)
// MUT: skip merge_object_meta → output has only branch0 fields
GST_START_TEST(CE_gather_3branch_merge_by_meta_id) {
    GatherPipeline gp(3);

    GstClockTime pts = 0;

    // Branch 0: object with box only
    GstBuffer *b0 = make_buf(pts, 0);
    DXFrameMeta *fm0 = dx_get_frame_meta(b0);
    DXObjectMeta *obj0 = dx_acquire_obj_meta_from_pool();
    obj0->_meta_id = 42;
    obj0->_box = {10.0f, 20.0f, 100.0f, 200.0f};
    obj0->_label = -1;
    obj0->_confidence = -1.0f;
    obj0->_track_id = -1;
    dx_add_obj_meta_to_frame(fm0, obj0);

    // Branch 1: same meta_id, with label + confidence
    GstBuffer *b1 = make_buf(pts, 0);
    DXFrameMeta *fm1 = dx_get_frame_meta(b1);
    DXObjectMeta *obj1 = dx_acquire_obj_meta_from_pool();
    obj1->_meta_id = 42;
    obj1->_box = {0, 0, 0, 0};
    obj1->_label = 5;
    obj1->_label_name = "person";
    obj1->_confidence = 0.95f;
    obj1->_track_id = -1;
    dx_add_obj_meta_to_frame(fm1, obj1);

    // Branch 2: same meta_id, with face_box + track_id
    GstBuffer *b2 = make_buf(pts, 0);
    DXFrameMeta *fm2 = dx_get_frame_meta(b2);
    DXObjectMeta *obj2 = dx_acquire_obj_meta_from_pool();
    obj2->_meta_id = 42;
    obj2->_box = {0, 0, 0, 0};
    obj2->_label = -1;
    obj2->_confidence = -1.0f;
    obj2->_track_id = 7;
    obj2->_face_box = {30.0f, 40.0f, 50.0f, 60.0f};
    obj2->_face_confidence = 0.88f;
    dx_add_obj_meta_to_frame(fm2, obj2);

    gp.push(gp.src0, b0);
    gp.push(gp.src1, b1);
    gp.push(gp.src2, b2);

    GstBuffer *out = gp.pull();
    fail_unless(out != nullptr, "must produce merged output");

    DXFrameMeta *fm_out = dx_get_frame_meta(out);
    fail_unless(fm_out != nullptr, "output must have DXFrameMeta");
    fail_unless_equals_int((int)fm_out->_object_meta_list.size(), 1);

    DXObjectMeta *merged = fm_out->_object_meta_list[0];
    fail_unless_equals_int(merged->_meta_id, 42);

    // From branch 0: box
    fail_unless(merged->_box[0] == 10.0f && merged->_box[2] == 100.0f,
                "box must come from branch 0");

    // From branch 1: label, label_name, confidence
    fail_unless_equals_int(merged->_label, 5);
    fail_unless_equals_string(merged->_label_name.c_str(), "person");
    fail_unless(merged->_confidence == 0.95f,
                "confidence must come from branch 1 (got %f)", merged->_confidence);

    // From branch 2: track_id, face_box, face_confidence
    fail_unless_equals_int(merged->_track_id, 7);
    fail_unless(merged->_face_box[0] == 30.0f && merged->_face_box[2] == 50.0f,
                "face_box must come from branch 2");
    fail_unless(merged->_face_confidence == 0.88f,
                "face_confidence must come from branch 2");

    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_gather_3branch_different_meta_ids: different meta_ids → separate objects
// Target: frame_meta_merge L157-173 (meta_id mismatch → copy_object_meta → append)
GST_START_TEST(CE_gather_3branch_different_meta_ids) {
    GatherPipeline gp(3);

    GstClockTime pts = 0;

    GstBuffer *b0 = make_buf(pts, 0);
    DXFrameMeta *fm0 = dx_get_frame_meta(b0);
    DXObjectMeta *o0 = add_object_to_frame(fm0, 1, 0.9f, 10, 20, 30, 40);
    o0->_meta_id = 100;

    GstBuffer *b1 = make_buf(pts, 0);
    DXFrameMeta *fm1 = dx_get_frame_meta(b1);
    DXObjectMeta *o1 = add_object_to_frame(fm1, 2, 0.8f, 50, 60, 70, 80);
    o1->_meta_id = 200;

    GstBuffer *b2 = make_buf(pts, 0);
    DXFrameMeta *fm2 = dx_get_frame_meta(b2);
    DXObjectMeta *o2 = add_object_to_frame(fm2, 3, 0.7f, 90, 100, 110, 120);
    o2->_meta_id = 300;

    gp.push(gp.src0, b0);
    gp.push(gp.src1, b1);
    gp.push(gp.src2, b2);

    GstBuffer *out = gp.pull();
    fail_unless(out != nullptr);

    DXFrameMeta *fm_out = dx_get_frame_meta(out);
    fail_unless(fm_out != nullptr);
    fail_unless_equals_int((int)fm_out->_object_meta_list.size(), 3);

    bool found_100 = false, found_200 = false, found_300 = false;
    for (auto *obj : fm_out->_object_meta_list) {
        if (obj->_meta_id == 100) {
            fail_unless_equals_int(obj->_label, 1);
            found_100 = true;
        }
        if (obj->_meta_id == 200) {
            fail_unless_equals_int(obj->_label, 2);
            found_200 = true;
        }
        if (obj->_meta_id == 300) {
            fail_unless_equals_int(obj->_label, 3);
            found_300 = true;
        }
    }
    fail_unless(found_100 && found_200 && found_300,
                "all 3 objects with different meta_ids must be in output");

    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_gather_merge_preserves_existing: merge_if_empty doesn't overwrite non-empty fields
// Target: merge_if_empty_int/float L54-65
// MUT: change merge_if_empty to always overwrite → branch 0 data lost
GST_START_TEST(CE_gather_merge_preserves_existing) {
    GatherPipeline gp(2);

    GstClockTime pts = 0;

    // Branch 0: object with box + label + confidence
    GstBuffer *b0 = make_buf(pts, 0);
    DXFrameMeta *fm0 = dx_get_frame_meta(b0);
    DXObjectMeta *o0 = dx_acquire_obj_meta_from_pool();
    o0->_meta_id = 1;
    o0->_box = {1.0f, 2.0f, 3.0f, 4.0f};
    o0->_label = 10;
    o0->_confidence = 0.99f;
    o0->_track_id = 55;
    dx_add_obj_meta_to_frame(fm0, o0);

    // Branch 1: same meta_id, tries to overwrite with different values
    GstBuffer *b1 = make_buf(pts, 0);
    DXFrameMeta *fm1 = dx_get_frame_meta(b1);
    DXObjectMeta *o1 = dx_acquire_obj_meta_from_pool();
    o1->_meta_id = 1;
    o1->_box = {99.0f, 99.0f, 99.0f, 99.0f};
    o1->_label = 77;
    o1->_confidence = 0.11f;
    o1->_track_id = 88;
    dx_add_obj_meta_to_frame(fm1, o1);

    gp.push(gp.src0, b0);
    gp.push(gp.src1, b1);

    GstBuffer *out = gp.pull();
    fail_unless(out != nullptr);

    DXFrameMeta *fm_out = dx_get_frame_meta(out);
    fail_unless(fm_out != nullptr);
    fail_unless_equals_int((int)fm_out->_object_meta_list.size(), 1);

    DXObjectMeta *merged = fm_out->_object_meta_list[0];

    // Branch 0 values must be preserved (merge_if_empty doesn't overwrite)
    fail_unless(merged->_box[0] == 1.0f,
                "box must be preserved from branch 0 (got %f)", merged->_box[0]);
    fail_unless_equals_int(merged->_label, 10);
    fail_unless(merged->_confidence == 0.99f,
                "confidence must be preserved from branch 0");
    fail_unless_equals_int(merged->_track_id, 55);

    gst_buffer_unref(out);
}
GST_END_TEST;

// CE_gather_3branch_pts_match: only same-PTS buffers are merged
// Target: gst_dxgather_aggregate L235-237 (pts != latest_pts → continue)
GST_START_TEST(CE_gather_3branch_pts_match) {
    GatherPipeline gp(3);

    // All same PTS
    GstClockTime pts = GST_SECOND;

    GstBuffer *b0 = make_buf(pts, 0);
    DXFrameMeta *fm0 = dx_get_frame_meta(b0);
    add_object_to_frame(fm0, 1, 0.9f, 10, 20, 30, 40);

    GstBuffer *b1 = make_buf(pts, 0);
    DXFrameMeta *fm1 = dx_get_frame_meta(b1);
    add_object_to_frame(fm1, 2, 0.8f, 50, 60, 70, 80);

    GstBuffer *b2 = make_buf(pts, 0);

    gp.push(gp.src0, b0);
    gp.push(gp.src1, b1);
    gp.push(gp.src2, b2);

    GstBuffer *out = gp.pull();
    fail_unless(out != nullptr);
    fail_unless(GST_BUFFER_PTS(out) == pts,
                "output PTS must match input PTS");

    gst_buffer_unref(out);
}
GST_END_TEST;

static Suite *dxgather_branches_suite(void) {
    Suite *s = suite_create("dxgather_branches");
    TCase *tc = tcase_create("3branch_merge");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_gather_3branch_merge_by_meta_id);
    tcase_add_test(tc, CE_gather_3branch_different_meta_ids);
    tcase_add_test(tc, CE_gather_merge_preserves_existing);
    tcase_add_test(tc, CE_gather_3branch_pts_match);
    return s;
}

GST_CHECK_MAIN(dxgather_branches);
