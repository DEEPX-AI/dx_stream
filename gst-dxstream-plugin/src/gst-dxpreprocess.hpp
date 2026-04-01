#ifndef GST_DXPREPROCESS_H
#define GST_DXPREPROCESS_H

#include "dxcommon.hpp"
#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include <map>
#include <vector>

// Forward declaration for preprocessor
class Preprocessor;

G_BEGIN_DECLS

#define GST_TYPE_DXPREPROCESS (gst_dxpreprocess_get_type())
G_DECLARE_FINAL_TYPE(GstDxPreprocess, gst_dxpreprocess, GST, DXPREPROCESS,
                     GstBaseTransform)

struct _GstDxPreprocess {
    GstBaseTransform _parent_instance;

    // Configuration
    struct {
        gchar *file_path;
        gchar *library_path;
        gchar *function_name;
    } _config;

    // Stream information
    struct {
        std::map<int, GstVideoInfo> info;
        int last_id;
    } _stream;

    // Preprocessing parameters
    struct {
        guint id;
        gchar *color_format;
        guint width;
        guint height;
        guint channel;
        gboolean keep_ratio;
        guint pad_value;
        gboolean transpose;
        std::vector<uint8_t> transpose_data;
    } _preprocess;

    // Object filtering
    struct {
        gboolean secondary_mode;
        gint target_class_id;
        guint min_width;
        guint min_height;
        int roi[4];
    } _object_filter;

    // Frame processing control
    struct {
        guint interval;
        std::map<int, guint> cnt;
        guint frame_count;
        double acc_fps;
        std::map<int, std::map<int, int>> track_cnt;
    } _frame_ctrl;

    // QoS
    struct {
        GstClockTime timestamp;
        GstClockTimeDiff timediff;
        GstClockTimeDiff throttling_delay;
    } _qos;

    // Plugin function pointers
    struct {
        void *library_handle;
        bool (*process_function)(GstBuffer *buf, DXFrameMeta *, DXObjectMeta *, void *);
        std::shared_ptr<Preprocessor> preprocessor;
    } _plugin;

    // Buffer caches (RAII pattern)
    struct {
        std::map<int, std::vector<uint8_t>> crop;
        std::map<int, std::vector<uint8_t>> convert;
        std::map<int, std::vector<uint8_t>> resized;
    } _buffers;
};

G_END_DECLS

#endif // GST_DXPREPROCESS_H