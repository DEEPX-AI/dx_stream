#include "dxgenbuffer.hpp"
#include "dxgenobj.hpp"

static gboolean plugin_init(GstPlugin *plugin) {
    if (!gst_element_register(plugin, "dxgenobj", GST_RANK_NONE,
                              GST_TYPE_DXGENOBJ)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxgenbuffer", GST_RANK_NONE,
                              GST_TYPE_DXGENBUFFER)) {
        return FALSE;
    }
    return TRUE;
}

#define PACKAGE "dxtest"

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, dxtest,
                  "DX TEST plugin", plugin_init, "1.0.0", "LGPL", "TEST DX",
                  "https://gstreamer.freedesktop.org")