/*
 * DX-STREAM User Meta Test
 * 
 * This test validates the safety and functionality of the DX-STREAM user metadata system,
 * including validation of required copy/release functions and deep copy operations.
 */

#include <gst/gst.h>
#include <glib.h>
#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// Include DX-STREAM metadata headers
#include "dx_stream/gst-dxframemeta.hpp"
#include "dx_stream/gst-dxobjectmeta.hpp"
#include "dx_stream/gst-dxusermeta.hpp"

// Test results tracking
static gint total_tests = 0;
static gint passed_tests = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        total_tests++; \
        if (condition) { \
            g_print("[PASS] %s\n", message); \
            passed_tests++; \
        } else { \
            g_print("[FAIL] %s\n", message); \
        } \
    } while(0)

// Custom data structures for user metadata examples
typedef struct {
    gint custom_type;           // User-defined type for filtering
    gchar *scene_description;
    gfloat scene_confidence;
    gint64 timestamp;
    gint object_count;
} FrameAnalyticsData;

typedef struct {
    gint custom_type;           // User-defined type for filtering
    gchar *algorithm_name;
    gfloat confidence_score;
    gfloat processing_time;
    std::vector<gfloat> attributes;
} ObjectAnalyticsData;

// User-defined custom types
const gint FRAME_ANALYTICS_TYPE = 100;
const gint OBJECT_ANALYTICS_TYPE = 200;

// Helper functions for custom data management
static void free_frame_analytics_data(gpointer data) {
    FrameAnalyticsData *analytics = (FrameAnalyticsData *)data;
    if (analytics) {
        g_free(analytics->scene_description);
        g_free(analytics);
    }
}

static gpointer copy_frame_analytics_data(gpointer data) {
    FrameAnalyticsData *src = (FrameAnalyticsData *)data;
    if (!src) return nullptr;
    
    FrameAnalyticsData *dst = g_new0(FrameAnalyticsData, 1);
    dst->custom_type = src->custom_type;
    dst->scene_description = g_strdup(src->scene_description);
    dst->scene_confidence = src->scene_confidence;
    dst->timestamp = src->timestamp;
    dst->object_count = src->object_count;
    
    return dst;
}

static void free_object_analytics_data(gpointer data) {
    ObjectAnalyticsData *analytics = (ObjectAnalyticsData *)data;
    if (analytics) {
        g_free(analytics->algorithm_name);
        analytics->attributes.clear();
        g_free(analytics);
    }
}

static gpointer copy_object_analytics_data(gpointer data) {
    ObjectAnalyticsData *src = (ObjectAnalyticsData *)data;
    if (!src) return nullptr;
    
    ObjectAnalyticsData *dst = g_new0(ObjectAnalyticsData, 1);
    dst->custom_type = src->custom_type;
    dst->algorithm_name = g_strdup(src->algorithm_name);
    dst->confidence_score = src->confidence_score;
    dst->processing_time = src->processing_time;
    dst->attributes = src->attributes;  // Copy vector
    
    return dst;
}

// Test 1: Validate that user meta requires both copy and release functions
void test_required_functions() {
    g_print("\n=== Test 1: Required Copy/Release Functions ===\n");
    
    FrameAnalyticsData *test_data = g_new0(FrameAnalyticsData, 1);
    test_data->custom_type = FRAME_ANALYTICS_TYPE;
    test_data->scene_description = g_strdup("Test scene");
    
    DXUserMeta *user_meta = dx_acquire_user_meta_from_pool();
    
    // Test 1.1: Setting data without release_func should fail
    gboolean result1 = dx_user_meta_set_data(user_meta, test_data, sizeof(FrameAnalyticsData), 
                                             DX_USER_META_FRAME, nullptr, copy_frame_analytics_data);
    TEST_ASSERT(!result1, "Setting user meta without release_func should fail");
    
    // Test 1.2: Setting data without copy_func should fail
    gboolean result2 = dx_user_meta_set_data(user_meta, test_data, sizeof(FrameAnalyticsData), 
                                             DX_USER_META_FRAME, free_frame_analytics_data, nullptr);
    TEST_ASSERT(!result2, "Setting user meta without copy_func should fail");
    
    // Test 1.3: Setting data without both functions should fail
    gboolean result3 = dx_user_meta_set_data(user_meta, test_data, sizeof(FrameAnalyticsData), 
                                             DX_USER_META_FRAME, nullptr, nullptr);
    TEST_ASSERT(!result3, "Setting user meta without both functions should fail");
    
    // Test 1.4: Setting data with both functions should succeed
    gboolean result4 = dx_user_meta_set_data(user_meta, test_data, sizeof(FrameAnalyticsData), 
                                             DX_USER_META_FRAME, free_frame_analytics_data, copy_frame_analytics_data);
    TEST_ASSERT(result4, "Setting user meta with both functions should succeed");
    
    dx_release_user_meta(user_meta);
}

// Test 2: Validate that adding user meta to frame/object requires functions
void test_add_validation() {
    g_print("\n=== Test 2: Add Validation ===\n");
    
    GstBuffer *buffer = gst_buffer_new();
    DXFrameMeta *frame_meta = dx_create_frame_meta(buffer);
    
    // Create user meta without functions
    DXUserMeta *invalid_meta = dx_acquire_user_meta_from_pool();
    invalid_meta->user_meta_data = g_strdup("test");
    invalid_meta->user_meta_size = 5;
    invalid_meta->user_meta_type = DX_USER_META_FRAME;
    // Don't set copy_func and release_func
    
    // Test 2.1: Adding user meta without functions should fail
    gboolean result1 = dx_add_user_meta_to_frame(frame_meta, invalid_meta);
    TEST_ASSERT(!result1, "Adding user meta without functions to frame should fail");
    
    // Create object and test object meta addition
    DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
    dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    
    // Test 2.2: Adding user meta without functions to object should fail
    gboolean result2 = dx_add_user_meta_to_obj(obj_meta, invalid_meta);
    TEST_ASSERT(!result2, "Adding user meta without functions to object should fail");
    
    // Clean up invalid meta manually since it wasn't added
    g_free(invalid_meta->user_meta_data);
    dx_release_user_meta(invalid_meta);
    
    gst_buffer_unref(buffer);
}

// Test 3: Deep copy validation
void test_deep_copy() {
    g_print("\n=== Test 3: Deep Copy Validation ===\n");
    
    // Create source buffer with metadata
    GstBuffer *src_buffer = gst_buffer_new();
    DXFrameMeta *src_frame_meta = dx_create_frame_meta(src_buffer);
    
    // Add frame user meta
    FrameAnalyticsData *analytics = g_new0(FrameAnalyticsData, 1);
    analytics->custom_type = FRAME_ANALYTICS_TYPE;
    analytics->scene_description = g_strdup("Original scene");
    analytics->scene_confidence = 0.95f;
    
    DXUserMeta *user_meta = dx_acquire_user_meta_from_pool();
    dx_user_meta_set_data(user_meta, analytics, sizeof(FrameAnalyticsData), 
                         DX_USER_META_FRAME, free_frame_analytics_data, copy_frame_analytics_data);
    dx_add_user_meta_to_frame(src_frame_meta, user_meta);
    
    // Add object with user meta
    DXObjectMeta *src_obj = dx_acquire_obj_meta_from_pool();
    src_obj->_label_name = g_string_new("person");
    dx_add_obj_meta_to_frame(src_frame_meta, src_obj);
    
    ObjectAnalyticsData *obj_analytics = g_new0(ObjectAnalyticsData, 1);
    obj_analytics->custom_type = OBJECT_ANALYTICS_TYPE;
    obj_analytics->algorithm_name = g_strdup("original_algorithm");
    obj_analytics->confidence_score = 0.88f;
    
    DXUserMeta *obj_user_meta = dx_acquire_user_meta_from_pool();
    dx_user_meta_set_data(obj_user_meta, obj_analytics, sizeof(ObjectAnalyticsData),
                         DX_USER_META_OBJECT, free_object_analytics_data, copy_object_analytics_data);
    dx_add_user_meta_to_obj(src_obj, obj_user_meta);
    
    // Create destination buffer and copy metadata
    GstBuffer *dst_buffer = gst_buffer_new();
    DXFrameMeta *dst_frame_meta = dx_create_frame_meta(dst_buffer);
    dx_frame_meta_copy(src_buffer, src_frame_meta, dst_buffer, dst_frame_meta);
    
    // Test 3.1: Frame metadata should be copied
    TEST_ASSERT(dst_frame_meta->_num_frame_user_meta == 1, "Frame user meta count should be copied");
    
    // Test 3.2: Frame user meta data should be deep copied
    GList *dst_frame_metas = dx_get_frame_user_metas(dst_frame_meta);
    DXUserMeta *dst_user_meta = (DXUserMeta *)dst_frame_metas->data;
    FrameAnalyticsData *dst_analytics = (FrameAnalyticsData *)dst_user_meta->user_meta_data;
    
    TEST_ASSERT(dst_analytics != analytics, "Frame user meta data should be different memory addresses");
    TEST_ASSERT(g_strcmp0(dst_analytics->scene_description, "Original scene") == 0, 
               "Frame user meta data content should be copied correctly");
    TEST_ASSERT(dst_analytics->scene_confidence == 0.95f, "Frame user meta float values should match");
    
    // Test 3.3: Object metadata should be copied
    TEST_ASSERT(g_list_length(dst_frame_meta->_object_meta_list) == 1, "Object count should be copied");
    
    // Test 3.4: Object user meta should be deep copied
    DXObjectMeta *dst_obj = (DXObjectMeta *)dst_frame_meta->_object_meta_list->data;
    TEST_ASSERT(dst_obj != src_obj, "Object meta should be different memory addresses");
    TEST_ASSERT(dst_obj->_num_obj_user_meta == 1, "Object user meta count should be copied");
    
    GList *dst_obj_metas = dx_get_object_user_metas(dst_obj);
    DXUserMeta *dst_obj_user_meta = (DXUserMeta *)dst_obj_metas->data;
    ObjectAnalyticsData *dst_obj_analytics = (ObjectAnalyticsData *)dst_obj_user_meta->user_meta_data;
    
    TEST_ASSERT(dst_obj_analytics != obj_analytics, "Object user meta data should be different memory addresses");
    TEST_ASSERT(g_strcmp0(dst_obj_analytics->algorithm_name, "original_algorithm") == 0,
               "Object user meta data content should be copied correctly");
    
    g_list_free(dst_frame_metas);
    g_list_free(dst_obj_metas);
    gst_buffer_unref(src_buffer);
    gst_buffer_unref(dst_buffer);
}

// Test 4: Functional workflow test
void test_functional_workflow() {
    g_print("\n=== Test 4: Functional Workflow ===\n");
    
    GstBuffer *buffer = gst_buffer_new();
    DXFrameMeta *frame_meta = dx_create_frame_meta(buffer);
    
    // Add frame analytics
    FrameAnalyticsData *analytics = g_new0(FrameAnalyticsData, 1);
    analytics->custom_type = FRAME_ANALYTICS_TYPE;
    analytics->scene_description = g_strdup("Test workflow scene");
    analytics->object_count = 2;
    
    DXUserMeta *frame_user_meta = dx_acquire_user_meta_from_pool();
    gboolean set_result = dx_user_meta_set_data(frame_user_meta, analytics, sizeof(FrameAnalyticsData),
                                               DX_USER_META_FRAME, free_frame_analytics_data, copy_frame_analytics_data);
    TEST_ASSERT(set_result, "Setting valid frame user meta should succeed");
    
    gboolean add_result = dx_add_user_meta_to_frame(frame_meta, frame_user_meta);
    TEST_ASSERT(add_result, "Adding valid frame user meta should succeed");
    
    // Create objects and add object analytics
    for (int i = 0; i < 2; i++) {
        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_label = i;
        obj_meta->_label_name = g_string_new(g_strdup_printf("object_%d", i));
        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
        
        ObjectAnalyticsData *obj_analytics = g_new0(ObjectAnalyticsData, 1);
        obj_analytics->custom_type = OBJECT_ANALYTICS_TYPE;
        obj_analytics->algorithm_name = g_strdup_printf("algorithm_%d", i);
        obj_analytics->confidence_score = 0.8f + (i * 0.1f);
        
        DXUserMeta *obj_user_meta = dx_acquire_user_meta_from_pool();
        gboolean obj_set_result = dx_user_meta_set_data(obj_user_meta, obj_analytics, sizeof(ObjectAnalyticsData),
                                                       DX_USER_META_OBJECT, free_object_analytics_data, copy_object_analytics_data);
        TEST_ASSERT(obj_set_result, "Setting valid object user meta should succeed");
        
        gboolean obj_add_result = dx_add_user_meta_to_obj(obj_meta, obj_user_meta);
        TEST_ASSERT(obj_add_result, "Adding valid object user meta should succeed");
    }
    
    // Validate retrieval
    GList *frame_metas = dx_get_frame_user_metas(frame_meta);
    TEST_ASSERT(g_list_length(frame_metas) == 1, "Should retrieve exactly 1 frame user meta");
    
    TEST_ASSERT(g_list_length(frame_meta->_object_meta_list) == 2, "Should have exactly 2 objects");
    
    // Validate object user metas
    int obj_count = 0;
    for (GList *l = frame_meta->_object_meta_list; l != nullptr; l = l->next) {
        DXObjectMeta *obj_meta = (DXObjectMeta *)l->data;
        GList *obj_metas = dx_get_object_user_metas(obj_meta);
        TEST_ASSERT(g_list_length(obj_metas) == 1, "Each object should have exactly 1 user meta");
        obj_count++;
        g_list_free(obj_metas);
    }
    TEST_ASSERT(obj_count == 2, "Should process exactly 2 objects");
    
    g_list_free(frame_metas);
    gst_buffer_unref(buffer);
}

// Main test function
int main(int argc, char *argv[]) {
    // Initialize GStreamer
    gst_init(&argc, &argv);
    
    g_print("DX-STREAM User Meta Safety Test\n");
    g_print("==============================\n");
    
    // Run all tests
    test_required_functions();
    test_add_validation(); 
    test_deep_copy();
    test_functional_workflow();
    
    // Print test results
    g_print("\n=== Test Results ===\n");
    g_print("Total tests: %d\n", total_tests);
    g_print("Passed: %d\n", passed_tests);
    g_print("Failed: %d\n", total_tests - passed_tests);
    
    if (passed_tests == total_tests) {
        g_print("[SUCCESS] All tests passed!\n");
        return 0;
    } else {
        g_print("[ERROR] Some tests failed!\n");
        return 1;
    }
}
