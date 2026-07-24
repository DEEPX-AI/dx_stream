// P1.2 — DXObjectMeta contract tests (TC1–TC11)
// Each TC maps 1:1 to contracts C1–C11 in PHASES.md.

#include <gst/check/gstcheck.h>
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include "gstdxstream/gst-dxusermeta.hpp"
#include "meta_spy.hpp"

#include <cmath>
#include <set>

using namespace dxtest;

// ---- TC1: acquire defaults (C1) ----
GST_START_TEST(TC1_acquire_defaults) {
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    fail_unless(o != nullptr);
    fail_if(o->_meta_id == 0, "meta_id must be non-zero (UUID-derived)");
    fail_unless_equals_int(o->_track_id, -1);
    fail_unless_equals_int(o->_label, -1);
    fail_unless(std::fabs(o->_confidence - (-1.0f)) < 1e-6f);
    fail_unless(o->_label_name.empty());
    for (int i = 0; i < 4; i++) {
        fail_unless(o->_box[i] == 0.0f);
        fail_unless(o->_face_box[i] == 0.0f);
    }
    fail_unless(std::fabs(o->_face_confidence - (-1.0f)) < 1e-6f);
    fail_unless_equals_int(o->_seg_width, 0);
    fail_unless_equals_int(o->_seg_height, 0);
    fail_unless((int)o->_keypoints.size() == 0);
    fail_unless((int)o->_body_feature.size() == 0);
    fail_unless((int)o->_obb.size() == 0);
    fail_unless((int)o->_face_landmarks.size() == 0);
    fail_unless((int)o->_face_feature.size() == 0);
    fail_unless((int)o->_seg_data.size() == 0);
    fail_unless((int)o->_obj_user_meta_list.size() == 0);
    fail_unless((int)o->_input_tensors.size() == 0);
    fail_unless((int)o->_output_tensors.size() == 0);
    dx_release_obj_meta(o);
}
GST_END_TEST;

// ---- TC2: meta_id uniqueness (C2) ----
GST_START_TEST(TC2_meta_id_unique) {
    std::set<int> ids;
    const int N = 100;
    std::vector<DXObjectMeta *> objs;
    for (int i = 0; i < N; i++) {
        DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
        ids.insert(o->_meta_id);
        objs.push_back(o);
    }
    for (auto *o : objs) dx_release_obj_meta(o);
    // >= 95% unique
    fail_unless((int)ids.size() >= 95,
                "meta_id uniqueness too low (<95/100)");
}
GST_END_TEST;

// ---- TC3: release(null) noop (C3) ----
GST_START_TEST(TC3_release_null) {
    dx_release_obj_meta(nullptr);  // pass if no crash
}
GST_END_TEST;

// ---- TC4: copy pins _meta_id (intentional, C4) ----
GST_START_TEST(TC4_copy_pins_meta_id) {
    DXObjectMeta *src = dx_acquire_obj_meta_from_pool();
    DXObjectMeta *dst = dx_acquire_obj_meta_from_pool();
    int src_id = src->_meta_id;
    int dst_id_before = dst->_meta_id;
    fail_if(src_id == dst_id_before, "precondition: ids differ before copy");
    dx_copy_obj_meta(src, dst);
    // intentional pinning: current implementation copies src->_meta_id as-is
    fail_unless_equals_int(dst->_meta_id, src_id);
    dx_release_obj_meta(src);
    dx_release_obj_meta(dst);
}
GST_END_TEST;

// ---- TC5: scalars copied (C5) ----
GST_START_TEST(TC5_copy_scalars) {
    DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
    s->_track_id = 11; s->_label = 22; s->_confidence = 0.75f;
    s->_label_name = "dog";
    s->_box = {1.f, 2.f, 3.f, 4.f};
    s->_face_box = {5.f, 6.f, 7.f, 8.f};
    s->_face_confidence = 0.5f;

    DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
    dx_copy_obj_meta(s, d);
    fail_unless_equals_int(d->_track_id, 11);
    fail_unless_equals_int(d->_label, 22);
    fail_unless(std::fabs(d->_confidence - 0.75f) < 1e-6f);
    fail_unless_equals_string(d->_label_name.c_str(), "dog");
    for (int i = 0; i < 4; i++) {
        fail_unless(d->_box[i] == s->_box[i]);
        fail_unless(d->_face_box[i] == s->_face_box[i]);
    }
    fail_unless(std::fabs(d->_face_confidence - 0.5f) < 1e-6f);
    dx_release_obj_meta(s);
    dx_release_obj_meta(d);
}
GST_END_TEST;

// ---- TC6: vector fields copied (C6) ----
GST_START_TEST(TC6_copy_vectors) {
    DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
    s->_keypoints = {0.1f, 0.2f, 0.3f};
    s->_body_feature = {1.f, 2.f};
    s->_obb = {1, 2, 3, 4, 0.5f};
    s->_face_landmarks = {10.f, 20.f};
    s->_face_feature = {0.9f};

    DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
    dx_copy_obj_meta(s, d);
    fail_unless_equals_int((int)d->_keypoints.size(), 3);
    fail_unless(d->_keypoints[1] == 0.2f);
    fail_unless_equals_int((int)d->_body_feature.size(), 2);
    fail_unless_equals_int((int)d->_obb.size(), 5);
    fail_unless(d->_obb[4] == 0.5f);
    fail_unless_equals_int((int)d->_face_landmarks.size(), 2);
    fail_unless_equals_int((int)d->_face_feature.size(), 1);
    dx_release_obj_meta(s);
    dx_release_obj_meta(d);
}
GST_END_TEST;

// ---- TC7: seg empty/non-empty branches (C7) ----
GST_START_TEST(TC7_copy_seg_branches) {
    // empty branch
    {
        DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
        s->_seg_width = 77; s->_seg_height = 77;  // should not be copied when empty
        DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
        dx_copy_obj_meta(s, d);
        fail_unless_equals_int((int)d->_seg_data.size(), 0);
        fail_unless_equals_int(d->_seg_width, 0);
        fail_unless_equals_int(d->_seg_height, 0);
        dx_release_obj_meta(s);
        dx_release_obj_meta(d);
    }
    // non-empty branch
    {
        DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
        s->_seg_data = {0xAB, 0xCD};
        s->_seg_width = 1; s->_seg_height = 2;
        DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
        dx_copy_obj_meta(s, d);
        fail_unless_equals_int((int)d->_seg_data.size(), 2);
        fail_unless_equals_int((int)d->_seg_data[0], 0xAB);
        fail_unless_equals_int(d->_seg_width, 1);
        fail_unless_equals_int(d->_seg_height, 2);
        dx_release_obj_meta(s);
        dx_release_obj_meta(d);
    }
}
GST_END_TEST;

// ---- TC8: user_meta deep copy via copy_func (C8) ----
GST_START_TEST(TC8_copy_user_metas_deep) {
    reset_spy();
    DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
    DXUserMeta *u = dx_acquire_user_meta_from_pool();
    SpyPayload *p = new_spy_payload(0xBEEF, "obj-um");
    fail_unless(dx_user_meta_set_data(u, p, sizeof(SpyPayload),
                                       DXUserMetaType::DX_USER_META_OBJECT,
                                       spy_release_cb, spy_copy_cb));
    fail_unless(dx_add_user_meta_to_obj(s, u));

    DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
    dx_copy_obj_meta(s, d);
    fail_unless_equals_int((int)d->_obj_user_meta_list.size(), 1);
    fail_unless_equals_int(g_spy_copy_count.load(), 1);
    fail_unless(d->_obj_user_meta_list[0]->user_meta_data != p);
    fail_unless(d->_obj_user_meta_list[0]->user_meta_data ==
                g_spy_last_copy_result.load());
    dx_release_obj_meta(s);
    dx_release_obj_meta(d);
    // src 1 + dst 1 = 2 releases
    fail_unless_equals_int(g_spy_release_count.load(), 2);
}
GST_END_TEST;

// ---- TC9: tensors shallow copy (C9) ----
GST_START_TEST(TC9_copy_tensors_shallow) {
    DXObjectMeta *s = dx_acquire_obj_meta_from_pool();
    dxs::DXTensors t;
    t._data = std::shared_ptr<void>(g_malloc0(16), g_free);
    s->_input_tensors[3] = t;
    long base = t._data.use_count();

    DXObjectMeta *d = dx_acquire_obj_meta_from_pool();
    dx_copy_obj_meta(s, d);
    fail_unless(d->_input_tensors.count(3) == 1);
    fail_unless(t._data.use_count() == base + 1);
    dx_release_obj_meta(s);
    dx_release_obj_meta(d);
}
GST_END_TEST;

// ---- TC10: copy with null args is noop (C10) ----
GST_START_TEST(TC10_copy_null_safe) {
    DXObjectMeta *o = dx_acquire_obj_meta_from_pool();
    dx_copy_obj_meta(nullptr, o);
    dx_copy_obj_meta(o, nullptr);
    // pass if no crash; also verify o's initial state is preserved
    fail_unless_equals_int(o->_label, -1);
    dx_release_obj_meta(o);
}
GST_END_TEST;

// ---- TC11: two acquire results have different ptrs (C11) ----
GST_START_TEST(TC11_pool_distinct_pointers) {
    DXObjectMeta *a = dx_acquire_obj_meta_from_pool();
    DXObjectMeta *b = dx_acquire_obj_meta_from_pool();
    fail_if(a == b);
    dx_release_obj_meta(a);
    dx_release_obj_meta(b);
}
GST_END_TEST;

static Suite *dxobjectmeta_suite(void) {
    Suite *s = suite_create("dxobjectmeta");
    TCase *tc = tcase_create("contract");
    suite_add_tcase(s, tc);
    tcase_add_test(tc, TC1_acquire_defaults);
    tcase_add_test(tc, TC2_meta_id_unique);
    tcase_add_test(tc, TC3_release_null);
    tcase_add_test(tc, TC4_copy_pins_meta_id);
    tcase_add_test(tc, TC5_copy_scalars);
    tcase_add_test(tc, TC6_copy_vectors);
    tcase_add_test(tc, TC7_copy_seg_branches);
    tcase_add_test(tc, TC8_copy_user_metas_deep);
    tcase_add_test(tc, TC9_copy_tensors_shallow);
    tcase_add_test(tc, TC10_copy_null_safe);
    tcase_add_test(tc, TC11_pool_distinct_pointers);
    return s;
}

GST_CHECK_MAIN(dxobjectmeta);
