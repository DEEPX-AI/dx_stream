#include "dxgenbuffer.hpp"
#include <gst/app/gstappsrc.h>
#include <gst/base/gstbasesrc.h>
#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PROP_0, PROP_IMAGE_PATH, PROP_FRAMERATE };

GST_DEBUG_CATEGORY_STATIC(gst_dxgenbuffer_debug_category);
#define GST_CAT_DEFAULT gst_dxgenbuffer_debug_category

G_DEFINE_TYPE(GstDxGenBuffer, gst_dxgenbuffer, GST_TYPE_PUSH_SRC);

GstBuffer *read_image_to_buffer(const gchar *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        GST_ERROR("Can't not found image file: %s", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    guint8 *buffer_data = (guint8 *)g_malloc(file_size);
    fread(buffer_data, 1, file_size, file);
    fclose(file);

    GstBuffer *buffer = gst_buffer_new_allocate(NULL, file_size, NULL);
    gst_buffer_fill(buffer, 0, buffer_data, file_size);
    g_free(buffer_data);

    return buffer;
}

GstFlowReturn gst_dxgenbuffer_create(GstPushSrc *src, GstBuffer **buffer) {
    GstDxGenBuffer *dxgen = (GstDxGenBuffer *)src;

    if (!dxgen->image_path) {
        GST_ERROR("image-path 프로퍼티가 설정되지 않았습니다!");
        return GST_FLOW_ERROR;
    }

    *buffer = read_image_to_buffer(dxgen->image_path);
    if (!*buffer) {
        return GST_FLOW_ERROR;
    }

    // 🔹 GstClock 가져오기 (NULL 체크 추가)
    GstElement *element = GST_ELEMENT(src);
    GstClock *clock = gst_element_get_clock(element);
    if (!clock) {
        GST_WARNING("Clock이 NULL입니다. 시스템 클럭을 사용합니다.");
        clock = gst_system_clock_obtain();
    }

    GstClockTime now = gst_clock_get_time(clock);
    GST_BUFFER_PTS(*buffer) = gst_util_uint64_scale(now, GST_SECOND, 1);
    GST_BUFFER_DTS(*buffer) = GST_BUFFER_PTS(*buffer);
    GST_BUFFER_DURATION(*buffer) = dxgen->frame_duration;
    return GST_FLOW_OK;
}

void gst_dxgenbuffer_set_property(GObject *object, guint prop_id,
                                  const GValue *value, GParamSpec *pspec) {
    GstDxGenBuffer *dxgen = (GstDxGenBuffer *)object;

    switch (prop_id) {
    case PROP_IMAGE_PATH:
        g_free(dxgen->image_path);
        dxgen->image_path = g_value_dup_string(value);
        break;
    case PROP_FRAMERATE:
        dxgen->framerate_n = gst_value_get_fraction_numerator(value);
        dxgen->framerate_d = gst_value_get_fraction_denominator(value);
        dxgen->frame_duration = gst_util_uint64_scale(
            dxgen->framerate_d, GST_SECOND, dxgen->framerate_n);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void gst_dxgenbuffer_get_property(GObject *object, guint prop_id, GValue *value,
                                  GParamSpec *pspec) {
    GstDxGenBuffer *dxgen = (GstDxGenBuffer *)object;

    switch (prop_id) {
    case PROP_IMAGE_PATH:
        g_value_set_string(value, dxgen->image_path);
        break;
    case PROP_FRAMERATE:
        gst_value_set_fraction(value, dxgen->framerate_n, dxgen->framerate_d);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_dxgenbuffer_init(GstDxGenBuffer *dxgen) {
    dxgen->image_path = g_strdup("");
    dxgen->framerate_n = 1;
    dxgen->framerate_d = 1;
    dxgen->frame_duration = GST_SECOND;

    gst_base_src_set_live(GST_BASE_SRC(dxgen), TRUE);
}

static void gst_dxgenbuffer_class_init(GstDxGenBufferClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    GstPushSrcClass *pushsrc_class = GST_PUSH_SRC_CLASS(klass);

    static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE(
        "src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_STATIC_CAPS("image/jpeg"));

    gst_element_class_add_pad_template(
        element_class, gst_static_pad_template_get(&src_template));

    gobject_class->set_property = gst_dxgenbuffer_set_property;
    gobject_class->get_property = gst_dxgenbuffer_get_property;

    g_object_class_install_property(
        gobject_class, PROP_IMAGE_PATH,
        g_param_spec_string("image-path", "Image Path",
                            "Path to the image file", NULL, G_PARAM_READWRITE));

    g_object_class_install_property(
        gobject_class, PROP_FRAMERATE,
        gst_param_spec_fraction(
            "framerate", "Framerate",
            "Framerate as a fraction (e.g., 30/1 for 30 FPS)", 1, 1, 120, 1, 30,
            1, G_PARAM_READWRITE));

    pushsrc_class->create = GST_DEBUG_FUNCPTR(gst_dxgenbuffer_create);

    gst_element_class_set_metadata(
        element_class, "DxGenBuffer", "Source/Custom",
        "Reads an image file as a buffer with specified framerate",
        "Your Name");
}
