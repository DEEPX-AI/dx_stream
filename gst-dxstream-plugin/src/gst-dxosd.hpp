#ifndef GST_DXOSD_H
#define GST_DXOSD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdint>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <map>

#ifdef HAVE_LIBRGA
#include "rga/RgaUtils.h"
#include "rga/im2d.hpp"
#else
#include "libyuv_transform/libyuv_transform.hpp"
#endif

G_BEGIN_DECLS

#define GST_TYPE_DXOSD (gst_dxosd_get_type())
G_DECLARE_FINAL_TYPE(GstDxOsd, gst_dxosd, GST, DXOSD, GstElement)

struct _GstDxOsd {
    GstElement _parent_instance;

    GstPad *_sinkpad;
    GstPad *_srcpad;

    gint _width;
    gint _height;

    GstVideoInfo _input_info;
    GstVideoInfo _output_info;
    GstCaps *_output_caps;

#ifdef HAVE_LIBRGA
#else
    std::map<int, uint8_t *> _resized_frame;
#endif
};

G_END_DECLS

#endif // GST_DXOSD_H
