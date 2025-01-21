#ifndef GST_DX_ROUTER_H
#define GST_DX_ROUTER_H

#include <gst/gst.h>
#include <map>
#include <mutex>

G_BEGIN_DECLS

#define GST_TYPE_DXROUTER (gst_dxrouter_get_type())
G_DECLARE_FINAL_TYPE(GstDxRouter, gst_dxrouter, GST, DXROUTER, GstElement)

struct _GstDxRouter {
    GstElement parent_instance;

    std::map<gint, GstPad *> _srcpads;
    GstPad *_sinkpad;
    int _cnt;
};

G_END_DECLS

#endif // GST_DX_ROUTER_H
