#include "gst-dxvnpudec.hpp"
#include "gst-dxvnpuenc.hpp"
#include <dxvnpu/dxvnpu_c_api.h>
#include <gst/gst.h>

static gboolean plugin_init(GstPlugin* plugin) {
    GstRank vnpu_codec_rank = GST_RANK_NONE;
    uint64_t device_count = 0;
    if (dxvnpu_get_device_count(&device_count) == DXVNPU_OK && device_count > 0) {
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
    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, dxstream_vnpu,
                  "DX Stream VNPU plugin", plugin_init, PACKAGE_VERSION, GST_LICENSE,
                  GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)
