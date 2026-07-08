// P4.1 — dxinputselector contract tests
// Core: GstAggregator-based N:1 multistream merging.
// clip() drops invalid PTS + auto-attaches DXFrameMeta (stream_id = pad_index).
// aggregate() selects min-PTS, manages per-stream wrapped EOS.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <cstring>
#include <vector>
#include <algorithm>

using namespace dxtest;

static const char *CAPS_RGB_4 =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static GstBuffer *make_buf(GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

struct AggPipe {
    GstElement *pipe, *agg, *sink;
    GstElement *src[4];
    int n;

    void start() { gst_element_set_state(pipe, GST_STATE_PLAYING); }

    void push(int i, GstBuffer *b) {
        gst_app_src_push_buffer(GST_APP_SRC(src[i]), b);
    }

    void eos(int i) {
        gst_app_src_end_of_stream(GST_APP_SRC(src[i]));
    }

    GstSample *pull(GstClockTime t = 5 * GST_SECOND) {
        return gst_app_sink_try_pull_sample(GST_APP_SINK(sink), t);
    }

    void stop() {
        gst_element_set_state(pipe, GST_STATE_NULL);
        gst_object_unref(pipe);
    }
};

static AggPipe make_agg_pipe(const char *elem, int n_src,
                             const char *caps_str = CAPS_RGB_4) {
    AggPipe p = {};
    p.n = n_src;
    p.pipe = gst_pipeline_new(nullptr);
    p.agg = gst_element_factory_make(elem, "agg");
    p.sink = gst_element_factory_make("appsink", "sink");
    g_object_set(p.sink, "sync", FALSE, nullptr);
    gst_bin_add_many(GST_BIN(p.pipe), p.agg, p.sink, nullptr);
    gst_element_link(p.agg, p.sink);

    GstCaps *caps = gst_caps_from_string(caps_str);
    for (int i = 0; i < n_src; i++) {
        char name[32];
        snprintf(name, sizeof(name), "src%d", i);
        p.src[i] = gst_element_factory_make("appsrc", name);
        g_object_set(p.src[i], "format", GST_FORMAT_TIME,
                     "is-live", FALSE, "caps", caps, nullptr);
        gst_bin_add(GST_BIN(p.pipe), p.src[i]);

        snprintf(name, sizeof(name), "sink_%d", i);
        GstPad *req = gst_element_get_request_pad(p.agg, name);
        fail_unless(req != nullptr, "request pad %s failed", name);
        GstPad *srcpad = gst_element_get_static_pad(p.src[i], "src");
        fail_unless(gst_pad_link(srcpad, req) == GST_PAD_LINK_OK);
        gst_object_unref(srcpad);
        gst_object_unref(req);
    }
    gst_caps_unref(caps);
    return p;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxinputselector", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxinputselector", nullptr);
    guint mqs = 999;
    g_object_get(e, "max-queue-size", &mqs, nullptr);
    fail_unless_equals_int(mqs, 2);

    g_object_set(e, "max-queue-size", 20u, nullptr);
    g_object_get(e, "max-queue-size", &mqs, nullptr);
    fail_unless_equals_int(mqs, 20);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxinputselector", nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_inputsel_src_domain_caps: src pad template + negotiated caps must be application/x-dxvideoraw.
// Target: gst-dxinputselector pad template + caps negotiation in change_state/PAUSED.
// MUT: changing src template back to video/x-raw makes domain boundary invisible.
GST_START_TEST(CE_inputsel_src_domain_caps) {
    GstElement *e = gst_element_factory_make("dxinputselector", nullptr);
    GstPadTemplate *src_t = gst_element_class_get_pad_template(
        GST_ELEMENT_GET_CLASS(e), "src");
    fail_unless(src_t != nullptr);
    GstCaps *tpl = gst_pad_template_get_caps(src_t);
    GstCaps *domain = gst_caps_from_string("application/x-dxvideoraw");
    fail_unless(gst_caps_can_intersect(tpl, domain),
                "src template must advertise application/x-dxvideoraw");
    gst_caps_unref(domain);

    // src template must NOT accept plain video/x-raw — boundary type.
    GstCaps *plain = gst_caps_from_string("video/x-raw,format=RGB");
    fail_if(gst_caps_can_intersect(tpl, plain),
            "src template must NOT accept video/x-raw");
    gst_caps_unref(plain);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_inputsel_wrapped_caps_stream_id: wrapped CAPS stream-id == sink pad index.
// Target: sink_event CAPS wrap: stream-id = get_sink_pad_index(pad).
// MUT: hardcode stream-id=0 → only pad 0 succeeds; pad 1/2 fail this TC.
GST_START_TEST(CE_inputsel_wrapped_caps_stream_id) {
    AggPipe p = make_agg_pipe("dxinputselector", 3);

    GstPad *srcpad = gst_element_get_static_pad(p.agg, "src");
    struct { std::vector<gint> caps_sids; } t = {};

    gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        [](GstPad *, GstPadProbeInfo *info, gpointer ud) -> GstPadProbeReturn {
            auto *u = static_cast<decltype(&t)>(ud);
            GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
            if (GST_EVENT_TYPE(ev) != GST_EVENT_CUSTOM_DOWNSTREAM)
                return GST_PAD_PROBE_OK;
            const GstStructure *s = gst_event_get_structure(ev);
            if (!s || !gst_structure_has_name(s, "application/x-dx-wrapped-event"))
                return GST_PAD_PROBE_OK;
            gint sid = -1;
            GstEvent *inner = nullptr;
            gst_structure_get_int(s, "stream-id", &sid);
            gst_structure_get(s, "event", GST_TYPE_EVENT, &inner, NULL);
            if (inner && GST_EVENT_TYPE(inner) == GST_EVENT_CAPS)
                u->caps_sids.push_back(sid);
            if (inner) gst_event_unref(inner);
            return GST_PAD_PROBE_OK;
        }, &t, nullptr);
    gst_object_unref(srcpad);

    p.start();
    for (int i = 0; i < 3; i++) p.push(i, make_buf(100 * GST_MSECOND * (i + 1)));
    GstSample *s = p.pull();
    if (s) gst_sample_unref(s);
    g_usleep(100000);

    fail_unless(t.caps_sids.size() == 3,
                "expected 3 wrapped CAPS (got %lu)",
                (unsigned long)t.caps_sids.size());
    std::sort(t.caps_sids.begin(), t.caps_sids.end());
    fail_unless_equals_int(t.caps_sids[0], 0);
    fail_unless_equals_int(t.caps_sids[1], 1);
    fail_unless_equals_int(t.caps_sids[2], 2);

    for (int i = 0; i < 3; i++) p.eos(i);
    p.stop();
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_inputsel_invalid_pts_drop: PTS=NONE → dropped in clip()
// Target: gst_dxinputselector_clip L70-73
// MUT: remove L70-73 → PTS_NONE buffer enters queue → aggregate hangs
GST_START_TEST(CE_inputsel_invalid_pts_drop) {
    AggPipe p = make_agg_pipe("dxinputselector", 1);
    p.start();

    p.push(0, make_buf(GST_CLOCK_TIME_NONE));
    p.push(0, make_buf(100 * GST_MSECOND));
    p.eos(0);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "valid-PTS buffer must arrive");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              100 * GST_MSECOND);
    gst_sample_unref(s);

    s = p.pull(500 * GST_MSECOND);
    fail_unless(s == nullptr, "PTS_NONE buffer must be dropped");

    p.stop();
}
GST_END_TEST;

// CE_inputsel_auto_frame_meta: buffer without meta → auto-created in clip()
// Target: gst_dxinputselector_clip L75-97 (L94: _stream_id = pad_index)
// MUT: remove L75 → duplicate meta. Remove L94 → stream_id not set
GST_START_TEST(CE_inputsel_auto_frame_meta) {
    AggPipe p = make_agg_pipe("dxinputselector", 1);
    p.start();

    p.push(0, make_buf(0));
    p.eos(0);

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "auto-meta buffer must arrive");
    GstBuffer *out = gst_sample_get_buffer(s);

    DXFrameMeta *fm = dx_get_frame_meta(out);
    fail_unless(fm != nullptr, "clip() must auto-create DXFrameMeta");
    fail_unless_equals_int(fm->_stream_id, 0);
    fail_unless_equals_int(fm->_width, 4);
    fail_unless_equals_int(fm->_height, 4);
    fail_unless_equals_string(fm->_format.c_str(), "RGB");
    fail_unless(fm->_frame_rate > 29.0f && fm->_frame_rate < 31.0f,
                "frame_rate must be ~30 (got %f)", fm->_frame_rate);

    gst_sample_unref(s);
    p.stop();
}
GST_END_TEST;

// CE_inputsel_min_pts_order: min PTS output first from 2 streams
// Target: gst_dxinputselector_aggregate L187-194
// MUT: replace min logic with peeked[0] → reverse PTS order
GST_START_TEST(CE_inputsel_min_pts_order) {
    AggPipe p = make_agg_pipe("dxinputselector", 2);
    p.start();

    // src0=200ms, src1=100ms → first output is 100ms (min)
    p.push(0, make_buf(200 * GST_MSECOND));
    p.push(1, make_buf(100 * GST_MSECOND));

    GstSample *s = p.pull();
    fail_unless(s != nullptr, "first output must arrive");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              100 * GST_MSECOND);
    gst_sample_unref(s);

    // src0 still has 200ms. Push 300ms to src1 → output 200ms (min of 200,300)
    p.push(1, make_buf(300 * GST_MSECOND));

    s = p.pull();
    fail_unless(s != nullptr, "second output must arrive");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              200 * GST_MSECOND);
    gst_sample_unref(s);

    // src1 has 300ms. Push 400ms to src0 → output 300ms
    p.push(0, make_buf(400 * GST_MSECOND));

    s = p.pull();
    fail_unless(s != nullptr, "third output must arrive");
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              300 * GST_MSECOND);
    gst_sample_unref(s);

    p.eos(0);
    p.eos(1);
    p.stop();
}
GST_END_TEST;

// CE_inputsel_per_stream_wrapped_eos: one stream EOS → wrapped EOS, all → global
// Target: aggregate L155-163, L178-180 (send_wrapped_eos)
// MUT: remove L160 count check → duplicate wrapped EOS sent
GST_START_TEST(CE_inputsel_per_stream_wrapped_eos) {
    AggPipe p = make_agg_pipe("dxinputselector", 2);

    GstPad *srcpad = gst_element_get_static_pad(p.agg, "src");
    struct { std::vector<int> eos_sids; int global_eos; } tracker = {{}, 0};

    gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
        [](GstPad *, GstPadProbeInfo *info, gpointer ud) -> GstPadProbeReturn {
            auto *t = static_cast<decltype(&tracker)>(ud);
            GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
            if (GST_EVENT_TYPE(ev) == GST_EVENT_EOS) {
                t->global_eos++;
            } else if (GST_EVENT_TYPE(ev) == GST_EVENT_CUSTOM_DOWNSTREAM) {
                const GstStructure *s = gst_event_get_structure(ev);
                if (s && gst_structure_has_name(s, "application/x-dx-wrapped-event")) {
                    gint sid = -1;
                    GstEvent *inner = nullptr;
                    gst_structure_get_int(s, "stream-id", &sid);
                    gst_structure_get(s, "event", GST_TYPE_EVENT, &inner, NULL);
                    if (inner && GST_EVENT_TYPE(inner) == GST_EVENT_EOS)
                        t->eos_sids.push_back(sid);
                    if (inner) gst_event_unref(inner);
                }
            }
            return GST_PAD_PROBE_OK;
        }, &tracker, nullptr);
    gst_object_unref(srcpad);

    p.start();

    // pad0=100ms, pad1=200ms → min=100ms → pad0 popped
    p.push(0, make_buf(100 * GST_MSECOND));
    p.push(1, make_buf(200 * GST_MSECOND));
    GstSample *s = p.pull();
    fail_unless(s != nullptr);
    fail_unless_equals_uint64(GST_BUFFER_PTS(gst_sample_get_buffer(s)),
                              100 * GST_MSECOND);
    gst_sample_unref(s);

    // pad0 empty, pad1 has 200ms. EOS pad0.
    // aggregate: pad0 EOS(no buf) → wrapped EOS(stream=0). pad1 has 200ms → pop → output
    p.eos(0);
    s = p.pull();
    fail_unless(s != nullptr);
    gst_sample_unref(s);

    g_usleep(50000);
    fail_unless(tracker.eos_sids.size() >= 1,
                "must send wrapped EOS for stream 0 (got %lu)",
                (unsigned long)tracker.eos_sids.size());
    fail_unless_equals_int(tracker.eos_sids[0], 0);
    fail_unless_equals_int(tracker.global_eos, 0);

    // pad1 EOS → all done → global EOS
    p.eos(1);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(p.pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    fail_unless(msg != nullptr && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS,
                "all streams EOS must produce global EOS");
    gst_message_unref(msg);
    gst_object_unref(bus);

    p.stop();
}
GST_END_TEST;

// CE_inputsel_caps_wrapping: sink CAPS → wrapped downstream event
// Target: gst_dxinputselector_sink_event L107-118
// MUT: remove wrapping → downstream does not receive per-stream CAPS
GST_START_TEST(CE_inputsel_caps_wrapping) {
    AggPipe p = make_agg_pipe("dxinputselector", 1);

    GstPad *srcpad = gst_element_get_static_pad(p.agg, "src");
    EventCounter ec = {};
    attach_event_counter(srcpad, &ec);
    gst_object_unref(srcpad);

    p.start();
    p.push(0, make_buf(0));
    p.eos(0);

    GstSample *s = p.pull();
    if (s) gst_sample_unref(s);

    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(p.pipe));
    GstMessage *msg = gst_bus_timed_pop_filtered(bus, 5 * GST_SECOND,
        (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
    if (msg) gst_message_unref(msg);
    gst_object_unref(bus);

    fail_unless(ec.n_wrapped >= 2,
                "CAPS+SEGMENT must be wrapped (got %d wrapped events)",
                ec.n_wrapped);

    p.stop();
}
GST_END_TEST;

static Suite *dxinputselector_suite(void) {
    Suite *s = suite_create("dxinputselector");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_inputsel_src_domain_caps);
    tcase_add_test(tc, CE_inputsel_wrapped_caps_stream_id);
    tcase_add_test(tc, CE_inputsel_invalid_pts_drop);
    tcase_add_test(tc, CE_inputsel_auto_frame_meta);
    tcase_add_test(tc, CE_inputsel_min_pts_order);
    tcase_add_test(tc, CE_inputsel_per_stream_wrapped_eos);
    tcase_add_test(tc, CE_inputsel_caps_wrapping);
    return s;
}

GST_CHECK_MAIN(dxinputselector);
