// Tracker output contract — what a tracker emits must be a real box, and every
// parameter it accepts must be observable in what it emits.
//
// Like test_tracker_resource_contract.cpp, this is a contract of the `Tracker`
// interface (init + update, see tracker/common/include/Tracker.hpp), not a
// property of one algorithm: no check below references OC_SORT types. To cover
// another implementation, change TRACKER, the parameter map, and PROBED_PARAMS.
//
// What a tracker should do with a malformed config value is NOT here. Falling
// back to a default and refusing to start are both defensible, so a library-level
// check has to pick one and would fail a different implementation for a choice
// that is not wrong. The contract that holds either way — a bad value must not
// take the process down — is testable only where the process is, and lives in
// test_dxtracker_config_contract.cpp.
//
// WHY THESE CONTRACTS NEED TESTS
//
// Both defects these checks pin down were present from this port's initial
// release commit and survived a full tracker rewrite in v3.0.0. Neither is
// subtle to observe once something looks; nothing did.
//
//   1. A track that had never been matched still reported a box, and the box was
//      (0,0,0,0). `last_observation` was initialised to zeros, and the emit path
//      asks `last_observation.sum() >= 0` to decide whether an observation
//      exists — a test the all-zero vector passes. Measured on 16 live cameras:
//      77 such boxes in five minutes, on every camera, most of them in the first
//      frames of the stream where `frame_count <= min_hits` publishes tracks
//      that have not been matched yet. The same predicate gates the
//      observation-centric re-update, so each one also seeded a fresh track's
//      filter with a trajectory running from the image origin.
//
//   2. The observation-centric momentum term was multiplied by the class label
//      instead of the detection confidence, so for label 0 the whole term was
//      multiplied by zero. `delta_t` and `inertia` exist only to shape that term.
//      For the class every deployment tracks — person, label 0 — both settings
//      did nothing at all, and changing either produced byte-identical output.
//
// A dead parameter is invisible by construction: there is no error, no log line,
// and the tracker keeps working. The only way to notice is to change the setting
// and require the output to react.
//
// ASSERT ON THE SHAPE, NOT THE VALUES
//
// Neither check states what the tracker should have emitted — that is the
// algorithm's business and differs per implementation. They state only that a
// box must be a box, and that a setting must matter. Both hold for any tracker
// worth the name, and both fail loudly on the two defects above.

#include <cmath>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "tracker/common/include/TrackerFactory.hpp"

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

namespace {

const char *TRACKER = "OC_SORT";

// Production edge configuration (dx_argus edge/configs/tracker_config.json).
std::map<std::string, std::string, std::less<>> tracker_params() {
    return {{"det_thresh", "0.4"},   {"max_age", "50"},
            {"min_hits", "3"},       {"iou_threshold", "0.3"},
            {"delta_t", "3"},        {"asso_func", "iou"},
            {"inertia", "0.2"},      {"use_byte", "false"}};
}

// Settings whose effect the tracker must be able to demonstrate, each with two
// values far enough apart that a live implementation cannot produce the same
// output for both on `moving_scene`.
//
// OBLIGATION WHEN THIS LIST CHANGES. A failure here has two possible causes and
// they are not distinguishable from the result alone: the setting is ignored, or
// the scene does not exercise it. Before removing an entry, establish which —
// deleting a probe that is telling the truth removes the only signal this class
// of defect produces. Before adding one, check that `moving_scene` can move it;
// two of these were unprobeable until the scene was given a spread of detection
// confidences, and a probe that cannot fail is worse than no probe.
struct ProbedParam {
    const char *key;
    const char *lo;
    const char *hi;
};
const std::vector<ProbedParam> PROBED_PARAMS = {
    {"delta_t", "1", "9"},           // observation-centric momentum: lookback
    {"inertia", "0.05", "0.9"},      // observation-centric momentum: weight
    {"asso_func", "iou", "giou"},    // association metric
    {"iou_threshold", "0.15", "0.6"},
    {"min_hits", "1", "8"},
    {"max_age", "2", "60"},
    {"det_thresh", "0.1", "0.85"},
    {"use_byte", "false", "true"},
};

struct Det {
    float x1, y1, x2, y2;
};

// dets layout expected by Tracker::update — [x1,y1,x2,y2,conf,label,input_idx],
// exactly what gst-dxtracker.cpp builds from the object meta list.
// `label` is 0 (person) on purpose: that is the class every deployment tracks,
// and it is the class defect 2 above silenced.
Eigen::MatrixXf make_dets(const std::vector<Det> &ds) {
    Eigen::MatrixXf m(static_cast<Eigen::Index>(ds.size()), 7);
    for (size_t i = 0; i < ds.size(); ++i) {
        auto r = static_cast<Eigen::Index>(i);
        m(r, 0) = ds[i].x1;
        m(r, 1) = ds[i].y1;
        m(r, 2) = ds[i].x2;
        m(r, 3) = ds[i].y2;
        // A spread, not a constant: with every detection at the same confidence
        // the settings that key off it (`det_thresh`, `use_byte`) cannot be
        // probed at all, and a real detector never produces one anyway. The
        // range straddles the configured det_thresh so both the high- and
        // low-confidence association paths carry traffic.
        m(r, 4) = 0.25f + 0.14f * static_cast<float>(i);
        m(r, 5) = 0.0f;                  // label: person
        m(r, 6) = static_cast<float>(i);
    }
    return m;
}

// Several similarly-sized subjects sweeping through a shared region on curved
// paths, each dropping out periodically so its track freezes and unfreezes.
//
// Two properties make this the scene a direction term is judged on, and both
// were learned by getting them wrong first:
//
//   The paths CURVE. A straight line at constant speed points the same way
//   however far back you look, so a lookback-window setting cannot change
//   anything on one. The first draft was linear and reported `delta_t` dead on
//   a build where it demonstrably is not.
//
//   The subjects CROWD. With two well-separated objects the overlap cost alone
//   decides every assignment, and a direction term that is working changes the
//   cost without changing the answer. The second draft had two objects and
//   still reported `delta_t` dead. Direction only breaks ties, so the scene has
//   to produce ties.
std::vector<Det> moving_scene(int f) {
    const float t = static_cast<float>(f);
    std::vector<Det> ds;
    for (int i = 0; i < 6; ++i) {
        const float ph = static_cast<float>(i) * 1.05f;
        // Different periods per subject, so they bunch up and separate again
        // rather than travelling in formation.
        if ((f + i) % (5 + i) == 0)
            continue;   // this subject is missed on this frame
        const float x = 300.f + 200.f * std::sin(t * 0.08f + ph);
        const float y = 240.f + 110.f * std::sin(t * 0.13f + ph * 1.7f);
        ds.push_back({x, y, x + 55.f, y + 130.f});
    }
    return ds;
}

// Every emitted row, flattened, so two runs can be compared without caring what
// the algorithm chose — only whether it chose the same thing twice.
std::vector<float> run_scene(const std::map<std::string, std::string, std::less<>> &params,
                             int frames) {
    auto trk = TrackerFactory::createTracker(TRACKER);
    auto p = params;
    trk->init(p);

    std::vector<float> out;
    for (int f = 0; f < frames; ++f) {
        auto ds = moving_scene(f);
        if (ds.empty())
            continue;   // gst-dxtracker skips the tracker on empty frames
        for (const auto &row : trk->update(make_dets(ds)))
            for (Eigen::Index c = 0; c < row.size(); ++c)
                out.push_back(row(c));
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------------------
// A box a tracker publishes is consumed downstream as the object's position: it
// is drawn on the frame, it is written to the message payload, and rules test it
// against zones an operator drew. A zero-area box at the image origin is none of
// those things, and nothing downstream can tell it apart from a real detection
// in the corner.
//
// The assertion is deliberately weak — positive area and finite coordinates —
// because the strong version ("the box should have been HERE") is the
// algorithm's business. Weak is enough: it fails on the defect, and no correct
// tracker can fail it.
// ---------------------------------------------------------------------------
GST_START_TEST(TO_emitted_boxes_are_real) {
    auto trk = TrackerFactory::createTracker(TRACKER);
    auto params = tracker_params();
    trk->init(params);

    int emitted = 0, degenerate = 0, at_origin = 0;

    // The first frames matter most: while `frame_count <= min_hits` a tracker
    // publishes tracks that have not been matched yet, which is exactly the
    // state an uninitialised `last_observation` describes.
    for (int f = 0; f < 60; ++f) {
        auto ds = moving_scene(f);
        if (ds.empty())
            continue;
        for (const auto &row : trk->update(make_dets(ds))) {
            fail_unless(row.size() >= 4, "a tracker row must carry a box");
            const float x1 = row(0), y1 = row(1), x2 = row(2), y2 = row(3);
            ++emitted;

            fail_unless(std::isfinite(x1) && std::isfinite(y1) &&
                            std::isfinite(x2) && std::isfinite(y2),
                        "frame %d: emitted a non-finite box "
                        "(%.1f,%.1f,%.1f,%.1f)",
                        f, x1, y1, x2, y2);

            if (!(x2 > x1 && y2 > y1))
                ++degenerate;
            if (x1 == 0.f && y1 == 0.f && x2 == 0.f && y2 == 0.f)
                ++at_origin;
        }
    }

    fail_unless(emitted > 0, "the scene produced no tracks at all");
    g_print("[DIAG] rows=%d degenerate=%d all-zero=%d\n", emitted, degenerate,
            at_origin);

    // Reported separately: an all-zero box says "this track has no observation
    // and the sentinel for that is not being honoured", which is a different
    // repair from a box that is merely inverted or empty.
    fail_unless(at_origin == 0,
                "%d of %d emitted boxes were (0,0,0,0) — a track with no "
                "observation is publishing its sentinel as a position",
                at_origin, emitted);
    fail_unless(degenerate == 0,
                "%d of %d emitted boxes had no area", degenerate, emitted);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// A setting the algorithm accepts but ignores is worse than one it rejects: the
// operator who raises it believes they changed something. This check does not
// say which way the output should move — only that it must move.
//
// Failure here is not always a defect in the parameter itself. It means the code
// path the parameter feeds is unreachable, and the cause can be anywhere along
// that path — for OC_SORT it was the association cost reading the wrong column,
// three files away from where `delta_t` is parsed.
// ---------------------------------------------------------------------------
GST_START_TEST(TO_declared_parameters_are_observable) {
    const int FRAMES = 80;
    const auto base = run_scene(tracker_params(), FRAMES);
    fail_unless(!base.empty(), "the scene produced no tracks at all");
    std::vector<std::string> dead;

    for (const auto &pp : PROBED_PARAMS) {
        auto lo_params = tracker_params();
        auto hi_params = tracker_params();
        lo_params[pp.key] = pp.lo;
        hi_params[pp.key] = pp.hi;

        const auto lo = run_scene(lo_params, FRAMES);
        const auto hi = run_scene(hi_params, FRAMES);

        // Same length and same values means the setting reached nothing that
        // the scene exercises.
        bool identical = (lo.size() == hi.size());
        if (identical)
            for (size_t i = 0; i < lo.size(); ++i)
                if (lo[i] != hi[i]) {
                    identical = false;
                    break;
                }

        // Values, not rows: the row width is the algorithm's business and this
        // file does not assume one.
        g_print("[DIAG] %-14s %-6s vs %-6s: %zu vs %zu values, %s\n", pp.key,
                pp.lo, pp.hi, lo.size(), hi.size(),
                identical ? "IDENTICAL" : "differs");

        if (identical)
            dead.push_back(pp.key);
    }

    // Report every dead setting, not just the first: they usually share one
    // cause (a term the whole group feeds), and seeing the group is what points
    // at it.
    std::string names;
    for (const auto &d : dead)
        names += (names.empty() ? "" : ", ") + d;
    fail_unless(dead.empty(),
                "%zu setting(s) produced byte-identical output across their "
                "probe values: %s. Either the setting reaches nothing, or "
                "moving_scene does not exercise it — establish which before "
                "changing PROBED_PARAMS",
                dead.size(), names.c_str());
}
GST_END_TEST;

static Suite *tracker_output_contract_suite(void) {
    Suite *s = suite_create("tracker_output_contract");
    TCase *tc = tcase_create("output_contract");
    tcase_set_timeout(tc, 120.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TO_emitted_boxes_are_real);
    tcase_add_test(tc, TO_declared_parameters_are_observable);
    return s;
}

GST_CHECK_MAIN(tracker_output_contract);
