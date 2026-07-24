// ALLOCATION peer-fail contract test — TDD red phase for
// refactor_plans/event_query_state_audit.md §P3.
//
// When peer ALLOCATION query fails, the element MUST NOT silently add its
// own allocation meta. Currently dxinfer sink_query adds DX_FRAME_META_API
// regardless of peer success — leaving downstream with a "failed" query
// that still claims to need DXFrameMeta allocation.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "./../metadata/gst-dxframemeta.hpp"

static gboolean query_has_dxframe_meta(GstQuery *q) {
    guint n = gst_query_get_n_allocation_metas(q);
    for (guint i = 0; i < n; i++) {
        GType api = gst_query_parse_nth_allocation_meta(q, i, NULL);
        if (api == DX_FRAME_META_API_TYPE) return TRUE;
    }
    return FALSE;
}

GST_START_TEST(ALLOC_dxinfer_no_peer_no_meta) {
    GstElement *e = gst_element_factory_make("dxinfer", nullptr);
    fail_unless(e != nullptr);

    GstPad *sinkpad = gst_element_get_static_pad(e, "sink");
    fail_unless(sinkpad != nullptr);
    gst_pad_set_active(sinkpad, TRUE);
    gst_pad_send_event(sinkpad, gst_event_new_flush_start());
    gst_pad_send_event(sinkpad, gst_event_new_flush_stop(TRUE));

    GstCaps *caps = gst_caps_from_string(
        "application/x-dxvideoraw, format=(string)ANY, "
        "width=(int)64, height=(int)64, framerate=(fraction)30/1");
    GstQuery *q = gst_query_new_allocation(caps, FALSE);
    gst_caps_unref(caps);

    gboolean ok = gst_pad_query(sinkpad, q);

    fail_if(ok, "peer-less ALLOCATION must return FALSE");
    fail_if(query_has_dxframe_meta(q),
            "dxinfer must NOT attach DXFrameMeta when peer query failed");

    gst_query_unref(q);
    gst_pad_set_active(sinkpad, FALSE);
    gst_object_unref(sinkpad);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *allocation_peer_fail_suite(void) {
    Suite *s = suite_create("allocation_peer_fail");
    TCase *tc = tcase_create("no_peer");
    tcase_set_timeout(tc, 10.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, ALLOC_dxinfer_no_peer_no_meta);
    return s;
}

GST_CHECK_MAIN(allocation_peer_fail);
