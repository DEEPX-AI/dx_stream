#include <gst/gst.h>

#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>

#include "dx_msgconvl_priv.hpp"
#include "libyuv_transform.hpp"

DxMsgContextPriv *dxcontext_create_contextPriv(void) {
    DxMsgContextPriv *contextPriv = g_new0(DxMsgContextPriv, 1);
    return contextPriv;
}

void dxcontext_delete_contextPriv(DxMsgContextPriv *contextPriv) {
    g_return_if_fail(contextPriv != nullptr);
    contextPriv->_object_include_list.clear();
    g_free(contextPriv);
}

/*** Config Example: for cctv-city-road2.mov
 *
 * {
 *     "infoSection": {
 *         "customId": 999
 *     },
 *     "filterSection": {
 *         "object": {
 *             "include" : ["person", "bus"]
 *         }
 *     }
 * }
 */

constexpr const char *CFG_KEY_INFO_SECTION = "infoSection";
constexpr const char *CFG_KEY_IS_CUSTOM_ID = "customId";
constexpr const char *CFG_KEY_FILTER_SECTION = "filterSection";
constexpr const char *CFG_KEY_FS_OBJECT = "object";
constexpr const char *CFG_KEY_FS_INCLUDE = "include";
constexpr int CFG_VALUE_INVALID = 0;

bool dxcontext_parse_json_config(const gchar *file,
                                 DxMsgContextPriv *contextPriv) {
    if (file == nullptr) {
        return false;
    }

    if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
        GST_WARNING("File does not exist: %s\n", file);
        return false;
    }

    JsonParser *parser = json_parser_new();
    GError *error = nullptr;

    if (!json_parser_load_from_file(parser, file, &error)) {
        GST_WARNING("Unable to parse JSON: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return false;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonObject *root_obj = json_node_get_object(root);

    contextPriv->_customId = CFG_VALUE_INVALID;

    if (json_object_has_member(root_obj, CFG_KEY_INFO_SECTION)) {
        JsonObject *info_section =
            json_object_get_object_member(root_obj, CFG_KEY_INFO_SECTION);
        if (json_object_has_member(info_section, CFG_KEY_IS_CUSTOM_ID)) {
            contextPriv->_customId =
                json_object_get_int_member(info_section, CFG_KEY_IS_CUSTOM_ID);
        }
    }

    JsonObject *filter_section =
        json_object_get_object_member(root_obj, CFG_KEY_FILTER_SECTION);
    if (filter_section == nullptr) {
        return false;
    }

    JsonObject *object_node =
        json_object_get_object_member(filter_section, CFG_KEY_FS_OBJECT);
    if (object_node == nullptr) {
        return false;
    }

    JsonArray *include_array =
        json_object_get_array_member(object_node, CFG_KEY_FS_INCLUDE);
    if (include_array == nullptr) {
        return false;
    }
    contextPriv->_object_include_list.clear();

    for (guint i = 0; i < json_array_get_length(include_array); i++) {
        const char *list_item_str =
            json_array_get_string_element(include_array, i);
        if (list_item_str != nullptr) {
            contextPriv->_object_include_list.push_back(list_item_str);
        } else {
            g_warning(
                "Item at index %u in '%s' array is not a string or is null.", i,
                CFG_KEY_FS_INCLUDE);
        }
    }

#if 0 /* debug purpose */
    g_print("Custom ID: %d\n", contextPriv->_customId);
    g_print("Include List: ");
    for (const auto &item : contextPriv->_object_include_list) {
        g_print("%s ", item.c_str());
    }
    g_print("\n");
#endif

    g_object_unref(parser);
    return true;
}

/*** Payload format: json
 *
 * {
 *   "streamId" : 0,
 *   "seqId" : 1,
 *   "width" : 1920,
 *   "height" : 1080,
 *   "customId" : 999,
 *   "objects" : [
 *     {
 *         OBJECT_TYPE: { ... }
 *     },
 *     ...
 *   ],
 *   "segment" : {
 *     "height" : 384,
 *     "width" : 768,
 *     "data" : 140040227284784
 *   }
 * }
 *
 *
 *** OBJECT_TYPE: "object" | "track" |  "face" | "pose"
 *
 *       "object" : {
 *         "id" : 0,
 *         "confidence" : 0.7830657958984375,
 *         "name" : "face",
 *         "box" : {
 *           "startX" : 828,
 *           "startY" : 271,
 *           "endX" : 887,
 *           "endY" : 350
 *         }
 *       }
 *
 *       "track" : {
 *         "id" : 1,
 *         "box" : {
 *           "startX" : 320,
 *           "startY" : 232,
 *           "endX" : 639,
 *           "endY" : 926
 *         }
 *       }
 *
 *       "face" : {
 *         "landmark" : [
 *           {
 *             "x" : 838,
 *             "y" : 299
 *           },
 *           ...
 *       }
 *
 *       "pose" : {
 *         "keypoints" : [
 *           {
 *             "kx" : 118,
 *             "ky" : 272,
 *             "ks" : 0.92519611120223999
 *           },
 *           ...
 *         ]
 *       },
 */

static void add_segment_to_json(JsonObject *jobj_root, DXObjectMeta *obj_meta) {
    if (obj_meta->_seg_cls_map.data.empty())
        return;

    GST_DEBUG("|SEGMENT| height: %d, width: %d, data: %p",
              obj_meta->_seg_cls_map.height, obj_meta->_seg_cls_map.width,
              obj_meta->_seg_cls_map.data.data());

    JsonObject *jobj_segment = json_object_new();
    json_object_set_int_member(jobj_segment, "height",
                               obj_meta->_seg_cls_map.height);
    json_object_set_int_member(jobj_segment, "width",
                               obj_meta->_seg_cls_map.width);
    json_object_set_int_member(
        jobj_segment, "data",
        reinterpret_cast<uintptr_t>(obj_meta->_seg_cls_map.data.data()));

    json_object_set_object_member(jobj_root, "segment", jobj_segment);
}

static void add_pose_to_json(JsonObject *jobj_parent, DXObjectMeta *obj_meta) {
    if (obj_meta->_keypoints.empty())
        return;

    GST_DEBUG("|POSE| keypoints: [ {%f, %f, %f}, ...]", obj_meta->_keypoints[0],
              obj_meta->_keypoints[1], obj_meta->_keypoints[2]);

    JsonArray *jarray = json_array_new();
    for (int k = 0; k < 17; k++) {
        float kx = obj_meta->_keypoints[k * 3 + 0];
        float ky = obj_meta->_keypoints[k * 3 + 1];
        float ks = obj_meta->_keypoints[k * 3 + 2];
        JsonObject *jobj_keyset = json_object_new();
        json_object_set_int_member(jobj_keyset, "kx", kx);
        json_object_set_int_member(jobj_keyset, "ky", ky);
        json_object_set_double_member(jobj_keyset, "ks", ks);
        json_array_add_element(
            jarray, json_node_init_object(json_node_alloc(), jobj_keyset));
    }
    JsonObject *jobj_object = json_object_new();
    json_object_set_array_member(jobj_object, "keypoints", jarray);
    json_object_set_object_member(jobj_parent, "pose", jobj_object);
}

static void add_face_to_json(JsonObject *jobj_parent, DXObjectMeta *obj_meta) {
    if (obj_meta->_face_landmarks.empty())
        return;

    GST_DEBUG("|JCP| [FACE] landmark: [{%f, %f}, ...]",
              obj_meta->_face_landmarks[0]._x, obj_meta->_face_landmarks[0]._y);

    JsonArray *jarray = json_array_new();
    for (auto &landmark : obj_meta->_face_landmarks) {
        JsonObject *jobj_point = json_object_new();
        json_object_set_int_member(jobj_point, "x", landmark._x);
        json_object_set_int_member(jobj_point, "y", landmark._y);
        json_array_add_element(
            jarray, json_node_init_object(json_node_alloc(), jobj_point));
    }
    JsonObject *jobj_object = json_object_new();
    json_object_set_array_member(jobj_object, "landmark", jarray);
    json_object_set_object_member(jobj_parent, "face", jobj_object);
}

static void add_track_to_json(JsonObject *jobj_parent, DXObjectMeta *obj_meta) {
    if (obj_meta->_track_id == -1)
        return;

    GST_DEBUG("|TRACK| TrackId: %d, Box: {%f, %f, %f, %f}", obj_meta->_track_id,
              obj_meta->_box[0], obj_meta->_box[1], obj_meta->_box[2],
              obj_meta->_box[3]);

    JsonObject *jobj_object = json_object_new();
    json_object_set_int_member(jobj_object, "id", obj_meta->_track_id);

    JsonObject *jobj_box = json_object_new();
    json_object_set_int_member(jobj_box, "startX", obj_meta->_box[0]);
    json_object_set_int_member(jobj_box, "startY", obj_meta->_box[1]);
    json_object_set_int_member(jobj_box, "endX", obj_meta->_box[2]);
    json_object_set_int_member(jobj_box, "endY", obj_meta->_box[3]);

    json_object_set_object_member(jobj_object, "box", jobj_box);
    json_object_set_object_member(jobj_parent, "track", jobj_object);
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
    json_object_set_int_member(jobj_object, "id", obj_meta->_label);
    json_object_set_double_member(jobj_object, "confidence",
                                  obj_meta->_confidence);
    json_object_set_string_member(jobj_object, "name",
                                  obj_meta->_label_name->str);

    JsonObject *jobj_box = json_object_new();
    json_object_set_int_member(jobj_box, "startX", obj_meta->_box[0]);
    json_object_set_int_member(jobj_box, "startY", obj_meta->_box[1]);
    json_object_set_int_member(jobj_box, "endX", obj_meta->_box[2]);
    json_object_set_int_member(jobj_box, "endY", obj_meta->_box[3]);

    json_object_set_object_member(jobj_object, "box", jobj_box);
    json_object_set_object_member(jobj_parent, "object", jobj_object);
}

static bool is_object_included(DxMsgContextPriv *contextPriv,
                               DXObjectMeta *obj_meta) {
    if (obj_meta->_label == -1)
        return true; // -1인 경우 필터링 안함
    if (contextPriv->_object_include_list.empty())
        return true;

    for (const auto &item : contextPriv->_object_include_list) {
        if (g_strcmp0(item.c_str(), obj_meta->_label_name->str) == 0) {
            return true;
        }
    }
    return false;
}

static void add_object_meta_to_json(JsonArray *jarray_objects,
                                    JsonObject *jobj_root,
                                    DXObjectMeta *obj_meta) {
    JsonNode *jnode_object = json_node_new(JSON_NODE_OBJECT);
    JsonObject *jobj_parent = json_object_new();

    add_segment_to_json(jobj_root, obj_meta);
    add_pose_to_json(jobj_parent, obj_meta);
    add_face_to_json(jobj_parent, obj_meta);
    add_track_to_json(jobj_parent, obj_meta);
    add_object_to_json(jobj_parent, obj_meta);

    // segment은 루트에 따로 넣고, 그 외는 objects 배열에 추가
    if (obj_meta->_seg_cls_map.data.empty()) {
        json_node_set_object(jnode_object, jobj_parent);
        json_array_add_element(jarray_objects, jnode_object);
    }
}

gchar *dxpayload_convert_to_json(DxMsgContext *context,
                                 DxMsgMetaInfo *meta_info) {
    DXFrameMeta *frame_meta = (DXFrameMeta *)(meta_info->_frame_meta);
    DxMsgContextPriv *contextPriv = (DxMsgContextPriv *)context->_priv_data;

    JsonNode *jnode_root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *jobj_root = json_object_new();

    // 기본 메타 정보 설정
    json_object_set_int_member(jobj_root, "streamId", frame_meta->_stream_id);
    json_object_set_int_member(jobj_root, "seqId", meta_info->_seq_id);
    json_object_set_int_member(jobj_root, "width", frame_meta->_width);
    json_object_set_int_member(jobj_root, "height", frame_meta->_height);
    json_object_set_int_member(jobj_root, "customId", contextPriv->_customId);

    // frame data 처리
    if (meta_info->_include_frame) {
        std::vector<uchar> buf;
        uint8_t *rgb_buffer = nullptr;
        CvtColor(frame_meta->_buf, &rgb_buffer, frame_meta->_width,
                 frame_meta->_height, frame_meta->_format, "RGB");
        cv::Mat rgb_surface = cv::Mat(frame_meta->_height, frame_meta->_width,
                                      CV_8UC3, rgb_buffer);
        cv::imencode(".jpg", rgb_surface, buf);
        auto base64_str = g_base64_encode(buf.data(), buf.size());
        json_object_set_string_member(jobj_root, "frameData", base64_str);
        g_free(base64_str);
        free(rgb_buffer);
    } else {
        json_object_set_string_member(jobj_root, "frameData", "");
    }

    // objects 배열 생성
    JsonArray *jarray_objects = json_array_new();
    json_object_set_array_member(jobj_root, "objects", jarray_objects);

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);
    for (size_t i = 0; i < object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);

        if (!is_object_included(contextPriv, obj_meta)) {
            continue;
        }
        add_object_meta_to_json(jarray_objects, jobj_root, obj_meta);
    }

    json_node_set_object(jnode_root, jobj_root);
    gchar *json_data = json_to_string(jnode_root, TRUE);

    json_node_free(jnode_root);
    json_object_unref(jobj_root);

    return json_data;
}
