#ifndef GST_DXPREPROCESS_H
#define GST_DXPREPROCESS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dxcommon.hpp"
#include "gst-dxmeta.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include <map>

#ifdef HAVE_LIBRGA
#include "rga/RgaUtils.h"
#include "rga/im2d.hpp"
#else
#include "libyuv_transform/libyuv_transform.hpp"
#endif

G_BEGIN_DECLS

#define GST_TYPE_DXPREPROCESS (gst_dxpreprocess_get_type())
G_DECLARE_FINAL_TYPE(GstDxPreprocess, gst_dxpreprocess, GST, DXPREPROCESS,
                     GstBaseTransform)

struct _GstDxPreprocess {
    GstBaseTransform _parent_instance;

    gchar *_config_file_path;
    gchar *_library_file_path;
    gchar *_function_name;

    std::map<int, GstVideoInfo> _input_info;
    int _last_stream_id;

    guint _preprocess_id;
    gchar *_color_format;
    guint _resize_width;
    guint _resize_height;
    guint _input_channel;
    gboolean _keep_ratio;
    guint _pad_value;

    gboolean _secondary_mode;
    gint _target_class_id;
    guint _min_object_width;
    guint _min_object_height;

    int _roi[4];

    guint _interval;
    std::map<int, guint> _cnt;
    guint _frame_count_for_fps;
    double _acc_fps;

    GstClockTime _qos_timestamp;
    GstClockTimeDiff _qos_timediff;
    GstClockTimeDiff _throttling_delay;

    std::map<int, std::map<int, int>> _track_cnt;

    void *_library_handle;
    bool (*_process_function)(DXFrameMeta *, DXObjectMeta *, void *);

#ifdef HAVE_LIBRGA
#else
    std::map<int, uint8_t *> _crop_frame;
    std::map<int, uint8_t *> _convert_frame;
    std::map<int, uint8_t *> _resized_frame;
#endif
};

G_END_DECLS

#endif // GST_DXPREPROCESS_H