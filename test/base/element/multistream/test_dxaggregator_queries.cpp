// Phase 7 — dxinputselector / dxgather query contract tests
// Core: Both are GstAggregator subclasses with custom src_query overrides.
// - dxinputselector adds max_queue_size * frame_duration to LATENCY
// - dxgather adds worst-frame-duration to LATENCY
// - Both have ALLOCATION query handling (sink adds DX_FRAME_META_API_TYPE)

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

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
    void stop() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }
};

static AggPipe make_agg_pipe(const char *elem, int n_src,
                             const char *caps_str = CAPS_RGB) {
    AggPipe p = {};
    p.n = n_src;
    p.pipe = gst_pipeline_new(nullptr);
    p.agg = gst_element_factory_make(elem, "agg");
    p.sink = gst_element_factory_make("appsink", "sink");
    g_object_set(p.sink, "sync", FALSE, nullptr);
    gst_bin_add_many(GST_BIN(p.pipe), p.agg, p.sink, nullptr);
    gst_element_link(p.agg, p.sink);
    GstCaps *caps = gst_caps_from_string(caps_str);
    for (int i = 0; i < n_src; i++) {
        char name[32];
        snprintf(name, sizeof(name), "src%d", i);
        p.src[i] = gst_element_factory_make("appsrc", name);
        g_object_set(p.src[i], "format", GST_FORMAT_TIME,
                     "is-live", TRUE, "caps", caps, nullptr);
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

// ===========================================================================
// dxinputselector queries
// ===========================================================================

// ---------------------------------------------------------------------------
// CE_inputsel_latency_query_adds_preroll
// Target: gst_dxinputselector_src_query L346-376
//   - LATENCY: adds max_queue_size * frame_duration to min_lat
// MUT: remove L360-363 → min_lat unchanged → sink sync too tight
// ---------------------------------------------------------------------------
GST_START_TEST(CE_inputsel_latency_query_adds_preroll) {
    AggPipe p = make_agg_pipe("dxinputselector", 1);
    g_object_set(p.agg, "max-queue-size", 5u, nullptr);
    p.start();

    // Push one buffer so caps are negotiated (framerate=30/1 → frame_dur=33.3ms)
    p.push(0, make_buf(0));
    GstSample *s = p.pull(2 * GST_SECOND);
    if (s) gst_sample_unref(s);

    GstPad *srcpad = gst_element_get_static_pad(p.agg, "src");
    GstQuery *q = gst_query_new_latency();
    gboolean ret = gst_pad_query(srcpad, q);
    fail_unless(ret, "LATENCY query must succeed on dxinputselector src");

    gboolean live = FALSE;
    GstClockTime min_lat = 0, max_lat = 0;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);

    // max_queue_size=5, frame_dur=33.3ms → self_buf ≈ 166ms
    // min_lat should include self_buf (at least 100ms to be safe)
    fail_unless(min_lat >= 100 * GST_MSECOND,
                "min_latency must include preroll buffer time "
                "(expected >= 100ms, got %" GST_TIME_FORMAT ")",
                GST_TIME_ARGS(min_lat));

    gst_query_unref(q);
    gst_object_unref(srcpad);
    p.eos(0);
    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_inputsel_sink_query_allocation_meta
// Target: gst_dxinputselector_sink_query L379-385
//   - ALLOCATION: adds DX_FRAME_META_API_TYPE
// MUT: remove L384 → upstream allocator doesn't know about DXFrameMeta
// ---------------------------------------------------------------------------
GST_START_TEST(CE_inputsel_sink_query_allocation_meta) {
    AggPipe p = make_agg_pipe("dxinputselector", 1);
    p.start();

    // Push so pipeline is running and pads linked
    p.push(0, make_buf(0));
    GstSample *s = p.pull(2 * GST_SECOND);
    if (s) gst_sample_unref(s);

    // Query allocation on sink_0 pad
    GstPad *sink0 = nullptr;
    for (GList *l = GST_ELEMENT(p.agg)->sinkpads; l; l = l->next) {
        sink0 = GST_PAD(l->data);
        break;
    }
    fail_unless(sink0 != nullptr, "must have at least one sink pad");

    GstCaps *caps = gst_caps_from_string(CAPS_RGB);
    GstQuery *q = gst_query_new_allocation(caps, FALSE);
    gboolean ret = gst_pad_query(sink0, q);
    gst_caps_unref(caps);

    if (ret) {
        guint n = gst_query_get_n_allocation_metas(q);
        gboolean found = FALSE;
        for (guint i = 0; i < n; i++) {
            if (gst_query_parse_nth_allocation_meta(q, i, nullptr) ==
                DX_FRAME_META_API_TYPE) {
                found = TRUE;
                break;
            }
        }
        fail_unless(found,
                    "ALLOCATION query on dxinputselector sink must include "
                    "DX_FRAME_META_API_TYPE");
    }

    gst_query_unref(q);
    p.eos(0);
    p.stop();
}
GST_END_TEST;

// ===========================================================================
// dxgather queries
// ===========================================================================

// ---------------------------------------------------------------------------
// CE_gather_latency_query_adds_frame
// Target: gst_dxgather_src_query L312-342
//   - LATENCY: adds worst frame duration (slowest sink framerate) to min_lat
// MUT: remove L336-339 → latency unchanged → sink sync mismatch
// ---------------------------------------------------------------------------
GST_START_TEST(CE_gather_latency_query_adds_frame) {
    AggPipe p = make_agg_pipe("dxgather", 2);
    p.start();

    // Push buffers to negotiate caps (framerate=30/1 → 33.3ms)
    p.push(0, make_buf(0));
    p.push(1, make_buf(0));
    GstSample *s = p.pull(2 * GST_SECOND);
    if (s) gst_sample_unref(s);

    GstPad *srcpad = gst_element_get_static_pad(p.agg, "src");
    GstQuery *q = gst_query_new_latency();
    gboolean ret = gst_pad_query(srcpad, q);
    fail_unless(ret, "LATENCY query must succeed on dxgather src");

    gboolean live = FALSE;
    GstClockTime min_lat = 0, max_lat = 0;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);

    // worst_frame = 33.3ms (both sinks are 30fps)
    // min_lat should include at least one frame duration
    fail_unless(min_lat >= 30 * GST_MSECOND,
                "min_latency must include frame duration "
                "(expected >= 30ms, got %" GST_TIME_FORMAT ")",
                GST_TIME_ARGS(min_lat));

    gst_query_unref(q);
    gst_object_unref(srcpad);
    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_gather_sink_query_allocation_meta
// Target: gst_dxgather_sink_query (similar to inputselector)
//   - ALLOCATION on sink adds DX_FRAME_META_API_TYPE
// ---------------------------------------------------------------------------
GST_START_TEST(CE_gather_sink_query_allocation_meta) {
    AggPipe p = make_agg_pipe("dxgather", 1);
    p.start();

    p.push(0, make_buf(0));
    GstSample *s = p.pull(2 * GST_SECOND);
    if (s) gst_sample_unref(s);

    GstPad *sink0 = nullptr;
    for (GList *l = GST_ELEMENT(p.agg)->sinkpads; l; l = l->next) {
        sink0 = GST_PAD(l->data);
        break;
    }
    fail_unless(sink0 != nullptr);

    GstCaps *caps = gst_caps_from_string(CAPS_RGB);
    GstQuery *q = gst_query_new_allocation(caps, FALSE);
    gboolean ret = gst_pad_query(sink0, q);
    gst_caps_unref(caps);

    if (ret) {
        guint n = gst_query_get_n_allocation_metas(q);
        gboolean found = FALSE;
        for (guint i = 0; i < n; i++) {
            if (gst_query_parse_nth_allocation_meta(q, i, nullptr) ==
                DX_FRAME_META_API_TYPE) {
                found = TRUE;
                break;
            }
        }
        fail_unless(found,
                    "ALLOCATION query on dxgather sink must include "
                    "DX_FRAME_META_API_TYPE");
    }

    gst_query_unref(q);
    p.eos(0);
    p.stop();
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_gather_partial_eos_continues
// Target: gst_dxgather_aggregate — one branch EOS, other continues
// MUT: premature GST_FLOW_EOS before all branches done → pipeline stops early
// ---------------------------------------------------------------------------
GST_START_TEST(CE_gather_partial_eos_continues) {
    AggPipe p = make_agg_pipe("dxgather", 2);
    p.start();

    // Push to both → gather outputs
    p.push(0, make_buf(0));
    p.push(1, make_buf(0));
    GstSample *s = p.pull();
    fail_unless(s != nullptr, "first output expected");
    gst_sample_unref(s);

    // EOS on branch 0 only
    p.eos(0);

    // Branch 1 still producing → gather should still output
    p.push(1, make_buf(100 * GST_MSECOND));
    s = p.pull(2 * GST_SECOND);
    fail_unless(s != nullptr,
                "After branch 0 EOS, branch 1 data must still flow");
    gst_sample_unref(s);

    // EOS both → global EOS
    p.eos(1);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(p.pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "All branches EOS must produce pipeline EOS");
    gst_message_unref(msg);
    gst_object_unref(bus);

    p.stop();
}
GST_END_TEST;

static Suite *aggregator_queries_suite(void) {
    Suite *s = suite_create("aggregator_queries");

    TCase *tc_isel = tcase_create("dxinputselector");
    tcase_set_timeout(tc_isel, 30.0);
    suite_add_tcase(s, tc_isel);
    tcase_add_test(tc_isel, CE_inputsel_latency_query_adds_preroll);
    tcase_add_test(tc_isel, CE_inputsel_sink_query_allocation_meta);

    TCase *tc_gath = tcase_create("dxgather");
    tcase_set_timeout(tc_gath, 30.0);
    suite_add_tcase(s, tc_gath);
    tcase_add_test(tc_gath, CE_gather_latency_query_adds_frame);
    tcase_add_test(tc_gath, CE_gather_sink_query_allocation_meta);
    tcase_add_test(tc_gath, CE_gather_partial_eos_continues);

    return s;
}

GST_CHECK_MAIN(aggregator_queries);
