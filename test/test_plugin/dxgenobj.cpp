#include "dxgenobj.hpp"
#include "gst-dxmeta.hpp"

enum {
    PROP_0,
    PROP_BOX,
    PROP_FACE_BOX,
    PROP_LABEL,
    PROP_CONFIDENCE,
    PROP_TRACK_ID,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {
    NULL,
};

GST_DEBUG_CATEGORY_STATIC(gst_dxgenobj_debug_category);
#define GST_CAT_DEFAULT gst_dxgenobj_debug_category

static GstFlowReturn gst_dxgenobj_transform_ip(GstBaseTransform *trans,
                                               GstBuffer *buf);
static gboolean gst_dxgenobj_start(GstBaseTransform *trans);
static gboolean gst_dxgenobj_stop(GstBaseTransform *trans);
static void gst_dxgenobj_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec);
static void gst_dxgenobj_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec);

G_DEFINE_TYPE_WITH_CODE(
    GstDxGenObj, gst_dxgenobj, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_dxgenobj_debug_category, "gst-dxgenobj", 0,
                            "debug category for gst-dxgenobj element"))

static GstElementClass *parent_class = NULL;

static void gst_dxgenobj_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec) {
    GstDxGenObj *self = GST_DXGENOBJ(object);

    switch (property_id) {
    case PROP_BOX:
        self->_box = g_value_get_boolean(value);
        break;
    case PROP_FACE_BOX:
        self->_face_box = g_value_get_boolean(value);
        break;
    case PROP_LABEL:
        self->_label = g_value_get_boolean(value);
        break;
    case PROP_CONFIDENCE:
        self->_confidence = g_value_get_boolean(value);
        break;
    case PROP_TRACK_ID:
        self->_track_id = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gst_dxgenobj_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec) {
    GstDxGenObj *self = GST_DXGENOBJ(object);

    switch (property_id) {
    case PROP_BOX:
        g_value_set_boolean(value, self->_box);
        break;
    case PROP_FACE_BOX:
        g_value_set_boolean(value, self->_face_box);
        break;
    case PROP_LABEL:
        g_value_set_boolean(value, self->_label);
        break;
    case PROP_CONFIDENCE:
        g_value_set_boolean(value, self->_confidence);
        break;
    case PROP_TRACK_ID:
        g_value_set_boolean(value, self->_track_id);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void gst_dxgenobj_class_init(GstDxGenObjClass *klass) {
    GST_DEBUG_CATEGORY_INIT(gst_dxgenobj_debug_category, "dxgenobj", 0,
                            "dxgenobj plugin");

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gobject_class->set_property = gst_dxgenobj_set_property;
    gobject_class->get_property = gst_dxgenobj_get_property;

    obj_properties[PROP_BOX] = g_param_spec_boolean(
        "box", "ADD RANDOM BOX", "ADD RANDOM BOX", FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_FACE_BOX] =
        g_param_spec_boolean("face-box", "ADD RANDOM FACE BOX",
                             "ADD RANDOM FACE BOX", FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_LABEL] =
        g_param_spec_boolean("label", "ADD RANDOM LABEL", "ADD RANDOM LABEL",
                             FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_CONFIDENCE] =
        g_param_spec_boolean("confidence", "ADD RANDOM CONFIDENCE",
                             "ADD RANDOM CONFIDENCE", FALSE, G_PARAM_READWRITE);

    obj_properties[PROP_TRACK_ID] =
        g_param_spec_boolean("track_id", "ADD RANDOM TRACK_ID",
                             "ADD RANDOM TRACK_ID", FALSE, G_PARAM_READWRITE);

    g_object_class_install_properties(gobject_class, N_PROPERTIES,
                                      obj_properties);

    GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
    gst_element_class_set_static_metadata(element_class, "DXGenObj", "Generic",
                                          "Gen DXObjectMeta",
                                          "Jo Sangil <sijo@deepx.ai>");

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                             GST_CAPS_ANY));

    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, GST_CAPS_ANY));

    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_dxgenobj_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_dxgenobj_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_dxgenobj_transform_ip);
    parent_class = GST_ELEMENT_CLASS(g_type_class_peek_parent(klass));
}

static void gst_dxgenobj_init(GstDxGenObj *self) {
    GST_DEBUG_OBJECT(self, "init");
    self->_box = FALSE;
    self->_face_box = FALSE;
    self->_label = FALSE;
    self->_confidence = FALSE;
    self->_track_id = FALSE;
}

static gboolean gst_dxgenobj_start(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "start");
    return TRUE;
}

static gboolean gst_dxgenobj_stop(GstBaseTransform *trans) {
    GST_DEBUG_OBJECT(trans, "stop");
    return TRUE;
}

static GstFlowReturn gst_dxgenobj_transform_ip(GstBaseTransform *trans,
                                               GstBuffer *buf) {
    GstDxGenObj *self = GST_DXGENOBJ(trans);
    DXFrameMeta *frame_meta =
        (DXFrameMeta *)gst_buffer_get_meta(buf, DX_FRAME_META_API_TYPE);

    if (!frame_meta) {
        frame_meta = dx_create_frame_meta(buf);
        frame_meta->_buf = buf;
        frame_meta->_format = "I420";
        frame_meta->_name = "test";
        DXObjectMeta *tmp_object_meta = dx_create_object_meta(buf);
        dx_add_object_meta_to_frame_meta(tmp_object_meta, frame_meta);
    }

    DXObjectMeta *object_meta =
        (DXObjectMeta *)gst_buffer_get_meta(buf, DX_OBJECT_META_API_TYPE);

    if (!object_meta) {
        g_error("No DXObjectMeta in GstBuffer \n");
    }

    if (self->_box) {
        object_meta->_box[0] = 100;
        object_meta->_box[1] = 150;
        object_meta->_box[2] = 1100;
        object_meta->_box[3] = 1500;
    }
    if (self->_face_box) {
        object_meta->_face_box[0] = 10;
        object_meta->_face_box[1] = 15;
        object_meta->_face_box[2] = 110;
        object_meta->_face_box[3] = 150;
    }
    if (self->_label) {
        object_meta->_label = 101;
    }
    if (self->_confidence) {
        object_meta->_confidence = 0.975;
    }
    if (self->_track_id) {
        object_meta->_track_id = 11;
    }

    return GST_FLOW_OK;
}
