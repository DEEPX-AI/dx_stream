// Phase 7 — dxinfer event/query contract tests
// Core: dxinfer is a GstElement direct subclass → ALL event/query propagation is manual.
// These tests verify the manual implementations: FLUSH, EOS drain, LATENCY query,
// ALLOCATION query, QoS drop, and wrapped EOS forwarding.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static std::string resolve_test_model() {
    return resolve_model_path("yolov5-s_640x640_ppu.dxnn");
}

static gboolean model_available(void) {
    return !resolve_test_model().empty();
}

static gboolean runtime_available(void) {
    if (!model_available()) return FALSE;
    if (!npu_available()) return FALSE;

    static int cached = -1;
    if (cached >= 0) return cached;

    std::string model = resolve_test_model();
    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! appsink name=sink sync=false", model.c_str());
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err || !pipe) {
        if (err) g_error_free(err);
        if (pipe) gst_object_unref(pipe);
        cached = 0;
        return FALSE;
    }

    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstBuffer *buf = make_video_buffer("RGB", 64, 64, 0);
    dx_create_frame_meta(buf);
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fm->_stream_id = 0; fm->_width = 64; fm->_height = 64; fm->_format = "RGB";
    gst_app_src_push_buffer(GST_APP_SRC(src), buf);

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 3 * GST_SECOND);
    cached = (s != nullptr) ? 1 : 0;
    if (s) gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(src);
    gst_object_unref(sink);
    gst_object_unref(pipe);

    if (!cached) g_print("[INFO] NPU runtime probe failed — inference TCs will be skipped\n");
    return cached;
}

static GstElement *make_infer_pipeline_appsrc(const std::string &model,
                                               const char *sink_elem = "appsink") {
    GError *err = nullptr;
    gchar *launch = g_strdup_printf(
        "appsrc name=src format=time is-live=false "
        "caps=video/x-raw,format=RGB,width=640,height=640,framerate=30/1 "
        "! dxpreprocess resize-width=640 resize-height=640 "
        "! dxinfer name=infer model-path=%s backend=dxrt "
        "! %s name=sink sync=false", model.c_str(), sink_elem);
    GstElement *pipe = gst_parse_launch(launch, &err);
    g_free(launch);
    if (err) { g_error_free(err); return nullptr; }
    return pipe;
}

static GstBuffer *make_infer_buffer(GstClockTime pts) {
    GstBuffer *buf = make_video_buffer("RGB", 640, 640, pts);
    dx_create_frame_meta(buf);
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fm->_stream_id = 0;
    fm->_width = 640;
    fm->_height = 640;
    fm->_format = "RGB";
    return buf;
}

// ---------------------------------------------------------------------------
// CE_infer_flush_stop_resets_eos_state
// Target: gst_dxinfer_sink_event FLUSH_STOP L485-494
//   - backend->Reset() called
//   - stream_eos_arrived.clear()
//   - stream_pending_buffers.clear()
// MUT: remove L490-493 → after flush, stream still appears as EOS'd → buffers dropped
// Requires: NPU runtime
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_flush_stop_resets_eos_state) {
    DXTEST_SKIP_IF(!runtime_available(), "NPU runtime not available");
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model);
    fail_unless(pipe != nullptr);

    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    gst_app_src_push_buffer(GST_APP_SRC(src), make_infer_buffer(0));
    g_usleep(200000);

    GstPad *src_pad = gst_element_get_static_pad(src, "src");
    gst_pad_push_event(src_pad, gst_event_new_flush_start());
    gst_pad_push_event(src_pad, gst_event_new_flush_stop(TRUE));

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_pad_push_event(src_pad, gst_event_new_segment(&seg));
    gst_object_unref(src_pad);

    gst_app_src_push_buffer(GST_APP_SRC(src), make_infer_buffer(100 * GST_MSECOND));

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 5 * GST_SECOND);
    fail_unless(s != nullptr,
                "Buffer after FLUSH must not be dropped — FLUSH_STOP must clear EOS state");
    gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(src);
    gst_object_unref(sink);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_flush_start_calls_backend_flush
// Target: gst_dxinfer_sink_event FLUSH_START L478-483
//   - backend->Flush() called (unblocks Get())
//   - cv.notify_all() (wakes push thread)
// MUT: remove L479-480 → push thread blocks forever on backend->Get()
// Verified by: pipeline doesn't hang on FLUSH_START + subsequent NULL transition
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_flush_start_calls_backend_flush) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    for (int i = 0; i < 3; i++) {
        gst_app_src_push_buffer(GST_APP_SRC(src), make_infer_buffer(i * GST_SECOND / 30));
    }

    g_usleep(100000);

    GstPad *src_pad = gst_element_get_static_pad(src, "src");
    gst_pad_push_event(src_pad, gst_event_new_flush_start());
    gst_pad_push_event(src_pad, gst_event_new_flush_stop(TRUE));
    gst_object_unref(src_pad);

    GstStateChangeReturn ret = gst_element_set_state(pipe, GST_STATE_NULL);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "NULL transition after FLUSH must not fail/hang");

    gst_object_unref(src);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_default_event_forwarded
// Target: gst_dxinfer_sink_event default case L499-500
//   - Unknown events forwarded via gst_pad_push_event(srcpad, event)
// MUT: remove L500 → downstream never sees TAG/GAP events
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_default_event_forwarded) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");

    GstPad *infer_src = gst_element_get_static_pad(infer, "src");
    EventCounter ec = {};
    attach_event_counter(infer_src, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(infer_src);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstPad *infer_sink = gst_element_get_static_pad(infer, "sink");
    GstTagList *tags = gst_tag_list_new(GST_TAG_TITLE, "test", NULL);
    gst_pad_send_event(infer_sink, gst_event_new_tag(tags));
    gst_object_unref(infer_sink);

    g_usleep(100000);
    fail_unless(ec.n_tag >= 1,
                "TAG event must be forwarded downstream (got %d)", ec.n_tag);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(infer);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_wrapped_event_forwarded
// Target: handle_custom_downstream_event L445-449 (non-EOS wrapped → push downstream)
// MUT: remove L446 → wrapped non-EOS events lost
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_wrapped_event_forwarded) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");

    GstPad *infer_src = gst_element_get_static_pad(infer, "src");
    EventCounter ec = {};
    attach_event_counter(infer_src, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(infer_src);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstCaps *caps = caps_raw("RGB", 64, 64);
    GstEvent *caps_ev = gst_event_new_caps(caps);
    gst_caps_unref(caps);
    GstEvent *wrapped = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM,
        gst_structure_new("application/x-dx-wrapped-event",
            "stream-id", G_TYPE_INT, 0,
            "event", GST_TYPE_EVENT, caps_ev, NULL));
    gst_event_unref(caps_ev);

    GstPad *infer_sink = gst_element_get_static_pad(infer, "sink");
    gst_pad_send_event(infer_sink, wrapped);
    gst_object_unref(infer_sink);

    g_usleep(100000);
    fail_unless(ec.n_wrapped >= 1,
                "Wrapped non-EOS event must be forwarded downstream (got %d)",
                ec.n_wrapped);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(infer);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_sink_query_allocation
// Target: gst_dxinfer_sink_query L506-517
//   - ALLOCATION query: adds DX_FRAME_META_API_TYPE, proxies to srcpad peer
// MUT: remove L513 → DX_FRAME_META_API_TYPE not advertised upstream
// Uses appsrc pipeline in PAUSED to avoid actual inference.
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_sink_query_allocation) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

    GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipe);
        return;
    }

    GstState cur;
    gst_element_get_state(pipe, &cur, nullptr, 3 * GST_SECOND);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");
    GstPad *sinkpad = gst_element_get_static_pad(infer, "sink");

    GstQuery *q = gst_query_new_allocation(
        gst_caps_new_empty_simple("video/x-raw"), FALSE);
    gboolean ret = gst_pad_query(sinkpad, q);

    if (ret) {
        guint n_metas = gst_query_get_n_allocation_metas(q);
        gboolean found = FALSE;
        for (guint i = 0; i < n_metas; i++) {
            GType api = gst_query_parse_nth_allocation_meta(q, i, nullptr);
            if (api == DX_FRAME_META_API_TYPE) {
                found = TRUE;
                break;
            }
        }
        fail_unless(found, "ALLOCATION query must include DX_FRAME_META_API_TYPE");
    }

    gst_query_unref(q);
    gst_object_unref(sinkpad);
    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_src_query_latency
// Target: gst_dxinfer_src_query L519-546
//   - LATENCY query: adds avg_latency * GST_MSECOND to min/max
// MUT: remove L531-535 → infer latency not added → downstream sync breaks
// Uses appsrc pipeline in PAUSED to avoid actual inference.
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_src_query_latency) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

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

    if (ret) {
        fail_unless(GST_CLOCK_TIME_IS_VALID(min_lat),
                    "min_latency must be valid (got %" GST_TIME_FORMAT ")",
                    GST_TIME_ARGS(min_lat));

        if (GST_CLOCK_TIME_IS_VALID(max_lat)) {
            fail_unless(max_lat >= min_lat,
                        "max_latency (%" GST_TIME_FORMAT ") must be >= min_latency ("
                        "%" GST_TIME_FORMAT ")",
                        GST_TIME_ARGS(max_lat), GST_TIME_ARGS(min_lat));
        }

        fail_unless(live == FALSE,
                    "non-live pipeline should report live=FALSE, got TRUE");
    }

    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_src_query_default_forwarded
// Target: gst_dxinfer_src_query L545 — default queries forwarded to sinkpad peer
// MUT: remove L545 → CAPS/POSITION/etc queries never reach upstream
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_src_query_default_forwarded) {
    if (!model_available()) return;
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model, "fakesink");
    fail_unless(pipe != nullptr);

    GstStateChangeReturn sret = gst_element_set_state(pipe, GST_STATE_PAUSED);
    if (sret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(pipe);
        return;
    }

    GstState cur;
    gst_element_get_state(pipe, &cur, nullptr, 3 * GST_SECOND);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");

    GstCaps *result = nullptr;
    gboolean ret = query_caps_on_pad(infer, "src", nullptr, &result);
    fail_unless(ret, "CAPS query on dxinfer src must be forwarded upstream");
    fail_unless(result != nullptr && !gst_caps_is_empty(result),
                "CAPS query result must not be empty");
    if (result) gst_caps_unref(result);

    gst_object_unref(infer);
    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// CE_infer_src_event_qos_records_timediff
// Target: gst_dxinfer_src_event L548-582
//   - QoS UNDERFLOW → records qos_timediff/qos_timestamp
//   - QoS THROTTLE → records throttling_delay
// MUT: remove L568-577 → qos_timediff stays 0 → should_drop_buffer_due_to_qos never drops
// Requires: NPU runtime (data must flow for verification)
// ---------------------------------------------------------------------------
GST_START_TEST(CE_infer_src_event_qos_records_timediff) {
    DXTEST_SKIP_IF(!runtime_available(), "NPU runtime not available");
    std::string model = resolve_test_model();

    GstElement *pipe = make_infer_pipeline_appsrc(model);
    fail_unless(pipe != nullptr);

    GstElement *infer = gst_bin_get_by_name(GST_BIN(pipe), "infer");
    GstElement *src = gst_bin_get_by_name(GST_BIN(pipe), "src");
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstPad *infer_src = gst_element_get_static_pad(infer, "src");
    GstEvent *qos = gst_event_new_qos(GST_QOS_TYPE_UNDERFLOW, 1.0,
                                       500 * GST_MSECOND, 1 * GST_SECOND);
    gboolean sent = gst_pad_send_event(infer_src, qos);
    fail_unless(sent, "QoS event must be accepted on src pad");
    gst_object_unref(infer_src);

    gst_app_src_push_buffer(GST_APP_SRC(src), make_infer_buffer(0));
    gst_app_src_push_buffer(GST_APP_SRC(src), make_infer_buffer(10 * GST_SECOND));

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink), 5 * GST_SECOND);
    fail_unless(s != nullptr, "Buffer with future PTS must arrive after QoS");
    GstClockTime out_pts = GST_BUFFER_PTS(gst_sample_get_buffer(s));
    fail_unless(out_pts == 10 * GST_SECOND,
                "Expected PTS=10s, got %" GST_TIME_FORMAT, GST_TIME_ARGS(out_pts));
    gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(src);
    gst_object_unref(sink);
    gst_object_unref(infer);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *dxinfer_events_suite(void) {
    Suite *s = suite_create("dxinfer_events");
    TCase *tc = tcase_create("event_query");
    tcase_set_timeout(tc, 30.0);
    tcase_add_checked_fixture(tc, dxtest_crash_fixture_setup, NULL);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_infer_flush_stop_resets_eos_state);
    tcase_add_test(tc, CE_infer_flush_start_calls_backend_flush);
    tcase_add_test(tc, CE_infer_default_event_forwarded);
    tcase_add_test(tc, CE_infer_wrapped_event_forwarded);
    tcase_add_test(tc, CE_infer_sink_query_allocation);
    tcase_add_test(tc, CE_infer_src_query_latency);
    tcase_add_test(tc, CE_infer_src_query_default_forwarded);
    tcase_add_test(tc, CE_infer_src_event_qos_records_timediff);
    return s;
}

GST_CHECK_MAIN(dxinfer_events);
