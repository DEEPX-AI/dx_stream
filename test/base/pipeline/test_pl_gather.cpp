// PL-F — dxgather merge correctness verification
// 2x appsrc → dxgather (sink_0, sink_1) → appsink
// Inject different object meta from each source → verify merge in output

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static GstBuffer *make_rgb_buffer(int w, int h, GstClockTime pts,
                                   int stream_id, int label, float conf) {
    gsize sz = w * h * 3;
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(buf, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(buf, &map);
    GST_BUFFER_PTS(buf) = pts;
    GST_BUFFER_DURATION(buf) = GST_SECOND / 30;

    DXFrameMeta *fm = make_frame_meta(buf, stream_id, w, h);
    add_object_to_frame(fm, label, conf, 10.0f, 10.0f, 50.0f, 50.0f);
    return buf;
}

GST_START_TEST(PL_F_gather_merges_objects) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src0 = gst_element_factory_make("appsrc", "src0");
    GstElement *src1 = gst_element_factory_make("appsrc", "src1");
    GstElement *agg = gst_element_factory_make("dxgather", "agg");
    GstElement *sink = gst_element_factory_make("appsink", "sink");

    fail_unless(src0 && src1 && agg && sink, "element creation failed");

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    g_object_set(src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(sink, "sync", FALSE, "drop", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src0, src1, agg, sink, nullptr);

    GstPad *agg_sink0 = gst_element_get_request_pad(agg, "sink_0");
    GstPad *agg_sink1 = gst_element_get_request_pad(agg, "sink_1");
    GstPad *src0_src = gst_element_get_static_pad(src0, "src");
    GstPad *src1_src = gst_element_get_static_pad(src1, "src");
    gst_pad_link(src0_src, agg_sink0);
    gst_pad_link(src1_src, agg_sink1);
    gst_object_unref(src0_src);
    gst_object_unref(src1_src);
    gst_object_unref(agg_sink0);
    gst_object_unref(agg_sink1);

    gst_element_link(agg, sink);
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstBuffer *b0 = make_rgb_buffer(64, 64, 100 * GST_MSECOND, 0, 1, 0.9f);
    GstBuffer *b1 = make_rgb_buffer(64, 64, 100 * GST_MSECOND, 0, 2, 0.8f);
    gst_app_src_push_buffer(GST_APP_SRC(src0), b0);
    gst_app_src_push_buffer(GST_APP_SRC(src1), b1);

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                  5 * GST_SECOND);
    fail_unless(s != nullptr, "gather must produce output");

    GstBuffer *out = gst_sample_get_buffer(s);
    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr, "output must have DXFrameMeta");
    fail_unless((int)fm->_object_meta_list.size() >= 2,
                "gather must merge objects from both sources (got %d)",
                (int)fm->_object_meta_list.size());
    gst_sample_unref(s);

    gst_app_src_end_of_stream(GST_APP_SRC(src0));
    gst_app_src_end_of_stream(GST_APP_SRC(src1));

    g_usleep(500 * 1000);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_F_gather_selects_latest_pts) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src0 = gst_element_factory_make("appsrc", "src0");
    GstElement *src1 = gst_element_factory_make("appsrc", "src1");
    GstElement *agg = gst_element_factory_make("dxgather", "agg");
    GstElement *sink = gst_element_factory_make("appsink", "sink");

    fail_unless(src0 && src1 && agg && sink);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    g_object_set(src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(sink, "sync", FALSE, "drop", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src0, src1, agg, sink, nullptr);

    GstPad *agg_sink0 = gst_element_get_request_pad(agg, "sink_0");
    GstPad *agg_sink1 = gst_element_get_request_pad(agg, "sink_1");
    GstPad *src0_src = gst_element_get_static_pad(src0, "src");
    GstPad *src1_src = gst_element_get_static_pad(src1, "src");
    gst_pad_link(src0_src, agg_sink0);
    gst_pad_link(src1_src, agg_sink1);
    gst_object_unref(src0_src);
    gst_object_unref(src1_src);
    gst_object_unref(agg_sink0);
    gst_object_unref(agg_sink1);

    gst_element_link(agg, sink);
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstClockTime early_pts = 100 * GST_MSECOND;
    GstClockTime late_pts = 200 * GST_MSECOND;
    GstBuffer *b0 = make_rgb_buffer(64, 64, early_pts, 0, 1, 0.9f);
    GstBuffer *b1 = make_rgb_buffer(64, 64, late_pts, 0, 2, 0.8f);
    gst_app_src_push_buffer(GST_APP_SRC(src0), b0);
    gst_app_src_push_buffer(GST_APP_SRC(src1), b1);

    GstSample *s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                                  5 * GST_SECOND);
    fail_unless(s != nullptr, "gather must produce output");
    GstClockTime out_pts = GST_BUFFER_PTS(gst_sample_get_buffer(s));
    fail_unless(out_pts == late_pts,
                "output PTS must be latest (expected %" G_GUINT64_FORMAT
                ", got %" G_GUINT64_FORMAT ")", late_pts, out_pts);
    gst_sample_unref(s);

    gst_app_src_end_of_stream(GST_APP_SRC(src0));
    gst_app_src_end_of_stream(GST_APP_SRC(src1));

    g_usleep(500 * 1000);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_F_gather_eos) {
    GstElement *pipe = gst_pipeline_new(nullptr);
    GstElement *src0 = gst_element_factory_make("appsrc", "src0");
    GstElement *src1 = gst_element_factory_make("appsrc", "src1");
    GstElement *agg = gst_element_factory_make("dxgather", "agg");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    fail_unless(src0 && src1 && agg && sink);

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=64,height=64,framerate=30/1");
    g_object_set(src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(sink, "sync", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src0, src1, agg, sink, nullptr);

    GstPad *agg_sink0 = gst_element_get_request_pad(agg, "sink_0");
    GstPad *agg_sink1 = gst_element_get_request_pad(agg, "sink_1");
    GstPad *src0_src = gst_element_get_static_pad(src0, "src");
    GstPad *src1_src = gst_element_get_static_pad(src1, "src");
    gst_pad_link(src0_src, agg_sink0);
    gst_pad_link(src1_src, agg_sink1);
    gst_object_unref(src0_src);
    gst_object_unref(src1_src);
    gst_object_unref(agg_sink0);
    gst_object_unref(agg_sink1);

    gst_element_link(agg, sink);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    gst_app_src_push_buffer(GST_APP_SRC(src0),
        make_rgb_buffer(64, 64, 100 * GST_MSECOND, 0, 1, 0.9f));
    gst_app_src_push_buffer(GST_APP_SRC(src1),
        make_rgb_buffer(64, 64, 100 * GST_MSECOND, 0, 2, 0.8f));

    g_usleep(200 * 1000);

    gst_app_src_end_of_stream(GST_APP_SRC(src0));
    gst_app_src_end_of_stream(GST_APP_SRC(src1));

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout waiting for EOS from gather");
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "expected EOS, got %s",
                gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
    gst_message_unref(msg);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_gather_suite(void) {
    Suite *s = suite_create("pl_gather");
    TCase *tc = tcase_create("gather");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_F_gather_merges_objects);
    tcase_add_test(tc, PL_F_gather_selects_latest_pts);
    tcase_add_test(tc, PL_F_gather_eos);
    return s;
}

GST_CHECK_MAIN(pl_gather);
