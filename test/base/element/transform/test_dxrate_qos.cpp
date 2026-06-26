// Phase 4 — dxrate QoS throttle event verification
// A8: throttle=true → upstream QoS THROTTLE event with correct delay

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"

using namespace dxtest;

static const char *CAPS_30FPS =
    "video/x-raw,format=I420,width=320,height=240,framerate=30/1";
static const guint BUF_SIZE = 320 * 240 * 3 / 2;

static void push_frames(GstHarness *h, guint n, guint fps) {
    GstClockTime dur = GST_SECOND / fps;
    for (guint i = 0; i < n; i++) {
        GstBuffer *b = gst_harness_create_buffer(h, BUF_SIZE);
        GST_BUFFER_PTS(b) = i * dur;
        GST_BUFFER_DURATION(b) = dur;
        gst_harness_push(h, b);
    }
}

// CE_rate_qos_throttle_sent: throttle=true, 30→10fps → QoS THROTTLE events sent upstream
// Target: gst_dxrate_send_qos_throttle L88-100
// MUT: remove L415-416 (throttle send) → no upstream QoS → fail
GST_START_TEST(CE_rate_qos_throttle_sent) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 10u, "throttle", TRUE, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 30, 30);

    int qos_count = 0;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h.h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            gdouble proportion;
            GstClockTimeDiff diff;
            GstClockTime timestamp;
            gst_event_parse_qos(ev, &type, &proportion, &diff, &timestamp);
            fail_unless_equals_int(type, GST_QOS_TYPE_THROTTLE);
            fail_unless(proportion > 0.0 && proportion <= 1.0,
                        "proportion must be (0, 1], got %f", proportion);
            fail_unless(diff > 0, "delay must be positive");
            qos_count++;
        }
        gst_event_unref(ev);
    }

    fail_unless(qos_count > 0,
                "throttle=true must generate QoS THROTTLE events (got %d)",
                qos_count);
}
GST_END_TEST;

// CE_rate_qos_throttle_disabled: throttle=false → no QoS events
// Target: gst_dxrate_process_buffer L415 (if count==0 && _throttle)
// MUT: remove _throttle check → QoS sent even when disabled → fail
GST_START_TEST(CE_rate_qos_throttle_disabled) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 10u, "throttle", FALSE, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 30, 30);

    int qos_count = 0;
    GstEvent *ev;
    while ((ev = gst_harness_try_pull_upstream_event(h.h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            qos_count++;
        }
        gst_event_unref(ev);
    }

    fail_unless_equals_int(qos_count, 0);
}
GST_END_TEST;

// CE_rate_qos_throttle_delay: delay ≈ frame_duration * 0.999
// Target: gst_dxrate_send_qos_throttle L94-95 (delay = scaled_time * THROTTLE_DELAY_RATIO)
GST_START_TEST(CE_rate_qos_throttle_delay) {
    Harness h("dxrate", [](GstElement *e) {
        g_object_set(e, "framerate", 10u, "throttle", TRUE, nullptr);
    });
    gst_harness_set_src_caps_str(h.h, CAPS_30FPS);

    push_frames(h.h, 30, 30);

    GstClockTime expected_delay = (GstClockTime)(GST_SECOND / 10 * 0.999);
    GstClockTimeDiff tolerance = expected_delay / 10;

    GstEvent *ev;
    gboolean found = FALSE;
    while ((ev = gst_harness_try_pull_upstream_event(h.h)) != nullptr) {
        if (GST_EVENT_TYPE(ev) == GST_EVENT_QOS) {
            GstQOSType type;
            gdouble proportion;
            GstClockTimeDiff diff;
            GstClockTime timestamp;
            gst_event_parse_qos(ev, &type, &proportion, &diff, &timestamp);

            if (!found) {
                GstClockTimeDiff err = diff > (GstClockTimeDiff)expected_delay
                    ? diff - (GstClockTimeDiff)expected_delay
                    : (GstClockTimeDiff)expected_delay - diff;
                fail_unless(err <= tolerance,
                            "QoS delay %" G_GINT64_FORMAT "ns must be ~%"
                            G_GUINT64_FORMAT "ns (±10%%)",
                            (gint64)diff, (guint64)expected_delay);
                found = TRUE;
            }
        }
        gst_event_unref(ev);
    }
    fail_unless(found, "must find at least one QoS event to check delay");
}
GST_END_TEST;

static Suite *dxrate_qos_suite(void) {
    Suite *s = suite_create("dxrate_qos");
    TCase *tc = tcase_create("qos_throttle");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_rate_qos_throttle_sent);
    tcase_add_test(tc, CE_rate_qos_throttle_disabled);
    tcase_add_test(tc, CE_rate_qos_throttle_delay);
    return s;
}

GST_CHECK_MAIN(dxrate_qos);
