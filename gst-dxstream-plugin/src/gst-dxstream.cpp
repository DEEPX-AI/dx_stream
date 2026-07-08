#include "gst-dxgather.hpp"
#include "gst-dxinfer.hpp"
#include "gst-dxinputselector.hpp"
#include "gst-dxosd.hpp"
#include "gst-dxoutputselector.hpp"
#include "gst-dxpostprocess.hpp"
#include "gst-dxpreprocess.hpp"
#include "gst-dxrate.hpp"
#include "gst-dxscale.hpp"
#include "gst-dxconvert.hpp"
#include "gst-dxtracker.hpp"
#include <gst/gst.h>

#ifndef DEEPX_V3
#include "gst-dxmsgbroker.hpp"
#include "gst-dxmsgconv.hpp"
#endif

#ifdef HAVE_DXVNPU
#include "gst-dxvnpudec.hpp"
#include "gst-dxvnpuenc.hpp"
#include "gst-dxvnpupipeline.hpp"
#include "gst-dxvnpuoverlay.hpp"
#endif

GST_DEBUG_CATEGORY(dxmeta_cat);
GST_DEBUG_CATEGORY(transform_kernel_cat);
GST_DEBUG_CATEGORY(inference_backend_cat);

static gboolean plugin_init(GstPlugin *plugin) {
    // debug category
    GST_DEBUG_CATEGORY_INIT(dxmeta_cat, "dxmeta", 0, "DX Metadata");
    GST_DEBUG_CATEGORY_INIT(transform_kernel_cat, "transform_kernel", 0,
                            "DX Transform Kernel");
    GST_DEBUG_CATEGORY_INIT(inference_backend_cat, "inference_backend", 0,
                            "DX Inference Backend");

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
    if (!gst_element_register(plugin, "dxscale", GST_RANK_NONE,
                              GST_TYPE_DXSCALE)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxconvert", GST_RANK_NONE,
                              GST_TYPE_DXCONVERT)) {
        return FALSE;
    }
#ifndef DEEPX_V3
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
    // VNPU Hardware Codec Elements
#ifdef HAVE_DXVNPU
    GstRank vnpu_codec_rank = GST_RANK_NONE;
    if (dxvnpu::GetDeviceCount() > 0) {
        vnpu_codec_rank = static_cast<GstRank>(GST_RANK_PRIMARY + 1);
    }
    if (!gst_element_register(plugin, "dxvnpudec", vnpu_codec_rank,
                              GST_TYPE_DXVNPUDEC)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxvnpuenc", vnpu_codec_rank,
                              GST_TYPE_DXVNPUENC)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxvnpupipeline", GST_RANK_NONE,
                              GST_TYPE_DXVNPUPIPELINE)) {
        return FALSE;
    }
    if (!gst_element_register(plugin, "dxvnpuoverlay", GST_RANK_NONE,
                              GST_TYPE_DXVNPUOVERLAY)) {
        return FALSE;
    }
#endif
    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, dxstream,
                  "DX Stream plugin", plugin_init, PACKAGE_VERSION, GST_LICENSE,
                  GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
