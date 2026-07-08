// Phase 2 — Per-stream EOS drain ordering
// B9: When sink_0 gets EOS, all its pending buffers must drain before wrapped EOS is sent.
// sink_1 must continue operating independently.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

struct DualStreamPipe {
    GstElement *pipe;
    GstElement *src0, *src1;
    GstElement *isel, *osel;
    GstElement *sink0, *sink1;
    GstBus *bus;

    static DualStreamPipe build() {
        DualStreamPipe p = {};
        p.pipe = gst_pipeline_new(nullptr);
        p.src0 = gst_element_factory_make("appsrc", "src0");
        p.src1 = gst_element_factory_make("appsrc", "src1");
        p.isel = gst_element_factory_make("dxinputselector", "isel");
        p.osel = gst_element_factory_make("dxoutputselector", "osel");
        GstElement *q0 = gst_element_factory_make("queue", "q0");
        GstElement *q1 = gst_element_factory_make("queue", "q1");
        p.sink0 = gst_element_factory_make("appsink", "sink0");
        p.sink1 = gst_element_factory_make("appsink", "sink1");

        fail_unless(p.src0 && p.src1 && p.isel && p.osel &&
                    q0 && q1 && p.sink0 && p.sink1);

        GstCaps *caps = gst_caps_from_string(
            "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
        g_object_set(p.src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                     "caps", caps, nullptr);
        g_object_set(p.src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                     "caps", caps, nullptr);
        g_object_set(p.sink0, "sync", FALSE, "async", FALSE, nullptr);
        g_object_set(p.sink1, "sync", FALSE, "async", FALSE, nullptr);
        gst_caps_unref(caps);

        gst_bin_add_many(GST_BIN(p.pipe), p.src0, p.src1, p.isel, p.osel,
                         q0, q1, p.sink0, p.sink1, nullptr);

        GstPad *isel_s0 = gst_element_get_request_pad(p.isel, "sink_0");
        GstPad *isel_s1 = gst_element_get_request_pad(p.isel, "sink_1");
        GstPad *src0_p = gst_element_get_static_pad(p.src0, "src");
        GstPad *src1_p = gst_element_get_static_pad(p.src1, "src");
        gst_pad_link(src0_p, isel_s0);
        gst_pad_link(src1_p, isel_s1);
        gst_object_unref(src0_p);
        gst_object_unref(src1_p);
        gst_object_unref(isel_s0);
        gst_object_unref(isel_s1);

        gst_element_link(p.isel, p.osel);

        GstPad *osel_s0 = gst_element_get_request_pad(p.osel, "src_0");
        GstPad *osel_s1 = gst_element_get_request_pad(p.osel, "src_1");
        GstPad *q0_sink = gst_element_get_static_pad(q0, "sink");
        GstPad *q1_sink = gst_element_get_static_pad(q1, "sink");
        gst_pad_link(osel_s0, q0_sink);
        gst_pad_link(osel_s1, q1_sink);
        gst_object_unref(osel_s0);
        gst_object_unref(osel_s1);
        gst_object_unref(q0_sink);
        gst_object_unref(q1_sink);

        gst_element_link(q0, p.sink0);
        gst_element_link(q1, p.sink1);

        p.bus = gst_pipeline_get_bus(GST_PIPELINE(p.pipe));
        return p;
    }

    GstBuffer *make_buf(GstClockTime pts, int stream_id) {
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

    void play() { gst_element_set_state(pipe, GST_STATE_PLAYING); }

    void teardown() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(bus);
        gst_object_unref(pipe);
    }
};

GST_START_TEST(PL_per_stream_eos_drain_order) {
    DualStreamPipe p = DualStreamPipe::build();
    p.play();

    for (int i = 0; i < 5; i++) {
        gst_app_src_push_buffer(GST_APP_SRC(p.src0),
            p.make_buf((i * 33) * GST_MSECOND, 0));
    }

    gst_app_src_push_buffer(GST_APP_SRC(p.src1),
        p.make_buf(100 * GST_MSECOND, 1));

    g_usleep(500 * 1000);

    int drained_s0 = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(p.sink0),
                                               1 * GST_SECOND)) != nullptr) {
        drained_s0++;
        gst_sample_unref(s);
    }

    gst_app_src_end_of_stream(GST_APP_SRC(p.src0));
    g_usleep(500 * 1000);

    fail_unless(drained_s0 >= 3,
                "stream0 must drain its buffers before EOS (got %d)", drained_s0);

    gst_app_src_push_buffer(GST_APP_SRC(p.src1),
        p.make_buf(200 * GST_MSECOND, 1));

    s = gst_app_sink_try_pull_sample(GST_APP_SINK(p.sink1), 5 * GST_SECOND);
    fail_unless(s != nullptr,
                "stream1 must still receive buffers after stream0 EOS");
    gst_sample_unref(s);

    gst_app_src_end_of_stream(GST_APP_SRC(p.src1));
    g_usleep(500 * 1000);

    p.teardown();
}
GST_END_TEST;

GST_START_TEST(PL_per_stream_eos_no_cross_contamination) {
    DualStreamPipe p = DualStreamPipe::build();
    p.play();

    gst_app_src_push_buffer(GST_APP_SRC(p.src0),
        p.make_buf(100 * GST_MSECOND, 0));
    gst_app_src_push_buffer(GST_APP_SRC(p.src1),
        p.make_buf(200 * GST_MSECOND, 1));

    GstSample *s0 = gst_app_sink_try_pull_sample(GST_APP_SINK(p.sink0),
                                                    5 * GST_SECOND);
    if (s0) {
        GstBuffer *b = gst_sample_get_buffer(s0);
        DXFrameMeta *fm = dx_get_frame_meta(b);
        fail_unless(fm != nullptr);
        fail_unless_equals_int(fm->_stream_id, 0);
        gst_sample_unref(s0);
    }

    gst_app_src_end_of_stream(GST_APP_SRC(p.src0));
    g_usleep(500 * 1000);

    for (int i = 0; i < 3; i++) {
        gst_app_src_push_buffer(GST_APP_SRC(p.src1),
            p.make_buf((300 + i * 33) * GST_MSECOND, 1));
    }

    int s1_count = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(p.sink1),
                                               2 * GST_SECOND)) != nullptr) {
        GstBuffer *b = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(b);
        if (fm) {
            fail_unless_equals_int(fm->_stream_id, 1);
        }
        s1_count++;
        gst_sample_unref(s);
    }
    fail_unless(s1_count >= 2,
                "stream1 must receive multiple buffers after stream0 EOS (got %d)",
                s1_count);

    gst_app_src_end_of_stream(GST_APP_SRC(p.src1));
    g_usleep(500 * 1000);

    p.teardown();
}
GST_END_TEST;

static Suite *pl_per_stream_eos_suite(void) {
    Suite *s = suite_create("pl_per_stream_eos");
    TCase *tc = tcase_create("eos_drain");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_per_stream_eos_drain_order);
    tcase_add_test(tc, PL_per_stream_eos_no_cross_contamination);
    return s;
}

GST_CHECK_MAIN(pl_per_stream_eos);
