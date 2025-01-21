#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gst-dxgather.hpp"
#include "gst-dxinfer.hpp"
#include "gst-dxmuxer.hpp"
#include "gst-dxosd.hpp"
#include "gst-dxpreprocess.hpp"
#include "gst-dxrate.hpp"
#include "gst-dxrouter.hpp"
#include "gst-dxtiler.hpp"
#include "gst-dxtracker.hpp"
#include <gst/gst.h>

#include "gst-dxmsgbroker.hpp"
#include "gst-dxmsgconv.hpp"

static gboolean plugin_init(GstPlugin *plugin) {
    if (!gst_element_register(plugin, "dxinfer", GST_RANK_NONE,
                              GST_TYPE_DXINFER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxmuxer", GST_RANK_NONE,
                              GST_TYPE_DXMUXER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxpreprocess", GST_RANK_NONE,
                              GST_TYPE_DXPREPROCESS)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxosd", GST_RANK_NONE, GST_TYPE_DXOSD)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxtiler", GST_RANK_NONE,
                              GST_TYPE_DXTILER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxtracker", GST_RANK_NONE,
                              GST_TYPE_DXTRACKER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxrate", GST_RANK_NONE,
                              GST_TYPE_DXRATE)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxmsgconv", GST_RANK_NONE,
                              GST_TYPE_DXMSGCONV)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxmsgbroker", GST_RANK_NONE,
                              GST_TYPE_DXMSGBROKER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxgather", GST_RANK_NONE,
                              GST_TYPE_DXGATHER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxrouter", GST_RANK_NONE,
                              GST_TYPE_DXROUTER)) {
        return FALSE;
    }
    return TRUE;
}

#ifndef PACKAGE
#define PACKAGE "gst-dxstream"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, dxstream,
                  "DX Stream plugin", plugin_init, PACKAGE_VERSION, GST_LICENSE,
                  GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
