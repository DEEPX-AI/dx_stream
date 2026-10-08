// Tracker resource contract — per-iteration cost and retention must not grow
// with the age of a live track.
//
// This is a contract of the `Tracker` interface (init + update, see
// tracker/common/include/Tracker.hpp), not a property of one algorithm: none of
// the checks below reference OC_SORT types. To cover another implementation,
// change TRACKER and the parameter map.
//
// WHY THIS CONTRACT NEEDS A TEST
//
// OC-SORT keeps every observation a track ever made. That is a property of the
// algorithm, not of this port -- the reference implementation does the same and
// its author says so ("This is a ugly way to maintain both self.observations and
// self.history_observations. Bear it for the moment.", noahcao/OC_SORT
// trackers/ocsort_tracker/ocsort.py:129). Harmless on the MOT benchmarks OC-SORT
// was written for, where a clip runs 30-60 seconds.
//
// Not harmless in a 24/7 pipeline, because this port additionally *copied* that
// history -- something the reference never does. A track that is never lost (a
// parked car, a static object in the background) accumulates one observation per
// frame for as long as the process lives, and every copy of it costs more than
// the last. Measured on 16 cameras at 10 fps: after 21.5 hours the pipeline had
// fallen from 160 to 68.2 detections/s (-57%) and edge RSS had grown from 350 MB
// to 1.06 GB, both restored instantly by a restart. Bypassing only the tracker
// under the same load held 160.0/s flat with RSS growth of 0.7 MB/h.
//
// A one-time port review would not have kept this out. The defect is older than
// v2.2.1, and the v3.0.0 tracker rewrite (12 files, +501/-410) went straight past
// it: that rewrite ADDED `KalmanBoxTracker::get_observations()` returning a
// `const&` -- an accessor whose whole point is to avoid copying the map -- and
// still passed its result into a `k_previous_obs()` that took the map by value.
// The refactor got half of it. These checks are what would have caught the rest.
//
// ASSERT ON THE DERIVATIVE, NOT THE ABSOLUTE
//
// "retained bytes < N" is the wrong shape here and would have been useless or a
// permanent false alarm: linear accumulation is the reference's own accepted
// behaviour. Every check below is a ratio, so accepted growth passes and only
// unaccepted growth (copies, super-linear cost) fails.
//
// WHAT EACH CHECK CATCHES (all three verified by building the same test against
// the tracker sources at v3.1.1 and at the fix, changing nothing else)
//
//   per_frame_cost_is_flat        16.21x -> 0.68x   k_previous_obs() taking the
//                                                   observation map by value
//   gaps_do_not_multiply_memory    1.52x -> 0.98x   unfreeze() deep-copying the
//                                                   history into a member, and
//                                                   freeze() never releasing its
//                                                   snapshot
//   flicker_cost_is_flat          16.11x -> 0.57x   freeze() deep-copying the
//                                                   history on every miss->hit
//                                                   cycle
//
// The third one is not redundant: with only the first two defects fixed, both of
// them pass (0.93x and 1.06x) while this one still fails at 3.78x. Cost on the
// gap path is a different code path from cost on the steady path.

// Standard and Eigen headers MUST precede gst/check: check.h defines `fail` as a
// macro, which collides with std::basic_ios::fail once <ios> lands afterwards.
#include <ctime>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// mallinfo2() is glibc >= 2.33. There is no portable way to ask the allocator how
// many bytes are in use, and RSS is too coarse at this size (page granularity and
// arena growth swamp the signal), so the retention check compiles out elsewhere
// rather than being replaced by something weaker.
#if defined(__GLIBC__) &&                                                      \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33))
#define DXTEST_HAVE_HEAP_PROBE 1
#include <malloc.h>
#else
#define DXTEST_HAVE_HEAP_PROBE 0
#endif

#include "tracker/common/include/TrackerFactory.hpp"

#include <gst/check/gstcheck.h>
#include <gst/gst.h>

namespace {

const char *TRACKER = "OC_SORT";

// Production edge configuration (dx_argus edge/configs/tracker_config.json).
// det_thresh 0.4 / max_age 50 differ from the SDK defaults (0.5 / 30); both make
// tracks live longer, so this is the configuration the defect was measured under.
std::map<std::string, std::string, std::less<>> tracker_params() {
    return {{"det_thresh", "0.4"},   {"max_age", "50"},
            {"min_hits", "3"},       {"iou_threshold", "0.3"},
            {"delta_t", "3"},        {"asso_func", "iou"},
            {"inertia", "0.2"},      {"use_byte", "false"}};
}

struct Det {
    float x1, y1, x2, y2;
};

// dets layout expected by Tracker::update — [x1,y1,x2,y2,conf,label,input_idx],
// exactly what gst-dxtracker.cpp:336-345 builds from the object meta list.
Eigen::MatrixXf make_dets(const std::vector<Det> &ds) {
    Eigen::MatrixXf m(static_cast<Eigen::Index>(ds.size()), 7);
    for (size_t i = 0; i < ds.size(); ++i) {
        auto r = static_cast<Eigen::Index>(i);
        m(r, 0) = ds[i].x1;
        m(r, 1) = ds[i].y1;
        m(r, 2) = ds[i].x2;
        m(r, 3) = ds[i].y2;
        m(r, 4) = 0.9f;                    // conf, well above det_thresh
        m(r, 5) = 0.0f;                    // label: person
        m(r, 6) = static_cast<float>(i);   // input_idx
    }
    return m;
}

// Two static objects far enough apart that neither association pass can confuse
// them. "resident" stands for the real-world immortal track (a parked car);
// "companion" keeps every frame non-empty so update() is always called, which is
// what gst-dxtracker.cpp does — it skips the tracker entirely on frames with no
// objects, so a frame with zero detections is not a case this element sees.
const Det RESIDENT = {100.f, 100.f, 180.f, 260.f};
const Det COMPANION = {600.f, 100.f, 680.f, 260.f};

// drop_*: this frame omits that detection, so its track goes unmatched and the
// filter freezes; the next frame re-matches it and unfreezes. That miss->hit
// cycle is the only path that reaches the freeze/unfreeze bookkeeping.
std::vector<Det> scene(bool drop_resident, bool drop_companion) {
    std::vector<Det> ds;
    if (!drop_resident)
        ds.push_back(RESIDENT);
    if (!drop_companion)
        ds.push_back(COMPANION);
    return ds;
}

// Processor time, not wall clock: the ratio must not depend on what else the
// machine is doing. std::clock is used rather than clock_gettime so this builds
// on MSVC too; CLOCKS_PER_SEC is 1e6 on glibc and 1e3 on Windows, and the windows
// below are sized so even the coarser tick is enough to separate 1x from 3x.
double cpu_seconds() {
    return static_cast<double>(std::clock()) / CLOCKS_PER_SEC;
}

#if DXTEST_HAVE_HEAP_PROBE
size_t heap_in_use() { return mallinfo2().uordblks; }
#endif

// Feed `frames` frames and return the CPU seconds spent on the window
// [from, from + window).
void windowed_cost(int frames, int window, int early_from, int late_from,
                   int flicker_period, int gap_len, double *out_early,
                   double *out_late) {
    auto trk = TrackerFactory::createTracker(TRACKER);
    auto params = tracker_params();
    trk->init(params);

    double mark = 0.0;
    for (int f = 0; f < frames; ++f) {
        if (f == early_from || f == late_from)
            mark = cpu_seconds();
        bool drop = flicker_period > 0 && (f % flicker_period) < gap_len;
        trk->update(make_dets(scene(drop, false)));
        if (f == early_from + window - 1)
            *out_early = cpu_seconds() - mark;
        if (f == late_from + window - 1)
            *out_late = cpu_seconds() - mark;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// One immortal track, no gaps. k_previous_obs() is called once per live track per
// frame (OCSort.cpp:152). Taking the observation map by value copies the entire
// history on every one of those calls, so cost per frame grows linearly with the
// track's age and the total is quadratic. The reference passes a dict, i.e. a
// reference (ocsort.py:11), and is flat.
// ---------------------------------------------------------------------------
GST_START_TEST(TR_per_frame_cost_is_flat) {
    const int FRAMES = 12000;
    const int WINDOW = 1000;
    const int EARLY_FROM = 200;   // skip warm-up
    const int LATE_FROM = FRAMES - WINDOW;

    double early = 0.0, late = 0.0;
    windowed_cost(FRAMES, WINDOW, EARLY_FROM, LATE_FROM, 0, 0, &early, &late);

    double ratio = (early > 0.0) ? late / early : 0.0;
    g_print("[DIAG] per-frame cost: early(%d..%d)=%.1f us  late(%d..%d)=%.1f us  "
            "ratio=%.2fx\n", EARLY_FROM, EARLY_FROM + WINDOW,
            1e6 * early / WINDOW, LATE_FROM, FRAMES, 1e6 * late / WINDOW, ratio);

    fail_unless(early > 0.0, "CPU clock produced no measurable early window");
    fail_unless(ratio <= 3.0,
                "per-frame tracker cost grew %.2fx between frames %d and %d of a "
                "single track's life (limit 3.0x) — the observation history is "
                "being copied per frame instead of passed by reference "
                "(Utilities.cpp k_previous_obs; upstream ocsort.py:11 takes the "
                "dict by reference)",
                ratio, EARLY_FROM, LATE_FROM);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// Same measurement, but the resident detection is missing every 4th frame: a
// marginal detection that drops in and out is ordinary, and each miss->hit cycle
// runs freeze() + unfreeze(). freeze() snapshots the observation history, so if
// that snapshot is an O(history) copy the cost per frame grows with track age
// even once the per-frame copy is gone. The reference pays O(history) here too,
// but as a list-of-references copy (kalmanfilter.py:407) — two orders of
// magnitude cheaper per element than copying Eigen vectors.
// ---------------------------------------------------------------------------
GST_START_TEST(TR_flicker_cost_is_flat) {
    const int FRAMES = 12000;
    const int WINDOW = 1000;
    const int EARLY_FROM = 200;
    const int LATE_FROM = FRAMES - WINDOW;
    const int FLICKER = 4;

    double early = 0.0, late = 0.0;
    windowed_cost(FRAMES, WINDOW, EARLY_FROM, LATE_FROM, FLICKER, 1, &early,
                  &late);

    double ratio = (early > 0.0) ? late / early : 0.0;
    g_print("[DIAG] flicker cost (gap every %d frames): early=%.1f us  "
            "late=%.1f us  ratio=%.2fx\n", FLICKER, 1e6 * early / WINDOW,
            1e6 * late / WINDOW, ratio);

    fail_unless(early > 0.0, "CPU clock produced no measurable early window");
    fail_unless(ratio <= 3.0,
                "per-frame cost grew %.2fx over a flickering track's life "
                "(limit 3.0x) — the freeze() snapshot is copying the whole "
                "observation history on every miss->hit cycle "
                "(KalmanFilter.cpp freeze)",
                ratio);
}
GST_END_TEST;

// ---------------------------------------------------------------------------
// Same length, same two tracks; run B additionally drops each detection once
// every 40 frames (offset so every frame still carries one object). Only run B
// reaches freeze()/unfreeze(). The reference leaves nothing behind there: the
// history it reads is a local alias (kalmanfilter.py:421) and the snapshot is
// released (:425), so B must retain no more than A — in fact slightly less,
// because a missed frame appends no observation.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Same measurement again, but the gap is LONGER THAN delta_t.
//
// This is the case the two checks above cannot reach, and missing it is why a
// build that passed all of them still lost 22% of its throughput over 46 hours
// in production. `k_previous_obs()` answers from `observations` in O(1) only
// while the track was seen within the last delta_t frames (Utilities.cpp); past
// that it falls through to a `std::max_element` over the whole map. Neither
// check above ever produces more than one consecutive miss, so both stay on the
// fast path forever and the fall-through is never measured.
//
// A gap longer than delta_t is ordinary: max_age (50 here) is precisely the
// number of consecutive misses a track is allowed before it is dropped, so
// every occlusion between delta_t+1 and max_age frames lands here.
//
// The gap is derived from delta_t rather than hardcoded — delta_t is a user
// setting (edge/configs/tracker_config.json), and a fixed number would silently
// stop testing this path the moment someone raises it.
// ---------------------------------------------------------------------------
GST_START_TEST(TR_cost_is_flat_across_gaps_longer_than_delta_t) {
    const int FRAMES = 30000;
    const int WINDOW = 2000;
    const int EARLY_FROM = 200;
    const int LATE_FROM = FRAMES - WINDOW;
    const int delta_t = std::stoi(tracker_params()["delta_t"]);
    const int GAP = delta_t + 7;       // > delta_t, well under max_age
    const int PERIOD = GAP + 40;

    double early = 0.0, late = 0.0;
    windowed_cost(FRAMES, WINDOW, EARLY_FROM, LATE_FROM, PERIOD, GAP, &early,
                  &late);

    double ratio = (early > 0.0) ? late / early : 0.0;
    g_print("[DIAG] long-gap cost (gap=%d > delta_t=%d, every %d): early=%.1f us  "
            "late=%.1f us  ratio=%.2fx\n", GAP, delta_t, PERIOD,
            1e6 * early / WINDOW, 1e6 * late / WINDOW, ratio);

    fail_unless(early > 0.0, "CPU clock produced no measurable early window");
    fail_unless(ratio <= 3.0,
                "per-frame cost grew %.2fx over a track's life when gaps exceed "
                "delta_t (limit 3.0x) — k_previous_obs() is falling through to a "
                "full scan of an observation map that grows with track age "
                "(Utilities.cpp max_element; the map must be trimmed to the keys "
                "its readers use, KalmanBoxTracker::update)",
                ratio);
}
GST_END_TEST;

#if DXTEST_HAVE_HEAP_PROBE
// ---------------------------------------------------------------------------
// Retention must not grow with a track's age.
//
// This check replaced an earlier one that compared retention *with gaps* against
// retention *without* them and allowed 1.15x. That shape was chosen while
// unbounded accumulation was still accepted as "the reference does it too", so
// only the ratio between the two runs could say anything. That premise is gone:
// `observations` is now trimmed to the last delta_t entries and `history_obs` to
// a fixed tail, both justified by enumerating every reader, so retention is
// O(1) in track age. Against a constant, the old ratio compares two small
// numbers whose difference is just the in-flight gap buffer -- it reports 1.56x
// while the absolute retention fell ~100x, i.e. it now fires on an improvement.
//
// The replacement asserts the property that actually matters for a 24/7
// pipeline and is strictly stronger: quadruple the frames, and retention must
// not quadruple. It still catches what the old one caught -- a deep copy in
// freeze()/unfreeze() makes the gapped run grow with age -- and it additionally
// catches plain unbounded accumulation, which the old one accepted by design.
// Verified to FAIL on the shipped build (linear retention) before being adopted.
// ---------------------------------------------------------------------------
GST_START_TEST(TR_retention_does_not_grow_with_track_age) {
    const int SHORT = 4000;
    const int LONG = 16000;   // 4x
    const int PERIOD = 40;

    auto run = [&](int frames, bool with_gaps) -> size_t {
        size_t before = heap_in_use();
        size_t retained = 0;
        {
            auto trk = TrackerFactory::createTracker(TRACKER);
            auto params = tracker_params();
            trk->init(params);
            for (int f = 0; f < frames; ++f) {
                bool drop_res = with_gaps && (f % PERIOD == 0);
                bool drop_comp = with_gaps && (f % PERIOD == PERIOD / 2);
                trk->update(make_dets(scene(drop_res, drop_comp)));
            }
            // Measured while the tracker is alive: this is per-track retention,
            // not a leak — a dying track frees everything with its unique_ptr.
            // Clamped rather than wrapped: these are unsigned, and a silent huge
            // value would make the ratio meaningless while still passing the
            // assertion below. 0 fails loudly instead.
            size_t after = heap_in_use();
            retained = (after > before) ? after - before : 0;
        }
        return retained;
    };

    // Long first: the allocator is warmer for the short run, so any bias from
    // arena growth works against the assertion rather than for it.
    size_t long_gap = run(LONG, true);
    size_t short_gap = run(SHORT, true);
    size_t long_flat = run(LONG, false);
    size_t short_flat = run(SHORT, false);

    double r_flat = (short_flat > 0)
                        ? static_cast<double>(long_flat) / short_flat : 0.0;
    double r_gap = (short_gap > 0)
                       ? static_cast<double>(long_gap) / short_gap : 0.0;

    g_print("[DIAG] retention vs age (%dx frames): no-gap %zu->%zu B (%.2fx)  "
            "gap-every-%d %zu->%zu B (%.2fx)\n",
            LONG / SHORT, short_flat, long_flat, r_flat, PERIOD, short_gap,
            long_gap, r_gap);

    fail_unless(short_flat > 0 && short_gap > 0,
                "short runs retained nothing — measurement is broken");
    // 4x the frames. Linear accumulation lands at 4.0x; a bound lands at 1.0x.
    fail_unless(r_flat <= 1.5,
                "retention grew %.2fx when the track lived %dx longer (limit "
                "1.50x): %zu B at %d frames vs %zu B at %d frames — the "
                "observation history is accumulating with track age instead of "
                "being trimmed to what its readers use",
                r_flat, LONG / SHORT, short_flat, SHORT, long_flat, LONG);
    fail_unless(r_gap <= 1.5,
                "with miss->hit cycles, retention grew %.2fx over a %dx longer "
                "life (limit 1.50x): %zu B vs %zu B — freeze()/unfreeze() are "
                "keeping history that upstream releases (kalmanfilter.py:421 "
                "aliases the list, :425 releases the snapshot)",
                r_gap, LONG / SHORT, short_gap, long_gap);
}
GST_END_TEST;
#endif

#if !DXTEST_HAVE_HEAP_PROBE
// Report the skip from inside a test, where the runner captures output. Printing
// it from the suite builder can be dropped or reordered by the framework.
GST_START_TEST(TR_retention_check_unavailable) {
    g_print("[INFO] retention check skipped: needs mallinfo2 (glibc >= 2.33)\n");
}
GST_END_TEST;
#endif

static Suite *tracker_resource_contract_suite(void) {
    Suite *s = suite_create("tracker_resource_contract");
    TCase *tc = tcase_create("resource_contract");
    tcase_set_timeout(tc, 300.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TR_per_frame_cost_is_flat);
    tcase_add_test(tc, TR_flicker_cost_is_flat);
    tcase_add_test(tc, TR_cost_is_flat_across_gaps_longer_than_delta_t);
#if DXTEST_HAVE_HEAP_PROBE
    tcase_add_test(tc, TR_retention_does_not_grow_with_track_age);
#else
    tcase_add_test(tc, TR_retention_check_unavailable);
#endif
    return s;
}

GST_CHECK_MAIN(tracker_resource_contract);
