#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gst-dxgather.hpp"
#include "gst-dxinfer.hpp"
#include "gst-dxinputselector.hpp"
#include "gst-dxosd.hpp"
#include "gst-dxoutputselector.hpp"
#include "gst-dxpostprocess.hpp"
#include "gst-dxpreprocess.hpp"
#include "gst-dxrate.hpp"
#include "gst-dxtracker.hpp"
#include <gst/gst.h>

#ifdef DEEPX_V3
#else
#include "gst-dxmsgbroker.hpp"
#include "gst-dxmsgconv.hpp"
#endif

static gboolean plugin_init(GstPlugin *plugin) {
    // Pipeline Design Elements
    if (!gst_element_register(plugin, "dxoutputselector", GST_RANK_NONE,
                              GST_TYPE_DXOUTPUTSELECTOR)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxinputselector", GST_RANK_NONE,
                              GST_TYPE_DXINPUTSELECTOR)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxgather", GST_RANK_NONE,
                              GST_TYPE_DXGATHER)) {
        return FALSE;
    }
    // Utility Elements
#ifdef DEEPX_V3
#else
    if (!gst_element_register(plugin, "dxmsgconv", GST_RANK_NONE,
                              GST_TYPE_DXMSGCONV)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxmsgbroker", GST_RANK_NONE,
                              GST_TYPE_DXMSGBROKER)) {
        return FALSE;
    }
#endif
    if (!gst_element_register(plugin, "dxrate", GST_RANK_NONE,
                              GST_TYPE_DXRATE)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxosd", GST_RANK_NONE, GST_TYPE_DXOSD)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxtracker", GST_RANK_NONE,
                              GST_TYPE_DXTRACKER)) {
        return FALSE;
    }
    // Inference Core Elements
    if (!gst_element_register(plugin, "dxpostprocess", GST_RANK_NONE,
                              GST_TYPE_DXPOSTPROCESS)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxinfer", GST_RANK_NONE,
                              GST_TYPE_DXINFER)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxpreprocess", GST_RANK_NONE,
                              GST_TYPE_DXPREPROCESS)) {
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
