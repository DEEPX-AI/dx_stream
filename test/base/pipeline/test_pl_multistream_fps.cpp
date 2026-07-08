// Phase 5 — Multi-stream heterogeneous framerate routing verification
// A3: multiple streams with different framerates through inputselector/outputselector

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static GstBuffer *make_stream_buf(int stream_id, GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, (guint8)(stream_id * 50 + 80), map.size);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, 4, 4);
    return b;
}

struct MultiPipe {
    GstElement *pipe;
    GstElement *src[4];
    GstElement *isel, *osel;
    GstElement *sink[4];
    int n;

    static MultiPipe build(int n_streams) {
        MultiPipe m = {};
        m.n = n_streams;
        m.pipe = gst_pipeline_new(nullptr);
        m.isel = gst_element_factory_make("dxinputselector", "isel");
        m.osel = gst_element_factory_make("dxoutputselector", "osel");
        gst_bin_add_many(GST_BIN(m.pipe), m.isel, m.osel, nullptr);
        gst_element_link(m.isel, m.osel);

        GstCaps *caps = gst_caps_from_string(
            "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");

        for (int i = 0; i < n_streams; i++) {
            char name[32];
            snprintf(name, sizeof(name), "src%d", i);
            m.src[i] = gst_element_factory_make("appsrc", name);
            g_object_set(m.src[i], "format", GST_FORMAT_TIME,
                         "is-live", FALSE, "caps", caps, nullptr);
            gst_bin_add(GST_BIN(m.pipe), m.src[i]);

            char pad_name[32];
            snprintf(pad_name, sizeof(pad_name), "sink_%d", i);
            GstPad *req = gst_element_get_request_pad(m.isel, pad_name);
            GstPad *srcpad = gst_element_get_static_pad(m.src[i], "src");
            gst_pad_link(srcpad, req);
            gst_object_unref(srcpad);
            gst_object_unref(req);

            GstElement *q = gst_element_factory_make("queue", nullptr);
            snprintf(name, sizeof(name), "sink%d", i);
            m.sink[i] = gst_element_factory_make("appsink", name);
            g_object_set(m.sink[i], "sync", FALSE, "async", FALSE, nullptr);
            gst_bin_add_many(GST_BIN(m.pipe), q, m.sink[i], nullptr);

            snprintf(pad_name, sizeof(pad_name), "src_%d", i);
            GstPad *out_req = gst_element_get_request_pad(m.osel, pad_name);
            GstPad *q_sink = gst_element_get_static_pad(q, "sink");
            gst_pad_link(out_req, q_sink);
            gst_object_unref(q_sink);
            gst_object_unref(out_req);

            gst_element_link(q, m.sink[i]);
        }
        gst_caps_unref(caps);
        return m;
    }

    void start() {
        gst_element_set_state(pipe, GST_STATE_PLAYING);
    }

    void stop() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }

    void push(int s, GstBuffer *buf) {
        gst_app_src_push_buffer(GST_APP_SRC(src[s]), buf);
    }

    void push_eos(int s) {
        gst_app_src_end_of_stream(GST_APP_SRC(src[s]));
    }

    GstBuffer *pull(int s, GstClockTime timeout = 2 * GST_SECOND) {
        GstSample *sample = gst_app_sink_try_pull_sample(
            GST_APP_SINK(sink[s]), timeout);
        if (!sample) return nullptr;
        GstBuffer *buf = gst_buffer_ref(gst_sample_get_buffer(sample));
        gst_sample_unref(sample);
        return buf;
    }
};

// PL_multistream_2_routing: 2 streams route to correct sinks via stream_id
// Target: dxoutputselector routes by DXFrameMeta._stream_id
GST_START_TEST(PL_multistream_2_routing) {
    MultiPipe mp = MultiPipe::build(2);
    mp.start();

    for (int i = 0; i < 5; i++) {
        GstClockTime pts = i * GST_SECOND / 30;
        mp.push(0, make_stream_buf(0, pts));
        mp.push(1, make_stream_buf(1, pts));
    }
    mp.push_eos(0);
    mp.push_eos(1);

    int count0 = 0, count1 = 0;
    GstBuffer *b;
    while ((b = mp.pull(0, GST_SECOND)) != nullptr) {
        DXFrameMeta *fm = dx_get_frame_meta(b);
        if (fm) fail_unless_equals_int(fm->_stream_id, 0);
        count0++;
        gst_buffer_unref(b);
    }
    while ((b = mp.pull(1, GST_SECOND)) != nullptr) {
        DXFrameMeta *fm = dx_get_frame_meta(b);
        if (fm) fail_unless_equals_int(fm->_stream_id, 1);
        count1++;
        gst_buffer_unref(b);
    }

    fail_unless(count0 > 0, "stream 0 must produce output (got %d)", count0);
    fail_unless(count1 > 0, "stream 1 must produce output (got %d)", count1);

    mp.stop();
}
GST_END_TEST;

// PL_multistream_3_heterogeneous: 3 streams with different effective frame rates
// Target: dxinputselector PTS-ordered merging, dxoutputselector routing
GST_START_TEST(PL_multistream_3_heterogeneous) {
    MultiPipe mp = MultiPipe::build(3);
    mp.start();

    for (int i = 0; i < 10; i++) {
        GstClockTime pts = i * GST_SECOND / 30;
        mp.push(0, make_stream_buf(0, pts));
        if (i % 2 == 0)
            mp.push(1, make_stream_buf(1, pts));
        if (i % 3 == 0)
            mp.push(2, make_stream_buf(2, pts));
    }
    for (int s = 0; s < 3; s++)
        mp.push_eos(s);

    g_usleep(500000);

    int counts[3] = {0, 0, 0};
    for (int s = 0; s < 3; s++) {
        GstBuffer *b;
        while ((b = mp.pull(s, GST_SECOND)) != nullptr) {
            DXFrameMeta *fm = dx_get_frame_meta(b);
            if (fm) fail_unless_equals_int(fm->_stream_id, s);
            counts[s]++;
            gst_buffer_unref(b);
        }
    }

    fail_unless(counts[0] > 0, "stream 0 must produce output (%d)", counts[0]);
    fail_unless(counts[1] > 0, "stream 1 must produce output (%d)", counts[1]);
    fail_unless(counts[2] > 0, "stream 2 must produce output (%d)", counts[2]);
    fail_unless(counts[0] >= counts[1],
                "stream 0 (every frame) >= stream 1 (every 2nd): %d vs %d",
                counts[0], counts[1]);

    mp.stop();
}
GST_END_TEST;

// PL_multistream_per_stream_eos_continues: stream 0 EOS → stream 1 continues
// Target: dxinputselector per-stream EOS, not global EOS
GST_START_TEST(PL_multistream_per_stream_eos_continues) {
    MultiPipe mp = MultiPipe::build(2);
    mp.start();

    for (int i = 0; i < 5; i++) {
        GstClockTime pts = i * GST_SECOND / 30;
        mp.push(0, make_stream_buf(0, pts));
        mp.push(1, make_stream_buf(1, pts));
    }
    mp.push_eos(0);

    for (int i = 5; i < 15; i++) {
        GstClockTime pts = i * GST_SECOND / 30;
        mp.push(1, make_stream_buf(1, pts));
    }
    mp.push_eos(1);

    g_usleep(500000);

    int count1 = 0;
    GstBuffer *b;
    while ((b = mp.pull(1, GST_SECOND)) != nullptr) {
        count1++;
        gst_buffer_unref(b);
    }

    fail_unless(count1 >= 10,
                "stream 1 must continue after stream 0 EOS (got %d)", count1);

    mp.stop();
}
GST_END_TEST;

static Suite *pl_multistream_fps_suite(void) {
    Suite *s = suite_create("pl_multistream_fps");
    TCase *tc = tcase_create("heterogeneous_fps");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_multistream_2_routing);
    tcase_add_test(tc, PL_multistream_3_heterogeneous);
    tcase_add_test(tc, PL_multistream_per_stream_eos_continues);
    return s;
}

GST_CHECK_MAIN(pl_multistream_fps);
