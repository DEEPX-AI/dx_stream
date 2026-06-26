// Sticky event ordering tests — GStreamer B.1 contract
//
// Contract: before the first buffer reaches downstream, the element must
// have forwarded (or emitted) STREAM_START → CAPS → SEGMENT in that order.
// BaseTransform does this automatically; GstElement subclasses (dxinfer) and
// GstAggregator subclasses (dxinputselector, dxgather) must do it manually.
//
// Method: attach EventTrace probe on the element's src pad, push a buffer,
// then verify the ordering via EventTrace::order_before().

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "event_probe.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

using namespace dxtest;

static const char *CAPS_RGB_64 =
    "video/x-raw,format=RGB,width=64,height=64,framerate=30/1";
static const guint BUF_SIZE_RGB_64 = 64 * 64 * 3;

// --- Helper: verify sticky event order on BaseTransform element ---
static void assert_sticky_order_basetransform(const char *element,
                                               const char *src_caps,
                                               void (*setup)(GstElement *) = nullptr) {
    GstElement *e = gst_element_factory_make(element, nullptr);
    fail_unless(e != nullptr, "factory_make %s", element);
    if (setup) setup(e);

    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_object_unref(e);
    fail_unless(h != nullptr);

    // Attach probe on src pad to record event order
    GstPad *srcpad = gst_element_get_static_pad(h->element, "src");
    EventTrace trace;
    trace.attach_downstream(srcpad);
    gst_object_unref(srcpad);

    // Set caps (triggers STREAM_START + CAPS + SEGMENT from harness)
    gst_harness_set_src_caps_str(h, src_caps);

    // Push one buffer
    GstBuffer *buf = gst_harness_create_buffer(h, BUF_SIZE_RGB_64);
    GST_BUFFER_PTS(buf) = 0;
    GST_BUFFER_DURATION(buf) = GST_SECOND / 30;
    gst_harness_push(h, buf);

    GstBuffer *out = gst_harness_try_pull(h);
    if (out) gst_buffer_unref(out);

    // Verify ordering
    fail_unless(trace.has(GST_EVENT_STREAM_START),
                "%s: STREAM_START never seen on src pad", element);
    fail_unless(trace.has(GST_EVENT_CAPS),
                "%s: CAPS never seen on src pad", element);
    fail_unless(trace.has(GST_EVENT_SEGMENT),
                "%s: SEGMENT never seen on src pad", element);

    fail_unless(trace.order_before(GST_EVENT_STREAM_START, GST_EVENT_CAPS),
                "%s: STREAM_START must come before CAPS", element);
    fail_unless(trace.order_before(GST_EVENT_CAPS, GST_EVENT_SEGMENT),
                "%s: CAPS must come before SEGMENT", element);

    gst_harness_teardown(h);
}

// === BaseTransform elements (positive controls + regression guards) ===

GST_START_TEST(STICKY_dxosd_order) {
    assert_sticky_order_basetransform("dxosd", CAPS_RGB_64);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxrate_order) {
    auto setup = [](GstElement *e) { g_object_set(e, "framerate", 30u, nullptr); };
    assert_sticky_order_basetransform("dxrate", CAPS_RGB_64, setup);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxscale_order) {
    auto setup = [](GstElement *e) { g_object_set(e, "width", 64u, "height", 64u, nullptr); };
    assert_sticky_order_basetransform("dxscale", CAPS_RGB_64, setup);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxconvert_order) {
    assert_sticky_order_basetransform("dxconvert", CAPS_RGB_64);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxpreprocess_order) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "resize-width", 64u, "resize-height", 64u, nullptr);
    };
    assert_sticky_order_basetransform("dxpreprocess", CAPS_RGB_64, setup);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxpostprocess_order) {
    assert_sticky_order_basetransform("dxpostprocess", CAPS_RGB_64);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxtracker_order) {
    assert_sticky_order_basetransform("dxtracker", CAPS_RGB_64);
}
GST_END_TEST;

GST_START_TEST(STICKY_dxmsgconv_order) {
    auto setup = [](GstElement *e) {
        g_object_set(e, "library-file-path",
                     dxtest::resolve_lib_path("libdx_msgconvl.so").c_str(), nullptr);
    };
    assert_sticky_order_basetransform("dxmsgconv", CAPS_RGB_64, setup);
}
GST_END_TEST;

static Suite *sticky_event_order_suite(void) {
    Suite *s = suite_create("sticky_event_order");
    TCase *tc = tcase_create("order");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, STICKY_dxosd_order);
    tcase_add_test(tc, STICKY_dxrate_order);
    tcase_add_test(tc, STICKY_dxscale_order);
    tcase_add_test(tc, STICKY_dxconvert_order);
    tcase_add_test(tc, STICKY_dxpreprocess_order);
    tcase_add_test(tc, STICKY_dxpostprocess_order);
    tcase_add_test(tc, STICKY_dxtracker_order);
    tcase_add_test(tc, STICKY_dxmsgconv_order);
    return s;
}

GST_CHECK_MAIN(sticky_event_order);
