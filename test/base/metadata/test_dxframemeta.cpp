// P1.1 — DXFrameMeta contract tests (TC1–TC16)
// Each TC maps 1:1 to contracts C1–C16 in PHASES.md.

#include <gst/check/gstcheck.h>
#include <gst/gst.h>
#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include "gstdxstream/gst-dxusermeta.hpp"
#include "meta_spy.hpp"

#include <cmath>
#include <cstring>

using namespace dxtest;

static GstBuffer *fresh_buf() {
    GstBuffer *b = gst_buffer_new_allocate(nullptr, 16, nullptr);
    return dx_create_frame_meta(b);
}

// ---- TC1: init defaults (C2) ----
GST_START_TEST(TC1_init_defaults) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    fail_unless(fm != nullptr);
    fail_unless_equals_int(fm->_stream_id, -1);
    fail_unless_equals_int(fm->_width, -1);
    fail_unless_equals_int(fm->_height, -1);
    fail_unless_equals_int(fm->_roi[0], -1);
    fail_unless_equals_int(fm->_roi[1], -1);
    fail_unless_equals_int(fm->_roi[2], -1);
    fail_unless_equals_int(fm->_roi[3], -1);
    fail_unless_equals_int(fm->_seg_width, 0);
    fail_unless_equals_int(fm->_seg_height, 0);
    fail_unless_equals_int(fm->_label, -1);
    fail_unless(fm->_label_confidence == 0.0f);
    fail_unless(fm->_format.empty());
    fail_unless(fm->_name.empty());
    fail_unless(fm->_label_name.empty());
    fail_unless_equals_int((int)fm->_object_meta_list.size(), 0);
    fail_unless_equals_int((int)fm->_frame_user_meta_list.size(), 0);
    fail_unless_equals_int((int)fm->_seg_data.size(), 0);
    fail_unless_equals_int((int)fm->_input_tensors.size(), 0);
    fail_unless_equals_int((int)fm->_output_tensors.size(), 0);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC2: create on shared buffer returns new writable buffer (C1) ----
GST_START_TEST(TC2_create_returns_writable) {
    GstBuffer *orig = gst_buffer_new_allocate(nullptr, 16, nullptr);
    GstBuffer *shared = gst_buffer_ref(orig);
    fail_if(gst_buffer_is_writable(orig));
    GstBuffer *out = dx_create_frame_meta(orig);
    fail_if(out == orig, "expected new writable buffer when input is shared");
    fail_unless(dx_get_frame_meta(out) != nullptr);
    fail_if(dx_get_frame_meta(shared), "original (shared) buffer must have no meta");
    gst_buffer_unref(shared);
    gst_buffer_unref(out);
}
GST_END_TEST;

// ---- TC3: double create on same buffer → 2 metas (C3) ----
GST_START_TEST(TC3_double_create) {
    GstBuffer *buf = fresh_buf();
    buf = dx_create_frame_meta(buf);
    fail_unless_equals_int(
        (int)gst_buffer_get_n_meta(buf, DX_FRAME_META_API_TYPE), 2);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC4: get on buffer without meta → NULL (C4) ----
GST_START_TEST(TC4_get_null_on_empty) {
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, 16, nullptr);
    fail_unless(dx_get_frame_meta(buf) == nullptr);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC5: null arg add → FALSE, list unchanged (C5) ----
GST_START_TEST(TC5_add_null_args) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXObjectMeta *obj = dx_acquire_obj_meta_from_pool();
    fail_if(dx_add_obj_meta_to_frame(nullptr, obj));
    fail_if(dx_add_obj_meta_to_frame(fm, nullptr));
    fail_unless_equals_int((int)fm->_object_meta_list.size(), 0);
    dx_release_obj_meta(obj);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC6: same obj added twice → list size==2 (C6) ----
GST_START_TEST(TC6_add_duplicate) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    fail_unless(dx_add_obj_meta_to_frame(fm, o));
    fail_unless(dx_add_obj_meta_to_frame(fm, o));
    fail_unless_equals_int((int)fm->_object_meta_list.size(), 2);
    // avoid double-free: clear list and release once manually
    fm->_object_meta_list.clear();
    dx_release_obj_meta(o);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC7: remove calls obj's user_meta release_func once (C7) ----
GST_START_TEST(TC7_remove_present) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0x1234, "obj-u");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_OBJECT,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_obj(o, u));
    fail_unless(dx_add_obj_meta_to_frame(fm, o));

    fail_unless(dx_remove_obj_meta_from_frame(fm, o));
    fail_unless_equals_int((int)fm->_object_meta_list.size(), 0);
    fail_unless_equals_int(g_spy_release_count.load(), 1);
    fail_unless(g_spy_last_release_arg.load() == p);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC8: remove absent obj → FALSE (C8) ----
GST_START_TEST(TC8_remove_absent) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    fail_if(dx_remove_obj_meta_from_frame(fm, o));
    fail_unless_equals_int((int)fm->_object_meta_list.size(), 0);
    dx_release_obj_meta(o);
    gst_buffer_unref(buf);
}
GST_END_TEST;

// ---- TC9: all scalars/strings/roi copied (C9) ----
GST_START_TEST(TC9_copy_scalars) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    src->_stream_id = 7;
    src->_width = 640; src->_height = 480;
    src->_format = "RGB"; src->_name = "cam-1";
    src->_frame_rate = 29.97f;
    src->_label = 42; src->_label_name = "person"; src->_label_confidence = 0.91f;
    src->_roi[0] = 1; src->_roi[1] = 2; src->_roi[2] = 3; src->_roi[3] = 4;

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless(dst != nullptr);
    fail_unless_equals_int(dst->_stream_id, 7);
    fail_unless_equals_int(dst->_width, 640);
    fail_unless_equals_int(dst->_height, 480);
    fail_unless_equals_string(dst->_format.c_str(), "RGB");
    fail_unless_equals_string(dst->_name.c_str(), "cam-1");
    fail_unless(std::fabs(dst->_frame_rate - 29.97f) < 1e-4f);
    fail_unless_equals_int(dst->_label, 42);
    fail_unless_equals_string(dst->_label_name.c_str(), "person");
    fail_unless(std::fabs(dst->_label_confidence - 0.91f) < 1e-6f);
    fail_unless_equals_int(dst->_roi[0], 1);
    fail_unless_equals_int(dst->_roi[3], 4);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC10: object list deep copy (C10) ----
GST_START_TEST(TC10_copy_objects_deep) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    o->_label = 5; o->_track_id = 99;
    o->_box = {1.0f, 2.0f, 3.0f, 4.0f};
    fail_unless(dx_add_obj_meta_to_frame(src, o));

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless_equals_int((int)dst->_object_meta_list.size(), 1);
    DXObjectMeta *dst_o = dst->_object_meta_list[0];
    fail_if(dst_o == o, "object pointer must be different (deep copy)");
    fail_unless_equals_int(dst_o->_label, 5);
    fail_unless_equals_int(dst_o->_track_id, 99);
    fail_unless(dst_o->_box[2] == 3.0f);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC11: copy_func called once per user_meta + new ptr (C11) ----
GST_START_TEST(TC11_copy_user_metas) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0xAA55, "src");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_FRAME,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_frame(src, u));

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless_equals_int((int)dst->_frame_user_meta_list.size(), 1);
    fail_unless_equals_int(g_spy_copy_count.load(), 1);
    fail_unless(g_spy_last_copy_src.load() == p);
    fail_unless(dst->_frame_user_meta_list[0]->user_meta_data != p);
    fail_unless(dst->_frame_user_meta_list[0]->user_meta_data ==
                g_spy_last_copy_result.load());
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC12: tensors shallow copy (use_count +1) (C12) ----
GST_START_TEST(TC12_copy_tensors_shallow) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    dxs::DXTensors t;
    t._data = std::shared_ptr<void>(g_malloc0(16), g_free);
    src->_input_tensors[0] = t;
    long base_count = t._data.use_count();

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless(dst->_input_tensors.count(0) == 1);
    long after = t._data.use_count();
    fail_unless(after == base_count + 1,
                "tensor shared_ptr use_count must increase by 1 on copy");
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC13: empty seg_data → dst has init defaults (C13) ----
GST_START_TEST(TC13_copy_seg_empty) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    src->_seg_width = 99;
    src->_seg_height = 99;

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless_equals_int((int)dst->_seg_data.size(), 0);
    fail_unless_equals_int(dst->_seg_width, 0);
    fail_unless_equals_int(dst->_seg_height, 0);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC14: non-empty seg_data copy + dims (C14) ----
GST_START_TEST(TC14_copy_seg_nonempty) {
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *src = dx_get_frame_meta(buf);
    src->_seg_data = {0x11, 0x22, 0x33, 0x44};
    src->_seg_width = 2; src->_seg_height = 2;

    GstBuffer *dup = gst_buffer_copy(buf);
    DXFrameMeta *dst = dx_get_frame_meta(dup);
    fail_unless_equals_int((int)dst->_seg_data.size(), 4);
    fail_unless_equals_int((int)dst->_seg_data[0], 0x11);
    fail_unless_equals_int((int)dst->_seg_data[3], 0x44);
    fail_unless_equals_int(dst->_seg_width, 2);
    fail_unless_equals_int(dst->_seg_height, 2);
    gst_buffer_unref(buf);
    gst_buffer_unref(dup);
}
GST_END_TEST;

// ---- TC15: transform skips if dst already has meta (C15) ----
GST_START_TEST(TC15_transform_skips_if_exists) {
    GstBuffer *buf = fresh_buf();
    dx_get_frame_meta(buf)->_stream_id = 11;

    GstBuffer *dst = gst_buffer_new_allocate(nullptr, 16, nullptr);
    dst = dx_create_frame_meta(dst);
    dx_get_frame_meta(dst)->_stream_id = 222;

    gst_buffer_copy_into(dst, buf, GST_BUFFER_COPY_META, 0, -1);

    fail_unless_equals_int(
        (int)gst_buffer_get_n_meta(dst, DX_FRAME_META_API_TYPE), 1);
    fail_unless_equals_int(dx_get_frame_meta(dst)->_stream_id, 222);
    gst_buffer_unref(buf);
    gst_buffer_unref(dst);
}
GST_END_TEST;

// ---- TC16: buffer free → all attached user_meta released (C16) ----
GST_START_TEST(TC16_free_releases_children) {
    reset_spy();
    GstBuffer *buf = fresh_buf();
    DXFrameMeta *fm = dx_get_frame_meta(buf);

    for (int i = 0; i < 2; i++) {
        DXUserMeta *u = dx_acquire_user_meta_from_pool();
        SpyPayload *p = new_spy_payload(i, "frame");
        fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                           DXUserMetaType::DX_USER_META_FRAME,
                                           spy_release_cb, spy_copy_cb));
        fail_unless(dx_add_user_meta_to_frame(fm, u));
    }
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    DXUserMeta *uo = dx_acquire_user_meta_from_pool();
    SpyPayload *po = new_spy_payload(99, "obj");
    fail_unless(dx_user_meta_set_data(uo, po, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_OBJECT,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_obj(o, uo));
    fail_unless(dx_add_obj_meta_to_frame(fm, o));

    gst_buffer_unref(buf);
    fail_unless_equals_int(g_spy_release_count.load(), 3);
}
GST_END_TEST;

static Suite *dxframemeta_suite(void) {
    Suite *s = suite_create("dxframemeta");
    TCase *tc = tcase_create("contract");
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TC1_init_defaults);
    tcase_add_test(tc, TC2_create_returns_writable);
    tcase_add_test(tc, TC3_double_create);
    tcase_add_test(tc, TC4_get_null_on_empty);
    tcase_add_test(tc, TC5_add_null_args);
    tcase_add_test(tc, TC6_add_duplicate);
    tcase_add_test(tc, TC7_remove_present);
    tcase_add_test(tc, TC8_remove_absent);
    tcase_add_test(tc, TC9_copy_scalars);
    tcase_add_test(tc, TC10_copy_objects_deep);
    tcase_add_test(tc, TC11_copy_user_metas);
    tcase_add_test(tc, TC12_copy_tensors_shallow);
    tcase_add_test(tc, TC13_copy_seg_empty);
    tcase_add_test(tc, TC14_copy_seg_nonempty);
    tcase_add_test(tc, TC15_transform_skips_if_exists);
    tcase_add_test(tc, TC16_free_releases_children);
    return s;
}

GST_CHECK_MAIN(dxframemeta);
