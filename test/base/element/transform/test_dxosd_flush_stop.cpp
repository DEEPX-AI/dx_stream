// dxosd FLUSH_STOP handler test — TDD red phase for
// refactor_plans_v2/04_test_design.md §2.6
//
// Defect: 01_defect_report.md §5 P2 (cpp:125-156) — sink_event has no
// FLUSH_STOP branch; _stream_info is only reset on PAUSED→READY state
// change. Contract (CLAUDE.md Part B.1 FLUSH semantics): FLUSH_STOP must
// reset stream-related state, otherwise post-seek frames are drawn against
// stale video_info.
//
// Oracle: after FLUSH_STOP, the _stream_info[0] entry must be gone. We
// verify indirectly: push a buffer with NO upstream CAPS re-push afterwards.
// If FLUSH_STOP cleared stream_info, the OSD has no video_info for
// stream_id=0 → bbox is not drawn → pixels remain at the original 0x80 fill.
// (The existing CE_osd_paused_to_ready_clears_stream_info test masks this
// because it re-sends CAPS via gst_harness_set_src_caps_str after the cycle.)

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "buffer_factory.hpp"
#include "meta_helpers.hpp"
#include <cstring>

using namespace dxtest;

static const char *RGB_CAPS =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";

GST_START_TEST(CE_osd_flush_stop_clears_stream_info) {
    Harness h("dxosd", RGB_CAPS, RGB_CAPS);

    // 1) Prime stream_info[0] via a first buffer (draw must succeed).
    {
        GstBuffer *b = make_video_buffer("RGB", 4, 4, 0);
        DXFrameMeta *fm = make_frame_meta(b, 0, 4, 4, "RGB");
        add_object_to_frame(fm, 0, 0.9f, 0.5f, 0.5f, 3.5f, 3.5f);
        gst_harness_push(h.h, b);
        GstBuffer *out = gst_harness_pull(h.h);
        fail_unless(out != nullptr);
        GstMapInfo m; gst_buffer_map(out, &m, GST_MAP_READ);
        gboolean drew = FALSE;
        for (gsize i = 0; i < m.size; i++) if (m.data[i] != 0x80) { drew = TRUE; break; }
        gst_buffer_unmap(out, &m);
        gst_buffer_unref(out);
        fail_unless(drew, "Prime pass: bbox must draw");
    }

    // 2) Send FLUSH_START + FLUSH_STOP (no CAPS re-push).
    fail_unless(gst_harness_push_event(h.h, gst_event_new_flush_start()));
    fail_unless(gst_harness_push_event(h.h, gst_event_new_flush_stop(TRUE)));

    GstSegment seg;
    gst_segment_init(&seg, GST_FORMAT_TIME);
    gst_harness_push_event(h.h, gst_event_new_segment(&seg));

    // 3) Push a second buffer without re-setting caps. If FLUSH_STOP did its
    //    job, _stream_info is empty and no draw happens.
    {
        GstBuffer *b = make_video_buffer("RGB", 4, 4, 100 * GST_MSECOND);
        DXFrameMeta *fm = make_frame_meta(b, 0, 4, 4, "RGB");
        add_object_to_frame(fm, 0, 0.9f, 0.5f, 0.5f, 3.5f, 3.5f);

        gst_harness_push(h.h, b);
        GstBuffer *out = gst_harness_pull(h.h);
        fail_unless(out != nullptr);
        GstMapInfo m; gst_buffer_map(out, &m, GST_MAP_READ);
        gsize nz = 0;
        for (gsize i = 0; i < m.size; i++) if (m.data[i] != 0x80) nz++;
        gst_buffer_unmap(out, &m);
        gst_buffer_unref(out);
        fail_unless(nz == 0,
                    "dxosd FLUSH_STOP must clear _stream_info — got %zu "
                    "non-fill pixels (drawing happened against stale info)",
                    nz);
    }
}
GST_END_TEST;

static Suite *dxosd_flush_stop_suite(void) {
    Suite *s = suite_create("dxosd_flush_stop");
    TCase *tc = tcase_create("flush");
    tcase_set_timeout(tc, 30.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, CE_osd_flush_stop_clears_stream_info);
    return s;
}

GST_CHECK_MAIN(dxosd_flush_stop);
