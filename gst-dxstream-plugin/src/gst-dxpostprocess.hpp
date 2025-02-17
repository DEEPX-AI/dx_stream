#ifndef GST_DXPOSTPROCESS_H
#define GST_DXPOSTPROCESS_H

#include "dxcommon.hpp"
#include "gst-dxmeta.hpp"
#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DXPOSTPROCESS (gst_dxpostprocess_get_type())
G_DECLARE_FINAL_TYPE(GstDxPostprocess, gst_dxpostprocess, GST, DXPOSTPROCESS,
                     GstBaseTransform)

struct _GstDxPostprocess {
    GstBaseTransform _parent_instance;

    gchar *_config_file_path;
    gchar *_library_file_path;
    gchar *_function_name;

    gboolean _secondary_mode;
    guint _infer_id;

    guint _frame_count_for_fps;
    double _acc_fps;

    void *_library_handle;
    void (*_postproc_function)(std::vector<dxs::DXTensor>, DXFrameMeta *,
                               DXObjectMeta *);
};

G_END_DECLS

#endif // GST_DXPOSTPROCESS_H