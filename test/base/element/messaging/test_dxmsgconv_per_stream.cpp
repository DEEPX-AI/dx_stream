// dxmsgconv per-stream seq_id test — TDD red phase for
// refactor_plans_v2/04_test_design.md §1.4
//
// Defect: 01_defect_report.md §11 P1 — `_seq_id` is element-global. In
// DOMAIN_MODE with multiple streams (dxinputselector → dxmsgconv), all
// streams share a single counter and produce interleaved sequence IDs
// (stream0=[0,2,4...], stream1=[1,3,5...]) instead of independent
// [0,1,2,...] per stream.
//
// Oracle: push interleaved buffers s0,s1,s0,s1. Parse output payload JSON
// (default library produces {"streamId":N,"seqId":M,...}). Per-stream
// expected: stream0 first seqId == 0 AND stream1 first seqId == 0.
// Currently stream1 first seqId == 1.

#include <gst/check/gstcheck.h>
#include <gst/check/gstharness.h>
#include <gst/gst.h>
#include "harness_helpers.hpp"
#include "meta_helpers.hpp"
#include "npu_env.hpp"
#include <cstring>
#include <string>
#include <vector>
#include <map>

using namespace dxtest;

static const char *CAPS_STR =
    "video/x-raw,format=RGB,width=4,height=4,framerate=30/1";
static std::string MSGCONV_LIB_PATH() {
    return dxtest::resolve_lib_path("libdx_msgconvl.so");
}
#define MSGCONV_LIB (MSGCONV_LIB_PATH().c_str())

static GstBuffer *make_buf_with_meta(GstClockTime pts, int stream_id) {
    gsize sz = 4 * 4 * 3;
    GstBuffer *b = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo m; gst_buffer_map(b, &m, GST_MAP_WRITE);
    std::memset(m.data, 0x80, sz); gst_buffer_unmap(b, &m);
    GST_BUFFER_PTS(b) = pts;
    GST_BUFFER_DURATION(b) = GST_SECOND / 30;
    DXFrameMeta *fm = make_frame_meta(b, stream_id, 4, 4);
    add_object_to_frame(fm, 1, 0.9f, 0, 0, 1, 1);
    return b;
}

// Extract integer value of "<key>": from a JSON snippet (first match).
static int find_int_field(const std::string &json, const char *key) {
    std::string needle = std::string("\"") + key + "\"";
    auto p = json.find(needle);
    if (p == std::string::npos) return -1;
    p = json.find(':', p);
    if (p == std::string::npos) return -1;
    p++;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t')) p++;
    int sign = 1;
    if (p < json.size() && json[p] == '-') { sign = -1; p++; }
    int val = 0;
    bool any = false;
    while (p < json.size() && json[p] >= '0' && json[p] <= '9') {
        val = val * 10 + (json[p] - '0'); p++; any = true;
    }
    return any ? sign * val : -1;
}

GST_START_TEST(PS_msgconv_per_stream_seq_id) {
    GstElement *e = gst_element_factory_make("dxmsgconv", nullptr);
    g_object_set(e, "library-file-path", MSGCONV_LIB, nullptr);
    GstHarness *h = gst_harness_new_with_element(e, "sink", "src");
    fail_unless(h != nullptr);
    gst_harness_set_src_caps_str(h, CAPS_STR);

    // Interleave: s0, s1, s0, s1
    std::vector<int> order = {0, 1, 0, 1};
    for (size_t i = 0; i < order.size(); i++) {
        GstBuffer *b = make_buf_with_meta((i * GST_SECOND) / 30, order[i]);
        fail_unless_equals_int(gst_harness_push(h, b), GST_FLOW_OK);
    }

    std::map<int, std::vector<int>> seqs_by_stream;
    for (size_t i = 0; i < order.size(); i++) {
        GstBuffer *out = gst_harness_pull(h);
        fail_unless(out != nullptr);
        GstDxMsgMeta *mm = (GstDxMsgMeta *)gst_buffer_get_meta(
            out, gst_dxmsg_meta_api_get_type());
        fail_unless(mm != nullptr, "buffer %zu missing DxMsgMeta", i);
        DxMsgPayload *pl = (DxMsgPayload *)mm->_payload;
        fail_unless(pl && pl->_data && pl->_size > 0);
        std::string json((const char *)pl->_data, pl->_size);
        int sid = find_int_field(json, "streamId");
        int seq = find_int_field(json, "seqId");
        fail_unless(sid >= 0 && seq >= 0,
                    "payload parse fail: %s", json.c_str());
        seqs_by_stream[sid].push_back(seq);
        gst_buffer_unref(out);
    }

    fail_unless(seqs_by_stream[0].size() == 2);
    fail_unless(seqs_by_stream[1].size() == 2);

    int s0_gap = seqs_by_stream[0][1] - seqs_by_stream[0][0];
    int s1_gap = seqs_by_stream[1][1] - seqs_by_stream[1][0];

    fail_unless(s0_gap == 1 && s1_gap == 1,
                "per-stream consecutive seqIds must differ by 1 — "
                "got stream0=[%d,%d] gap=%d, stream1=[%d,%d] gap=%d "
                "(element-global counter interleaves sequences)",
                seqs_by_stream[0][0], seqs_by_stream[0][1], s0_gap,
                seqs_by_stream[1][0], seqs_by_stream[1][1], s1_gap);

    gst_harness_teardown(h);
    gst_object_unref(e);
}
GST_END_TEST;

static Suite *dxmsgconv_per_stream_suite(void) {
    Suite *s = suite_create("dxmsgconv_per_stream");
    TCase *tc = tcase_create("per_stream");
    tcase_set_timeout(tc, 15.0);
    suite_add_tcase(s, tc);
    tcase_add_test(tc, PS_msgconv_per_stream_seq_id);
    return s;
}

GST_CHECK_MAIN(dxmsgconv_per_stream);
