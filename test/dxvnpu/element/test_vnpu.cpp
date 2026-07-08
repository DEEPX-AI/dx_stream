// P8 — VNPU Element tests
// dxvnpudec, dxvnpuenc, dxvnpupipeline, dxvnpuoverlay
// Shell TCs (factory, property, state) + element-specific contracts

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include "harness_helpers.hpp"

using namespace dxtest;

// ============================================================
// dxvnpudec — GstVideoDecoder
// ============================================================

GST_START_TEST(CA1_vnpudec_factory_make) {
    GstElement *e = gst_element_factory_make("dxvnpudec", nullptr);
    fail_unless(e != nullptr, "dxvnpudec must be registered");
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_vnpudec_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxvnpudec", nullptr);

    // output-format default = NV12 (=23)
    gint fmt = 0;
    g_object_get(e, "output-format", &fmt, nullptr);
    fail_unless_equals_int(fmt, GST_VIDEO_FORMAT_NV12);

    // output-width/height default = 0
    gint w = -1, h = -1;
    g_object_get(e, "output-width", &w, "output-height", &h, nullptr);
    fail_unless_equals_int(w, 0);
    fail_unless_equals_int(h, 0);

    // set and get back
    g_object_set(e, "output-width", 1280, "output-height", 720, nullptr);
    g_object_get(e, "output-width", &w, "output-height", &h, nullptr);
    fail_unless_equals_int(w, 1280);
    fail_unless_equals_int(h, 720);

    g_object_set(e, "output-format", (gint)GST_VIDEO_FORMAT_RGB, nullptr);
    g_object_get(e, "output-format", &fmt, nullptr);
    fail_unless_equals_int(fmt, GST_VIDEO_FORMAT_RGB);

    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_vnpudec_full_cycle) {
    GstElement *e = gst_element_factory_make("dxvnpudec", nullptr);
    // VideoDecoder needs negotiated caps for PLAYING, but NULL→READY→NULL should work
    fail_unless(gst_element_set_state(e, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    fail_unless(gst_element_set_state(e, GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
    fail_unless(gst_element_set_state(e, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    fail_unless(gst_element_set_state(e, GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CC1_vnpudec_pad_templates) {
    GstElementFactory *f = gst_element_factory_find("dxvnpudec");
    fail_unless(f != nullptr);

    const GList *templates = gst_element_factory_get_static_pad_templates(f);
    gboolean has_sink = FALSE, has_src = FALSE;
    for (const GList *l = templates; l; l = l->next) {
        auto *t = (GstStaticPadTemplate *)l->data;
        if (t->direction == GST_PAD_SINK) {
            has_sink = TRUE;
            GstCaps *caps = gst_static_caps_get(&t->static_caps);
            fail_unless(gst_caps_can_intersect(caps,
                gst_caps_from_string("video/x-h264,stream-format=byte-stream,alignment=au")));
            gst_caps_unref(caps);
        }
        if (t->direction == GST_PAD_SRC) {
            has_src = TRUE;
            GstCaps *caps = gst_static_caps_get(&t->static_caps);
            fail_unless(gst_caps_can_intersect(caps,
                gst_caps_from_string("video/x-raw,format=NV12")));
            gst_caps_unref(caps);
        }
    }
    fail_unless(has_sink, "must have sink pad template");
    fail_unless(has_src, "must have src pad template");
    gst_object_unref(f);
}
GST_END_TEST;

// CVD1: dxvnpudec set_latency call — gst_video_decoder_set_latency (L433)
// Called in set_format, but set_format requires HW pipeline.
// Instead verify LATENCY query response defaults (VideoDecoder default = min 0)
GST_START_TEST(CE_vnpudec_latency_query) {
    GstElement *e = gst_element_factory_make("dxvnpudec", nullptr);
    gst_element_set_state(e, GST_STATE_READY);

    GstPad *src = gst_element_get_static_pad(e, "src");
    fail_unless(src != nullptr, "src pad must exist");

    GstQuery *q = gst_query_new_latency();
    gboolean handled = gst_pad_query(src, q);
    if (handled) {
        gboolean live = FALSE;
        GstClockTime min_lat = 0, max_lat = 0;
        gst_query_parse_latency(q, &live, &min_lat, &max_lat);
        // VideoDecoder defaults: min=0 before set_format, live depends on upstream
        fail_unless(min_lat != GST_CLOCK_TIME_NONE,
                    "latency min must be valid");
    }
    gst_query_unref(q);
    gst_object_unref(src);

    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// ============================================================
// dxvnpuenc — GstVideoEncoder
// ============================================================

GST_START_TEST(CA1_vnpuenc_factory_make) {
    GstElement *e = gst_element_factory_make("dxvnpuenc", nullptr);
    fail_unless(e != nullptr, "dxvnpuenc must be registered");
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_vnpuenc_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxvnpuenc", nullptr);

    // codec default (H.264 enum value)
    gint codec = -1;
    g_object_get(e, "codec", &codec, nullptr);
    fail_unless(codec >= 0, "codec default must be set");

    // bitrate default = 4096
    guint br = 0;
    g_object_get(e, "bitrate", &br, nullptr);
    fail_unless_equals_int(br, 4096);

    // set and get back
    g_object_set(e, "bitrate", (guint)8000, nullptr);
    g_object_get(e, "bitrate", &br, nullptr);
    fail_unless_equals_int(br, 8000);

    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_vnpuenc_full_cycle) {
    GstElement *e = gst_element_factory_make("dxvnpuenc", nullptr);
    fail_unless(gst_element_set_state(e, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    fail_unless(gst_element_set_state(e, GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
    fail_unless(gst_element_set_state(e, GST_STATE_READY) != GST_STATE_CHANGE_FAILURE);
    fail_unless(gst_element_set_state(e, GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CC1_vnpuenc_pad_templates) {
    GstElementFactory *f = gst_element_factory_find("dxvnpuenc");
    fail_unless(f != nullptr);

    const GList *templates = gst_element_factory_get_static_pad_templates(f);
    gboolean has_sink = FALSE, has_src = FALSE;
    for (const GList *l = templates; l; l = l->next) {
        auto *t = (GstStaticPadTemplate *)l->data;
        if (t->direction == GST_PAD_SINK) {
            has_sink = TRUE;
            GstCaps *caps = gst_static_caps_get(&t->static_caps);
            fail_unless(gst_caps_can_intersect(caps,
                gst_caps_from_string("video/x-raw,format=NV12")));
            gst_caps_unref(caps);
        }
        if (t->direction == GST_PAD_SRC) {
            has_src = TRUE;
            GstCaps *caps = gst_static_caps_get(&t->static_caps);
            fail_unless(gst_caps_can_intersect(caps,
                gst_caps_from_string("video/x-h264,stream-format=byte-stream,alignment=au")));
            gst_caps_unref(caps);
        }
    }
    fail_unless(has_sink);
    fail_unless(has_src);
    gst_object_unref(f);
}
GST_END_TEST;

// CVE1: dxvnpuenc set_latency — gst_video_encoder_set_latency (L329-330)
GST_START_TEST(CE_vnpuenc_latency_query) {
    GstElement *e = gst_element_factory_make("dxvnpuenc", nullptr);
    gst_element_set_state(e, GST_STATE_READY);

    GstPad *src = gst_element_get_static_pad(e, "src");
    fail_unless(src != nullptr);

    GstQuery *q = gst_query_new_latency();
    gboolean handled = gst_pad_query(src, q);
    if (handled) {
        gboolean live = FALSE;
        GstClockTime min_lat = 0, max_lat = 0;
        gst_query_parse_latency(q, &live, &min_lat, &max_lat);
        fail_unless(min_lat != GST_CLOCK_TIME_NONE);
    }
    gst_query_unref(q);
    gst_object_unref(src);

    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// ============================================================
// dxvnpupipeline — GstElement (request pads)
// ============================================================

GST_START_TEST(CA1_vnpupipeline_factory_make) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);
    fail_unless(e != nullptr, "dxvnpupipeline must be registered");
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_vnpupipeline_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);

    gchar *mp = nullptr;
    g_object_get(e, "model-path", &mp, nullptr);
    fail_unless(mp == nullptr, "model-path default must be null");

    guint iid = 99;
    g_object_get(e, "inference-id", &iid, nullptr);
    fail_unless_equals_int(iid, 0);

    gboolean kr = FALSE;
    g_object_get(e, "keep-ratio", &kr, nullptr);
    fail_unless(kr == TRUE);

    gboolean ort = FALSE;
    g_object_get(e, "use-ort", &ort, nullptr);
    fail_unless(ort == TRUE);

    gint did = 0;
    g_object_get(e, "device-id", &did, nullptr);
    fail_unless_equals_int(did, -1);

    gboolean hdmi = TRUE;
    g_object_get(e, "use-vnpu-hdmi", &hdmi, nullptr);
    fail_unless(hdmi == FALSE);

    gint mhc = 0;
    g_object_get(e, "max-hdmi-channels", &mhc, nullptr);
    fail_unless_equals_int(mhc, 32);

    // set and get back
    g_object_set(e, "model-path", "/tmp/test.dxnn", "inference-id", (guint)5, nullptr);
    g_object_get(e, "model-path", &mp, nullptr);
    fail_unless_equals_string(mp, "/tmp/test.dxnn");
    g_free(mp);

    g_object_get(e, "inference-id", &iid, nullptr);
    fail_unless_equals_int(iid, 5);

    gst_object_unref(e);
}
GST_END_TEST;

// CVP1: NULL→READY fails when model-path is not set (change_state L399-403)
GST_START_TEST(CE_vnpupipeline_no_model_error) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);
    // model-path not set → NULL→READY must fail
    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_READY);
    fail_unless(ret == GST_STATE_CHANGE_FAILURE,
                "NULL→READY without model-path must fail (got %d)", ret);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CE_vnpupipeline_bad_model_error) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);
    g_object_set(e, "model-path", "/nonexistent/vnpu-model.dxnn", nullptr);
    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_READY);
    fail_unless(ret == GST_STATE_CHANGE_FAILURE,
                "NULL→READY with nonexistent model-path must fail (got %d)", ret);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CVP1: requesting a pad creates both sink+src pair (request_new_pad L315-371)
GST_START_TEST(CE_vnpupipeline_request_pad_creates_pair) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);

    GstPad *sink0 = gst_element_get_request_pad(e, "sink_0");
    fail_unless(sink0 != nullptr, "request pad sink_0 must be created");

    GstPad *src0 = gst_element_get_static_pad(e, "src_0");
    fail_unless(src0 != nullptr, "src_0 must be auto-created with sink_0");

    gst_object_unref(src0);
    gst_element_release_request_pad(e, sink0);
    gst_object_unref(sink0);
    gst_object_unref(e);
}
GST_END_TEST;

// CVP1: verify query/event functions attached — sending query to sink pad must not crash
GST_START_TEST(CE_vnpupipeline_sink_query_attached) {
    GstElement *e = gst_element_factory_make("dxvnpupipeline", nullptr);
    g_object_set(e, "model-path", "/tmp/dummy.dxnn", nullptr);

    GstPad *sink0 = gst_element_get_request_pad(e, "sink_0");
    fail_unless(sink0 != nullptr);

    // CAPS query on sink pad — should not crash
    GstQuery *q = gst_query_new_caps(nullptr);
    gst_pad_query(sink0, q);
    gst_query_unref(q);

    // LATENCY query on src pad
    GstPad *src0 = gst_element_get_static_pad(e, "src_0");
    fail_unless(src0 != nullptr);
    GstQuery *lq = gst_query_new_latency();
    gst_pad_query(src0, lq);
    gst_query_unref(lq);

    gst_object_unref(src0);
    gst_element_release_request_pad(e, sink0);
    gst_object_unref(sink0);
    gst_object_unref(e);
}
GST_END_TEST;

// ============================================================
// dxvnpuoverlay — GstBaseSink
// ============================================================

GST_START_TEST(CA1_vnpuoverlay_factory_make) {
    GstElement *e = gst_element_factory_make("dxvnpuoverlay", nullptr);
    fail_unless(e != nullptr, "dxvnpuoverlay must be registered");
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_vnpuoverlay_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxvnpuoverlay", nullptr);

    gchar *mp = nullptr;
    g_object_get(e, "model-path", &mp, nullptr);
    fail_unless(mp == nullptr, "model-path default null");

    gboolean kr = FALSE;
    g_object_get(e, "keep-ratio", &kr, nullptr);
    fail_unless(kr == TRUE);

    gint did = 0;
    g_object_get(e, "device-id", &did, nullptr);
    fail_unless_equals_int(did, -1);

    gint gc = 0;
    g_object_get(e, "group-count", &gc, nullptr);
    fail_unless_equals_int(gc, 1);

    // set and get back
    g_object_set(e, "model-path", "/tmp/overlay.dxnn",
                 "keep-ratio", FALSE,
                 "device-id", 2,
                 "group-count", 4, nullptr);

    g_object_get(e, "model-path", &mp, nullptr);
    fail_unless_equals_string(mp, "/tmp/overlay.dxnn");
    g_free(mp);

    g_object_get(e, "keep-ratio", &kr, nullptr);
    fail_unless(kr == FALSE);

    g_object_get(e, "device-id", &did, nullptr);
    fail_unless_equals_int(did, 2);

    g_object_get(e, "group-count", &gc, nullptr);
    fail_unless_equals_int(gc, 4);

    gst_object_unref(e);
}
GST_END_TEST;

// CVO1: start() fails when model-path is not set (gst_dxvnpuoverlay_start L174-178)
// GstBaseSink start() is called during READY→PAUSED transition
GST_START_TEST(CE_vnpuoverlay_no_model_error) {
    GstElement *e = gst_element_factory_make("dxvnpuoverlay", nullptr);
    // model-path not set → PAUSED must fail
    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_PAUSED);
    fail_unless(ret == GST_STATE_CHANGE_FAILURE,
                "READY→PAUSED without model-path must fail (got %d)", ret);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CE_vnpuoverlay_bad_model_error) {
    GstElement *e = gst_element_factory_make("dxvnpuoverlay", nullptr);
    g_object_set(e, "model-path", "/nonexistent/vnpu-model.dxnn", nullptr);
    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_PAUSED);
    fail_unless(ret == GST_STATE_CHANGE_FAILURE,
                "READY→PAUSED with nonexistent model-path must fail (got %d)", ret);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// ============================================================
// Suite
// ============================================================
static Suite *vnpu_suite(void) {
    Suite *s = suite_create("vnpu");

    TCase *tc_dec = tcase_create("dxvnpudec");
    tcase_set_timeout(tc_dec, 10.0);
    tcase_add_test(tc_dec, CA1_vnpudec_factory_make);
    tcase_add_test(tc_dec, CA2_vnpudec_property_defaults_and_set);
    tcase_add_test(tc_dec, CB3_vnpudec_full_cycle);
    tcase_add_test(tc_dec, CC1_vnpudec_pad_templates);
    tcase_add_test(tc_dec, CE_vnpudec_latency_query);
    suite_add_tcase(s, tc_dec);

    TCase *tc_enc = tcase_create("dxvnpuenc");
    tcase_set_timeout(tc_enc, 10.0);
    tcase_add_test(tc_enc, CA1_vnpuenc_factory_make);
    tcase_add_test(tc_enc, CA2_vnpuenc_property_defaults_and_set);
    tcase_add_test(tc_enc, CB3_vnpuenc_full_cycle);
    tcase_add_test(tc_enc, CC1_vnpuenc_pad_templates);
    tcase_add_test(tc_enc, CE_vnpuenc_latency_query);
    suite_add_tcase(s, tc_enc);

    TCase *tc_pl = tcase_create("dxvnpupipeline");
    tcase_set_timeout(tc_pl, 10.0);
    tcase_add_test(tc_pl, CA1_vnpupipeline_factory_make);
    tcase_add_test(tc_pl, CA2_vnpupipeline_property_defaults_and_set);
    tcase_add_test(tc_pl, CE_vnpupipeline_no_model_error);
    tcase_add_test(tc_pl, CE_vnpupipeline_bad_model_error);
    tcase_add_test(tc_pl, CE_vnpupipeline_request_pad_creates_pair);
    tcase_add_test(tc_pl, CE_vnpupipeline_sink_query_attached);
    suite_add_tcase(s, tc_pl);

    TCase *tc_ov = tcase_create("dxvnpuoverlay");
    tcase_set_timeout(tc_ov, 10.0);
    tcase_add_test(tc_ov, CA1_vnpuoverlay_factory_make);
    tcase_add_test(tc_ov, CA2_vnpuoverlay_property_defaults_and_set);
    tcase_add_test(tc_ov, CE_vnpuoverlay_no_model_error);
    tcase_add_test(tc_ov, CE_vnpuoverlay_bad_model_error);
    suite_add_tcase(s, tc_ov);

    return s;
}

GST_CHECK_MAIN(vnpu);
