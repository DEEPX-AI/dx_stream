// Phase 4 — QoS event propagation through transform chain
// B6: QoS events propagate upstream through dxscale, dxconvert, dxrate

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

using namespace dxtest;

static const char *CAPS_30FPS =
    "video/x-raw,format=I420,width=64,height=64,framerate=30/1";

static void send_qos_upstream(GstHarness *h, GstQOSType type,
                               gdouble proportion, GstClockTimeDiff diff,
                               GstClockTime timestamp) {
    GstPad *element_srcpad = gst_pad_get_peer(h->sinkpad);
    GstEvent *qos = gst_event_new_qos(type, proportion, diff, timestamp);
    gst_pad_send_event(element_srcpad, qos);
    gst_object_unref(element_srcpad);
}

// CE_qos_underflow_forwarded: QoS UNDERFLOW event propagates upstream through dxscale+dxconvert
// Target: GstBaseTransform default upstream QoS forwarding
// MUT: override src_event and swallow QoS → upstream never sees it
GST_START_TEST(CE_qos_underflow_forwarded) {
    GstHarness *h = gst_harness_new_parse(
        "dxscale width=32 height=32 ! dxconvert");
    gst_harness_set_src_caps_str(h, CAPS_30FPS);

    send_qos_upstream(h, GST_QOS_TYPE_UNDERFLOW, 0.5,
                      50 * GST_MSECOND, 1 * GST_SECOND);

    int qos_count = 0;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            gdouble proportion;
            GstClockTimeDiff diff;
            GstClockTime timestamp;
            gst_event_parse_qos(ev, &type, &proportion, &diff, &timestamp);
            fail_unless_equals_int(type, GST_QOS_TYPE_UNDERFLOW);
            fail_unless(proportion == 0.5,
                        "proportion must be preserved (got %f)", proportion);
            fail_unless(diff == (GstClockTimeDiff)(50 * GST_MSECOND),
                        "diff must be preserved");
            fail_unless(timestamp == 1 * GST_SECOND,
                        "timestamp must be preserved");
            qos_count++;
        }
        gst_event_unref(ev);
    }

    fail_unless(qos_count > 0,
                "QoS UNDERFLOW must propagate upstream (got %d events)",
                qos_count);

    gst_harness_teardown(h);
}
GST_END_TEST;

// CE_qos_throttle_forwarded: QoS THROTTLE event propagates upstream through dxconvert
// Target: GstBaseTransform default upstream QoS forwarding
GST_START_TEST(CE_qos_throttle_forwarded) {
    Harness h("dxconvert");
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    send_qos_upstream(h.h, GST_QOS_TYPE_THROTTLE, 1.0,
                      100 * GST_MSECOND, 2 * GST_SECOND);

    int qos_count = 0;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h.h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            gst_event_parse_qos(ev, &type, nullptr, nullptr, nullptr);
            fail_unless_equals_int(type, GST_QOS_TYPE_THROTTLE);
            qos_count++;
        }
        gst_event_unref(ev);
    }

    fail_unless(qos_count > 0,
                "QoS THROTTLE must propagate upstream through dxconvert");
}
GST_END_TEST;

// CE_qos_overflow_forwarded: QoS OVERFLOW event propagates upstream through dxrate
// Target: GstBaseTransform default upstream QoS forwarding
GST_START_TEST(CE_qos_overflow_forwarded) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 15u, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    send_qos_upstream(h.h, GST_QOS_TYPE_OVERFLOW, 2.0,
                      -10 * (GstClockTimeDiff)GST_MSECOND, 3 * GST_SECOND);

    int qos_count = 0;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h.h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            GstClockTimeDiff diff;
            gst_event_parse_qos(ev, &type, nullptr, &diff, nullptr);
            fail_unless_equals_int(type, GST_QOS_TYPE_OVERFLOW);
            fail_unless(diff == -10 * (GstClockTimeDiff)GST_MSECOND,
                        "diff must be preserved for OVERFLOW");
            qos_count++;
        }
        gst_event_unref(ev);
    }

    fail_unless(qos_count > 0,
                "QoS OVERFLOW must propagate upstream through dxrate");
}
GST_END_TEST;

// CE_qos_chain_preserves_fields: QoS through multi-element chain preserves all fields
// Target: dxscale → dxconvert → dxrate chain preserves QoS event fields
GST_START_TEST(CE_qos_chain_preserves_fields) {
    GstHarness *h = gst_harness_new_parse(
        "dxscale width=32 height=32 ! dxconvert ! dxrate framerate=15");
    gst_harness_set_src_caps_str(h, CAPS_30FPS);

    GstClockTimeDiff expected_diff = 75 * GST_MSECOND;
    GstClockTime expected_ts = 5 * GST_SECOND;

    send_qos_upstream(h, GST_QOS_TYPE_UNDERFLOW, 0.8,
                      expected_diff, expected_ts);

    gboolean found = FALSE;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            gdouble proportion;
            GstClockTimeDiff diff;
            GstClockTime timestamp;
            gst_event_parse_qos(ev, &type, &proportion, &diff, &timestamp);
            fail_unless_equals_int(type, GST_QOS_TYPE_UNDERFLOW);
            fail_unless(proportion == 0.8,
                        "proportion preserved through chain (got %f)", proportion);
            fail_unless(diff == expected_diff,
                        "diff preserved through chain");
            fail_unless(timestamp == expected_ts,
                        "timestamp preserved through chain");
            found = TRUE;
        }
        gst_event_unref(ev);
    }

    fail_unless(found, "QoS must propagate through the entire transform chain");

    gst_harness_teardown(h);
}
GST_END_TEST;

static Suite *qos_propagation_suite(void) {
    Suite *s = suite_create("qos_propagation");
    TCase *tc = tcase_create("qos_events");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_qos_underflow_forwarded);
    tcase_add_test(tc, CE_qos_throttle_forwarded);
    tcase_add_test(tc, CE_qos_overflow_forwarded);
    tcase_add_test(tc, CE_qos_chain_preserves_fields);
    return s;
}

GST_CHECK_MAIN(qos_propagation);
