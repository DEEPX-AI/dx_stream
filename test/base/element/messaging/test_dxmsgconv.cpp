// P5.1 — dxmsgconv contract tests
// Core: GstBaseTransform in-place. Converts DXFrameMeta → DxMsgMeta via dlopen-based custom library.
// Missing library-file-path → READY failure. No DXFrameMeta → passthrough.
// message-interval controls conversion frequency. config-file-path loads JSON settings.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"

#include <cstring>

using namespace dxtest;

static const char *CAPS_STR =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

static std::string MSGCONV_LIB_PATH() {
    return dxtest::resolve_lib_path("libdx_msgconvl.so");
}
#define MSGCONV_LIB (MSGCONV_LIB_PATH().c_str())

static GstBuffer *make_buf(GstClockTime pts) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo map;
    gst_buffer_map(b, &map, GST_MAP_WRITE);
    memset(map.data, 0x80, sz);
    gst_buffer_unmap(b, &map);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    return b;
}

static GstBuffer *make_buf_with_meta(GstClockTime pts, int stream_id,
                                     int n_objects = 0) {
    GstBuffer *b = make_buf(pts);
    DXFrameMeta *fm = make_frame_meta(b, stream_id, 4, 4);
    for (int i = 0; i < n_objects; i++) {
        add_object_to_frame(fm, i, 0.9f, 10 * i, 20, 30, 40);
    }
    return b;
}

static bool has_msg_meta(GstBuffer *buf) {
    GstMeta *meta = gst_buffer_get_meta(buf, gst_dxmsg_meta_api_get_type());
    return meta != nullptr;
}

// ---- Shell TCs ----

GST_START_TEST(CA1_factory_make) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    fail_unless(e != nullptr);
    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CA2_property_defaults_and_set) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);

    gchar *lib = nullptr;
    gchar *cfg = nullptr;
    gint interval = -1;
    gboolean inc = TRUE;

    g_object_get(e, "library-file-path", &lib,
                 "config-file-path", &cfg,
                 "message-interval", &interval,
                 "include-frame", &inc, nullptr);

    fail_unless(lib == nullptr, "library-file-path default must be null");
    fail_unless(cfg == nullptr, "config-file-path default must be null");
    fail_unless_equals_int(interval, 1);
    fail_unless(inc == FALSE, "include-frame default must be FALSE");

    g_free(lib);
    g_free(cfg);

    g_object_set(e, "message-interval", 5, nullptr);
    g_object_get(e, "message-interval", &interval, nullptr);
    fail_unless_equals_int(interval, 5);

    g_object_set(e, "include-frame", TRUE, nullptr);
    g_object_get(e, "include-frame", &inc, nullptr);
    fail_unless(inc == TRUE);

    gst_object_unref(e);
}
GST_END_TEST;

GST_START_TEST(CB3_full_cycle) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB, nullptr);
    full_state_cycle(e);
    full_state_cycle(e);
    gst_object_unref(e);
}
GST_END_TEST;

// ---- Element-specific TCs ----

// CE_msgconv_no_library_error: library-file-path not set → READY failure
// Target: gst_dxmsgconv_start L238-243
// MUT: remove L238-243 → nullptr dereference on dlopen
GST_START_TEST(CE_msgconv_no_library_error) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_empty_library_error: empty library-file-path → READY failure before dlopen
// Target: gst_dxmsgconv_start validates required library-file-path property value
GST_START_TEST(CE_msgconv_empty_library_error) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", "", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_bad_library_error: nonexistent library → READY failure
// Target: gst_dxmsgconv_start L245-250
// MUT: remove L246-250 → nullptr function pointers → crash on first buffer
GST_START_TEST(CE_msgconv_bad_library_error) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", "/nonexistent/libfake.so", nullptr);
    assert_state_fails(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_bad_config_path_uses_properties: config-file-path load failure is non-fatal
// Target: gst_dxmsgconv_start validates library-file-path, not config load status
GST_START_TEST(CE_msgconv_bad_config_path_uses_properties) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "config-file-path", "/nonexistent/dxmsgconv.json",
                 "library-file-path", MSGCONV_LIB, nullptr);
    assert_state(e, GST_STATE_PAUSED);
    gst_element_set_state(e, GST_STATE_NULL);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_no_meta_passthrough: buffer without DXFrameMeta → passthrough, no DxMsgMeta
// Target: gst_dxmsgconv_transform_ip L459-462
// MUT: remove L460-462 → null deref in convert()
GST_START_TEST(CE_msgconv_no_meta_passthrough) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_harness_set_src_caps_str(h, CAPS_STR);

    GstBuffer *in = make_buf(0);
    GstFlowReturn ret = gst_harness_push(h, in);
    fail_unless_equals_int(ret, GST_FLOW_OK);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr, "passthrough buffer must arrive");
    fail_unless(!has_msg_meta(out), "no-meta buffer must not have DxMsgMeta");
    gst_buffer_unref(out);

    gst_harness_teardown(h);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_payload_attached: DXFrameMeta + objects → DxMsgMeta._payload attached
// Target: convert() L410 (dx_add_payload_to_buffer)
// MUT: remove L410 → DxMsgMeta not attached
GST_START_TEST(CE_msgconv_payload_attached) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_harness_set_src_caps_str(h, CAPS_STR);

    GstBuffer *in = make_buf_with_meta(0, 0, 2);
    GstFlowReturn ret = gst_harness_push(h, in);
    fail_unless_equals_int(ret, GST_FLOW_OK);

    GstBuffer *out = gst_harness_pull(h);
    fail_unless(out != nullptr, "output buffer must arrive");
    fail_unless(has_msg_meta(out),
                "buffer with frame_meta must have DxMsgMeta attached");

    GstDxMsgMeta *mm = (GstDxMsgMeta *)gst_buffer_get_meta(
        out, gst_dxmsg_meta_api_get_type());
    fail_unless(mm->_payload != nullptr, "payload must be non-null");
    DxMsgPayload *pl = (DxMsgPayload *)mm->_payload;
    fail_unless(pl->_size > 0, "payload size must be > 0");
    fail_unless(pl->_data != nullptr, "payload data must be non-null");

    gst_buffer_unref(out);
    gst_harness_teardown(h);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_message_interval: message-interval=3 → only every 3rd buffer is converted
// Target: convert() L383-384 (seq_id % message_interval)
// MUT: remove L383 condition → all buffers converted
GST_START_TEST(CE_msgconv_message_interval) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB,
                 "message-interval", 3, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    gst_harness_set_src_caps_str(h, CAPS_STR);

    int converted = 0;
    for (int i = 0; i < 6; i++) {
        GstBuffer *in = make_buf_with_meta(i * GST_SECOND / 30, 0, 1);
        gst_harness_push(h, in);
        GstBuffer *out = gst_harness_pull(h);
        fail_unless(out != nullptr);
        if (has_msg_meta(out)) converted++;
        gst_buffer_unref(out);
    }

    fail_unless_equals_int(converted, 2);

    gst_harness_teardown(h);
    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_dlclose_reopen: READY→NULL→READY → dlopen re-invocation succeeds
// Target: stop() L292-295 (dlclose) + start() L245 (dlopen)
// MUT: remove stop L292-295 dlclose → double open on second start (works but handle leak)
GST_START_TEST(CE_msgconv_dlclose_reopen) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB, nullptr);

    full_state_cycle(e);

    GstStateChangeReturn ret = gst_element_set_state(e, GST_STATE_PLAYING);
    fail_unless(ret != GST_STATE_CHANGE_FAILURE,
                "second READY after dlclose must succeed");
    gst_element_set_state(e, GST_STATE_NULL);

    gst_object_unref(e);
}
GST_END_TEST;

// CE_msgconv_config_loads_properties: config-file-path → loads properties from JSON
// Target: parse_config() L78-93
// MUT: remove parse_config → config values not applied
GST_START_TEST(CE_msgconv_config_loads_properties) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);

    std::string cfg = resolve_config_path("msgconv_config.json");
    fail_unless(!cfg.empty(), "msgconv_config.json must exist");
    g_object_set(e, "config-file-path", cfg.c_str(), nullptr);

    gchar *lib = nullptr;
    gint interval = -1;
    g_object_get(e, "library-file-path", &lib,
                 "message-interval", &interval, nullptr);

    fail_unless(lib != nullptr, "config must load library_file_path");
    fail_unless_equals_int(interval, 1);

    g_free(lib);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxmsgconv_suite(void) {
    Suite *s = suite_create("dxmsgconv");
    TCase *tc = tcase_create("contract");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CA1_factory_make);
    tcase_add_test(tc, CA2_property_defaults_and_set);
    tcase_add_test(tc, CB3_full_cycle);
    tcase_add_test(tc, CE_msgconv_no_library_error);
    tcase_add_test(tc, CE_msgconv_empty_library_error);
    tcase_add_test(tc, CE_msgconv_bad_library_error);
    tcase_add_test(tc, CE_msgconv_bad_config_path_uses_properties);
    tcase_add_test(tc, CE_msgconv_no_meta_passthrough);
    tcase_add_test(tc, CE_msgconv_payload_attached);
    tcase_add_test(tc, CE_msgconv_message_interval);
    tcase_add_test(tc, CE_msgconv_dlclose_reopen);
    tcase_add_test(tc, CE_msgconv_config_loads_properties);
    return s;
}

GST_CHECK_MAIN(dxmsgconv);
