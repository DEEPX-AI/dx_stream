// Phase 5 — JSON config-file multi-property override verification
// A12: config-file-path → multiple properties loaded from JSON for each element

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "npu_env.hpp"

#include <glib/gstdio.h>
#include <cstring>
#ifdef _WIN32
#include <io.h>
#define write _write
#define close _close
#else
#include <unistd.h>
#endif

using namespace dxtest;

static gchar *write_temp_json(const char *content) {
    gchar *path = g_build_filename(g_get_tmp_dir(), "dxtest_config_XXXXXX.json", NULL);
    int fd = g_mkstemp(path);
    fail_unless(fd >= 0, "g_mkstemp failed for pattern: %s", path);

    gsize len = strlen(content);
    gsize written = 0;
    while (written < len) {
        gssize n = write(fd, content + written, len - written);
        fail_unless(n > 0, "write to temp file failed");
        written += (gsize)n;
    }
    close(fd);
    return path;
}

// CE_msgconv_config_overrides: config-file-path loads library_file_path + message_interval + include_frame
// Target: gst-dxmsgconv.cpp parse_config L53-96
// MUT: skip parse_config → properties stay at defaults
GST_START_TEST(CE_msgconv_config_overrides) {
    const char *json =
        "{\n"
        "  \"library_file_path\": \"/opt/test/libmsgconv.so\",\n"
        "  \"message_interval\": 5,\n"
        "  \"include_frame\": true\n"
        "}\n";
    gchar *path = write_temp_json(json);

    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "config-file-path", path, nullptr);

    gchar *lib = nullptr;
    gint interval = 0;
    gboolean include_frame = FALSE;
    g_object_get(e, "library-file-path", &lib, nullptr);
    g_object_get(e, "message-interval", &interval, nullptr);
    g_object_get(e, "include-frame", &include_frame, nullptr);

    fail_unless_equals_string(lib, "/opt/test/libmsgconv.so");
    fail_unless_equals_int(interval, 5);
    fail_unless(include_frame == TRUE, "include_frame must be TRUE from config");

    g_free(lib);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

// CE_postproc_config_overrides: config-file-path loads library_file_path + function_name + inference_id + secondary_mode
// Target: gst-dxpostprocess.cpp parse_config L38-85
GST_START_TEST(CE_postproc_config_overrides) {
    const char *json =
        "{\n"
        "  \"library_file_path\": \"/opt/test/libpost.so\",\n"
        "  \"function_name\": \"MY_PPU\",\n"
        "  \"inference_id\": 42,\n"
        "  \"secondary_mode\": true\n"
        "}\n";
    gchar *path = write_temp_json(json);

    GstElement *e = gst_element_factory_make("dxpostprocess", nullptr);
    g_object_set(e, "config-file-path", path, nullptr);

    gchar *lib = nullptr, *func = nullptr;
    guint iid = 0;
    gboolean sec = FALSE;
    g_object_get(e, "library-file-path", &lib,
                 "function-name", &func,
                 "inference-id", &iid,
                 "secondary-mode", &sec, nullptr);

    fail_unless_equals_string(lib, "/opt/test/libpost.so");
    fail_unless_equals_string(func, "MY_PPU");
    fail_unless_equals_int(iid, 42);
    fail_unless(sec == TRUE, "secondary_mode must be TRUE from config");

    g_free(lib);
    g_free(func);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

// CE_tracker_config_overrides: config-file-path loads tracker_name + params
// Target: gst-dxtracker.cpp parse_config L59-103
GST_START_TEST(CE_tracker_config_overrides) {
    const char *json =
        "{\n"
        "  \"tracker_name\": \"OC-SORT\",\n"
        "  \"params\": {\n"
        "    \"max_age\": 30,\n"
        "    \"min_hits\": 3,\n"
        "    \"iou_threshold\": 0.3\n"
        "  }\n"
        "}\n";
    gchar *path = write_temp_json(json);

    GstElement *e = gst_element_factory_make("dxtracker", nullptr);
    g_object_set(e, "config-file-path", path, nullptr);

    gchar *name = nullptr;
    g_object_get(e, "tracker-name", &name, nullptr);
    fail_unless_equals_string(name, "OC-SORT");

    g_free(name);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

// CE_config_property_wins_over_config: explicit property set after config overrides config value
// Target: property set order: config loaded first, then explicit g_object_set overwrites
GST_START_TEST(CE_config_property_wins_over_config) {
    const char *json =
        "{\n"
        "  \"library_file_path\": \"/from/config.so\",\n"
        "  \"message_interval\": 10\n"
        "}\n";
    gchar *path = write_temp_json(json);

    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "config-file-path", path, nullptr);

    g_object_set(e, "message-interval", 99, nullptr);

    gint interval = 0;
    g_object_get(e, "message-interval", &interval, nullptr);
    fail_unless_equals_int(interval, 99);

    gchar *lib = nullptr;
    g_object_get(e, "library-file-path", &lib, nullptr);
    fail_unless_equals_string(lib, "/from/config.so");

    g_free(lib);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

// CE_config_invalid_json: malformed JSON → element doesn't crash, properties unchanged
// Target: parse_config error handling (json_parser_load_from_file failure path)
GST_START_TEST(CE_config_invalid_json) {
    const char *json = "{ invalid json garbage !!}}}";
    gchar *path = write_temp_json(json);

    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", "/original/lib.so", nullptr);
    g_object_set(e, "config-file-path", path, nullptr);

    gchar *lib = nullptr;
    g_object_get(e, "library-file-path", &lib, nullptr);
    fail_unless_equals_string(lib, "/original/lib.so");

    g_free(lib);
    gst_object_unref(e);
    g_unlink(path);
    g_free(path);
}
GST_END_TEST;

static Suite *config_override_suite(void) {
    Suite *s = suite_create("config_override");
    TCase *tc = tcase_create("json_config");
    tcase_set_timeout(tc, 20.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_msgconv_config_overrides);
    tcase_add_test(tc, CE_postproc_config_overrides);
    tcase_add_test(tc, CE_tracker_config_overrides);
    tcase_add_test(tc, CE_config_property_wins_over_config);
    tcase_add_test(tc, CE_config_invalid_json);
    return s;
}

GST_CHECK_MAIN(config_override);
