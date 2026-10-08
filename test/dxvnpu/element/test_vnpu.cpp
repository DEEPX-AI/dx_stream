// P8 — VNPU codec element tests

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

    gint device_id = 0;
    g_object_get(e, "device-id", &device_id, nullptr);
    fail_unless_equals_int(device_id, -1);

    g_object_set(e, "device-id", 2, nullptr);
    g_object_get(e, "device-id", &device_id, nullptr);
    fail_unless_equals_int(device_id, 2);

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

    return s;
}

GST_CHECK_MAIN(vnpu);
