#include "utils.hpp"

const gchar *get_buffer_meta_type(GstElement *element) {
    GstCaps *caps;
    GstStructure *structure;

    GstPad *sink_pad;
    sink_pad = gst_element_get_static_pad(element, "sink");
    if (!sink_pad) {
        g_warning("Failed to get sink pad!");
        return NULL;
    }

    /* Get the current caps of the sink pad */
    caps = gst_pad_get_current_caps(sink_pad);
    if (!caps) {
        g_warning("Sink pad has no caps set!");
        gst_object_unref(sink_pad);
        return NULL;
    }

    /* Get the structure of the caps */
    structure = gst_caps_get_structure(caps, 0);
    if (!structure) {
        g_warning("Failed to get caps structure!");
        gst_caps_unref(caps);
        gst_object_unref(sink_pad);
        return NULL;
    }

    /* Unref the caps after use */
    gst_caps_unref(caps);
    gst_object_unref(sink_pad);

    return gst_structure_get_name(structure);
}
