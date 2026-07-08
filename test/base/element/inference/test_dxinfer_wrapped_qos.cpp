// dxinfer wrapped CUSTOM_UPSTREAM QoS test — TDD red phase for
// refactor_plans/event_query_state_audit.md §P1.
//
// ⚠️ Implementation-coupled: accesses GstDxInfer private fields (_timing_ctx)
// because the QoS effect (buffer drop timing) requires a full NPU inference
// pipeline to observe externally. This is a pragmatic trade-off — if a public
// QoS query API is added, this test should be rewritten to use observable
// behavior instead of private state inspection.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "gst-dxinfer.hpp"
#include "utils.hpp"

static const GstClockTime QOS_TS = 5 * GST_SECOND;
static const GstClockTimeDiff QOS_DIFF = 3 * GST_MSECOND;

GST_START_TEST(WQoS_wrapped_underflow_updates_timediff) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    fail_unless(e != nullptr);
    GstDxInfer *self = (GstDxInfer *)e;
    self->_timing_ctx.qos_timediff = 0;
    self->_timing_ctx.qos_timestamp = 0;

    GstPad *srcpad = gst_element_get_static_pad(e, "src");
    fail_unless(srcpad != nullptr);
    gst_pad_set_active(srcpad, TRUE);
    gst_pad_send_event(srcpad, gst_event_new_flush_start());
    gst_pad_send_event(srcpad, gst_event_new_flush_stop(TRUE));

    GstEvent *qos = gst_event_new_qos(GST_QOS_TYPE_UNDERFLOW, 1.0,
                                       QOS_DIFF, QOS_TS);
    GstEvent *wrapped = dx_event_wrap_upstream(0, qos);
    gst_pad_send_event(srcpad, wrapped);

    fail_unless(self->_timing_ctx.qos_timediff == QOS_DIFF,
                "wrapped QoS UNDERFLOW must update qos_timediff "
                "(expected %" G_GINT64_FORMAT " got %" G_GINT64_FORMAT
                ") — dxinfer src_event likely missing wrapped-upstream branch",
                QOS_DIFF, self->_timing_ctx.qos_timediff);
    fail_unless(self->_timing_ctx.qos_timestamp == QOS_TS,
                "wrapped QoS UNDERFLOW must update qos_timestamp");

    gst_pad_set_active(srcpad, FALSE);
    gst_object_unref(srcpad);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(WQoS_wrapped_throttle_updates_delay) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    fail_unless(e != nullptr);
    GstDxInfer *self = (GstDxInfer *)e;
    self->_timing_ctx.throttling_delay = 0;

    GstPad *srcpad = gst_element_get_static_pad(e, "src");
    fail_unless(srcpad != nullptr);
    gst_pad_set_active(srcpad, TRUE);
    gst_pad_send_event(srcpad, gst_event_new_flush_start());
    gst_pad_send_event(srcpad, gst_event_new_flush_stop(TRUE));

    GstEvent *qos = gst_event_new_qos(GST_QOS_TYPE_THROTTLE, 1.0,
                                       QOS_DIFF, QOS_TS);
    GstEvent *wrapped = dx_event_wrap_upstream(1, qos);
    gst_pad_send_event(srcpad, wrapped);

    fail_unless(self->_timing_ctx.throttling_delay == QOS_DIFF,
                "wrapped QoS THROTTLE must update throttling_delay "
                "(expected %" G_GINT64_FORMAT " got %" G_GINT64_FORMAT ")",
                QOS_DIFF, self->_timing_ctx.throttling_delay);

    gst_pad_set_active(srcpad, FALSE);
    gst_object_unref(srcpad);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxinfer_wrapped_qos_suite(void) {
    Suite *s = suite_create("dxinfer_wrapped_qos");
    TCase *tc = tcase_create("upstream");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, WQoS_wrapped_underflow_updates_timediff);
    tcase_add_test(tc, WQoS_wrapped_throttle_updates_delay);
    return s;
}

GST_CHECK_MAIN(dxinfer_wrapped_qos);
