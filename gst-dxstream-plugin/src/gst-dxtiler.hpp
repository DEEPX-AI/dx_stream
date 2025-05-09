#ifndef GST_DXTILER_H
#define GST_DXTILER_H

#include <gst/gst.h>
#include <mutex>

G_BEGIN_DECLS

#define GST_TYPE_DXTILER (gst_dxtiler_get_type())
G_DECLARE_FINAL_TYPE(GstDxTiler, gst_dxtiler, GST, DXTILER, GstElement)

struct _GstDxTiler {
    GstElement _parent_instance;
    GstElement *_pipeline;
    GstPad *_srcpad;
    GstCaps *_caps;
    GstBuffer *_outbuffer;
    std::mutex _buffer_lock;

    gchar *_config_file_path;
    gint _width;
    gint _height;

    GstClockTime _last_pts;

    gint _cols;
    gint _rows;

    uint8_t *_resized_frame;
    uint8_t *_convert_frame;
};

G_END_DECLS

#endif // GST_DXTILER_H