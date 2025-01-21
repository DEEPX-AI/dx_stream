#ifndef GST_DXTRACKER_H
#define GST_DXTRACKER_H

#include "OCSort.hpp"
#include "dxcommon.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS

#define GST_TYPE_DXTRACKER (gst_dxtracker_get_type())
G_DECLARE_FINAL_TYPE(GstDxTracker, gst_dxtracker, GST, DXTRACKER,
                     GstBaseTransform)

struct _GstDxTracker {
    GstBaseTransform _parent_instance;

    gchar *_config_file_path;
    gchar *_tracker_name;
    std::map<std::string, std::string> _params;

    gboolean _first_frame_processed;

    std::map<int, std::unique_ptr<Tracker>> _trackers;
};

G_END_DECLS

#endif // GST_DXTRACKER_H