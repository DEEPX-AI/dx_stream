#include <gst/gst.h>

inline gint get_sink_pad_index(GstPad *sinkpad) {
    const gchar *pad_name = gst_pad_get_name(sinkpad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "sink_")) {
        pad_index = atoi(pad_name + 5);
    }
    return pad_index;
}

inline gint get_src_pad_index(GstPad *srcpad) {
    const gchar *pad_name = gst_pad_get_name(srcpad);
    gint pad_index = -1;
    if (g_str_has_prefix(pad_name, "src_")) {
        pad_index = atoi(pad_name + 4);
    }
    return pad_index;
}
