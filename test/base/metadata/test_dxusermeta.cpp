// P1.3 — DXUserMeta contract tests (TC1–TC17)
// Each TC maps 1:1 to contracts C1–C17 in PHASES.md.

#include <gst/check/gstcheck.h>
#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include "gstdxstream/gst-dxusermeta.hpp"
#include "meta_spy.hpp"

using namespace dxtest;

static GstBuffer *fresh_buf() {
    GstBuffer *b = gst_buffer_new_allocate(nullptr, 16, nullptr);
    return dx_create_frame_meta(b);
}

// ---- TC1: acquire defaults (C1) ----
GST_START_TEST(TC1_acquire_defaults) {
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    fail_unless(u != nullptr);
    fail_unless(u->user_meta_data == nullptr);
    fail_unless_equals_int((int)u->user_meta_size, 0);
    fail_unless(u->release_func == nullptr);
    fail_unless(u->copy_func == nullptr);
    fail_unless((int)u->user_meta_type ==
                (int)DXUserMetaType::DX_USER_META_FRAME);
    dx_release_user_meta(u);
}
GST_END_TEST;

// ---- TC2: release(null) noop (C2) ----
GST_START_TEST(TC2_release_null) {
    dx_release_user_meta(nullptr);
}
GST_END_TEST;

// ---- TC3: release calls release_func with data (C3) ----
GST_START_TEST(TC3_release_calls_release_func) {
    reset_spy();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0x10, "x");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    dx_release_user_meta(u);
    fail_unless_equals_int(g_spy_release_count.load(), 1);
    fail_unless(g_spy_last_release_arg.load() == p);
}
GST_END_TEST;

// ---- TC4: release with no release_func does not crash (C4) ----
// Normal set_data enforces release_func, so construct scenario by direct field manipulation
GST_START_TEST(TC4_release_no_release_func) {
    reset_spy();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0, "y");
    u->user_meta_data = p;
    u->release_func = nullptr;
    u->copy_func = nullptr;
    dx_release_user_meta(u);
    fail_unless_equals_int(g_spy_release_count.load(), 0);
    g_free(p);  // manual cleanup (release_func not set, avoid leak)
}
GST_END_TEST;

// ---- TC5: release with null data does not call release_func (C5) ----
GST_START_TEST(TC5_release_null_data) {
    reset_spy();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    u->user_meta_data = nullptr;
    u->release_func = spy_release_cb;
    u->copy_func = spy_copy_cb;
    dx_release_user_meta(u);
    fail_unless_equals_int(g_spy_release_count.load(), 0);
}
GST_END_TEST;

// ---- TC6: set_data on null user_meta returns FALSE (C6) ----
GST_START_TEST(TC6_set_data_null_meta) {
    SpyPayload *p = new_spy_payload(0, "z");
    fail_if(dx_user_meta_set_data(nullptr, p, sizeof(SpyPayload),
                                   DXUserMetaType::DX_USER_META_FRAME,
                                   spy_release_cb, spy_copy_cb));
    g_free(p);
}
GST_END_TEST;

// ---- TC7: set_data with null release or copy func → FALSE (C7) ----
GST_START_TEST(TC7_set_data_null_funcs) {
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0, "z");
    fail_if(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                   DXUserMetaType::DX_USER_META_FRAME,
                                   nullptr, spy_copy_cb));
    fail_if(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                   DXUserMetaType::DX_USER_META_FRAME,
                                   spy_release_cb, nullptr));
    g_free(p);
    dx_release_user_meta(u);
}
GST_END_TEST;

// ---- TC8: set_data basic (C8) ----
GST_START_TEST(TC8_set_data_basic) {
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0xCAFE, "t");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_OBJECT,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(u->user_meta_data == p);
    fail_unless_equals_int((int)u->user_meta_size, (int)sizeof(SpyPayload));
    fail_unless((int)u->user_meta_type ==
                (int)DXUserMetaType::DX_USER_META_OBJECT);
    fail_unless(u->release_func == spy_release_cb);
    fail_unless(u->copy_func == spy_copy_cb);
    dx_release_user_meta(u);
}
GST_END_TEST;

// ---- TC9: second set_data triggers old-release (C9, critical) ----
GST_START_TEST(TC9_set_data_replaces_old) {
    reset_spy();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p1 = new_spy_payload(1, "first");
    fail_unless(dx_user_meta_set_data(u, p1, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    SpyPayload *p2 = new_spy_payload(2, "second");
    fail_unless(dx_user_meta_set_data(u, p2, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    // first data's release_func must be called inside set_data
    fail_unless_equals_int(g_spy_release_count.load(), 1);
    fail_unless(g_spy_last_release_arg.load() == p1);
    fail_unless(u->user_meta_data == p2);
    dx_release_user_meta(u);
    // final release releases p2 once more
    fail_unless_equals_int(g_spy_release_count.load(), 2);
}
GST_END_TEST;

// ---- TC10: add_user_meta_to_frame null args (C10) ----
GST_START_TEST(TC10_add_frame_null) {
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0, "");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_if(dx_add_user_meta_to_frame(nullptr, u));
    fail_if(dx_add_user_meta_to_frame((DXFrameMeta *)0x1, nullptr));
    dx_release_user_meta(u);
}
GST_END_TEST;

// ---- TC11: add_user_meta_to_frame without funcs → FALSE (C11) ----
GST_START_TEST(TC11_add_frame_no_funcs) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    // add without funcs set → FALSE
    fail_if(dx_add_user_meta_to_frame(fm, u));
    fail_unless_equals_int((int)fm->_frame_user_meta_list.size(), 0);
    dx_release_user_meta(u);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC12: add_user_meta_to_frame normal (C12) ----
GST_START_TEST(TC12_add_frame_normal) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0, "");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_frame(fm, u));
    fail_unless_equals_int((int)fm->_frame_user_meta_list.size(), 1);
    fail_unless(fm->_frame_user_meta_list[0] == u);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC13: add_user_meta_to_obj variants (C13) ----
GST_START_TEST(TC13_add_object_variants) {
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    fail_if(dx_add_user_meta_to_obj(nullptr, u));
    fail_if(dx_add_user_meta_to_obj(o, nullptr));
    // funcs not set → FALSE
    fail_if(dx_add_user_meta_to_obj(o, u));
    fail_unless_equals_int((int)o->_obj_user_meta_list.size(), 0);
    // after funcs set → TRUE
    SpyPayload *p = new_spy_payload(0, "");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_OBJECT,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_obj(o, u));
    fail_unless_equals_int((int)o->_obj_user_meta_list.size(), 1);
    dx_release_obj_meta(o);
}
GST_END_TEST;

// ---- TC14: gst_buffer_copy invokes copy_func (C14) ----
GST_START_TEST(TC14_copy_func_invocation) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0x1111, "src");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_frame(fm, u));

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dfm = dx_get_frame_meta(dup);
    fail_unless_equals_int(g_spy_copy_count.load(), 1);
    fail_unless(g_spy_last_copy_src.load() == p);
    fail_unless(dfm->_frame_user_meta_list[0]->user_meta_data ==
                g_spy_last_copy_result.load());
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC15: dst's release_func == src's; data ptr differs (C15) ----
GST_START_TEST(TC15_dst_independence) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(7, "src");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_frame(fm, u));

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dfm = dx_get_frame_meta(dup);
    DXUserMeta *du = dfm->_frame_user_meta_list[0];
    fail_unless(du->release_func == spy_release_cb);
    fail_unless(du->copy_func == spy_copy_cb);
    fail_unless(du->user_meta_data != p);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC16: release balance: src+dst free → release_count==2 (C16) ----
GST_START_TEST(TC16_release_balance) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0x55, "x");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_frame(fm, u));
    GstBuffer *dup = gst_buffer_copy(buf);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
    fail_unless_equals_int(g_spy_release_count.load(), 2);
}
GST_END_TEST;

// ---- TC17: get_*_user_metas null safe (C17) ----
GST_START_TEST(TC17_get_lists_null_safe) {
    fail_unless(dx_get_frame_user_metas(nullptr) == nullptr);
    fail_unless(dx_get_object_user_metas(nullptr) == nullptr);
}
GST_END_TEST;

static Suite *dxusermeta_suite(void) {
    Suite *s = suite_create("dxusermeta");
    TCase *tc = tcase_create("contract");
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TC1_acquire_defaults);
    tcase_add_test(tc, TC2_release_null);
    tcase_add_test(tc, TC3_release_calls_release_func);
    tcase_add_test(tc, TC4_release_no_release_func);
    tcase_add_test(tc, TC5_release_null_data);
    tcase_add_test(tc, TC6_set_data_null_meta);
    tcase_add_test(tc, TC7_set_data_null_funcs);
    tcase_add_test(tc, TC8_set_data_basic);
    tcase_add_test(tc, TC9_set_data_replaces_old);
    tcase_add_test(tc, TC10_add_frame_null);
    tcase_add_test(tc, TC11_add_frame_no_funcs);
    tcase_add_test(tc, TC12_add_frame_normal);
    tcase_add_test(tc, TC13_add_object_variants);
    tcase_add_test(tc, TC14_copy_func_invocation);
    tcase_add_test(tc, TC15_dst_independence);
    tcase_add_test(tc, TC16_release_balance);
    tcase_add_test(tc, TC17_get_lists_null_safe);
    return s;
}

GST_CHECK_MAIN(dxusermeta);
