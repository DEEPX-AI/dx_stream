// GstBuffer creation helper with standard caps
// Use when raw buffers are needed instead of videotestsrc (specific PTS / meta attachment, etc.).

#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>

namespace dxtest {

inline GstCaps *caps_raw(const char *format = "RGB", int w = 640, int h = 480,
                          int fps_n = 30, int fps_d = 1) {
    return gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, format,
                               "width", G_TYPE_INT, w,
                               "height", G_TYPE_INT, h,
                               "framerate", GST_TYPE_FRACTION, fps_n, fps_d,
                               nullptr);
}

inline GstCaps *caps_tensors() {
    return gst_caps_new_empty_simple("other/tensors");
}

inline gsize plane_size(const char *format, int w, int h) {
    if (g_strcmp0(format, "I420") == 0 ||
        g_strcmp0(format, "NV12") == 0 ||
        g_strcmp0(format, "YV12") == 0) return (gsize)w * h * 3 / 2;
    if (g_strcmp0(format, "RGB") == 0 ||
        g_strcmp0(format, "BGR") == 0) return (gsize)w * h * 3;
    if (g_strcmp0(format, "RGBA") == 0 ||
        g_strcmp0(format, "BGRA") == 0) return (gsize)w * h * 4;
    if (g_strcmp0(format, "GRAY8") == 0) return (gsize)w * h;
    return (gsize)w * h * 3;  // fallback
}

inline GstBuffer *make_video_buffer(const char *format, int w, int h,
                                     GstClockTime pts = 0,
                                     GstClockTime duration = GST_SECOND / 30) {
    gsize sz = plane_size(format, w, h);
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GstMapInfo info;
    if (gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        memset(info.data, 0x80, info.size);  // mid-gray
        gst_buffer_unmap(buf, &info);
    }
    GST_BUFFER_PTS(buf) = pts;
    GST_BUFFER_DTS(buf) = pts;
    GST_BUFFER_DURATION(buf) = duration;
    return buf;
}

inline GstBuffer *make_buffer_no_pts(gsize sz = 1024) {
    GstBuffer *buf = gst_buffer_new_allocate(nullptr, sz, nullptr);
    GST_BUFFER_PTS(buf) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DTS(buf) = GST_CLOCK_TIME_NONE;
    return buf;
}

}  // namespace dxtest
