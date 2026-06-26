#ifndef GST_DXVNPUOVERLAY_H
#define GST_DXVNPUOVERLAY_H

#include <gst/base/gstbasesink.h>
#include <dxvnpu/dxvnpu_api.h>
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include <memory>

G_BEGIN_DECLS

#define GST_TYPE_DXVNPUOVERLAY (gst_dxvnpuoverlay_get_type())
G_DECLARE_FINAL_TYPE(GstDxVnpuOverlay, gst_dxvnpuoverlay, GST,
                     DXVNPUOVERLAY, GstBaseSink)

struct _GstDxVnpuOverlay {
    GstBaseSink parent_instance;

    // Properties
    gchar *model_path;
    gboolean keep_ratio;
    gint device_id;
    gint group_count;

    // Runtime
    std::shared_ptr<dxvnpu::OverlayRenderer> overlay_renderer;
    int model_w{0};
    int model_h{0};
};

G_END_DECLS

#endif // GST_DXVNPUOVERLAY_H
