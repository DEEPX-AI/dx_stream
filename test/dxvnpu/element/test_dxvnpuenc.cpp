#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/check/gstcheck.h>
#include <dxvnpu/dxvnpu_c_api.h>

#include <cstring>

static gboolean require_device() {
    if (g_strcmp0(g_getenv("DXVNPU_TEST_DEVICE"), "1") == 0) return TRUE;
    g_test_skip("DXVNPU hardware test: set DXVNPU_TEST_DEVICE=1 on a configured device");
    return FALSE;
}

struct Collector {
    GMutex lock;
    GCond ready;
    guint samples;
    const gchar* output_caps;
};

static GstCaps* sink_template_caps() {
    GstElementFactory* factory = gst_element_factory_find("dxvnpuenc");
    fail_unless(factory != nullptr, "dxvnpuenc factory is not registered");
    const GList* templates = gst_element_factory_get_static_pad_templates(factory);
    for (const GList* item = templates; item; item = item->next) {
        auto* tmpl = static_cast<GstStaticPadTemplate*>(item->data);
        if (tmpl->direction == GST_PAD_SINK) {
            GstCaps* caps = gst_static_caps_get(&tmpl->static_caps);
            gst_object_unref(factory);
            return caps;
        }
    }
    gst_object_unref(factory);
    fail("dxvnpuenc lacks a sink pad template");
    return nullptr;
}

static void assert_caps(GstCaps* caps, const gchar* value, gboolean accepted) {
    GstCaps* candidate = gst_caps_from_string(value);
    const gboolean intersects = gst_caps_can_intersect(caps, candidate);
    gst_caps_unref(candidate);
    fail_unless(intersects == accepted, "%s must be %s", value,
                accepted ? "accepted" : "rejected");
}

static GstFlowReturn collect_sample(GstAppSink* sink, gpointer data) {
    auto* collector = static_cast<Collector*>(data);
    GstSample* sample = gst_app_sink_pull_sample(sink);
    if (!sample)
        return GST_FLOW_ERROR;
    GstCaps* caps = gst_sample_get_caps(sample);
    GstCaps* expected = gst_caps_from_string(collector->output_caps);
    fail_unless(caps && gst_caps_can_intersect(caps, expected));
    gst_caps_unref(expected);
    gst_sample_unref(sample);

    g_mutex_lock(&collector->lock);
    ++collector->samples;
    g_cond_signal(&collector->ready);
    g_mutex_unlock(&collector->lock);
    return GST_FLOW_OK;
}

static GstBuffer* nv12_frame() {
    constexpr gsize size = 1920 * 1080 * 3 / 2;
    GstBuffer* input = gst_buffer_new_allocate(nullptr, size, nullptr);
    GstMapInfo map;
    fail_unless(gst_buffer_map(input, &map, GST_MAP_WRITE));
    std::memset(map.data, 0x80, map.size);
    gst_buffer_unmap(input, &map);
    GST_BUFFER_DURATION(input) = GST_SECOND / 5;
    return input;
}

static void encode_before_eos(dxvnpu_codec_t codec, const gchar* output_caps,
                              gboolean flush = FALSE) {
    if (!require_device()) return;
    constexpr guint width = 1920;
    constexpr guint height = 1080;
    GstElement* pipeline = gst_pipeline_new(nullptr);
    GstElement* source = gst_element_factory_make("appsrc", nullptr);
    GstElement* encoder = gst_element_factory_make("dxvnpuenc", nullptr);
    GstElement* sink = gst_element_factory_make("appsink", nullptr);
    fail_unless(pipeline && source && encoder && sink);
    g_object_set(encoder, "codec", codec, nullptr);

    GstCaps* input_caps = gst_caps_from_string(
        "video/x-raw,format=NV12,width=1920,height=1080,framerate=5/1");
    g_object_set(source, "caps", input_caps, "format", GST_FORMAT_TIME,
                 "is-live", FALSE, "block", TRUE, nullptr);
    gst_caps_unref(input_caps);
    Collector collector = {};
    collector.output_caps = output_caps;
    g_mutex_init(&collector.lock);
    g_cond_init(&collector.ready);
    g_object_set(sink, "emit-signals", TRUE, "sync", FALSE, "max-buffers", 2u,
                 "drop", FALSE, nullptr);
    g_signal_connect(sink, "new-sample", G_CALLBACK(collect_sample), &collector);
    gst_bin_add_many(GST_BIN(pipeline), source, encoder, sink, nullptr);
    fail_unless(gst_element_link_many(source, encoder, sink, nullptr));
    fail_unless(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);

    GstBuffer* input = nv12_frame();
    GST_BUFFER_PTS(input) = 0;
    GST_BUFFER_DURATION(input) = GST_SECOND / 5;
    fail_unless_equals_int(gst_app_src_push_buffer(GST_APP_SRC(source), input), GST_FLOW_OK);

    g_mutex_lock(&collector.lock);
    const gint64 deadline = g_get_monotonic_time() + 2 * G_TIME_SPAN_SECOND;
    while (collector.samples == 0 && g_get_monotonic_time() < deadline)
        g_cond_wait_until(&collector.ready, &collector.lock, deadline);
    guint samples = collector.samples;
    g_mutex_unlock(&collector.lock);
    fail_unless(samples > 0, "encoder must push output before EOS");

    if (flush) {
        GstPad* srcpad = gst_element_get_static_pad(source, "src");
        fail_unless(gst_pad_push_event(srcpad, gst_event_new_flush_start()));
        fail_unless(gst_pad_push_event(srcpad, gst_event_new_flush_stop(TRUE)));
        gst_object_unref(srcpad);
        input = nv12_frame();
        GST_BUFFER_PTS(input) = GST_SECOND;
        fail_unless_equals_int(gst_app_src_push_buffer(GST_APP_SRC(source), input), GST_FLOW_OK);
        g_mutex_lock(&collector.lock);
        const gint64 second_deadline = g_get_monotonic_time() + 2 * G_TIME_SPAN_SECOND;
        while (collector.samples < 2 && g_get_monotonic_time() < second_deadline)
            g_cond_wait_until(&collector.ready, &collector.lock, second_deadline);
        samples = collector.samples;
        g_mutex_unlock(&collector.lock);
        fail_unless(samples >= 2, "encoder must resume output after flush");
    }

    fail_unless_equals_int(gst_app_src_end_of_stream(GST_APP_SRC(source)), GST_FLOW_OK);
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* message = gst_bus_timed_pop_filtered(
        bus, 5 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(message && GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS);
    gst_message_unref(message);
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_cond_clear(&collector.ready);
    g_mutex_clear(&collector.lock);
}

GST_START_TEST(CA1_h264_output_arrives_before_eos) {
    encode_before_eos(DXVNPU_CODEC_H264,
                      "video/x-h264,stream-format=byte-stream,alignment=au");
}
GST_END_TEST;

GST_START_TEST(CA0_host_input_caps_contract) {
    GstCaps* caps = sink_template_caps();
    assert_caps(caps, "video/x-raw,format=NV12,width=1920,height=1080", TRUE);
    assert_caps(caps, "video/x-raw(memory:DXVNPU),format=NV12,width=1920,height=1080", FALSE);
    gst_caps_unref(caps);
}
GST_END_TEST;

GST_START_TEST(CA3_h264_flush_resumes_output) {
    encode_before_eos(DXVNPU_CODEC_H264,
                      "video/x-h264,stream-format=byte-stream,alignment=au", TRUE);
}
GST_END_TEST;

GST_START_TEST(CA2_h265_output_arrives_before_eos) {
    encode_before_eos(DXVNPU_CODEC_H265,
                      "video/x-h265,stream-format=byte-stream,alignment=au");
}
GST_END_TEST;

static Suite* dxvnpuenc_suite() {
    Suite* suite = suite_create("dxvnpuenc");
    TCase* testcase = tcase_create("acceptance");
    tcase_add_test(testcase, CA0_host_input_caps_contract);
    tcase_add_test(testcase, CA1_h264_output_arrives_before_eos);
    tcase_add_test(testcase, CA2_h265_output_arrives_before_eos);
    tcase_add_test(testcase, CA3_h264_flush_resumes_output);
    suite_add_tcase(suite, testcase);
    return suite;
}

GST_CHECK_MAIN(dxvnpuenc);
