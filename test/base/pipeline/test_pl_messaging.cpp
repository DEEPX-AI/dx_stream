// PL-H — dxmsgconv message conversion verification
// identity(probe) → dxmsgconv(libdx_msgconvl.so) → appsink
// DxMsgMeta payload attachment + message-interval behavior verification
// SKIP: when msgconv library is not present

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include "pipeline_tc_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <string>

using namespace dxtest;

static std::string MSGCONV_LIB_PATH() {
    return dxtest::resolve_lib_path("libdx_msgconvl.so");
}
#define MSGCONV_LIB (MSGCONV_LIB_PATH().c_str())

static GstPadProbeReturn inject_meta(GstPad *, GstPadProbeInfo *info, gpointer) {
    GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
    buf = gst_buffer_make_writable(buf);
    GST_PAD_PROBE_INFO_DATA(info) = buf;
    if (!dx_get_frame_meta(buf)) {
        DXFrameMeta *fm = make_frame_meta(buf, 0, 64, 64);
        add_object_to_frame(fm, 1, 0.9f, 10, 10, 50, 50);
    }
    return GST_PAD_PROBE_OK;
}

static std::string msgconv_pipe(int num_buffers, int interval = 1) {
    char desc[1024];
    snprintf(desc, sizeof(desc),
        "videotestsrc num-buffers=%d "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! identity name=adder "
        "! dxmsgconv library-file-path=%s message-interval=%d "
        "! appsink name=sink sync=false drop=true",
        num_buffers, MSGCONV_LIB, interval);
    return desc;
}

GST_START_TEST(PL_H_msgconv_attaches_payload) {
    DXTEST_SKIP_IF(!path_exists(MSGCONV_LIB), "msgconv library not found");

    std::string desc = msgconv_pipe(5);
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr, "parse_launch failed: %s",
                err ? err->message : "unknown");
    if (err) g_error_free(err);

    GstElement *adder = gst_bin_get_by_name(GST_BIN(pipe), "adder");
    fail_unless(adder != nullptr);
    GstPad *src_pad = gst_element_get_static_pad(adder, "src");
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER, inject_meta, nullptr, nullptr);
    gst_object_unref(src_pad);
    gst_object_unref(adder);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    int with_payload = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              3 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        GstMeta *meta = gst_buffer_get_meta(buf, gst_dxmsg_meta_api_get_type());
        if (meta) with_payload++;
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless(with_payload >= 3,
                "at least 3/5 buffers must have DxMsgMeta (got %d)", with_payload);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_H_msgconv_interval) {
    DXTEST_SKIP_IF(!path_exists(MSGCONV_LIB), "msgconv library not found");

    std::string desc = msgconv_pipe(12, 3);
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(desc.c_str(), &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    GstElement *adder = gst_bin_get_by_name(GST_BIN(pipe), "adder");
    fail_unless(adder != nullptr);
    GstPad *src_pad = gst_element_get_static_pad(adder, "src");
    gst_pad_add_probe(src_pad, GST_PAD_PROBE_TYPE_BUFFER, inject_meta, nullptr, nullptr);
    gst_object_unref(src_pad);
    gst_object_unref(adder);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipe), "sink");
    fail_unless(sink != nullptr);

    int total = 0;
    int with_payload = 0;
    GstSample *s;
    while ((s = gst_app_sink_try_pull_sample(GST_APP_SINK(sink),
                                              3 * GST_SECOND)) != nullptr) {
        GstBuffer *buf = gst_sample_get_buffer(s);
        total++;
        GstMeta *meta = gst_buffer_get_meta(buf, gst_dxmsg_meta_api_get_type());
        if (meta) with_payload++;
        gst_sample_unref(s);
    }
    gst_object_unref(sink);

    gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    fail_unless(with_payload < total,
                "interval=3: payload count (%d) must be less than total (%d)",
                with_payload, total);
    fail_unless(with_payload >= 2,
                "interval=3 with 12 buffers: at least 2 payloads (got %d)",
                with_payload);

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static std::string msgconv_pipe_noapp(int num_buffers) {
    char desc[1024];
    snprintf(desc, sizeof(desc),
        "videotestsrc num-buffers=%d "
        "! video/x-raw,format=RGB,width=64,height=64,framerate=30/1 "
        "! identity name=adder "
        "! dxmsgconv library-file-path=%s "
        "! fakesink sync=false",
        num_buffers, MSGCONV_LIB);
    return desc;
}

GST_START_TEST(PL_H_msgconv_eos) {
    DXTEST_SKIP_IF(!path_exists(MSGCONV_LIB), "msgconv library not found");
    test_eos_propagation(msgconv_pipe_noapp(5).c_str());
}
GST_END_TEST;

GST_START_TEST(PL_H_msgconv_lifecycle) {
    DXTEST_SKIP_IF(!path_exists(MSGCONV_LIB), "msgconv library not found");
    test_lifecycle_cycle(msgconv_pipe_noapp(3).c_str(), 3);
}
GST_END_TEST;

static Suite *pl_messaging_suite(void) {
    Suite *s = suite_create("pl_messaging");
    TCase *tc = tcase_create("msgconv");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_H_msgconv_attaches_payload);
    tcase_add_test(tc, PL_H_msgconv_interval);
    tcase_add_test(tc, PL_H_msgconv_eos);
    tcase_add_test(tc, PL_H_msgconv_lifecycle);
    return s;
}

GST_CHECK_MAIN(pl_messaging);
