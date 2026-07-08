// Phase 2 — Event propagation chain verification
// Verifies STREAM_START, CAPS, SEGMENT, EOS traverse the full pipeline.
// Probes at each element boundary count events.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "event_probe.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static const char *CHAIN_PIPE =
    "videotestsrc num-buffers=5 "
    "! video/x-raw,format=I420,width=64,height=64,framerate=30/1 "
    "! identity name=probe_src "
    "! dxscale width=32 height=32 "
    "! identity name=probe_mid "
    "! dxconvert "
    "! video/x-raw,format=RGB "
    "! identity name=probe_sink "
    "! fakesink sync=false";

GST_START_TEST(PL_event_chain_stream_start) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(CHAIN_PIPE, &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    EventTrace t_src, t_mid, t_sink;

    GstElement *e;
    GstPad *pad;

    e = gst_bin_get_by_name(GST_BIN(pipe), "probe_src");
    pad = gst_element_get_static_pad(e, "src");
    t_src.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    e = gst_bin_get_by_name(GST_BIN(pipe), "probe_mid");
    pad = gst_element_get_static_pad(e, "src");
    t_mid.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    e = gst_bin_get_by_name(GST_BIN(pipe), "probe_sink");
    pad = gst_element_get_static_pad(e, "src");
    t_sink.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout");
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS);
    gst_message_unref(msg);

    fail_unless(t_src.has(GST_EVENT_STREAM_START),
                "STREAM_START must appear at source output");
    fail_unless(t_mid.has(GST_EVENT_STREAM_START),
                "STREAM_START must traverse dxscale");
    fail_unless(t_sink.has(GST_EVENT_STREAM_START),
                "STREAM_START must traverse dxconvert");

    fail_unless(t_src.has(GST_EVENT_CAPS), "CAPS at source");
    fail_unless(t_mid.has(GST_EVENT_CAPS), "CAPS after dxscale");
    fail_unless(t_sink.has(GST_EVENT_CAPS), "CAPS after dxconvert");

    fail_unless(t_src.has(GST_EVENT_SEGMENT), "SEGMENT at source");
    fail_unless(t_mid.has(GST_EVENT_SEGMENT), "SEGMENT after dxscale");
    fail_unless(t_sink.has(GST_EVENT_SEGMENT), "SEGMENT after dxconvert");

    fail_unless(t_src.has(GST_EVENT_EOS), "EOS at source");
    fail_unless(t_mid.has(GST_EVENT_EOS), "EOS after dxscale");
    fail_unless(t_sink.has(GST_EVENT_EOS), "EOS after dxconvert");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_event_chain_ordering) {
    GError *err = nullptr;
    GstElement *pipe = gst_parse_launch(CHAIN_PIPE, &err);
    fail_unless(pipe != nullptr);
    if (err) g_error_free(err);

    EventTrace trace;
    GstElement *e = gst_bin_get_by_name(GST_BIN(pipe), "probe_sink");
    GstPad *pad = gst_element_get_static_pad(e, "src");
    trace.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 10 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) gst_message_unref(msg);

    fail_unless(trace.order_before(GST_EVENT_STREAM_START, GST_EVENT_CAPS),
                "STREAM_START must come before CAPS");
    fail_unless(trace.order_before(GST_EVENT_CAPS, GST_EVENT_SEGMENT),
                "CAPS must come before SEGMENT");
    fail_unless(trace.order_before(GST_EVENT_SEGMENT, GST_EVENT_EOS),
                "SEGMENT must come before EOS");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

GST_START_TEST(PL_event_chain_infer_pipeline) {
    DXTEST_SKIP_IF(!npu_available(), "NPU not available");
    std::string model = resolve_model_path("YoloV5S_PPU.dxnn");
    DXTEST_SKIP_IF(model.empty(), "model not found");
    std::string pp = dxtest::resolve_lib_path("libpostprocess_ppu.so");
    DXTEST_SKIP_IF(!path_exists(pp), "postprocess lib not found");
    std::string image = resolve_resource_path("images/test.jpg");
    DXTEST_SKIP_IF(image.empty(), "test image not found");

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "filesrc location=%s ! jpegdec ! videoconvert "
        "! video/x-raw,format=RGB "
        "! identity name=pre_infer "
        "! dxpreprocess resize-width=640 resize-height=640 keep-ratio=true pad-value=114 "
        "! dxinfer model-path=%s backend=dxrt "
        "! identity name=post_infer "
        "! dxpostprocess library-file-path=%s function-name=YOLOV5S_PPU "
        "! fakesink sync=false",
        image.c_str(), model.c_str(), pp.c_str());

    GError *gerr = nullptr;
    GstElement *pipe = gst_parse_launch(buf, &gerr);
    fail_unless(pipe != nullptr);
    if (gerr) g_error_free(gerr);

    EventTrace pre_trace, post_trace;

    GstElement *e;
    GstPad *pad;

    e = gst_bin_get_by_name(GST_BIN(pipe), "pre_infer");
    pad = gst_element_get_static_pad(e, "src");
    pre_trace.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    e = gst_bin_get_by_name(GST_BIN(pipe), "post_infer");
    pad = gst_element_get_static_pad(e, "src");
    post_trace.attach_downstream(pad);
    gst_object_unref(pad);
    gst_object_unref(e);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipe));
    gst_element_set_state(pipe, GST_STATE_PLAYING);

    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 30 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr, "timeout");
    fail_unless(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "expected EOS through inference pipeline");
    gst_message_unref(msg);

    fail_unless(pre_trace.has(GST_EVENT_STREAM_START));
    fail_unless(post_trace.has(GST_EVENT_STREAM_START),
                "STREAM_START must pass through dxinfer");
    fail_unless(post_trace.has(GST_EVENT_EOS),
                "EOS must pass through dxinfer");

    gst_element_set_state(pipe, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipe);
}
GST_END_TEST;

static Suite *pl_event_chain_suite(void) {
    Suite *s = suite_create("pl_event_chain");
    TCase *tc = tcase_create("event_propagation");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PL_event_chain_stream_start);
    tcase_add_test(tc, PL_event_chain_ordering);
    tcase_add_test(tc, PL_event_chain_infer_pipeline);
    return s;
}

GST_CHECK_MAIN(pl_event_chain);
