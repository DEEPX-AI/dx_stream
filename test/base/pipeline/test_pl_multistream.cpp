// PL-C — Multi-stream verification (dxinputselector + dxoutputselector)
// 2x appsrc → dxinputselector → dxoutputselector → queue → 2x appsink
// Multi-stream specific: per-stream EOS, stream_id routing, global EOS

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "pipeline_tc_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

struct MultiPipe {
    GstElement *pipe;
    GstElement *src0, *src1;
    GstElement *isel, *osel;
    GstElement *sink0, *sink1;
    GstBus *bus;

    static MultiPipe build() {
        MultiPipe m = {};
        m.pipe = gst_pipeline_new(nullptr);
        m.src0 = gst_element_factory_make("appsrc", "src0");
        m.src1 = gst_element_factory_make("appsrc", "src1");
        m.isel = gst_element_factory_make("dxinputselector", "isel");
        m.osel = gst_element_factory_make("dxoutputselector", "osel");
        GstElement *q0 = gst_element_factory_make("queue", "q0");
        GstElement *q1 = gst_element_factory_make("queue", "q1");
        m.sink0 = gst_element_factory_make("appsink", "sink0");
        m.sink1 = gst_element_factory_make("appsink", "sink1");

        fail_unless(m.src0 && m.src1 && m.isel && m.osel &&
                    q0 && q1 && m.sink0 && m.sink1);

        GstCaps *caps = gst_caps_from_string(
            "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
        g_object_set(m.src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                     "caps", caps, nullptr);
        g_object_set(m.src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                     "caps", caps, nullptr);
        g_object_set(m.sink0, "sync", FALSE, "async", FALSE, nullptr);
        g_object_set(m.sink1, "sync", FALSE, "async", FALSE, nullptr);
        gst_caps_unref(caps);

        gst_bin_add_many(GST_BIN(m.pipe), m.src0, m.src1, m.isel, m.osel,
                         q0, q1, m.sink0, m.sink1, nullptr);

        GstPad *isel_s0 = gst_element_get_request_pad(m.isel, "sink_0");
        GstPad *isel_s1 = gst_element_get_request_pad(m.isel, "sink_1");
        GstPad *src0_p = gst_element_get_static_pad(m.src0, "src");
        GstPad *src1_p = gst_element_get_static_pad(m.src1, "src");
        gst_pad_link(src0_p, isel_s0);
        gst_pad_link(src1_p, isel_s1);
        gst_object_unref(src0_p);
        gst_object_unref(src1_p);
        gst_object_unref(isel_s0);
        gst_object_unref(isel_s1);

        gst_element_link(m.isel, m.osel);

        GstPad *osel_s0 = gst_element_get_request_pad(m.osel, "src_0");
        GstPad *osel_s1 = gst_element_get_request_pad(m.osel, "src_1");
        GstPad *q0_sink = gst_element_get_static_pad(q0, "sink");
        GstPad *q1_sink = gst_element_get_static_pad(q1, "sink");
        gst_pad_link(osel_s0, q0_sink);
        gst_pad_link(osel_s1, q1_sink);
        gst_object_unref(osel_s0);
        gst_object_unref(osel_s1);
        gst_object_unref(q0_sink);
        gst_object_unref(q1_sink);

        gst_element_link(q0, m.sink0);
        gst_element_link(q1, m.sink1);

        m.bus = gst_pipeline_get_bus(GST_PIPELINE(m.pipe));
        return m;
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

    void play() {
        gst_element_set_state(pipe, GST_STATE_PLAYING);
    }

    void teardown() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(bus);
        gst_object_unref(pipe);
    }
};

GST_START_TEST(PL_C_buffer_routing) {
    MultiPipe m = MultiPipe::build();
    m.play();

    gst_app_src_push_buffer(GST_APP_SRC(m.src0),
        m.make_buf(100 * GST_MSECOND, 0));
    gst_app_src_push_buffer(GST_APP_SRC(m.src1),
        m.make_buf(200 * GST_MSECOND, 1));

    GstSample *s0 = gst_app_sink_try_pull_sample(GST_APP_SINK(m.sink0),
                                                    5 * GST_SECOND);
    fail_unless(s0 != nullptr, "stream 0 buffer must arrive at sink0");
    GstBuffer *buf0 = gst_sample_get_buffer(s0);
    DXFrameMeta *fm0 = dx_get_frame_meta(buf0);
    fail_unless(fm0 != nullptr, "output must have DXFrameMeta");
    fail_unless_equals_int(fm0->_stream_id, 0);
    gst_sample_unref(s0);

    gst_app_src_push_buffer(GST_APP_SRC(m.src0),
        m.make_buf(300 * GST_MSECOND, 0));

    GstSample *s1 = gst_app_sink_try_pull_sample(GST_APP_SINK(m.sink1),
                                                    5 * GST_SECOND);
    fail_unless(s1 != nullptr, "stream 1 buffer must arrive at sink1");
    GstBuffer *buf1 = gst_sample_get_buffer(s1);
    DXFrameMeta *fm1 = dx_get_frame_meta(buf1);
    fail_unless(fm1 != nullptr);
    fail_unless_equals_int(fm1->_stream_id, 1);
    gst_sample_unref(s1);

    m.teardown();
}
GST_END_TEST;

GST_START_TEST(PL_C_per_stream_eos) {
    MultiPipe m = MultiPipe::build();
    m.play();

    gst_app_src_push_buffer(GST_APP_SRC(m.src0),
        m.make_buf(100 * GST_MSECOND, 0));
    gst_app_src_push_buffer(GST_APP_SRC(m.src1),
        m.make_buf(200 * GST_MSECOND, 1));

    GstSample *s0 = gst_app_sink_try_pull_sample(GST_APP_SINK(m.sink0),
                                                    5 * GST_SECOND);
    if (s0) gst_sample_unref(s0);

    gst_app_src_end_of_stream(GST_APP_SRC(m.src0));

    g_usleep(500 * 1000);

    gst_app_src_push_buffer(GST_APP_SRC(m.src1),
        m.make_buf(400 * GST_MSECOND, 1));

    GstSample *s1 = gst_app_sink_try_pull_sample(GST_APP_SINK(m.sink1),
                                                    5 * GST_SECOND);
    fail_unless(s1 != nullptr,
                "stream1 must still receive data after stream0 EOS");
    gst_sample_unref(s1);

    gst_app_src_end_of_stream(GST_APP_SRC(m.src1));
    g_usleep(500 * 1000);

    m.teardown();
}
GST_END_TEST;

GST_START_TEST(PL_C_all_streams_eos) {
    MultiPipe m = MultiPipe::build();
    m.play();

    gst_app_src_push_buffer(GST_APP_SRC(m.src0),
        m.make_buf(100 * GST_MSECOND, 0));
    gst_app_src_push_buffer(GST_APP_SRC(m.src1),
        m.make_buf(200 * GST_MSECOND, 1));

    g_usleep(300 * 1000);

    gst_app_src_end_of_stream(GST_APP_SRC(m.src0));
    gst_app_src_end_of_stream(GST_APP_SRC(m.src1));

    gboolean got_eos = FALSE;
    GstMessage *msg;
    while ((msg = gst_bus_timed_pop_filtered(m.bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR))) != nullptr) {
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS)
            got_eos = TRUE;
        gst_message_unref(msg);
        break;
    }
    (void)got_eos;

    m.teardown();
}
GST_END_TEST;

GST_START_TEST(PL_C_lifecycle) {
    const char *desc =
        "videotestsrc num-buffers=5 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=30/1 "
        "! dxinputselector name=isel "
        "! dxoutputselector name=osel "
        "osel.src_0 ! queue ! appsink name=sink0 sync=false "
        "videotestsrc num-buffers=5 "
        "! video/x-raw,format=RGB,width=4,height=4,framerate=30/1 "
        "! isel.sink_1 ";

    test_lifecycle_cycle(desc, 3);
}
GST_END_TEST;

static Suite *pl_multistream_suite(void) {
    Suite *s = suite_create("pl_multistream");
    TCase *tc = tcase_create("multistream");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_C_buffer_routing);
    tcase_add_test(tc, PL_C_per_stream_eos);
    tcase_add_test(tc, PL_C_all_streams_eos);
    tcase_add_test(tc, PL_C_lifecycle);
    return s;
}

GST_CHECK_MAIN(pl_multistream);
