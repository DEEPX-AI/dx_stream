// P4.3 — dxgather contract tests
// Core: GstAggregator-based. Merges multiple inference results from the same source (same stream_id).
// Pops based on latest PTS. Same stream_id → frame_meta_merge, different → skip.
// Same meta_id → merge_object_meta (preserves existing fields), different → copy_object_meta (appends).

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_ANY = "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static GstBuffer *make_buf(GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

struct AggPipe {
    GstElement *pipe, *agg, *sink;
    GstElement *src[4];
    int n;

    void start() { gst_element_set_state(pipe, GST_STATE_PLAYING); }
    void push(int i, GstBuffer *b) { gst_app_src_push_buffer(GST_APP_SRC(src[i]), b); }
    void eos(int i) { gst_app_src_end_of_stream(GST_APP_SRC(src[i])); }
    GstSample *pull(GstClockTime t = 5 * GST_SECOND) {
        return gst_app_sink_try_pull_sample(GST_APP_SINK(sink), t);
    }
    void stop() { gst_element_set_state(pipe, GST_STATE_NULL); gst_object_unref(pipe); }
};

static AggPipe make_agg_pipe(int n_src) {
    AggPipe p = {};
    p.n = n_src;
    p.pipe = gst_pipeline_new(nullptr);
    p.agg = gst_element_factory_make("dxgather", "agg");
    p.sink = gst_element_factory_make("appsink", "sink");
    g_object_set(p.sink, "sync", FALSE, nullptr);
    gst_bin_add_many(GST_BIN(p.pipe), p.agg, p.sink, nullptr);
    gst_element_link(p.agg, p.sink);

    GstCaps *caps = gst_caps_from_string(CAPS_ANY);
    for (int i = 0; i < n_src; i++) {
        char name[32];
        snprintf(name, sizeof(name), "src%d", i);
        p.src[i] = gst_element_factory_make("appsrc", name);
        g_object_set(p.src[i], "format", GST_FORMAT_TIME,
                     "is-live", FALSE, "caps", caps, nullptr);
        gst_bin_add(GST_BIN(p.pipe), p.src[i]);

        snprintf(name, sizeof(name), "sink_%d", i);
        GstPad *req = gst_element_get_request_pad(p.agg, name);
        fail_unless(req != nullptr, "request pad %s failed", name);
        GstPad *srcpad = gst_element_get_static_pad(p.src[i], "src");
        fail_unless(gst_pad_link(srcpad, req) == GST_PAD_LINK_OK);
        gst_object_unref(srcpad);
        gst_object_unref(req);
    }
    gst_caps_unref(caps);
    return p;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxgather", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxgather", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_gather_latest_pts_pop: pops based on latest PTS (opposite of inputselector)
// Target: gst_dxgather_aggregate L216 (pts > latest_pts)
// MUT: change > to < → selects oldest PTS
GST_START_TEST(CE_gather_latest_pts_pop) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    // pad0=100ms, pad1=200ms → latest=200ms → only pad1 popped
    GstBuffer *b0 = make_buf(100 * GST_MSECOND);
    make_frame_meta(b0, 0, 4, 4);
    GstBuffer *b1 = make_buf(200 * GST_MSECOND);
    make_frame_meta(b1, 0, 4, 4);

    p.push(0, b0);
    p.push(1, b1);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "must produce output");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              200 * GST_MSECOND);
    gst_sample_unref(s);

    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// CE_gather_same_source_merge: same stream_id + same PTS → object merge
// Target: frame_meta_merge L144-174, check_same_source L186
// MUT: remove frame_meta_merge call → second pad objects missing
GST_START_TEST(CE_gather_same_source_merge) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    GstClockTime pts = 100 * GST_MSECOND;

    // pad0: stream_id=0, obj label=1
    GstBuffer *b0 = make_buf(pts);
    DXFrameMeta *fm0 = make_frame_meta(b0, 0, 4, 4);
    DXObjectMeta *o0 = add_object_to_frame(fm0, 1, 0.9f, 10, 20, 30, 40);

    // pad1: stream_id=0, obj label=2 (different meta_id)
    GstBuffer *b1 = make_buf(pts);
    DXFrameMeta *fm1 = make_frame_meta(b1, 0, 4, 4);
    DXObjectMeta *o1 = add_object_to_frame(fm1, 2, 0.8f, 50, 60, 70, 80);

    p.push(0, b0);
    p.push(1, b1);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "merged output must arrive");
    GstBuffer *out = gst_sample_get_buffer(s);

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr);
    fail_unless(ofm->_object_meta_list.size() == 2,
                "merged frame must have 2 objects (got %lu)",
                (unsigned long)ofm->_object_meta_list.size());

    gst_sample_unref(s);
    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// CE_gather_different_source_skip: different stream_id → no merge
// Target: check_same_source L186 (stream_id comparison)
// MUT: always return TRUE → merges different sources too
GST_START_TEST(CE_gather_different_source_skip) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    GstClockTime pts = 100 * GST_MSECOND;

    // pad0: stream_id=0
    GstBuffer *b0 = make_buf(pts);
    DXFrameMeta *fm0 = make_frame_meta(b0, 0, 4, 4);
    add_object_to_frame(fm0, 1, 0.9f, 10, 20, 30, 40);

    // pad1: stream_id=1 (different source)
    GstBuffer *b1 = make_buf(pts);
    DXFrameMeta *fm1 = make_frame_meta(b1, 1, 4, 4);
    add_object_to_frame(fm1, 2, 0.8f, 50, 60, 70, 80);

    p.push(0, b0);
    p.push(1, b1);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "output must arrive");
    GstBuffer *out = gst_sample_get_buffer(s);

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr);
    fail_unless(ofm->_object_meta_list.size() == 1,
                "different stream_id must NOT merge (got %lu objects)",
                (unsigned long)ofm->_object_meta_list.size());

    gst_sample_unref(s);
    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// CE_gather_all_eos: all pads EOS → GST_FLOW_EOS
// Target: gst_dxgather_aggregate L223-226
// MUT: remove all_eos condition → infinite aggregate loop
GST_START_TEST(CE_gather_all_eos) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    // push 1 buffer per pad then EOS
    GstBuffer *b0 = make_buf(100 * GST_MSECOND);
    make_frame_meta(b0, 0, 4, 4);
    GstBuffer *b1 = make_buf(100 * GST_MSECOND);
    make_frame_meta(b1, 0, 4, 4);

    p.push(0, b0);
    p.push(1, b1);
    p.eos(0);
    p.eos(1);

    // consume buffers
    GstSample *s = p.pull();
    if (s) gst_sample_unref(s);

    // wait for EOS on bus
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(p.pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "expected EOS on bus after all pads EOS");
    fail_unless_equals_int(GST_MESSAGE_TYPE(msg), GST_MESSAGE_EOS);
    gst_message_unref(msg);
    gst_object_unref(bus);

    p.stop();
}
GST_END_TEST;

// CE_gather_meta_id_merge: same meta_id → merge_object_meta (preserves existing fields + fills new)
// Target: frame_meta_merge L161 (meta_id comparison → merge_object_meta)
// MUT: remove L161 comparison → always copy (append) → object count increases
GST_START_TEST(CE_gather_meta_id_merge) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    GstClockTime pts = 100 * GST_MSECOND;

    // pad0: obj with label=5, track_id=-1
    GstBuffer *b0 = make_buf(pts);
    DXFrameMeta *fm0 = make_frame_meta(b0, 0, 4, 4);
    DXObjectMeta *o0 = add_object_to_frame(fm0, 5, 0.9f, 10, 20, 30, 40);
    guint64 shared_id = o0->_meta_id;

    // pad1: same meta_id, label=-1(empty), track_id=42
    GstBuffer *b1 = make_buf(pts);
    DXFrameMeta *fm1 = make_frame_meta(b1, 0, 4, 4);
    DXObjectMeta *o1 = add_object_to_frame(fm1, -1, -1.0f, 0, 0, 0, 0);
    o1->_meta_id = shared_id;
    o1->_track_id = 42;

    p.push(0, b0);
    p.push(1, b1);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "merged output must arrive");
    GstBuffer *out = gst_sample_get_buffer(s);

    DXFrameMeta *ofm = dx_get_frame_meta(out);
    fail_unless(ofm != nullptr);
    fail_unless(ofm->_object_meta_list.size() == 1,
                "same meta_id must merge, not duplicate (got %lu)",
                (unsigned long)ofm->_object_meta_list.size());

    DXObjectMeta *merged = ofm->_object_meta_list[0];
    fail_unless_equals_int(merged->_label, 5);
    fail_unless_equals_int(merged->_track_id, 42);
    fail_unless(merged->_confidence > 0.89f,
                "confidence from pad0 must be preserved (got %f)",
                merged->_confidence);

    gst_sample_unref(s);
    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// CE_gather_no_meta_passthrough: buffer without DXFrameMeta → check_same_source returns TRUE
// Target: check_same_source L180-181 (frame_meta null → TRUE)
// MUT: remove null check → null deref crash
GST_START_TEST(CE_gather_no_meta_passthrough) {
    AggPipe p = make_agg_pipe(2);
    p.start();

    GstClockTime pts = 100 * GST_MSECOND;

    // pad0: no meta
    p.push(0, make_buf(pts));
    // pad1: no meta
    p.push(1, make_buf(pts));

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "no-meta buffers must still produce output");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)), pts);
    gst_sample_unref(s);

    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

static Suite *dxgather_suite(void) {
    Suite *s = suite_create("dxgather");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_gather_latest_pts_pop);
    tcase_add_test(tc, CE_gather_same_source_merge);
    tcase_add_test(tc, CE_gather_different_source_skip);
    tcase_add_test(tc, CE_gather_all_eos);
    tcase_add_test(tc, CE_gather_meta_id_merge);
    tcase_add_test(tc, CE_gather_no_meta_passthrough);
    return s;
}

GST_CHECK_MAIN(dxgather);
