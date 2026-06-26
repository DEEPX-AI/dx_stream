// Phase 2 — FLUSH state reset verification
// Sends FLUSH_START/FLUSH_STOP and verifies stateful elements reset.
// Key areas: dxrate prevbuf, dxpreprocess interval counter, dxtracker trackers.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static GstBuffer *make_rgb_buf(int w, int h, GstClockTime pts, int stream_id) {
    gsize sz = w * h * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, w, h);
    return b;
}

GST_START_TEST(PL_flush_dxrate_prevbuf_reset) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src = gst_element_factory_make("appsrc", "src");
    GstElement *rate = gst_element_factory_make("dxrate", "rate");
    GstElement *sink = gst_element_factory_make("appsink", "sink");
    fail_unless(src && rate && sink);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=16,height=16,framerate=30/1");
    g_object_set(src, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(rate, "framerate", (guint)15, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src, rate, sink, nullptr);
    gst_element_link_many(src, rate, sink, nullptr);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    for (int i = 0; i < 5; i++) {
        gst_app_src_push_buffer(GST_APP_SRC(src),
            make_rgb_buf(16, 16, i * GST_SECOND / 30, 0));
    }

    g_usleep(200 * 1000);

    GstPad *rate_sink = gst_element_get_static_pad(rate, "sink");
    gst_pad_send_event(rate_sink, gst_event_new_flush_start());
    gst_pad_send_event(rate_sink, gst_event_new_flush_stop(TRUE));
    gst_object_unref(rate_sink);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    GstPad *src_pad = gst_element_get_static_pad(src, "src");
    gst_pad_push_event(src_pad, gst_event_new_segment(&seg));
    gst_object_unref(src_pad);

    GstClockTime new_pts_base = 10 * GST_SECOND;
    gst_app_src_push_buffer(GST_APP_SRC(src),
        make_rgb_buf(16, 16, new_pts_base, 0));
    gst_app_src_push_buffer(GST_APP_SRC(src),
        make_rgb_buf(16, 16, new_pts_base + GST_SECOND / 30, 0));

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                  5 * GST_SECOND);
    fail_unless(s != nullptr,
                "dxrate must produce output after flush+new segment");
    GstBuffer *buf = gst_sample_get_buffer(s);
    GstClockTime out_pts = GST_BUFFER_PTS(buf);
    fail_unless(out_pts >= new_pts_base,
                "output PTS must be from new segment, not pre-flush");
    gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_flush_dxtracker_clear) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src = gst_element_factory_make("appsrc", "src");
    GstElement *tracker = gst_element_factory_make("dxtracker", "tracker");
    GstElement *sink = gst_element_factory_make("appsink", "sink");
    fail_unless(src && tracker && sink);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    g_object_set(src, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src, tracker, sink, nullptr);
    gst_element_link_many(src, tracker, sink, nullptr);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    for (int i = 0; i < 3; i++) {
        GstBuffer *b = make_rgb_buf(64, 64, i * GST_SECOND / 30, 0);
        DXFrameMeta *fm = dx_get_frame_meta(b);
        add_object_to_frame(fm, 1, 0.9f, 10, 10, 50, 50);
        gst_app_src_push_buffer(GST_APP_SRC(src), b);
    }

    g_usleep(200 * 1000);

    GstPad *trk_sink = gst_element_get_static_pad(tracker, "sink");
    gst_pad_send_event(trk_sink, gst_event_new_flush_start());
    gst_pad_send_event(trk_sink, gst_event_new_flush_stop(TRUE));
    gst_object_unref(trk_sink);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    GstPad *src_pad = gst_element_get_static_pad(src, "src");
    gst_pad_push_event(src_pad, gst_event_new_segment(&seg));
    gst_object_unref(src_pad);

    GstBuffer *b = make_rgb_buf(64, 64, 0, 0);
    DXFrameMeta *fm = dx_get_frame_meta(b);
    add_object_to_frame(fm, 1, 0.9f, 10, 10, 50, 50);
    gst_app_src_push_buffer(GST_APP_SRC(src), b);

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                  5 * GST_SECOND);
    fail_unless(s != nullptr, "tracker must pass buffer after flush");
    GstBuffer *out = gst_sample_get_buffer(s);
    DXFrameMeta *out_fm = dx_get_frame_meta(out);
    fail_unless(out_fm != nullptr);
    if (!out_fm->_object_meta_list.empty()) {
        int track_id = out_fm->_object_meta_list[0]->_track_id;
        // OCSort: id_count starts at 0, first ++id_count=1, output = id+1 = 2
        fail_unless(track_id == 2,
                    "first track_id after flush must be 2 (OCSort restarts id_count=0, got %d)",
                    track_id);
    }
    gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_flush_transform_chain) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src = gst_element_factory_make("appsrc", "src");
    GstElement *scale = gst_element_factory_make("dxscale", "scale");
    GstElement *conv = gst_element_factory_make("dxconvert", "conv");
    GstElement *sink = gst_element_factory_make("appsink", "sink");
    fail_unless(src && scale && conv && sink);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=I420,width=64,height=64,framerate=30/1");
    g_object_set(src, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(scale, "width", 32, "height", 32, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src, scale, conv, sink, nullptr);
    gst_element_link_many(src, scale, conv, sink, nullptr);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    gst_app_src_push_buffer(GST_APP_SRC(src),
        make_rgb_buf(64, 64, 0, 0));
    g_usleep(100 * 1000);

    GstPad *scale_sink = gst_element_get_static_pad(scale, "sink");
    gst_pad_send_event(scale_sink, gst_event_new_flush_start());
    gst_pad_send_event(scale_sink, gst_event_new_flush_stop(TRUE));
    gst_object_unref(scale_sink);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    GstPad *src_pad = gst_element_get_static_pad(src, "src");
    gst_pad_push_event(src_pad, gst_event_new_segment(&seg));
    gst_object_unref(src_pad);

    gst_app_src_push_buffer(GST_APP_SRC(src),
        make_rgb_buf(64, 64, GST_SECOND, 0));

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                  5 * GST_SECOND);
    fail_unless(s != nullptr,
                "transform chain must work after flush");
    gst_sample_unref(s);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_flush_suite(void) {
    Suite *s = suite_create("pl_flush");
    TCase *tc = tcase_create("flush_reset");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_flush_dxrate_prevbuf_reset);
    tcase_add_test(tc, PL_flush_dxtracker_clear);
    tcase_add_test(tc, PL_flush_transform_chain);
    return s;
}

GST_CHECK_MAIN(pl_flush);
