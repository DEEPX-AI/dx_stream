// dxtracker config contract — a numeric value the parser cannot read must not
// cost the camera its analytics.
//
// SCOPE, STATED HONESTLY. This is not a law about trackers. When a config value
// will not parse, two designs are defensible: substitute the documented default
// and carry on, or refuse to start and say so. This element chooses the first,
// and the choice is deliberate — tracker_config.json is edited by hand by
// operators, the values here are numeric tuning knobs rather than things the
// tracker cannot run without, and a 24/7 recorder should not lose a camera's
// analytics over a stray character. The check below therefore FAILS a tracker
// that chooses the second design. That is a regression guard on a decision, not
// a universal truth, and anyone porting this file to an element that decided the
// other way should change the assertion rather than the implementation.
//
// What IS universal, and what the check also covers by simply completing: the
// failure must not remove the process. `init()` is called from a GStreamer chain
// function, and an exception that escapes it unwinds through C frames.
//
// WHY THIS NEEDS A TEST
//
// `dxtracker` reads tracker_config.json into a string map and calls the
// algorithm's `init()` from `transform_ip`, i.e. from inside a GStreamer chain
// function. An exception thrown there unwinds through C frames: no catch site,
// no error message, SIGABRT and a core dump. The path was reachable from a
// single-character typo in a file operators edit by hand — `"max_age": "abc"`
// and `"max_age": ""` both did it.
//
// The file already had a test named CE_tracker_exception_handled whose comment
// claimed the try/catch as its target. It pushes a frame with an empty object
// list, which returns before the tracker is called at all, and it passed on a
// build that had no try/catch to speak of. A test can name a guarantee the code
// does not have; only exercising the path finds that out.
//
// The check does not assert which error surfaces or that one surfaces at all.
// It asserts three things: the pipeline keeps accepting buffers, the tracker
// keeps assigning ids, and the process reaches the end of the function.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"

#include <glib/gstdio.h>

#include <string>
#include <vector>

using namespace dxtest;

namespace {

const char *CAPS_320 =
    "video/x-raw,format=RGB,width=320,height=240,framerate=30/1";
const guint BUF_SIZE = 320 * 240 * 3;

// Values that reach the parser verbatim: json-glib hands the element the string
// as written, so anything an operator can type in the file arrives here.
struct BadValue {
    const char *key;
    const char *value;
    const char *why;
};
const BadValue BAD_VALUES[] = {
    {"max_age", "\"abc\"", "not a number"},
    {"max_age", "\"\"", "empty string"},
    {"min_hits", "\"-\"", "sign with no digits"},
    {"delta_t", "\"3.5.1\"", "garbage after a valid prefix"},
    {"inertia", "\"  \"", "whitespace only"},
    {"iou_threshold", "\"1e999\"", "parses, then overflows"},
    {"det_thresh", "\"0.4f\"", "a C literal suffix"},
};

// A config file with every key at a sane value except `key`, which gets `value`
// verbatim. Returns the path; caller frees.
gchar *write_config(const char *key, const char *value) {
    // The element only reads keys under "params" (gst-dxtracker.cpp
    // parse_config); a flat object is loaded and silently ignored, which is
    // itself worth remembering — an operator who drops the wrapper gets
    // defaults with no warning.
    GString *j = g_string_new("{\n  \"tracker_name\": \"OC_SORT\",\n  \"params\": {\n");
    const char *defaults[][2] = {
        {"det_thresh", "\"0.4\""},     {"max_age", "\"50\""},
        {"min_hits", "\"3\""},         {"iou_threshold", "\"0.3\""},
        {"delta_t", "\"3\""},          {"asso_func", "\"iou\""},
        {"inertia", "\"0.2\""},        {"use_byte", "\"false\""},
    };
    for (size_t i = 0; i < G_N_ELEMENTS(defaults); ++i) {
        const bool overridden = (key && g_strcmp0(defaults[i][0], key) == 0);
        g_string_append_printf(j, "    \"%s\": %s%s\n", defaults[i][0],
                               overridden ? value : defaults[i][1],
                               i + 1 < G_N_ELEMENTS(defaults) ? "," : "");
    }
    g_string_append(j, "  }\n}\n");

    gchar *path = g_build_filename(g_get_tmp_dir(), "dxtracker_bad_config.json",
                                   nullptr);
    GError *err = nullptr;
    if (!g_file_set_contents(path, j->str, -1, &err)) {
        g_string_free(j, TRUE);
        g_free(path);
        fail("could not write the config file: %s", err ? err->message : "?");
        return nullptr;
    }
    g_string_free(j, TRUE);
    return path;
}

} // namespace

// ---------------------------------------------------------------------------
// Every value below is one keystroke away in a file an operator edits, and the
// element has to survive all of them. Objects are attached because the tracker
// is only reached on frames that carry some — a frame with an empty list returns
// before `init()`, which is why the pre-existing test that pushed one proved
// nothing about this path.
// ---------------------------------------------------------------------------
// Returns true if the element, configured from `path`, is still assigning track
// ids after four frames of the same two objects. `flow_ok` reports separately
// whether every push was accepted, so "the pipeline stopped" and "the pipeline
// ran but tracked nothing" stay distinguishable — they are different defects
// and the second is the one that hides.
bool tracks_with_config(const gchar *path, bool *flow_ok) {
    Harness h("dxtracker");
    g_object_set(h.h->element, "config-file-path", path, nullptr);
    gst_harness_set_src_caps_str(h.h, CAPS_320);

    *flow_ok = true;
    bool tracked = false;
    // Enough frames that the verdict is not decided by one association: the
    // first draft ran four, which put the check one frame after min_hits and on
    // the edge of the tracker's own decision to drop an unmatched box. Two
    // builds whose tracker sources differ only in comments disagreed there.
    for (int f = 0; f < 12; ++f) {
        GstBuffer *b = gst_harness_create_buffer(h.h, BUF_SIZE);
        GST_BUFFER_PTS(b) = f * (GST_SECOND / 30);
        GST_BUFFER_DURATION(b) = GST_SECOND / 30;
        DXFrameMeta *fm = make_frame_meta(b, 0, 320, 240);
        add_object_to_frame(fm, 0, 0.9f, 40.f, 60.f, 55.f, 130.f, -1);
        add_object_to_frame(fm, 0, 0.8f, 180.f, 70.f, 55.f, 130.f, -1);

        GstFlowReturn r = gst_harness_push(h.h, b);
        if (r != GST_FLOW_OK)
            *flow_ok = false;
        GstBuffer *out = gst_harness_try_pull(h.h);
        if (!out) {
            *flow_ok = false;
            continue;
        }
        // "some object carried an id at some point", not "this object did on
        // this frame". Which box survives an association is the algorithm's
        // business and varies between implementations; that ids are handed out
        // at all is what a working tracker owes its caller.
        DXFrameMeta *ofm = dx_get_frame_meta(out);
        if (ofm)
            for (auto *o : ofm->_object_meta_list)
                if (o->_track_id != -1)
                    tracked = true;
        gst_buffer_unref(out);
    }
    return tracked;
}

// ---------------------------------------------------------------------------
// Every value below is one keystroke away in a file an operator edits, and the
// element has to keep working through all of them. Objects are attached because
// the tracker is only reached on frames that carry some — a frame with an empty
// list returns before `init()`, which is why the pre-existing
// CE_tracker_exception_handled, which pushes exactly that, proved nothing about
// this path despite naming it.
//
// The control runs first and is not optional. Without it every result here is
// unreadable: a build that cannot track at all fails all seven and looks like
// seven config defects. That happened on the first run of this test.
// ---------------------------------------------------------------------------
GST_START_TEST(DC_malformed_config_value_keeps_the_tracker_working) {
    bool flow_ok = false;
    gchar *good = write_config(nullptr, nullptr);   // no key overridden
    const bool control = tracks_with_config(good, &flow_ok);
    g_print("[DIAG] %-14s   %-8s (%s): %s\n", "(control)", "valid",
            "every value sane", control ? "tracking" : "NOT TRACKING");
    g_unlink(good);
    g_free(good);
    fail_unless(control && flow_ok,
                "the control config does not track — nothing below can be read "
                "as a config defect until this passes");

    std::vector<std::string> stopped, silent;
    for (size_t i = 0; i < G_N_ELEMENTS(BAD_VALUES); ++i) {
        const BadValue &bv = BAD_VALUES[i];
        gchar *path = write_config(bv.key, bv.value);
        const bool tracked = tracks_with_config(path, &flow_ok);
        g_print("[DIAG] %-14s = %-8s (%s): %s\n", bv.key, bv.value, bv.why,
                !flow_ok ? "PIPELINE STOPPED"
                         : (tracked ? "kept running, still tracking"
                                    : "FLOWS BUT TRACKS NOTHING"));
        if (!flow_ok)
            stopped.push_back(std::string(bv.key) + "=" + bv.value);
        else if (!tracked)
            silent.push_back(std::string(bv.key) + "=" + bv.value);
        g_unlink(path);
        g_free(path);
    }

    std::string a, b;
    for (const auto &v : stopped) a += (a.empty() ? "" : ", ") + v;
    for (const auto &v : silent)  b += (b.empty() ? "" : ", ") + v;

    // Separated because they are different repairs, and because the second is
    // the worse outcome: an element that reports the bad value and then returns
    // without building a tracker leaves a pipeline that looks healthy from every
    // angle except the one that matters.
    fail_unless(stopped.empty(),
                "%zu value(s) stopped the pipeline: %s", stopped.size(),
                a.c_str());
    fail_unless(silent.empty(),
                "%zu value(s) left the pipeline flowing with no tracking at "
                "all: %s", silent.size(), b.c_str());

    // Reaching this line also proves the process survived, which an exception
    // thrown out of the chain function would not have allowed.
}
GST_END_TEST;

static Suite *dxtracker_config_contract_suite(void) {
    Suite *s = suite_create("dxtracker_config_contract");
    TCase *tc = tcase_create("config_contract");
    tcase_set_timeout(tc, 60.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, DC_malformed_config_value_keeps_the_tracker_working);
    return s;
}

GST_CHECK_MAIN(dxtracker_config_contract);
