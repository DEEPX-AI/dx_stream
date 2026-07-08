// dxoutputselector LATENCY contract — TDD red phase for
// refactor_plans/event_query_state_audit.md §P2.
//
// dxoutputselector currently delegates LATENCY src_query to its sinkpad
// peer without adding self-time. Per plan §2.6, every processing element
// must add ≥1 ns to upstream latency.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

static const GstClockTime UPSTREAM_LAT = 10 * GST_MSECOND;

static gboolean answer_latency(GstPad *pad, GstObject *parent,
                               GstQuery *query) {
    (void)pad; (void)parent;
    if (GST_QUERY_TYPE(query) == GST_QUERY_LATENCY) {
        gst_query_set_latency(query, TRUE, UPSTREAM_LAT, GST_CLOCK_TIME_NONE);
        return TRUE;
    }
    return FALSE;
}

GST_START_TEST(LAT_dxoutputselector_adds_self_time) {
    GstElement *e = gst_element_factory_make("dxoutputselector", nullptr);
    fail_unless(e != nullptr);

    GstPad *peer_src = gst_pad_new("peer_src", GST_PAD_SRC);
    gst_pad_set_query_function(peer_src, answer_latency);
    gst_pad_set_active(peer_src, TRUE);

    GstPad *sinkpad = gst_element_get_static_pad(e, "sink");
    fail_unless(sinkpad != nullptr);
    fail_unless(gst_pad_link(peer_src, sinkpad) == GST_PAD_LINK_OK);

    GstPad *srcpad = gst_element_get_request_pad(e, "src_%u");
    fail_unless(srcpad != nullptr);

    GstQuery *q = gst_query_new_latency();
    gboolean ok = gst_pad_query(srcpad, q);
    fail_unless(ok, "LATENCY query must succeed");

    gboolean live = FALSE;
    GstClockTime min_lat = 0, max_lat = 0;
    gst_query_parse_latency(q, &live, &min_lat, &max_lat);
    gst_query_unref(q);

    fail_unless(min_lat > UPSTREAM_LAT,
                "dxoutputselector: min_latency must be > upstream (%"
                GST_TIME_FORMAT ") — got %" GST_TIME_FORMAT,
                GST_TIME_ARGS(UPSTREAM_LAT), GST_TIME_ARGS(min_lat));

    gst_element_release_request_pad(e, srcpad);
    gst_object_unref(srcpad);
    gst_pad_unlink(peer_src, sinkpad);
    gst_object_unref(sinkpad);
    gst_object_unref(peer_src);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxoutputselector_latency_suite(void) {
    Suite *s = suite_create("dxoutputselector_latency");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, LAT_dxoutputselector_adds_self_time);
    return s;
}

GST_CHECK_MAIN(dxoutputselector_latency);
