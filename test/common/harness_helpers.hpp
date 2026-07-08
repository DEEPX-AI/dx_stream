// GstHarness wrapper for L1 element-level verification
// Used in: base/element/**/*.cpp
//
// GstHarness is a standard tool that attaches src/sink pads to a single element
// for direct buffer/event/query injection.
// This file provides wrappers for commonly used patterns (FLUSH injection, QOS injection, LATENCY query, etc.).

#pragma once

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>

#include <functional>
#include <string>
#include <vector>

#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

static void _dxtest_crash_handler(int sig) {
    void *frames[64];
    int n = backtrace(frames, 64);
    static const char hdr[] = "\n=== CRASH BACKTRACE ===\n";
    write(STDOUT_FILENO, hdr, sizeof(hdr) - 1);
    backtrace_symbols_fd(frames, n, STDOUT_FILENO);
    write(STDERR_FILENO, hdr, sizeof(hdr) - 1);
    backtrace_symbols_fd(frames, n, STDERR_FILENO);
    _exit(128 + sig);
}

inline void dxtest_crash_fixture_setup() {
    struct sigaction sa = {};
    sa.sa_handler = _dxtest_crash_handler;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
}
#else
// Windows: no backtrace API — crash handler is a no-op.
// The OS will still produce a crash dialog / Watson report on AV.
inline void dxtest_crash_fixture_setup() {}
#endif

namespace dxtest {

// ---------------------------------------------------------------------------
// Harness lifecycle
// ---------------------------------------------------------------------------

struct Harness {
    GstHarness *h = nullptr;

    explicit Harness(const char *element_name, const char *src_caps = nullptr,
                     const char *sink_caps = nullptr) {
        h = gst_harness_new_with_padnames(element_name, "sink", "src");
        fail_unless(h != nullptr, "Failed to create harness for %s", element_name);
        if (src_caps) {
            gst_harness_set_src_caps_str(h, src_caps);
        }
        if (sink_caps) {
            gst_harness_set_sink_caps_str(h, sink_caps);
        }
    }

    explicit Harness(const char *element_name,
                     const std::function<void(GstElement *)> &setup,
                     const char *src_caps = nullptr,
                     const char *sink_caps = nullptr) {
        GstElement *e = gst_element_factory_make(element_name, nullptr);
        fail_unless(e != nullptr, "Failed to make element %s", element_name);
        setup(e);
        h = gst_harness_new_with_element(e, "sink", "src");
        gst_object_unref(e);
        fail_unless(h != nullptr, "Failed to create harness for %s", element_name);
        if (src_caps) {
            gst_harness_set_src_caps_str(h, src_caps);
        }
        if (sink_caps) {
            gst_harness_set_sink_caps_str(h, sink_caps);
        }
    }

    // Constructor for elements that only have request pads (e.g. dxinputselector)
    static Harness *make_empty(const char *element_name) {
        auto *o = new Harness();
        o->h = gst_harness_new_empty();
        GstElement *e = gst_element_factory_make(element_name, nullptr);
        fail_unless(e != nullptr, "Failed to make element %s", element_name);
        gst_harness_add_element_full(o->h, e, nullptr, nullptr, nullptr, nullptr);
        gst_object_unref(e);
        return o;
    }

    ~Harness() {
        if (h) gst_harness_teardown(h);
    }

    GstElement *element() const { return h->element; }

private:
    Harness() = default;
};

// ---------------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------------

inline void assert_state(GstElement *e, GstState target) {
    GstStateChangeReturn ret = gst_element_set_state(e, target);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "state change to %s FAILED",
                gst_element_state_get_name(target));
    GstState cur;
    ret = gst_element_get_state(e, &cur, nullptr, 10 * GST_SECOND);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "state change to %s timed out after 10s",
                gst_element_state_get_name(target));
    fail_unless_equals_int(cur, target);
}

inline void assert_state_fails(GstElement *e, GstState target) {
    GstStateChangeReturn ret = gst_element_set_state(e, target);
    fail_unless(ret == GST_STATE_CHANGE_FAILURE,
                "state change to %s should FAIL but returned %s",
                gst_element_state_get_name(target),
                gst_element_state_change_return_get_name(ret));
}

// NULL→READY→PAUSED→PLAYING→PAUSED→READY→NULL
inline void full_state_cycle(GstElement *e) {
    assert_state(e, GST_STATE_READY);
    assert_state(e, GST_STATE_PAUSED);
    assert_state(e, GST_STATE_PLAYING);
    assert_state(e, GST_STATE_PAUSED);
    assert_state(e, GST_STATE_READY);
    gst_element_set_state(e, GST_STATE_NULL);
}

// ---------------------------------------------------------------------------
// Event injection
// ---------------------------------------------------------------------------

inline gboolean push_flush(GstHarness *h) {
    gboolean r = gst_harness_push_event(h, gst_event_new_flush_start());
    r &= gst_harness_push_event(h, gst_event_new_flush_stop(TRUE));
    return r;
}

inline gboolean push_qos(GstHarness *h, GstQOSType type, gdouble proportion,
                        GstClockTimeDiff diff, GstClockTime ts) {
    GstEvent *ev = gst_event_new_qos(type, proportion, diff, ts);
    return gst_harness_push_upstream_event(h, ev);
}

inline gboolean push_gap(GstHarness *h, GstClockTime ts, GstClockTime duration) {
    return gst_harness_push_event(h, gst_event_new_gap(ts, duration));
}

inline gboolean push_eos(GstHarness *h) {
    return gst_harness_push_event(h, gst_event_new_eos());
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------

// GstHarness has limited query helpers in GStreamer 1.16.
// Implemented as helpers that send queries directly to element's src/sink pads.
inline gboolean query_latency_on_src(GstElement *e, gboolean *live,
                                     GstClockTime *min, GstClockTime *max) {
    GstPad *srcpad = gst_element_get_static_pad(e, "src");
    if (!srcpad) return FALSE;
    GstQuery *q = gst_query_new_latency();
    gboolean r = gst_pad_query(srcpad, q);
    if (r) gst_query_parse_latency(q, live, min, max);
    gst_query_unref(q);
    gst_object_unref(srcpad);
    return r;
}

inline gboolean query_caps_on_pad(GstElement *e, const char *pad_name,
                                  GstCaps *filter, GstCaps **result) {
    GstPad *pad = gst_element_get_static_pad(e, pad_name);
    if (!pad) return FALSE;
    GstQuery *q = gst_query_new_caps(filter);
    gboolean r = gst_pad_query(pad, q);
    if (r) {
        GstCaps *c;
        gst_query_parse_caps_result(q, &c);
        *result = gst_caps_copy(c);
    }
    gst_query_unref(q);
    gst_object_unref(pad);
    return r;
}

// ---------------------------------------------------------------------------
// Probe helpers
// ---------------------------------------------------------------------------

struct EventCounter {
    int n_stream_start = 0;
    int n_caps = 0;
    int n_segment = 0;
    int n_tag = 0;
    int n_eos = 0;
    int n_flush_start = 0;
    int n_flush_stop = 0;
    int n_gap = 0;
    int n_qos = 0;
    int n_reconfigure = 0;
    int n_wrapped = 0;
    int n_unknown_downstream = 0;
};

static inline GstPadProbeReturn event_counter_probe(GstPad *,
                                                    GstPadProbeInfo *info,
                                                    gpointer udata) {
    auto *c = static_cast<EventCounter *>(udata);
    GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
    if (!ev) return GST_PAD_PROBE_OK;
    switch (GST_EVENT_TYPE(ev)) {
        case GST_EVENT_STREAM_START: c->n_stream_start++; break;
        case GST_EVENT_CAPS:         c->n_caps++; break;
        case GST_EVENT_SEGMENT:      c->n_segment++; break;
        case GST_EVENT_TAG:          c->n_tag++; break;
        case GST_EVENT_EOS:          c->n_eos++; break;
        case GST_EVENT_FLUSH_START:  c->n_flush_start++; break;
        case GST_EVENT_FLUSH_STOP:   c->n_flush_stop++; break;
        case GST_EVENT_GAP:          c->n_gap++; break;
        case GST_EVENT_QOS:          c->n_qos++; break;
        case GST_EVENT_RECONFIGURE:  c->n_reconfigure++; break;
        case GST_EVENT_CUSTOM_DOWNSTREAM: {
            const GstStructure *s = gst_event_get_structure(ev);
            if (s && gst_structure_has_name(s, "application/x-dx-wrapped-event")) {
                c->n_wrapped++;
            } else {
                c->n_unknown_downstream++;
            }
            break;
        }
        default: break;
    }
    return GST_PAD_PROBE_OK;
}

inline gulong attach_event_counter(GstPad *pad, EventCounter *c,
                                   GstPadProbeType type = GST_PAD_PROBE_TYPE_EVENT_BOTH) {
    return gst_pad_add_probe(pad, type, event_counter_probe, c, nullptr);
}

}  // namespace dxtest
