#include "dx_stream/gst-dxframemeta.hpp"
#include "dx_stream/gst-dxobjectmeta.hpp"
#include "dx_stream/gst-dxusermeta.hpp"
#include <gst/gst.h>
#include <glib.h>
#include <iostream>
#include <string>

// Forward declarations
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

// Custom data structures for user metadata
typedef struct {
    guint custom_type;
    gchar *scene_type;          // "indoor", "outdoor", "street", etc.
    gfloat brightness_level;    // 0.0 - 1.0
    gfloat motion_level;        // 0.0 - 1.0 (amount of motion in frame)
    gint64 timestamp;
    gint detected_objects;
} FrameAnalyticsData;

typedef struct {
    guint custom_type;
    gchar *object_type;         // "person", "car", "bike", etc.
    gfloat detection_confidence;
    gfloat quality_score;       // object quality assessment
    gint tracking_id;
    gfloat bbox_x, bbox_y, bbox_w, bbox_h;
} ObjectDetectionData;

typedef struct {
    guint custom_type;
    gfloat overall_quality;     // frame quality score
    gfloat sharpness;
    gfloat exposure;
    gchar *quality_grade;       // "excellent", "good", "poor"
} QualityAssessmentData;

// Custom type definitions
#define FRAME_ANALYTICS_TYPE     1001
#define OBJECT_DETECTION_TYPE    2001  
#define QUALITY_ASSESSMENT_TYPE  1002

// Release functions
static void free_frame_analytics_data(gpointer data) {
    FrameAnalyticsData *analytics = (FrameAnalyticsData *)data;
    if (analytics) {
        g_free(analytics->scene_type);
        g_free(analytics);
    }
}

static void free_object_detection_data(gpointer data) {
    ObjectDetectionData *detection = (ObjectDetectionData *)data;
    if (detection) {
        g_free(detection->object_type);
        g_free(detection);
    }
}

static void free_quality_assessment_data(gpointer data) {
    QualityAssessmentData *quality = (QualityAssessmentData *)data;
    if (quality) {
        g_free(quality->quality_grade);
        g_free(quality);
    }
}

// Copy functions
static gpointer copy_frame_analytics_data(gpointer data) {
    FrameAnalyticsData *src = (FrameAnalyticsData *)data;
    if (!src) return nullptr;
    
    FrameAnalyticsData *dst = g_new0(FrameAnalyticsData, 1);
    dst->custom_type = src->custom_type;
    dst->scene_type = g_strdup(src->scene_type);
    dst->brightness_level = src->brightness_level;
    dst->motion_level = src->motion_level;
    dst->timestamp = src->timestamp;
    dst->detected_objects = src->detected_objects;
    
    return dst;
}

static gpointer copy_object_detection_data(gpointer data) {
    ObjectDetectionData *src = (ObjectDetectionData *)data;
    if (!src) return nullptr;
    
    ObjectDetectionData *dst = g_new0(ObjectDetectionData, 1);
    dst->custom_type = src->custom_type;
    dst->object_type = g_strdup(src->object_type);
    dst->detection_confidence = src->detection_confidence;
    dst->quality_score = src->quality_score;
    dst->tracking_id = src->tracking_id;
    dst->bbox_x = src->bbox_x;
    dst->bbox_y = src->bbox_y;
    dst->bbox_w = src->bbox_w;
    dst->bbox_h = src->bbox_h;
    
    return dst;
}

static gpointer copy_quality_assessment_data(gpointer data) {
    QualityAssessmentData *src = (QualityAssessmentData *)data;
    if (!src) return nullptr;
    
    QualityAssessmentData *dst = g_new0(QualityAssessmentData, 1);
    dst->custom_type = src->custom_type;
    dst->overall_quality = src->overall_quality;
    dst->sharpness = src->sharpness;
    dst->exposure = src->exposure;
    dst->quality_grade = g_strdup(src->quality_grade);
    
    return dst;
}

// Simulation helper functions
static const char* get_random_scene_type() {
    static const char* scenes[] = {"indoor", "outdoor", "street", "office", "home", "park"};
    return scenes[g_random_int_range(0, 6)];
}

static const char* get_random_object_type() {
    static const char* objects[] = {"person", "car", "bike", "dog", "cat", "truck", "bus"};
    return objects[g_random_int_range(0, 7)];
}

static const char* get_quality_grade(gfloat score) {
    if (score >= 0.8) return "excellent";
    else if (score >= 0.6) return "good";
    else if (score >= 0.4) return "fair";
    else return "poor";
}

// PROBE 1: Add user metadata to buffer
static GstPadProbeReturn add_metadata_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) return GST_PAD_PROBE_OK;
    
    // Make buffer writable if needed
    if (!gst_buffer_is_writable(buffer)) {
        buffer = gst_buffer_make_writable(buffer);
        info->data = buffer;
    }
    
    static gint frame_count = 0;
    static gint tracking_id_counter = 1000;
    frame_count++;
    
    g_print("\n[PROBE 1] Adding metadata to frame #%d\n", frame_count);
    
    // Get or create frame metadata
    DXFrameMeta *frame_meta = dx_get_frame_meta(buffer);
    if (!frame_meta) {
        frame_meta = dx_create_frame_meta(buffer);
        g_print("  > Created new DXFrameMeta\n");
    }
    
    // === Add Frame Analytics Data ===
    FrameAnalyticsData *frame_analytics = g_new0(FrameAnalyticsData, 1);
    frame_analytics->custom_type = FRAME_ANALYTICS_TYPE;
    frame_analytics->scene_type = g_strdup(get_random_scene_type());
    frame_analytics->brightness_level = g_random_double();
    frame_analytics->motion_level = g_random_double();
    frame_analytics->timestamp = g_get_monotonic_time();
    
    // Simulate detected objects count
    frame_analytics->detected_objects = g_random_int_range(1, 5);
    
    DXUserMeta *frame_user_meta = dx_acquire_user_meta_from_pool();
    if (dx_user_meta_set_data(frame_user_meta, frame_analytics, sizeof(FrameAnalyticsData),
                             DX_USER_META_FRAME, free_frame_analytics_data, copy_frame_analytics_data)) {
        dx_add_user_meta_to_frame(frame_meta, frame_user_meta);
        g_print("  > Added frame analytics: scene='%s', brightness=%.2f, motion=%.2f\n",
                frame_analytics->scene_type, frame_analytics->brightness_level, frame_analytics->motion_level);
    }
    
    // === Add Quality Assessment Data ===
    QualityAssessmentData *quality_data = g_new0(QualityAssessmentData, 1);
    quality_data->custom_type = QUALITY_ASSESSMENT_TYPE;
    quality_data->overall_quality = g_random_double();
    quality_data->sharpness = g_random_double();
    quality_data->exposure = g_random_double();
    quality_data->quality_grade = g_strdup(get_quality_grade(quality_data->overall_quality));
    
    DXUserMeta *quality_user_meta = dx_acquire_user_meta_from_pool();
    if (dx_user_meta_set_data(quality_user_meta, quality_data, sizeof(QualityAssessmentData),
                             DX_USER_META_FRAME, free_quality_assessment_data, copy_quality_assessment_data)) {
        dx_add_user_meta_to_frame(frame_meta, quality_user_meta);
        g_print("  > Added quality assessment: grade='%s', score=%.2f\n",
                quality_data->quality_grade, quality_data->overall_quality);
    }
    
    // === Create and Add Object Metadata ===
    for (int i = 0; i < frame_analytics->detected_objects; i++) {
        // Create object metadata
        DXObjectMeta *obj_meta = dx_acquire_obj_meta_from_pool();
        obj_meta->_label = i % 3; // Simulate different classes
        
        // Set bounding box (simulate random positions)
        obj_meta->_box[0] = g_random_double_range(0.0, 800.0);
        obj_meta->_box[1] = g_random_double_range(0.0, 800.0);
        obj_meta->_box[2] = g_random_double_range(50.0, 250.0);
        obj_meta->_box[3] = g_random_double_range(50.0, 250.0);
        
        // Add object detection user metadata
        ObjectDetectionData *obj_detection = g_new0(ObjectDetectionData, 1);
        obj_detection->custom_type = OBJECT_DETECTION_TYPE;
        obj_detection->object_type = g_strdup(get_random_object_type());
        obj_detection->detection_confidence = 0.5 + g_random_double() * 0.5; // 0.5-1.0
        obj_detection->quality_score = g_random_double();
        obj_detection->tracking_id = tracking_id_counter++;
        obj_detection->bbox_x = obj_meta->_box[0];
        obj_detection->bbox_y = obj_meta->_box[1];
        obj_detection->bbox_w = obj_meta->_box[2];
        obj_detection->bbox_h = obj_meta->_box[3];
        
        DXUserMeta *obj_user_meta = dx_acquire_user_meta_from_pool();
        if (dx_user_meta_set_data(obj_user_meta, obj_detection, sizeof(ObjectDetectionData),
                                 DX_USER_META_OBJECT, free_object_detection_data, copy_object_detection_data)) {
            dx_add_user_meta_to_obj(obj_meta, obj_user_meta);
            g_print("  > Added object %d: type='%s', confidence=%.2f, bbox=(%.1f,%.1f,%.1f,%.1f)\n",
                    i+1, obj_detection->object_type, obj_detection->detection_confidence,
                    obj_detection->bbox_x, obj_detection->bbox_y, obj_detection->bbox_w, obj_detection->bbox_h);
        }
        
        // Add object to frame
        dx_add_obj_meta_to_frame(frame_meta, obj_meta);
    }
    
    g_print("  > Frame #%d: Added %d objects with metadata\n", frame_count, frame_analytics->detected_objects);
    
    return GST_PAD_PROBE_OK;
}

// PROBE 2: Read and display user metadata
static GstPadProbeReturn read_metadata_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) return GST_PAD_PROBE_OK;
    
    static gint display_count = 0;
    display_count++;
    
    g_print("\n[PROBE 2] Reading metadata from frame #%d\n", display_count);
    
    // Get frame metadata
    DXFrameMeta *frame_meta = dx_get_frame_meta(buffer);
    if (!frame_meta) {
        g_print("  * No frame metadata found\n");
        return GST_PAD_PROBE_OK;
    }
    
    // === Read Frame User Metadata ===
    GList *frame_user_metas = dx_get_frame_user_metas(frame_meta);
    g_print("  > Found %d frame user metadata entries\n", g_list_length(frame_user_metas));
    
    for (GList *l = frame_user_metas; l != nullptr; l = l->next) {
        DXUserMeta *user_meta = (DXUserMeta *)l->data;
        
        if (user_meta->user_meta_data) {
            guint *custom_type = (guint *)user_meta->user_meta_data;
            
            switch (*custom_type) {
                case FRAME_ANALYTICS_TYPE: {
                    FrameAnalyticsData *analytics = (FrameAnalyticsData *)user_meta->user_meta_data;
                    g_print("    * Frame Analytics: scene='%s', brightness=%.2f, motion=%.2f, objects=%d\n",
                           analytics->scene_type, analytics->brightness_level, 
                           analytics->motion_level, analytics->detected_objects);
                    break;
                }
                case QUALITY_ASSESSMENT_TYPE: {
                    QualityAssessmentData *quality = (QualityAssessmentData *)user_meta->user_meta_data;
                    g_print("    * Quality Assessment: grade='%s', score=%.2f, sharpness=%.2f\n",
                           quality->quality_grade, quality->overall_quality, quality->sharpness);
                    break;
                }
            }
        }
    }
    g_list_free(frame_user_metas);
    
    // === Read Object Metadata ===
    gint object_count = 0;
    for (GList *l = frame_meta->_object_meta_list; l != nullptr; l = l->next) {
        DXObjectMeta *obj_meta = (DXObjectMeta *)l->data;
        object_count++;
        
        g_print("    * Object #%d: class=%d, bbox=(%.1f,%.1f,%.1f,%.1f)\n",
               object_count, obj_meta->_label,
               obj_meta->_box[0], obj_meta->_box[1],
               obj_meta->_box[2], obj_meta->_box[3]);
        
        // Read object user metadata
        GList *obj_user_metas = dx_get_object_user_metas(obj_meta);
        for (GList *ul = obj_user_metas; ul != nullptr; ul = ul->next) {
            DXUserMeta *obj_user_meta = (DXUserMeta *)ul->data;
            
            if (obj_user_meta->user_meta_data) {
                guint *custom_type = (guint *)obj_user_meta->user_meta_data;
                
                if (*custom_type == OBJECT_DETECTION_TYPE) {
                    ObjectDetectionData *detection = (ObjectDetectionData *)obj_user_meta->user_meta_data;
                    g_print("      > Detection: type='%s', confidence=%.2f, quality=%.2f\n",
                           detection->object_type, detection->detection_confidence, detection->quality_score);
                }
            }
        }
        g_list_free(obj_user_metas);
    }
    
    g_print("  > Total objects processed: %d\n", object_count);
    
    return GST_PAD_PROBE_OK;
}

// Callback function for pad-added signal of decodebin
static void on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
    GstElement *convert1 = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_static_pad(convert1, "sink");
    
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (caps) {
        const gchar *name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
        
        if (g_str_has_prefix(name, "video")) {
            if (gst_pad_can_link(pad, sinkpad)) {
                gst_pad_link(pad, sinkpad);
                g_print("Linked demuxer to converter\n");
            }
        }
        
        gst_caps_unref(caps);
    }
    
    gst_object_unref(sinkpad);
}

// Message handling
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("\nEnd of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    
    if (argc != 2) {
        g_printerr("Usage: %s <video_file_path>\n", argv[0]);
        g_printerr("Example: %s /path/to/video.mp4\n", argv[0]);
        return -1;
    }
    
    g_print("🚀 DX-Stream User Meta Pipeline Example\n");
    g_print("=======================================\n");
    g_print("📹 Input file: %s\n\n", argv[1]);
    
    // Create elements
    GstElement *pipeline = gst_pipeline_new("usermeta-pipeline");
    GstElement *source = gst_element_factory_make("filesrc", "file-source");
    GstElement *demuxer = gst_element_factory_make("decodebin", "demuxer");
    GstElement *convert1 = gst_element_factory_make("videoconvert", "convert1");
    GstElement *convert2 = gst_element_factory_make("videoconvert", "convert2");
    GstElement *sink = gst_element_factory_make("fakesink", "video-output");
    
    if (!pipeline || !source || !demuxer || !convert1 || !convert2 || !sink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }
    
    // Set properties
    g_object_set(G_OBJECT(source), "location", argv[1], NULL);
    g_object_set(G_OBJECT(sink), "sync", FALSE, NULL);
    
    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, demuxer, convert1, convert2, sink, NULL);
    
    // Link elements (decodebin will be linked dynamically)
    gst_element_link(source, demuxer);
    gst_element_link(convert1, convert2);
    gst_element_link(convert2, sink);
    
    // Connect pad-added signal for decodebin
    g_signal_connect(demuxer, "pad-added", G_CALLBACK(on_pad_added), convert1);
    
    // Add probes
    GstPad *convert1_src = gst_element_get_static_pad(convert1, "src");
    gst_pad_add_probe(convert1_src, GST_PAD_PROBE_TYPE_BUFFER, add_metadata_probe, nullptr, nullptr);
    gst_object_unref(convert1_src);
    
    GstPad *convert2_src = gst_element_get_static_pad(convert2, "src");
    gst_pad_add_probe(convert2_src, GST_PAD_PROBE_TYPE_BUFFER, read_metadata_probe, nullptr, nullptr);
    gst_object_unref(convert2_src);
    
    // Set up message handling
    GMainLoop *loop = g_main_loop_new(nullptr, FALSE);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);
    
    // Start playing
    g_print("🎬 Starting pipeline...\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    // Run main loop
    g_main_loop_run(loop);
    
    // Cleanup
    g_print("\nCleaning up...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_main_loop_unref(loop);
    
    g_print("User Meta Pipeline Example completed!\n");
    return 0;
}
