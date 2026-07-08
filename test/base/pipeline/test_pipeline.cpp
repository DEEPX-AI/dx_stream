// P6 — Pipeline integration tests
// Core: build real pipelines using gst_parse_launch or C API.
// Only verify scenarios possible without inference models.
// Verify no bus ERROR + EOS reached + output buffer properties.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static std::string MSGCONV_LIB_PATH() {
    return dxtest::resolve_lib_path("libdx_msgconvl.so");
}
#define MSGCONV_LIB (MSGCONV_LIB_PATH().c_str())

struct PipeRun {
    GstElement *pipe;
    GstBus *bus;
    bool has_error;
    gchar *error_msg;

    static PipeRun from_string(const char *desc) {
        GError *err = nullptr;
        PipeRun r = {};
        r.pipe = gst_parse_launch(desc, &err);
        fail_unless(r.pipe != nullptr, "parse_launch failed: %s",
                    err ? err->message : "unknown");
        if (err) g_error_free(err);
        r.bus = gst_pipeline_get_bus(GST_PIPELINE(r.pipe));
        r.has_error = false;
        r.error_msg = nullptr;
        return r;
    }

    void play() {
        gst_element_set_state(pipe, GST_STATE_PLAYING);
    }

    bool wait_eos(GstClockTime timeout = 10 * GST_SECOND) {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, timeout,
            (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        if (!msg) return false;
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            has_error = true;
            GError *gerr = nullptr;
            gst_message_parse_error(msg, &gerr, nullptr);
            error_msg = g_strdup(gerr->message);
            g_error_free(gerr);
            gst_message_unref(msg);
            return false;
        }
        gst_message_unref(msg);
        return true;
    }

    GstElement *get(const char *name) {
        return gst_bin_get_by_name(GST_BIN(pipe), name);
    }

    GstSample *pull(const char *sink_name, GstClockTime t = 5 * GST_SECOND) {
        GstElement *s = get(sink_name);
        if (!s) return nullptr;
        GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(s), t);
        gst_object_unref(s);
        return sample;
    }

    void stop() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(bus);
        gst_object_unref(pipe);
        g_free(error_msg);
    }
};

static GstPadProbeReturn add_meta_probe(GstPad *, GstPadProbeInfo *info,
                                        gpointer ud) {
    int stream_id = GPOINTER_TO_INT(ud);
    GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
    buf = gst_buffer_make_writable(buf);
    GST_PAD_PROBE_INFO_DATA(info) = buf;
    if (!dx_get_frame_meta(buf)) {
        DXFrameMeta *fm = make_frame_meta(buf, stream_id, 64, 64);
        add_object_to_frame(fm, 1, 0.9f, 10, 10, 50, 50);
    }
    return GST_PAD_PROBE_OK;
}

// ---- Pipeline TCs ----

// PL_transform_chain: dxscale → dxconvert → dxrate transform chain
// Verify: EOS reached after resolution/format/framerate conversion, no errors
GST_START_TEST(PL_transform_chain) {
    PipeRun r = PipeRun::from_string(
        "videotestsrc num-buffers=10 "
        "! video/x-raw,format=I420,width=640,height=480,framerate=30/1 "
        "! dxscale width=320 height=240 "
        "! dxconvert "
        "! video/x-raw,format=RGB "
        "! dxrate framerate=15 "
        "! appsink name=sink sync=false");

    r.play();

    int count = 0;
    GstSample *s;
    while ((s = r.pull("sink")) != nullptr) {
        GstCaps *caps = gst_sample_get_caps(s);
        GstVideoInfo info;
        fail_unless(gst_video_info_from_caps(&info, caps));
        fail_unless_equals_int(GST_VIDEO_INFO_WIDTH(&info), 320);
        fail_unless_equals_int(GST_VIDEO_INFO_HEIGHT(&info), 240);
        fail_unless_equals_int(GST_VIDEO_INFO_FORMAT(&info), GST_VIDEO_FORMAT_RGB);
        count++;
        gst_sample_unref(s);
    }

    fail_unless(r.wait_eos(), "pipeline must reach EOS (error: %s)",
                r.error_msg ? r.error_msg : "timeout");
    fail_unless(!r.has_error, "no bus error expected (got: %s)",
                r.error_msg ? r.error_msg : "");
    fail_unless(count > 0, "must produce output buffers (got %d)", count);

    r.stop();
}
GST_END_TEST;

// PL_transform_meta_preserved: DXFrameMeta preserved through transform chain
// Verify: input meta propagated to output
GST_START_TEST(PL_transform_meta_preserved) {
    PipeRun r = PipeRun::from_string(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! identity name=adder "
        "! dxscale width=32 height=32 "
        "! appsink name=sink sync=false drop=true");

    GstElement *adder = r.get("adder");
    fail_unless(adder != nullptr, "identity 'adder' must exist");
    GstPad *adder_src = gst_element_get_static_pad(adder, "src");
    gst_pad_add_probe(adder_src, GST_PAD_PROBE_TYPE_BUFFER,
                      add_meta_probe, GINT_TO_POINTER(7), nullptr);
    gst_object_unref(adder_src);
    gst_object_unref(adder);

    r.play();

    int meta_count = 0;
    GstSample *s;
    while ((s = r.pull("sink")) != nullptr) {
        GstBuffer *out = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(out);
        if (fm) {
            fail_unless_equals_int(fm->_stream_id, 7);
            fail_unless(!fm->_object_meta_list.empty(),
                        "object meta list must not be empty");
            meta_count++;
        }
        gst_sample_unref(s);
    }

    fail_unless(r.wait_eos(), "pipeline must reach EOS (error: %s)",
                r.error_msg ? r.error_msg : "timeout");
    fail_unless(!r.has_error);
    fail_unless(meta_count > 0,
                "at least one buffer must have DXFrameMeta (got %d)", meta_count);

    r.stop();
}
GST_END_TEST;

// PL_multistream_routing: inputselector → outputselector routing
// Verify: 2 sources → inputselector merge → outputselector split → each sink receives data
GST_START_TEST(PL_multistream_routing) {
    GstElement *pipe = gst_pipeline_new(nullptr);

    GstElement *src0 = gst_element_factory_make("appsrc", "src0");
    GstElement *src1 = gst_element_factory_make("appsrc", "src1");
    GstElement *isel = gst_element_factory_make("dxinputselector", "isel");
    GstElement *osel = gst_element_factory_make("dxoutputselector", "osel");
    GstElement *q0 = gst_element_factory_make("queue", "q0");
    GstElement *q1 = gst_element_factory_make("queue", "q1");
    GstElement *sink0 = gst_element_factory_make("appsink", "sink0");
    GstElement *sink1 = gst_element_factory_make("appsink", "sink1");

    GstCaps *caps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");

    g_object_set(src0, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(src1, "format", GST_FORMAT_TIME, "is-live", FALSE,
                 "caps", caps, nullptr);
    g_object_set(sink0, "sync", FALSE, "async", FALSE, nullptr);
    g_object_set(sink1, "sync", FALSE, "async", FALSE, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(GST_BIN(pipe), src0, src1, isel, osel,
                     q0, q1, sink0, sink1, nullptr);

    GstPad *isel_sink0 = gst_element_get_request_pad(isel, "sink_0");
    GstPad *isel_sink1 = gst_element_get_request_pad(isel, "sink_1");
    GstPad *src0_src = gst_element_get_static_pad(src0, "src");
    GstPad *src1_src = gst_element_get_static_pad(src1, "src");
    gst_pad_link(src0_src, isel_sink0);
    gst_pad_link(src1_src, isel_sink1);
    gst_object_unref(src0_src);
    gst_object_unref(src1_src);
    gst_object_unref(isel_sink0);
    gst_object_unref(isel_sink1);

    gst_element_link(isel, osel);

    GstPad *osel_src0 = gst_element_get_request_pad(osel, "src_0");
    GstPad *osel_src1 = gst_element_get_request_pad(osel, "src_1");
    GstPad *q0_sink = gst_element_get_static_pad(q0, "sink");
    GstPad *q1_sink = gst_element_get_static_pad(q1, "sink");
    gst_pad_link(osel_src0, q0_sink);
    gst_pad_link(osel_src1, q1_sink);
    gst_object_unref(osel_src0);
    gst_object_unref(osel_src1);
    gst_object_unref(q0_sink);
    gst_object_unref(q1_sink);

    gst_element_link(q0, sink0);
    gst_element_link(q1, sink1);

    gst_element_set_state(pipe, GST_STATE_PLAYING);

    auto setup_osel_pad = [&](int id) {
        char pn[32], sid[32];
        snprintf(pn, sizeof(pn), "src_%d", id);
        snprintf(sid, sizeof(sid), "stream%d", id);
        GstPad *p = gst_element_get_static_pad(osel, pn);
        if (p) {
            gst_pad_push_event(p, gst_event_new_stream_start(sid));
            GstCaps *c = gst_caps_from_string(
                "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
            gst_pad_push_event(p, gst_event_new_caps(c));
            gst_caps_unref(c);
            GstSegment seg;
            gst_segment_init(&seg, GST_FORMAT_TIME);
            gst_pad_push_event(p, gst_event_new_segment(&seg));
            gst_object_unref(p);
        }
    };
    setup_osel_pad(0);
    setup_osel_pad(1);

    auto make_buf = [](GstClockTime pts) -> GstBuffer * {
        gsize sz = 4 * 4 * 3;
        GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
        GstMapInfo map;
        gst_buffer_map(b, &map, GST_MAP_WRITE);
        memset(map.data, 0x80, sz);
        gst_buffer_unmap(b, &map);
        GST_BUFFER_PTS(b) = pts;
        GST_BUFFER_DURATION(b) = GST_SECOND / 30;
        return b;
    };

    gst_app_src_push_buffer(GST_APP_SRC(src0), make_buf(100 * GST_MSECOND));
    gst_app_src_push_buffer(GST_APP_SRC(src1), make_buf(200 * GST_MSECOND));

    GstSample *s0 = gst_app_sink_try_pull_sample(GST_APP_SINK(sink0),
                                                  5 * GST_SECOND);
    fail_unless(s0 != nullptr, "stream_id=0 must arrive at sink0");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s0)),
                              100 * GST_MSECOND);
    gst_sample_unref(s0);

    gst_app_src_push_buffer(GST_APP_SRC(src0), make_buf(300 * GST_MSECOND));

    GstSample *s1 = gst_app_sink_try_pull_sample(GST_APP_SINK(sink1),
                                                  5 * GST_SECOND);
    fail_unless(s1 != nullptr, "stream_id=1 must arrive at sink1");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s1)),
                              200 * GST_MSECOND);
    gst_sample_unref(s1);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(pipe);
}
GST_END_TEST;

// PL_tracker_pipeline: videotestsrc + probe meta → dxtracker integration
// Verify: track_id assigned after processing multiple frames
GST_START_TEST(PL_tracker_pipeline) {
    PipeRun r = PipeRun::from_string(
        "videotestsrc num-buffers=10 "
        "! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 "
        "! identity name=adder "
        "! dxtracker "
        "! appsink name=sink sync=false");

    GstElement *adder = r.get("adder");
    fail_unless(adder != nullptr);
    GstPad *adder_src = gst_element_get_static_pad(adder, "src");
    gst_pad_add_probe(adder_src, GST_PAD_PROBE_TYPE_BUFFER,
                      add_meta_probe, GINT_TO_POINTER(0), nullptr);
    gst_object_unref(adder_src);
    gst_object_unref(adder);

    r.play();

    int count = 0;
    int tracked = 0;
    GstSample *s;
    while ((s = r.pull("sink")) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        DXFrameMeta *fm = dx_get_frame_meta(buf);
        if (fm && !fm->_object_meta_list.empty()) {
            for (auto *obj : fm->_object_meta_list) {
                if (obj->_track_id >= 0) tracked++;
            }
        }
        count++;
        gst_sample_unref(s);
    }

    fail_unless(r.wait_eos(), "tracker pipeline must reach EOS (error: %s)",
                r.error_msg ? r.error_msg : "timeout");
    fail_unless(!r.has_error);
    fail_unless(count >= 5, "must produce >=5 buffers (got %d)", count);
    fail_unless(tracked > 0,
                "tracker must assign track_id to at least one object");

    r.stop();
}
GST_END_TEST;

// PL_msgconv_pipeline: videotestsrc + probe meta → dxmsgconv integration
// Verify: DxMsgMeta payload attached
GST_START_TEST(PL_msgconv_pipeline) {
    char desc[1024];
    snprintf(desc, sizeof(desc),
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! identity name=adder "
        "! dxmsgconv library-file-path=%s "
        "! appsink name=sink sync=false",
        MSGCONV_LIB);

    PipeRun r = PipeRun::from_string(desc);

    GstElement *adder = r.get("adder");
    fail_unless(adder != nullptr);
    GstPad *adder_src = gst_element_get_static_pad(adder, "src");
    gst_pad_add_probe(adder_src, GST_PAD_PROBE_TYPE_BUFFER,
                      add_meta_probe, GINT_TO_POINTER(0), nullptr);
    gst_object_unref(adder_src);
    gst_object_unref(adder);

    r.play();

    int with_payload = 0;
    GstSample *s;
    while ((s = r.pull("sink")) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        GstMeta *meta = gst_buffer_get_meta(buf, gst_dxmsg_meta_api_get_type());
        if (meta) with_payload++;
        gst_sample_unref(s);
    }

    fail_unless(r.wait_eos(), "msgconv pipeline must reach EOS (error: %s)",
                r.error_msg ? r.error_msg : "timeout");
    fail_unless(!r.has_error);
    fail_unless(with_payload >= 2,
                "at least 2 buffers must have DxMsgMeta (got %d)", with_payload);

    r.stop();
}
GST_END_TEST;

// PL_scale_convert_caps: dxscale + dxconvert caps negotiation
// Verify: output caps converted as requested
GST_START_TEST(PL_scale_convert_caps) {
    PipeRun r = PipeRun::from_string(
        "videotestsrc num-buffers=3 "
        "! video/x-raw,format=I420,width=320,height=240,framerate=30/1 "
        "! dxscale width=160 height=120 "
        "! dxconvert "
        "! video/x-raw,format=BGR "
        "! appsink name=sink sync=false drop=true");

    r.play();

    int count = 0;
    GstSample *s;
    while ((s = r.pull("sink")) != nullptr) {
        GstCaps *caps = gst_sample_get_caps(s);
        GstVideoInfo info;
        fail_unless(gst_video_info_from_caps(&info, caps));
        fail_unless_equals_int(GST_VIDEO_INFO_WIDTH(&info), 160);
        fail_unless_equals_int(GST_VIDEO_INFO_HEIGHT(&info), 120);
        fail_unless_equals_int(GST_VIDEO_INFO_FORMAT(&info), GST_VIDEO_FORMAT_BGR);
        count++;
        gst_sample_unref(s);
    }

    fail_unless(r.wait_eos(), "pipeline must reach EOS (error: %s)",
                r.error_msg ? r.error_msg : "timeout");
    fail_unless(!r.has_error);
    fail_unless(count > 0, "must produce output buffers (got %d)", count);

    r.stop();
}
GST_END_TEST;

static Suite *pipeline_suite(void) {
    Suite *s = suite_create("pipeline");
    TCase *tc = tcase_create("integration");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_transform_chain);
    tcase_add_test(tc, PL_transform_meta_preserved);
    tcase_add_test(tc, PL_multistream_routing);
    tcase_add_test(tc, PL_tracker_pipeline);
    tcase_add_test(tc, PL_msgconv_pipeline);
    tcase_add_test(tc, PL_scale_convert_caps);
    return s;
}

GST_CHECK_MAIN(pipeline);
