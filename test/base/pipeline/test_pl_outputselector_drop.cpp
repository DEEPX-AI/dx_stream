// Phase 2 — dxoutputselector event drop verification (pin test)
// B10: dxoutputselector drops global EOS (non-wrapped).
// Wrapped EOS is forwarded to the correct src pad.
// This is intentional behavior for multi-stream — tests pin it.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "event_probe.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static GstBuffer *make_osel_buf(GstClockTime pts, int stream_id) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, 4, 4);
    return b;
}

struct OselTestEnv {
    GstElement *pipe, *src, *isel, *osel, *sink0, *sink1;
    GstBus *bus;
    EventTrace *trace0;
    EventTrace *trace1;
};

static OselTestEnv create_osel_env() {
    OselTestEnv e = {};
    e.trace0 = new EventTrace();
    e.trace1 = new EventTrace();
    e.pipe = gst_pipeline_new(nullptr);
    e.src = gst_element_factory_make("appsrc", "src");
    e.isel = gst_element_factory_make("dxinputselector", "isel");
    e.osel = gst_element_factory_make("dxoutputselector", "osel");
    GstElement *q0 = gst_element_factory_make("queue", "q0");
    GstElement *q1 = gst_element_factory_make("queue", "q1");
    e.sink0 = gst_element_factory_make("appsink", "sink0");
    e.sink1 = gst_element_factory_make("appsink", "sink1");

    fail_unless(e.src && e.isel && e.osel && q0 && q1 && e.sink0 && e.sink1);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
    g_object_set(e.src, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(e.sink0, "sync", FALSE, "async", FALSE, nullptr);
    g_object_set(e.sink1, "sync", FALSE, "async", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(e.pipe), e.src, e.isel, e.osel,
                     q0, q1, e.sink0, e.sink1, nullptr);

    GstPad *isel_s0 = gst_element_get_request_pad(e.isel, "sink_0");
    GstPad *src_p = gst_element_get_static_pad(e.src, "src");
    gst_pad_link(src_p, isel_s0);
    gst_object_unref(src_p);
    gst_object_unref(isel_s0);

    gst_element_link(e.isel, e.osel);

    GstPad *osel_s0 = gst_element_get_request_pad(e.osel, "src_0");
    GstPad *osel_s1 = gst_element_get_request_pad(e.osel, "src_1");
    GstPad *q0_sink = gst_element_get_static_pad(q0, "sink");
    GstPad *q1_sink = gst_element_get_static_pad(q1, "sink");
    gst_pad_link(osel_s0, q0_sink);
    gst_pad_link(osel_s1, q1_sink);

    GstPad *q0_src = gst_element_get_static_pad(q0, "src");
    GstPad *q1_src = gst_element_get_static_pad(q1, "src");
    e.trace0->attach_downstream(q0_src);
    e.trace1->attach_downstream(q1_src);
    gst_object_unref(q0_src);
    gst_object_unref(q1_src);

    gst_object_unref(osel_s0);
    gst_object_unref(osel_s1);
    gst_object_unref(q0_sink);
    gst_object_unref(q1_sink);

    gst_element_link(q0, e.sink0);
    gst_element_link(q1, e.sink1);

    e.bus = gst_pipeline_get_bus(GST_PIPELINE(e.pipe));
    return e;
}

static void destroy_osel_env(OselTestEnv *e) {
    gst_element_set_state(e->pipe, GST_STATE_NULL);
    gst_object_unref(e->bus);
    gst_object_unref(e->pipe);
    delete e->trace0;
    delete e->trace1;
}

GST_START_TEST(PL_osel_wrapped_eos_routes_correctly) {
    OselTestEnv e = create_osel_env();
    gst_element_set_state(e.pipe, GST_STATE_PLAYING);

    gst_app_src_push_buffer(GST_APP_SRC(e.src),
        make_osel_buf(100 * GST_MSECOND, 0));

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(e.sink0),
                                                   5 * GST_SECOND);
    if (s) gst_sample_unref(s);

    gst_app_src_end_of_stream(GST_APP_SRC(e.src));
    g_usleep(1000 * 1000);

    int sink0_eos = e.trace0->count(GST_EVENT_EOS);
    fail_unless(sink0_eos >= 1,
                "sink0 must receive EOS for stream 0 (got %d)", sink0_eos);

    destroy_osel_env(&e);
}
GST_END_TEST;

GST_START_TEST(PL_osel_data_reaches_correct_sink) {
    OselTestEnv e = create_osel_env();
    gst_element_set_state(e.pipe, GST_STATE_PLAYING);

    gst_app_src_push_buffer(GST_APP_SRC(e.src),
        make_osel_buf(100 * GST_MSECOND, 0));

    GstSample *s0 = gst_app_sink_try_pull_sample(GST_APP_SINK(e.sink0),
                                                     5 * GST_SECOND);
    fail_unless(s0 != nullptr, "stream 0 data must reach sink0");
    GstBuffer *buf = gst_sample_get_buffer(s0);
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fail_unless(fm != nullptr);
    fail_unless_equals_int(fm->_stream_id, 0);
    gst_sample_unref(s0);

    gst_app_src_end_of_stream(GST_APP_SRC(e.src));
    g_usleep(500 * 1000);

    destroy_osel_env(&e);
}
GST_END_TEST;

static Suite *pl_osel_drop_suite(void) {
    Suite *s = suite_create("pl_outputselector_drop");
    TCase *tc = tcase_create("event_drop");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_osel_wrapped_eos_routes_correctly);
    tcase_add_test(tc, PL_osel_data_reaches_correct_sink);
    return s;
}

GST_CHECK_MAIN(pl_osel_drop);
