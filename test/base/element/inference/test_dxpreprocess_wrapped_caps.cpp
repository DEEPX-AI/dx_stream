// Phase 3 — dxpreprocess wrapped CAPS event handling
// B15: wrapped CAPS re-push → per-stream video_info registered, event forwarded
// Tests wrapped event path in multi-stream scenarios.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_RGB_64 =
    "video/x-raw,format=RGB,width=64,height=64,framerate=30/1";
static const guint RGB_64_SIZE = 64 * 64 * 3;

static GstHarness *make_preprocess_harness(guint rw, guint rh) {
    GstElement *e = gst_element_factory_make("dxpreprocess", nullptr);
    g_object_set(e, "resize-width", rw, "resize-height", rh, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    return h;
}

static GstEvent *make_wrapped_caps_event(int stream_id, const char *caps_str) {
    GstCaps *caps = gst_caps_from_string(caps_str);
    GstEvent *caps_event = gst_event_new_caps(caps);
    gst_caps_unref(caps);

    GstStructure *s = gst_structure_new("application/x-dx-wrapped-event",
        "stream-id", G_TYPE_INT, stream_id,
        "event", GST_TYPE_EVENT, caps_event,
        NULL);
    GstEvent *wrapped = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
    gst_event_unref(caps_event);
    return wrapped;
}

static GstBuffer *make_rgb_buf(int w, int h, GstClockTime pts, int stream_id) {
    gsize sz = w * h * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    make_frame_meta(b, stream_id, w, h, "RGB");
    return b;
}

// CE_preprocess_wrapped_caps_registers: wrapped CAPS for stream N → per-stream info stored
// Target: gst_dxpreprocess_sink_event L472-478 (unwrap → set_input_info)
// MUT: remove L477-478 → _stream.info empty → GstSrcFrame uses null vinfo_ptr
GST_START_TEST(CE_preprocess_wrapped_caps_registers) {
    GstHarness *h = make_preprocess_harness(32, 32);
    gst_harness_set_src_caps_str(h, CAPS_RGB_64);

    GstEvent *wrapped = make_wrapped_caps_event(5, CAPS_RGB_64);
    gboolean pushed = gst_harness_push_event(h, wrapped);
    fail_unless(pushed, "wrapped CAPS event must be accepted");

    GstBuffer *b = make_rgb_buf(64, 64, 0, 5);
    GstFlowReturn r = gst_harness_push(h, b);
    fail_unless(r == GST_FLOW_OK,
                "buffer for wrapped stream must process (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out = gst_harness_try_pull(h);
    fail_unless(out != nullptr, "must produce output after wrapped CAPS + buffer");

    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr, "output must have DXFrameMeta");
    fail_unless_equals_int(fm->_stream_id, 5);

    gst_buffer_unref(out);
    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_wrapped_caps_forwarded: wrapped event passes through to downstream
// Target: gst_dxpreprocess_sink_event L492 (gst_pad_push_event)
// MUT: remove L492 → downstream never sees wrapped event
GST_START_TEST(CE_preprocess_wrapped_caps_forwarded) {
    GstHarness *h = make_preprocess_harness(32, 32);
    gst_harness_set_src_caps_str(h, CAPS_RGB_64);

    EventCounter ec = {};
    GstPad *srcpad = gst_element_get_static_pad(h->element, "src");
    attach_event_counter(srcpad, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(srcpad);

    GstEvent *wrapped = make_wrapped_caps_event(3, CAPS_RGB_64);
    gst_harness_push_event(h, wrapped);

    fail_unless_equals_int(ec.n_wrapped, 1);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_wrapped_non_caps_forwarded: wrapped non-CAPS event → forwarded unchanged
// Target: gst_dxpreprocess_sink_event L477 (only CAPS type triggers set_input_info)
GST_START_TEST(CE_preprocess_wrapped_non_caps_forwarded) {
    GstHarness *h = make_preprocess_harness(32, 32);
    gst_harness_set_src_caps_str(h, CAPS_RGB_64);

    EventCounter ec = {};
    GstPad *srcpad = gst_element_get_static_pad(h->element, "src");
    attach_event_counter(srcpad, &ec, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM);
    gst_object_unref(srcpad);

    GstEvent *eos_event = gst_event_new_eos();
    GstStructure *s = gst_structure_new("application/x-dx-wrapped-event",
        "stream-id", G_TYPE_INT, 7,
        "event", GST_TYPE_EVENT, eos_event,
        NULL);
    GstEvent *wrapped = gst_event_new_custom(GST_EVENT_CUSTOM_DOWNSTREAM, s);
    gst_event_unref(eos_event);

    gst_harness_push_event(h, wrapped);

    fail_unless_equals_int(ec.n_wrapped, 1);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_flush_clears_stream_info: FLUSH_STOP → _stream.info cleared
// Target: gst_dxpreprocess_sink_event L466-468 (FLUSH_STOP → info.clear())
// MUT: remove L467 → stale stream info from previous segment
GST_START_TEST(CE_preprocess_flush_clears_stream_info) {
    GstHarness *h = make_preprocess_harness(32, 32);
    gst_harness_set_src_caps_str(h, CAPS_RGB_64);

    GstEvent *wrapped = make_wrapped_caps_event(5, CAPS_RGB_64);
    gst_harness_push_event(h, wrapped);

    GstBuffer *b1 = make_rgb_buf(64, 64, 0, 5);
    gst_harness_push(h, b1);
    GstBuffer *out1 = gst_harness_try_pull(h);
    fail_unless(out1 != nullptr);
    gst_buffer_unref(out1);

    push_flush(h);

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_harness_push_event(h, gst_event_new_segment(&seg));

    wrapped = make_wrapped_caps_event(5,
        "video/x-raw,format=RGB,width=128,height=128,framerate=30/1");
    gst_harness_push_event(h, wrapped);

    GstBuffer *b2 = make_rgb_buf(64, 64, GST_SECOND, 5);
    GstFlowReturn r = gst_harness_push(h, b2);
    fail_unless(r == GST_FLOW_OK,
                "buffer after flush + re-register must work (got %s)",
                gst_flow_get_name(r));

    GstBuffer *out2 = gst_harness_try_pull(h);
    fail_unless(out2 != nullptr, "must produce output after flush + re-register");
    gst_buffer_unref(out2);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_preprocess_multiple_streams: different stream_ids registered independently
// Target: set_input_info L451 (per-stream registration)
GST_START_TEST(CE_preprocess_multiple_streams) {
    GstHarness *h = make_preprocess_harness(32, 32);
    gst_harness_set_src_caps_str(h, CAPS_RGB_64);

    gst_harness_push_event(h, make_wrapped_caps_event(0, CAPS_RGB_64));
    gst_harness_push_event(h, make_wrapped_caps_event(1, CAPS_RGB_64));
    gst_harness_push_event(h, make_wrapped_caps_event(2, CAPS_RGB_64));

    for (int sid = 0; sid < 3; sid++) {
        GstBuffer *b = make_rgb_buf(64, 64, sid * GST_SECOND / 30, sid);
        GstFlowReturn r = gst_harness_push(h, b);
        fail_unless(r == GST_FLOW_OK,
                    "stream %d buffer must process (got %s)",
                    sid, gst_flow_get_name(r));

        GstBuffer *out = gst_harness_try_pull(h);
        fail_unless(out != nullptr, "stream %d must produce output", sid);

        DXFrameMeta *fm = dx_get_frame_meta(out);
        fail_unless(fm != nullptr);
        fail_unless_equals_int(fm->_stream_id, sid);
        gst_buffer_unref(out);
    }

    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *dxpreprocess_wrapped_suite(void) {
    Suite *s = suite_create("dxpreprocess_wrapped");
    TCase *tc = tcase_create("wrapped_caps");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_preprocess_wrapped_caps_registers);
    tcase_add_test(tc, CE_preprocess_wrapped_caps_forwarded);
    tcase_add_test(tc, CE_preprocess_wrapped_non_caps_forwarded);
    tcase_add_test(tc, CE_preprocess_flush_clears_stream_info);
    tcase_add_test(tc, CE_preprocess_multiple_streams);
    return s;
}

GST_CHECK_MAIN(dxpreprocess_wrapped);
