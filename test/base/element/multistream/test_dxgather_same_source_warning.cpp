// dxgather check_same_source warning test — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.10
//
// Defect: 01_defect_report.md — gst-dxgather.cpp:248-253 silently drops a
// peeked buffer when `check_same_source(merged, buf) == FALSE` (only
// `gst_buffer_unref(buf)`, no warning/message). When two different-source
// streams (different stream_id) feed dxgather, every aggregate cycle drops
// data without any operator-visible signal.
//
// Oracle: install a GST debug log handler capturing WARNING-level entries
// from the "dxgather" category. After pushing two buffers (stream_id=0 and
// stream_id=1) with same PTS, expect ≥1 warning. Current code: 0.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include <cstring>
#include <atomic>

using namespace dxtest;

static std::atomic<guint> g_dxgather_warning_count{0};

static void capture_warning_log(GstDebugCategory *category,
                                GstDebugLevel level,
                                const gchar * /*file*/, const gchar * /*function*/,
                                gint /*line*/, GObject * /*object*/,
                                GstDebugMessage * /*message*/, gpointer /*data*/) {
    if (level != GST_LEVEL_WARNING) return;
    const gchar *name = gst_debug_category_get_name(category);
    if (name && std::strcmp(name, "dxgather") == 0) {
        g_dxgather_warning_count.fetch_add(1);
    }
}

static const char *CAPS = "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static GstBuffer *make_buf(GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo m; gst_buffer_map(b, &m, GST_MAP_WRITE);
    std::memset(m.data, 0x80, sz); gst_buffer_unmap(b, &m);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

GST_START_TEST(CE_gather_same_source_mismatch_logs_warning) {
    g_dxgather_warning_count.store(0);
    gst_debug_set_default_threshold(GST_LEVEL_WARNING);
    gst_debug_add_log_function(capture_warning_log, nullptr, nullptr);

    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *agg  = gst_element_factory_make("dxgather", "agg");
    GstElement *sink = gst_element_factory_make("appsink", "sink");
    g_object_set(sink, "sync", FALSE, nullptr);
    GstElement *src0 = gst_element_factory_make("appsrc", "src0");
    GstElement *src1 = gst_element_factory_make("appsrc", "src1");
    GstCaps *caps = gst_caps_from_string(CAPS);
    g_object_set(src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src0, src1, agg, sink, nullptr);
    fail_unless(gst_element_link(agg, sink));
    GstPad *r0 = gst_element_get_request_pad(agg, "sink_0");
    GstPad *r1 = gst_element_get_request_pad(agg, "sink_1");
    GstPad *sp0 = gst_element_get_static_pad(src0, "src");
    GstPad *sp1 = gst_element_get_static_pad(src1, "src");
    fail_unless(gst_pad_link(sp0, r0) == GST_PAD_LINK_OK);
    fail_unless(gst_pad_link(sp1, r1) == GST_PAD_LINK_OK);
    gst_object_unref(sp0); gst_object_unref(sp1);
    gst_object_unref(r0);  gst_object_unref(r1);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstClockTime pts = 100 * GST_MSECOND;

    // pad0 stream_id=0, pad1 stream_id=1 — different sources, same PTS.
    GstBuffer *b0 = make_buf(pts);
    make_frame_meta(b0, 0, 4, 4);
    GstBuffer *b1 = make_buf(pts);
    make_frame_meta(b1, 1, 4, 4);

    gst_app_src_push_buffer(GST_APP_SRC(src0), b0);
    gst_app_src_push_buffer(GST_APP_SRC(src1), b1);

    // Drain output (may produce one buffer; we don't care about contents).
    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                2 * GST_SECOND);
    if (s) gst_sample_unref(s);

    gst_app_src_end_of_stream(GST_APP_SRC(src0));
    gst_app_src_end_of_stream(GST_APP_SRC(src1));

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 3 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) gst_message_unref(msg);
    gst_object_unref(bus);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
    gst_debug_remove_log_function(capture_warning_log);

    guint warnings = g_dxgather_warning_count.load();
    fail_unless(warnings >= 1,
                "dxgather silently dropped different-source buffer — "
                "expected ≥1 WARNING from 'dxgather' category, got %u",
                warnings);
}
GST_END_TEST;

static Suite *dxgather_same_source_warning_suite(void) {
    Suite *s = suite_create("dxgather_same_source_warning");
    TCase *tc = tcase_create("warning");
    tcase_set_timeout(tc, 15.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_gather_same_source_mismatch_logs_warning);
    return s;
}

GST_CHECK_MAIN(dxgather_same_source_warning);
