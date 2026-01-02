# User Metadata Guide

This guide provides comprehensive information on using the DX-STREAM User Metadata system for storing custom data alongside inference results.

## Overview

The DX-STREAM User Metadata system allows developers to attach custom data to frames and objects throughout the pipeline. This enables storing additional analytics results, custom features, tracking information, or any application-specific data.

### Key Features

- **Simplified Type System**: Two main categories - Frame and Object metadata
- **Memory Safety**: Required copy and release functions ensure proper memory management
- **Lifecycle Management**: Automatic cleanup when metadata is no longer needed
- **Pipeline Integration**: Seamless integration with existing DX-STREAM elements

## User Metadata Types

```cpp
typedef enum {
    DX_USER_META_FRAME = 0x1000,   // Frame-level user metadata
    DX_USER_META_OBJECT = 0x2000,  // Object-level user metadata
} DXUserMetaType;
```

**When to use Frame-level metadata:**
- Scene-level analytics (crowd count, scene classification)
- Frame-wide processing results
- Pipeline statistics or timing information
- Global configuration or state data

**When to use Object-level metadata:**
- Per-object features or analytics
- Object-specific tracking data
- Custom object attributes or classifications
- Object relationship information

## Basic Usage Pattern

### 1. Define Your Data Structure

```cpp
// Example: Custom analytics data for frames
typedef struct {
    gint total_objects;
    gfloat scene_confidence;
    gchar *scene_type;
    guint64 processing_time_us;
} SceneAnalytics;

// Example: Custom feature data for objects  
typedef struct {
    gfloat custom_score;
    gint feature_dim;
    gfloat *feature_vector;
    gchar *algorithm_name;
} CustomObjectFeature;
```

### 2. Implement Required Functions

**Copy Function** (Deep copy your data):
```cpp
static gpointer scene_analytics_copy(gconstpointer src) {
    const SceneAnalytics *src_data = (const SceneAnalytics *)src;
    SceneAnalytics *dst_data = g_new0(SceneAnalytics, 1);
    
    dst_data->total_objects = src_data->total_objects;
    dst_data->scene_confidence = src_data->scene_confidence;
    dst_data->scene_type = g_strdup(src_data->scene_type);  // Deep copy string
    dst_data->processing_time_us = src_data->processing_time_us;
    
    return dst_data;
}

static gpointer custom_feature_copy(gconstpointer src) {
    const CustomObjectFeature *src_data = (const CustomObjectFeature *)src;
    CustomObjectFeature *dst_data = g_new0(CustomObjectFeature, 1);
    
    dst_data->custom_score = src_data->custom_score;
    dst_data->feature_dim = src_data->feature_dim;
    
    // Deep copy feature vector
    dst_data->feature_vector = g_new(gfloat, src_data->feature_dim);
    memcpy(dst_data->feature_vector, src_data->feature_vector, 
           src_data->feature_dim * sizeof(gfloat));
    
    dst_data->algorithm_name = g_strdup(src_data->algorithm_name);
    
    return dst_data;
}
```

**Release Function** (Clean up your data):
```cpp
static void scene_analytics_free(gpointer data) {
    SceneAnalytics *analytics = (SceneAnalytics *)data;
    g_free(analytics->scene_type);
    g_free(analytics);
}

static void custom_feature_free(gpointer data) {
    CustomObjectFeature *feature = (CustomObjectFeature *)data;
    g_free(feature->feature_vector);
    g_free(feature->algorithm_name);
    g_free(feature);
}
```

### 3. Create and Attach Metadata

**Frame Metadata Example:**
```cpp
void add_scene_analytics_to_frame(DXFrameMeta *frame_meta, 
                                 gint object_count, 
                                 const gchar *scene_type) {
    // Acquire user metadata from pool
    DXUserMeta *user_meta = dx_acquire_user_meta_from_pool();
    
    // Create and populate your data
    SceneAnalytics *analytics = g_new0(SceneAnalytics, 1);
    analytics->total_objects = object_count;
    analytics->scene_confidence = 0.89f;
    analytics->scene_type = g_strdup(scene_type);
    analytics->processing_time_us = g_get_monotonic_time();
    
    // Set data with required functions
    gboolean success = dx_user_meta_set_data(user_meta,
                                            analytics,
                                            sizeof(SceneAnalytics),
                                            DX_USER_META_FRAME,
                                            scene_analytics_free,    // Required cleanup
                                            scene_analytics_copy);   // Required copy
    
    if (success) {
        // Attach to frame
        dx_add_user_meta_to_frame(frame_meta, user_meta);
    } else {
        g_warning("Failed to set user metadata");
        dx_release_user_meta(user_meta);
    }
}
```

**Object Metadata Example:**
```cpp
void add_custom_feature_to_object(DXObjectMeta *obj_meta, 
                                 const gchar *algorithm_name,
                                 gfloat *features, 
                                 gint feature_count) {
    // Acquire user metadata from pool
    DXUserMeta *user_meta = dx_acquire_user_meta_from_pool();
    
    // Create and populate feature data
    CustomObjectFeature *feature_data = g_new0(CustomObjectFeature, 1);
    feature_data->custom_score = 0.92f;
    feature_data->feature_dim = feature_count;
    feature_data->feature_vector = g_new(gfloat, feature_count);
    memcpy(feature_data->feature_vector, features, feature_count * sizeof(gfloat));
    feature_data->algorithm_name = g_strdup(algorithm_name);
    
    // Set data with required functions
    gboolean success = dx_user_meta_set_data(user_meta,
                                            feature_data,
                                            sizeof(CustomObjectFeature),
                                            DX_USER_META_OBJECT,
                                            custom_feature_free,     // Required cleanup
                                            custom_feature_copy);    // Required copy
    
    if (success) {
        // Attach to object
        dx_add_user_meta_to_obj(obj_meta, user_meta);
    } else {
        g_warning("Failed to set object user metadata");
        dx_release_user_meta(user_meta);
    }
}
```

### 4. Retrieve and Use Metadata

**Reading Frame Metadata:**
```cpp
void process_frame_metadata(DXFrameMeta *frame_meta) {
    GList *user_metas = dx_get_frame_user_metas(frame_meta);
    
    for (GList *l = user_metas; l != nullptr; l = l->next) {
        DXUserMeta *user_meta = (DXUserMeta *)l->data;
        
        // Check if this is frame-type metadata
        if (user_meta->user_meta_type == DX_USER_META_FRAME) {
            SceneAnalytics *analytics = (SceneAnalytics *)user_meta->user_meta_data;
            
            g_print("Scene Analysis Results:\n");
            g_print("  Objects: %d\n", analytics->total_objects);
            g_print("  Scene: %s (confidence: %.2f)\n", 
                   analytics->scene_type, analytics->scene_confidence);
            g_print("  Processing time: %lu μs\n", analytics->processing_time_us);
        }
    }
}
```

**Reading Object Metadata:**
```cpp
void process_object_metadata(DXObjectMeta *obj_meta) {
    GList *user_metas = dx_get_object_user_metas(obj_meta);
    
    for (GList *l = user_metas; l != nullptr; l = l->next) {
        DXUserMeta *user_meta = (DXUserMeta *)l->data;
        
        // Check if this is object-type metadata
        if (user_meta->user_meta_type == DX_USER_META_OBJECT) {
            CustomObjectFeature *feature = (CustomObjectFeature *)user_meta->user_meta_data;
            
            g_print("Custom Feature for Object %d:\n", obj_meta->_meta_id);
            g_print("  Algorithm: %s\n", feature->algorithm_name);
            g_print("  Score: %.3f\n", feature->custom_score);
            g_print("  Feature dimensions: %d\n", feature->feature_dim);
            
            // Access feature vector
            for (gint i = 0; i < feature->feature_dim && i < 5; i++) {
                g_print("  Feature[%d]: %.3f\n", i, feature->feature_vector[i]);
            }
        }
    }
}
```

## Complete Example: Custom Analytics Element

Here's a complete example of a custom element that adds analytics metadata:

```cpp
// In your custom postprocess function
extern "C" void custom_analytics_postprocess(GstBuffer *buf,
                                            std::vector<dxs::DXTensor> network_output,
                                            DXFrameMeta *frame_meta,
                                            DXObjectMeta *object_meta)
{
    // Add frame-level analytics
    add_scene_analytics_to_frame(frame_meta, 
                                g_list_length(frame_meta->_object_meta_list),
                                "indoor_scene");
    
    // Add custom features to each object
    GList *obj_list = frame_meta->_object_meta_list;
    for (GList *l = obj_list; l != nullptr; l = l->next) {
        DXObjectMeta *obj = (DXObjectMeta *)l->data;
        
        // Generate dummy features for demonstration
        gfloat features[128];
        for (gint i = 0; i < 128; i++) {
            features[i] = g_random_double_range(0.0, 1.0);
        }
        
        add_custom_feature_to_object(obj, "custom_algorithm_v1", features, 128);
    }
}
```

## Best Practices

### Memory Management
- **Always provide both copy and release functions**: The system requires both for proper memory management
- **Deep copy all allocated memory**: Your copy function must properly duplicate all heap-allocated data
- **Clean up completely**: Your release function must free all memory allocated by your data structure

### Performance Considerations
- **Pool allocation**: Use `dx_acquire_user_meta_from_pool()` for efficient memory allocation
- **Minimal data size**: Keep metadata structures compact for better performance
- **Lazy evaluation**: Only compute expensive metadata when actually needed

### Error Handling
- **Check return values**: Always verify that `dx_user_meta_set_data()` succeeds
- **Graceful degradation**: Handle metadata failures without breaking the pipeline
- **Logging**: Use appropriate log levels for debugging metadata issues

### Integration Tips
- **Type verification**: Always check `user_meta_type` before casting metadata
- **Iterator safety**: Handle the case where metadata lists might be empty
- **Thread safety**: Be aware that metadata may be accessed from multiple threads

## Troubleshooting

### Common Issues

**Segmentation faults when accessing metadata:**
- Verify that copy and release functions are properly implemented
- Check that you're not accessing freed memory
- Ensure deep copying of all pointer data

**Memory leaks:**
- Verify that your release function frees all allocated memory
- Check that you're not creating circular references
- Use valgrind to identify leak sources

**Metadata not found:**
- Verify that metadata type matches when retrieving
- Check that metadata was successfully attached
- Ensure you're checking the correct metadata list

### Debug Tips

**Enable debug logging:**
```cpp
// Add debug prints in your functions
g_print("Setting user metadata: type=%u, size=%zu\n", 
        meta_type, size);
```

**Validate data structures:**
```cpp
// Add validation in your copy function
g_return_val_if_fail(src != nullptr, nullptr);
g_return_val_if_fail(((MyData*)src)->magic == MY_DATA_MAGIC, nullptr);
```

**Memory tracking:**
```cpp
// Track allocations for debugging
static gint allocation_count = 0;

static void my_data_free(gpointer data) {
    allocation_count--;
    g_print("Free: %d allocations remaining\n", allocation_count);
    // ... actual cleanup ...
}
```

This completes the User Metadata Guide. The system is designed to be simple yet flexible, providing the essential functionality needed for most use cases while maintaining memory safety and performance.