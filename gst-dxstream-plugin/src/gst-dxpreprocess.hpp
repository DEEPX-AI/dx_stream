#ifndef GST_DXPREPROCESS_H
#define GST_DXPREPROCESS_H

#include "dxcommon.hpp"
#include "gst-dxmeta.hpp"
#include "memory_pool.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_DXPREPROCESS (gst_dxpreprocess_get_type())
G_DECLARE_FINAL_TYPE(GstDxPreprocess, gst_dxpreprocess, GST, DXPREPROCESS,
                     GstBaseTransform)

struct _GstDxPreprocess {
    GstBaseTransform _parent_instance;

    MemoryPool _pool;

    guint _pool_size;
    gchar *_config_file_path;
    gchar *_library_file_path;
    gchar *_function_name;

    guint _preprocess_id;
    gchar *_color_format;
    guint _resize_width;
    guint _resize_height;
    guint _input_channel;
    gboolean _keep_ratio;
    guint _pad_value;
    guint _align_factor;

    gboolean _secondary_mode;
    gint _target_class_id;
    guint _min_object_width;
    guint _min_object_height;

    int _roi[4];

    guint _interval;
    guint _cnt;
    guint _frame_count_for_fps;
    double _acc_fps;

    GstClockTime _qos_timestamp;
    GstClockTimeDiff _qos_timediff;
    GstClockTimeDiff _throttling_delay;

    std::map<int, std::map<int, int>> _track_cnt;

    void *_library_handle;
    cv::Mat (*_process_function)(DXFrameMeta *, DXObjectMeta *);
};

G_END_DECLS

#endif // GST_DXPREPROCESS_H