#include <gst/gst.h>

#include <json-glib/json-glib.h>

#include "dx_msgconvl_priv.hpp"

DxMsgContextPriv *dxcontext_create_contextPriv(void) {
    DxMsgContextPriv *contextPriv = g_new0(DxMsgContextPriv, 1);
    return contextPriv;
}

void dxcontext_delete_contextPriv(DxMsgContextPriv *contextPriv) {
    g_return_if_fail(contextPriv != nullptr);
    contextPriv->_object_include_list.clear();
    g_free(contextPriv);
}

static void add_object_to_json(JsonObject *jobj_parent,
                               DXObjectMeta *obj_meta) {
    if (obj_meta->_label == -1)
        return;

    GST_DEBUG("|OBJECT| LabelId: %d, Confidence: %.2f, Box: {%f, %f, %f, %f}, "
              "Label Name: %s",
              obj_meta->_label, obj_meta->_confidence, obj_meta->_box[0],
              obj_meta->_box[1], obj_meta->_box[2], obj_meta->_box[3],
              obj_meta->_label_name->str);

    JsonObject *jobj_object = json_object_new();
    json_object_set_int_member(jobj_object, "label_id", obj_meta->_label);
    json_object_set_int_member(jobj_object, "track_id", obj_meta->_track_id);
    json_object_set_double_member(jobj_object, "confidence",
                                  obj_meta->_confidence);
    json_object_set_string_member(jobj_object, "name",
                                  obj_meta->_label_name->str);

    JsonObject *jobj_box = json_object_new();
    json_object_set_double_member(jobj_box, "startX", obj_meta->_box[0]);
    json_object_set_double_member(jobj_box, "startY", obj_meta->_box[1]);
    json_object_set_double_member(jobj_box, "endX", obj_meta->_box[2]);
    json_object_set_double_member(jobj_box, "endY", obj_meta->_box[3]);
    json_object_set_object_member(jobj_object, "box", jobj_box);
    
    // body_feature 추가
    if (!obj_meta->_body_feature.empty()) {
        JsonArray *jarray_body = json_array_new();
        for (auto &val : obj_meta->_body_feature) {
            json_array_add_element(jarray_body, json_node_init_double(json_node_alloc(), val));
        }
        json_object_set_array_member(jobj_object, "body_feature", jarray_body);
    }
    
    // segment 추가 (object 내부로)
    if (!obj_meta->_seg_cls_map.data.empty()) {
        JsonObject *jobj_segment = json_object_new();
        json_object_set_int_member(jobj_segment, "height",
                                   obj_meta->_seg_cls_map.height);
        json_object_set_int_member(jobj_segment, "width",
                                   obj_meta->_seg_cls_map.width);
        json_object_set_int_member(
            jobj_segment, "data",
            reinterpret_cast<uintptr_t>(obj_meta->_seg_cls_map.data.data()));
        json_object_set_object_member(jobj_object, "segment", jobj_segment);
    }
    
    // pose 추가 (object 내부로)
    if (!obj_meta->_keypoints.empty()) {
        JsonArray *jarray = json_array_new();
        for (int k = 0; k < 17; k++) {
            float kx = obj_meta->_keypoints[k * 3 + 0];
            float ky = obj_meta->_keypoints[k * 3 + 1];
            float ks = obj_meta->_keypoints[k * 3 + 2];
            JsonObject *jobj_keyset = json_object_new();
            json_object_set_double_member(jobj_keyset, "kx", kx);
            json_object_set_double_member(jobj_keyset, "ky", ky);
            json_object_set_double_member(jobj_keyset, "ks", ks);
            json_array_add_element(
                jarray, json_node_init_object(json_node_alloc(), jobj_keyset));
        }
        JsonObject *jobj_pose = json_object_new();
        json_object_set_array_member(jobj_pose, "keypoints", jarray);
        json_object_set_object_member(jobj_object, "pose", jobj_pose);
    }
    
    // face 추가 (object 내부로)
    if (!obj_meta->_face_landmarks.empty()) {
        JsonArray *jarray = json_array_new();
        for (auto &landmark : obj_meta->_face_landmarks) {
            JsonObject *jobj_point = json_object_new();
            json_object_set_double_member(jobj_point, "x", landmark._x);
            json_object_set_double_member(jobj_point, "y", landmark._y);
            json_array_add_element(
                jarray, json_node_init_object(json_node_alloc(), jobj_point));
        }
        JsonObject *jobj_face = json_object_new();
        json_object_set_array_member(jobj_face, "landmark", jarray);
        
        // face_box 추가
        JsonObject *jobj_face_box = json_object_new();
        json_object_set_double_member(jobj_face_box, "startX", obj_meta->_face_box[0]);
        json_object_set_double_member(jobj_face_box, "startY", obj_meta->_face_box[1]);
        json_object_set_double_member(jobj_face_box, "endX", obj_meta->_face_box[2]);
        json_object_set_double_member(jobj_face_box, "endY", obj_meta->_face_box[3]);
        json_object_set_object_member(jobj_face, "box", jobj_face_box);
        
        // face_confidence 추가
        json_object_set_double_member(jobj_face, "confidence", obj_meta->_face_confidence);
        
        // face_feature 추가
        if (!obj_meta->_face_feature.empty()) {
            JsonArray *jarray_face = json_array_new();
            for (auto &val : obj_meta->_face_feature) {
                json_array_add_element(jarray_face, json_node_init_double(json_node_alloc(), val));
            }
            json_object_set_array_member(jobj_face, "face_feature", jarray_face);
        }
        
        json_object_set_object_member(jobj_object, "face", jobj_face);
    }
    
    json_object_set_object_member(jobj_parent, "object", jobj_object);
}

static void add_object_meta_to_json(JsonArray *jarray_objects,
                                    DXObjectMeta *obj_meta) {
    JsonNode *jnode_object = json_node_new(JSON_NODE_OBJECT);
    JsonObject *jobj_parent = json_object_new();

    add_object_to_json(jobj_parent, obj_meta);

    json_node_set_object(jnode_object, jobj_parent);
    json_array_add_element(jarray_objects, jnode_object);
}

/*
 * Sample JSON Output Format:
 * {
 *   "streamId": 0,
 *   "seqId": 123,
 *   "width": 1920,
 *   "height": 1080,
 *   "objects": [
 *     {
 *       "object": {
 *         "label_id": 1,
 *         "track_id": 42,
 *         "confidence": 0.87,
 *         "name": "person",
 *         "box": {
 *           "startX": 300.0,
 *           "startY": 400.0,
 *           "endX": 500.0,
 *           "endY": 600.0
 *         },
 *         "body_feature": [0.321, 0.654, 0.987],
 *         "segment": {
 *           "height": 1080,
 *           "width": 1920,
 *           "data": 140712345678912
 *         },
 *         "pose": {
 *           "keypoints": [
 *             {"kx": 100.5, "ky": 200.3, "ks": 0.8},
 *             {"kx": 105.2, "ky": 205.7, "ks": 0.9}
 *           ]
 *         },
 *         "face": {
 *           "landmark": [
 *             {"x": 150.2, "y": 180.5},
 *             {"x": 155.8, "y": 185.3}
 *           ],
 *           "box": {
 *             "startX": 100.0,
 *             "startY": 150.0,
 *             "endX": 200.0,
 *             "endY": 250.0
 *           },
 *           "confidence": 0.95,
 *           "face_feature": [0.123, 0.456, 0.789]
 *         }
 *       }
 *     }
 *   ]
 * }
 */
gchar *dxpayload_convert_to_json(DxMsgContext *context,
                                 GstDxMsgMetaInfo *meta_info) {
    DXFrameMeta *frame_meta = (DXFrameMeta *)(meta_info->_frame_meta);
    DxMsgContextPriv *contextPriv = (DxMsgContextPriv *)context->_priv_data;

    JsonNode *jnode_root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *jobj_root = json_object_new();

    json_object_set_int_member(jobj_root, "streamId", frame_meta->_stream_id);
    json_object_set_int_member(jobj_root, "seqId", meta_info->_seq_id);
    json_object_set_int_member(jobj_root, "width", frame_meta->_width);
    json_object_set_int_member(jobj_root, "height", frame_meta->_height);

    JsonArray *jarray_objects = json_array_new();
    json_object_set_array_member(jobj_root, "objects", jarray_objects);

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);
    for (size_t i = 0; i < object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
        add_object_meta_to_json(jarray_objects, obj_meta);
    }

    json_node_set_object(jnode_root, jobj_root);
    gchar *json_data = json_to_string(jnode_root, TRUE);

    json_node_free(jnode_root);
    json_object_unref(jobj_root);

    return json_data;
}
