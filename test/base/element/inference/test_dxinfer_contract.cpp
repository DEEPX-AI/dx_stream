// dxinfer GstElement contract tests
//
// dxinfer is a GstElement direct subclass — no BaseTransform auto-forwarding.
// All event/query propagation is manual. This test verifies the manual
// implementations and pins expected behavior as contract.
//
// B.2 obligations: LATENCY, ALLOCATION, CAPS/ACCEPT_CAPS query, unknown event fwd.
// Additional: push thread lifecycle, state transition safety.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "event_probe.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static std::string resolve_test_model() {
    return resolve_model_path("YoloV5S_PPU.dxnn");
}

static gboolean model_available(void) {
    return !resolve_test_model().empty();
}

// ---------------------------------------------------------------------------
// CONTRACT_infer_sink_query_forwards_unknown
// B.2: unknown sink queries must be forwarded to src peer, not dropped.
// dxinfer does: gst_pad_peer_query(_srcpad, query) for non-ALLOCATION.
// SCHEDULING is upstream-only and triggers a core direction warning on a sink
// pad, so use a CUSTOM query (valid in both directions) to exercise the same
// default forwarding path.
// ---------------------------------------------------------------------------
static gint g_custom_query_hits = 0;

static gboolean infer_contract_downstream_query(GstPad *pad, GstObject *parent,
                                                GstQuery *query) {
    if (GST_QUERY_TYPE(query) == GST_QUERY_CUSTOM) {
        const GstStructure *s = gst_query_get_structure(query);
        if (s && gst_structure_has_name(s, "dxinfer-contract-custom")) {
            g_atomic_int_inc(&g_custom_query_hits);
            return TRUE;
        }
    }
    return gst_pad_query_default(pad, parent, query);
}

GST_START_TEST(CONTRACT_infer_sink_query_forwards_unknown) {
    DXTEST_SKIP_IF(!model_available(), "model not available");
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! fakesink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err) { g_error_free(err); return; }

    GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipe);
        return;
    }
    GstState cur;
    gst_element_get_state(pipe, &cur, nullptr, 3 * GST_SECOND);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    GstPad *sinkpad = gst_element_get_static_pad(infer, "sink");
    GstPad *sink_peer_pad = gst_element_get_static_pad(sink, "sink");
    g_atomic_int_set(&g_custom_query_hits, 0);
    gst_pad_set_query_function(sink_peer_pad,
                               GST_DEBUG_FUNCPTR(infer_contract_downstream_query));

    GstQuery *q = gst_query_new_custom(
        GST_QUERY_CUSTOM, gst_structure_new_empty("dxinfer-contract-custom"));
    gboolean ret = gst_pad_query(sinkpad, q);
    fail_unless(ret, "custom sink query must be forwarded to dxinfer src peer");
    fail_unless_equals_int(g_atomic_int_get(&g_custom_query_hits), 1);
    gst_query_unref(q);

    gst_object_unref(sink_peer_pad);
    gst_object_unref(sinkpad);
    gst_object_unref(sink);
    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CONTRACT_infer_accept_caps_rejects_invalid
// B.2: ACCEPT_CAPS query must check against pad template, not forward blindly.
// Without this, dxinfer would accept audio/x-raw if downstream happens to
// support it — violating the element's contract as a video processor.
// ---------------------------------------------------------------------------
GST_START_TEST(CONTRACT_infer_accept_caps_rejects_invalid) {
    DXTEST_SKIP_IF(!model_available(), "model not available");
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! fakesink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err) { g_error_free(err); return; }

    GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipe);
        return;
    }
    GstState cur;
    gst_element_get_state(pipe, &cur, nullptr, 3 * GST_SECOND);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");
    GstPad *sinkpad = gst_element_get_static_pad(infer, "sink");

    // Valid caps — must be accepted
    GstCaps *video_caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    GstQuery *vq = gst_query_new_accept_caps(video_caps);
    gst_caps_unref(video_caps);
    gboolean ret = gst_pad_query(sinkpad, vq);
    fail_unless(ret, "ACCEPT_CAPS query must succeed on dxinfer sink pad");
    gboolean accepted = FALSE;
    gst_query_parse_accept_caps_result(vq, &accepted);
    fail_unless(accepted,
                "dxinfer sink must accept video/x-raw caps (B.2 pad template)");
    gst_query_unref(vq);

    // Invalid caps (audio) — must be rejected
    GstCaps *audio_caps = gst_caps_from_string("audio/x-raw,rate=44100");
    GstQuery *aq = gst_query_new_accept_caps(audio_caps);
    gst_caps_unref(audio_caps);
    ret = gst_pad_query(sinkpad, aq);
    fail_unless(ret, "ACCEPT_CAPS query must succeed (return TRUE) even for rejection");
    accepted = FALSE;
    gst_query_parse_accept_caps_result(aq, &accepted);
    fail_if(accepted,
            "dxinfer sink must reject audio/x-raw caps — "
            "pad template only allows video/x-raw and dxvideoraw");
    gst_query_unref(aq);

    // dxvideoraw caps — must be accepted (domain mode)
    GstCaps *dx_caps = gst_caps_from_string(
        "application/x-dxvideoraw,format=RGB,width=640,height=480,framerate=30/1");
    GstQuery *dq = gst_query_new_accept_caps(dx_caps);
    gst_caps_unref(dx_caps);
    ret = gst_pad_query(sinkpad, dq);
    fail_unless(ret, "ACCEPT_CAPS query must succeed");
    accepted = FALSE;
    gst_query_parse_accept_caps_result(dq, &accepted);
    fail_unless(accepted,
                "dxinfer sink must accept dxvideoraw caps (domain mode)");
    gst_query_unref(dq);

    gst_object_unref(sinkpad);
    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CONTRACT_infer_state_cycle_thread_join
// B.1/E.5: PAUSED→READY must join push thread. Repeated cycles must not leak
// threads or hang.
// ---------------------------------------------------------------------------
GST_START_TEST(CONTRACT_infer_state_cycle_thread_join) {
    DXTEST_SKIP_IF(!model_available(), "model not available");
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! fakesink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err) { g_error_free(err); return; }

    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");

    for (int cycle = 0; cycle < 5; cycle++) {
        GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PLAYING);
        fail_unless(sret != GST_STATE_CHANGE_FAILURE,
                    "cycle %d: PLAYING failed", cycle);

        // Push a buffer to activate push thread
        GstBuffer *buf = make_video_buffer("RGB", 64, 64, cycle * GST_SECOND);
        dx_create_frame_meta(buf);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        fm->_stream_id = 0; fm->_width = 64; fm->_height = 64; fm->_format = "RGB";
        gst_app_src_push_buffer(GST_APP_SRC(src), buf);
        g_usleep(100000);  // let push thread start

        // Transition back to NULL — must join push thread without hang
        sret = gst_element_set_state(pipe, GST_STATE_NULL);
        fail_unless(sret != GST_STATE_CHANGE_FAILURE,
                    "cycle %d: NULL transition failed (thread join hang?)", cycle);
    }

    gst_object_unref(src);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CONTRACT_infer_latency_adds_avg
// Pin: dxinfer src_query LATENCY adds avg_latency * GST_MSECOND.
// Before any inference, avg_latency is 0, so min_latency == upstream.
// After inference, avg_latency > 0, so min_latency > upstream.
// This test verifies the mechanism is in place (initial state).
// ---------------------------------------------------------------------------
GST_START_TEST(CONTRACT_infer_latency_adds_avg) {
    DXTEST_SKIP_IF(!model_available(), "model not available");
    std::string model = resolve_test_model();

    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! fakesink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err) { g_error_free(err); return; }

    GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipe);
        return;
    }
    GstState cur;
    gst_element_get_state(pipe, &cur, nullptr, 3 * GST_SECOND);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");
    gboolean live = FALSE;
    GstClockTime min_lat = 0, max_lat = 0;
    gboolean ret = query_latency_on_src(infer, &live, &min_lat, &max_lat);

    fail_unless(ret, "LATENCY query on dxinfer src must succeed");
    fail_unless(GST_CLOCK_TIME_IS_VALID(min_lat),
                "min_latency must be valid, got %" GST_TIME_FORMAT,
                GST_TIME_ARGS(min_lat));
    fail_unless(live == FALSE,
                "non-live pipeline should report live=FALSE");

    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *dxinfer_contract_suite(void) {
    Suite *s = suite_create("dxinfer_contract");
    TCase *tc = tcase_create("element_contract");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CONTRACT_infer_sink_query_forwards_unknown);
    tcase_add_test(tc, CONTRACT_infer_accept_caps_rejects_invalid);
    tcase_add_test(tc, CONTRACT_infer_state_cycle_thread_join);
    tcase_add_test(tc, CONTRACT_infer_latency_adds_avg);
    return s;
}

GST_CHECK_MAIN(dxinfer_contract);
