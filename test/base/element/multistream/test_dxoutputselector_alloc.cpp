// dxoutputselector ALLOCATION routing test — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.3
//
// Defect: 01_defect_report.md §2 P2 — src_query default branch forwarded
// ALL non-LATENCY/CAPS queries (including ALLOCATION) upstream via
// gst_pad_peer_query(_sinkpad, q). Forwarding ALLOCATION across the
// dxvideoraw boundary breaks pool negotiation because the sink peer
// negotiated dxvideoraw caps but the query asks video/x-raw.
//
// GStreamer enforces ALLOCATION as a downstream-serialized query invoked
// only on sinkpads (gst_pad_query "wrong direction" assertion). The
// production contract: outputselector's src_query handler must answer
// ALLOCATION locally without forwarding to its sink peer.
//
// Oracle: directly invoke the srcpad's query handler via its installed
// query function and confirm the sink peer never sees the ALLOCATION.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

static guint g_sink_peer_alloc_seen = 0;

static GstPadProbeReturn count_alloc_probe(GstPad * /*pad*/,
                                            GstPadProbeInfo *info,
                                            gpointer /*ud*/) {
    GstQuery *q = GST_PAD_PROBE_INFO_QUERY(info);
    if (q && GST_QUERY_TYPE(q) == GST_QUERY_ALLOCATION) {
        g_atomic_int_inc((gint *)&g_sink_peer_alloc_seen);
    }
    return GST_PAD_PROBE_OK;
}

static GstStaticPadTemplate dxvr_src_tmpl =
    GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("application/x-dxvideoraw,"
                                            "format=(string)RGB,"
                                            "width=(int)4,height=(int)4,"
                                            "framerate=(fraction)30/1"));

GST_START_TEST(CE_outputsel_alloc_does_not_leak_to_sink_peer) {
    g_sink_peer_alloc_seen = 0;

    GstElement *e = gst_element_factory_make("dxoutputselector", "out");
    fail_unless(e != nullptr);

    GstPad *fake_upstream = gst_pad_new_from_static_template(&dxvr_src_tmpl,
                                                              "src");
    fail_unless(gst_pad_set_active(fake_upstream, TRUE));

    GstPad *sinkpad = gst_element_get_static_pad(e, "sink");
    fail_unless(sinkpad != nullptr);
    fail_unless_equals_int(gst_pad_link(fake_upstream, sinkpad),
                           GST_PAD_LINK_OK);

    gulong pid = gst_pad_add_probe(fake_upstream,
                                   GST_PAD_PROBE_TYPE_QUERY_UPSTREAM,
                                   count_alloc_probe, nullptr, nullptr);

    fail_unless(gst_element_set_state(e, GST_STATE_PAUSED) !=
                GST_STATE_CHANGE_FAILURE);

    GstPad *src0 = gst_element_get_request_pad(e, "src_0");
    fail_unless(src0 != nullptr);

    GstPad *fake_downstream = gst_pad_new("sink", GST_PAD_SINK);
    gst_pad_set_active(fake_downstream, TRUE);
    fail_unless_equals_int(gst_pad_link(src0, fake_downstream),
                           GST_PAD_LINK_OK);

    // Bypass gst_pad_query's direction check by invoking the registered
    // query function directly — the production contract (src_query handler)
    // is what we need to exercise.
    GstCaps *acaps = gst_caps_from_string(
        "video/x-raw,format=RGB,width=4,height=4,framerate=30/1");
    GstQuery *q = gst_query_new_allocation(acaps, FALSE);
    gst_caps_unref(acaps);

    GstPadQueryFunction qfn = GST_PAD_QUERYFUNC(src0);
    fail_unless(qfn != nullptr, "src_query handler not installed");
    qfn(src0, GST_OBJECT(e), q);
    gst_query_unref(q);

    fail_unless(g_sink_peer_alloc_seen == 0,
                "dxoutputselector leaked ALLOCATION across the dxvideoraw "
                "boundary to its sink peer (count=%u). C.6 violation.",
                g_sink_peer_alloc_seen);

    gst_pad_remove_probe(fake_upstream, pid);
    gst_pad_unlink(src0, fake_downstream);
    gst_pad_unlink(fake_upstream, sinkpad);
    gst_element_release_request_pad(e, src0);
    gst_object_unref(src0);
    gst_object_unref(sinkpad);
    gst_object_unref(fake_upstream);
    gst_object_unref(fake_downstream);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *outputsel_alloc_suite(void) {
    Suite *s = suite_create("outputsel_alloc");
    TCase *tc = tcase_create("direction");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_outputsel_alloc_does_not_leak_to_sink_peer);
    return s;
}

GST_CHECK_MAIN(outputsel_alloc);
