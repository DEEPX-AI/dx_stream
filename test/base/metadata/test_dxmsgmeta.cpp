// P1.4 — GstDxMsgMeta contract tests (TC1–TC6)
// Maps contracts C1–C9 from PHASES.md to TC1–TC6.

#include <gst/check/gstcheck.h>
#include "gstdxstream/gst-dxmsgmeta.hpp"

#include <cstring>
#include <string>

static GstBuffer *fresh_buf() {
    return gst_buffer_new_allocate(nullptr, 16, nullptr);
}

static void fill_payload(DxMsgPayload *p, const std::string &json) {
    p->_size = (guint)json.size();
    p->_data = g_memdup(json.data(), p->_size);
}

// ---- TC1: create + get basic (C1, C2) ----
GST_START_TEST(TC1_create_get_basic) {
    GstBuffer *empty = fresh_buf();
    fail_unless(dx_get_msg_meta(empty) == nullptr);
    GstBuffer *buf = dx_create_msg_meta(empty);
    GstDxMsgMeta *m = dx_get_msg_meta(buf);
    fail_unless(m != nullptr);
    fail_unless(m->_payload == nullptr);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC2: double create → n_meta == 2 (C3) ----
GST_START_TEST(TC2_double_create) {
    GstBuffer *buf = dx_create_msg_meta(fresh_buf());
    buf = dx_create_msg_meta(buf);
    fail_unless_equals_int(
        (int)gst_buffer_get_n_meta(buf, GST_DXMSG_META_API_TYPE), 2);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC3: add_payload deep copy (C4, C5) ----
GST_START_TEST(TC3_add_payload_deep_copy) {
    GstBuffer *buf = fresh_buf();
    DxMsgPayload p;
    std::string json = "{\"k\":\"v\"}";
    fill_payload(&p, json);
    dx_add_payload_to_buffer(buf, &p);

    GstDxMsgMeta *m = dx_get_msg_meta(buf);
    fail_unless(m != nullptr);
    fail_unless(m->_payload != nullptr);
    auto *stored = (DxMsgPayload *)m->_payload;
    fail_unless(stored->_data != p._data, "must be deep copy (different ptr)");
    fail_unless_equals_int((int)stored->_size, (int)p._size);
    fail_unless(std::memcmp(stored->_data, p._data, p._size) == 0);

    // modifying original bytes must not affect stored meta content
    ((char *)p._data)[0] = '!';
    fail_if(((char *)stored->_data)[0] == '!',
            "stored payload must be independent of original buffer");

    g_free(p._data);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC4: transform deep copy (C6) ----
GST_START_TEST(TC4_transform_deep_copy) {
    GstBuffer *src = fresh_buf();
    DxMsgPayload p;
    std::string json = "{\"a\":1}";
    fill_payload(&p, json);
    dx_add_payload_to_buffer(src, &p);
    g_free(p._data);

    GstBuffer *dup = gst_buffer_copy(src);
    GstDxMsgMeta *sm = dx_get_msg_meta(src);
    GstDxMsgMeta *dm = dx_get_msg_meta(dup);
    fail_unless(dm != nullptr);
    auto *sp = (DxMsgPayload *)sm->_payload;
    auto *dp = (DxMsgPayload *)dm->_payload;
    fail_unless(sp->_data != dp->_data, "transform must deep-copy payload");
    fail_unless_equals_int((int)dp->_size, (int)sp->_size);
    fail_unless(std::memcmp(sp->_data, dp->_data, sp->_size) == 0);

    gst_buffer_unref(src);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC5: transform with null payload → dst null (C7) ----
GST_START_TEST(TC5_transform_null_payload) {
    GstBuffer *src = dx_create_msg_meta(fresh_buf());
    fail_unless(dx_get_msg_meta(src)->_payload == nullptr);
    GstBuffer *dup = gst_buffer_copy(src);
    GstDxMsgMeta *dm = dx_get_msg_meta(dup);
    fail_unless(dm != nullptr);
    fail_unless(dm->_payload == nullptr);
    gst_buffer_unref(src);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC6 (known issue pin): calling add_payload twice on the same buffer
//      keeps 1 meta and overwrites _payload with the second (first payload leaks). ----
GST_START_TEST(TC6_double_add_pin) {
    GstBuffer *buf = fresh_buf();
    DxMsgPayload p1, p2;
    std::string j1 = "first", j2 = "SECOND";
    fill_payload(&p1, j1);
    fill_payload(&p2, j2);

    dx_add_payload_to_buffer(buf, &p1);
    void *first_stored = ((DxMsgPayload *)dx_get_msg_meta(buf)->_payload)->_data;
    dx_add_payload_to_buffer(buf, &p2);

    // Current implementation pin:
    //   - dx_create_msg_meta adds a new meta each time → n_meta == 2
    //   - dx_get_msg_meta returns the first meta (gst_buffer_get_meta behavior)
    //   - On second add, dx_add_payload_to_buffer overwrites first meta's _payload
    //     → first payload becomes dangling and leaks.
    //   - This is a known bug pinned here. Update this TC if implementation is fixed.
    int n = (int)gst_buffer_get_n_meta(buf, GST_DXMSG_META_API_TYPE);
    fail_unless(n == 2,
                "current impl creates a 2nd meta on double add_payload");

    auto *first_meta_payload = (DxMsgPayload *)dx_get_msg_meta(buf)->_payload;
    fail_unless(first_meta_payload != nullptr);
    // known leak: first payload pointer is overwritten by second call
    fail_if(first_meta_payload->_data == first_stored,
            "known leak pin: first meta's _payload is overwritten on 2nd add");

    g_free(p1._data);
    g_free(p2._data);
    gst_buffer_unref(buf);
}
GST_END_TEST;

static Suite *dxmsgmeta_suite(void) {
    Suite *s = suite_create("dxmsgmeta");
    TCase *tc = tcase_create("contract");
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TC1_create_get_basic);
    tcase_add_test(tc, TC2_double_create);
    tcase_add_test(tc, TC3_add_payload_deep_copy);
    tcase_add_test(tc, TC4_transform_deep_copy);
    tcase_add_test(tc, TC5_transform_null_payload);
    tcase_add_test(tc, TC6_double_add_pin);
    return s;
}

GST_CHECK_MAIN(dxmsgmeta);
