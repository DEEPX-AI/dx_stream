#include <gst/gst.h>

#include <json-glib/json-glib.h>
#include <opencv2/opencv.hpp>

#include "dx_msgconvl_priv.hpp"
#include "dx_stream/format_convert.hpp"

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

#define CFG_KEY_INFO_SECTION "infoSection"
#define CFG_KEY_IS_CUSTOM_ID "customId"
#define CFG_KEY_FILTER_SECTION "filterSection"
#define CFG_KEY_FS_OBJECT "object"
#define CFG_KEY_FS_INCLUDE "include"
#define CFG_VALUE_INVALID 0

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
    GError *error = NULL;

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

    if (json_object_has_member(root_obj, CFG_KEY_FILTER_SECTION)) {
        JsonObject *filter_section =
            json_object_get_object_member(root_obj, CFG_KEY_FILTER_SECTION);
        if (json_object_has_member(filter_section, CFG_KEY_FS_OBJECT)) {
            JsonObject *object = json_object_get_object_member(
                filter_section, CFG_KEY_FS_OBJECT);
            if (json_object_has_member(object, CFG_KEY_FS_INCLUDE)) {
                JsonArray *include_array =
                    json_object_get_array_member(object, CFG_KEY_FS_INCLUDE);
                contextPriv->_object_include_list.clear();
                for (guint i = 0; i < json_array_get_length(include_array);
                     i++) {
                    contextPriv->_object_include_list.push_back(
                        json_array_get_string_element(include_array, i));
                }
            }
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

gchar *dxpayload_convert_to_json(DxMsgContext *context,
                                 DxMsgMetaInfo *meta_info) {
    DXFrameMeta *frame_meta = (DXFrameMeta *)(meta_info->_frame_meta);

    JsonNode *jnode_root = json_node_new(JSON_NODE_OBJECT);
    JsonObject *jobj_root = json_object_new();

    json_object_set_int_member(jobj_root, "streamId", frame_meta->_stream_id);
    json_object_set_int_member(jobj_root, "seqId", meta_info->_seq_id);
    json_object_set_int_member(jobj_root, "width", frame_meta->_width);
    json_object_set_int_member(jobj_root, "height", frame_meta->_height);

    DxMsgContextPriv *contextPriv = (DxMsgContextPriv *)context->_priv_data;
    json_object_set_int_member(jobj_root, "customId", contextPriv->_customId);

    // frame data
    if (meta_info->_include_frame) {
        std::vector<uchar> buf;

        uint8_t *rgb_buffer =
            CvtColor(frame_meta->_buf, frame_meta->_width, frame_meta->_height,
                     frame_meta->_format, "RGB");
        cv::Mat rgb_surface = cv::Mat(frame_meta->_height, frame_meta->_width,
                                      CV_8UC3, rgb_buffer);
        cv::imencode(".jpg", rgb_surface, buf);
        auto base64_str = g_base64_encode(buf.data(), buf.size());
        json_object_set_string_member(jobj_root, "frameData", base64_str);
        g_free(base64_str);
        free(rgb_buffer);
    } else {
        // json_object_set_string_member(jobj_root, "frameData", "");
    }

    JsonArray *jarray_objects = json_array_new();
    json_object_set_array_member(jobj_root, "objects", jarray_objects);

    unsigned int object_length = g_list_length(frame_meta->_object_meta_list);
    for (int i = 0; i < object_length; i++) {
        DXObjectMeta *obj_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);

        // context's filter: match include
        if (obj_meta->_label != -1 &&
            contextPriv->_object_include_list.size() > 0) {
            bool is_include = false;
            for (const auto &item : contextPriv->_object_include_list) {
                if (g_strcmp0(item.c_str(), obj_meta->_label_name->str) == 0) {
                    is_include = true;
                    break;
                }
            }
            if (!is_include) {
                continue;
            }
        }

        JsonNode *jnode_object = json_node_new(JSON_NODE_OBJECT);
        JsonObject *jobj_parent = json_object_new();
        JsonObject *jobj_segment = nullptr;

        if (obj_meta->_seg_cls_map.data != nullptr) {
            GST_DEBUG("|SEGMENT| height: %d, width: %d, data: %p",
                      obj_meta->_seg_cls_map.height,
                      obj_meta->_seg_cls_map.width,
                      obj_meta->_seg_cls_map.data);
            jobj_segment = json_object_new();
            json_object_set_int_member(jobj_segment, "height",
                                       obj_meta->_seg_cls_map.height);
            json_object_set_int_member(jobj_segment, "width",
                                       obj_meta->_seg_cls_map.width);
            json_object_set_int_member(
                jobj_segment, "data",
                reinterpret_cast<uintptr_t>(obj_meta->_seg_cls_map.data));
        }

        if (obj_meta->_keypoints.size() > 0) {
            GST_DEBUG("|POSE| keypoints: [ {%f, %f, %f}, ...]",
                      obj_meta->_keypoints[0], obj_meta->_keypoints[1],
                      obj_meta->_keypoints[2]);

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
                    jarray,
                    json_node_init_object(json_node_alloc(), jobj_keyset));
            }

            JsonObject *jobj_object = json_object_new();
            json_object_set_array_member(jobj_object, "keypoints", jarray);
            json_object_set_object_member(jobj_parent, "pose", jobj_object);
        }

        if (obj_meta->_face_landmarks.size() > 0) {
            GST_DEBUG("|JCP| [FACE] landmark: [{%f, %f}, ...",
                      obj_meta->_face_landmarks[0]._x,
                      obj_meta->_face_landmarks[0]._y);

            JsonArray *jarray = json_array_new();
            for (auto &landmark : obj_meta->_face_landmarks) {
                JsonObject *jobj_point = json_object_new();
                json_object_set_int_member(jobj_point, "x", landmark._x);
                json_object_set_int_member(jobj_point, "y", landmark._y);
                json_array_add_element(
                    jarray,
                    json_node_init_object(json_node_alloc(), jobj_point));
            }

            JsonObject *jobj_object = json_object_new();
            json_object_set_array_member(jobj_object, "landmark", jarray);
            json_object_set_object_member(jobj_parent, "face", jobj_object);
        }

        if (obj_meta->_track_id != -1) {
            GST_DEBUG("|TRACK| TrackId: %d, Box: {%f, %f, %f, %f}",
                      obj_meta->_track_id, obj_meta->_box[0], obj_meta->_box[1],
                      obj_meta->_box[2], obj_meta->_box[3]);

            JsonObject *jobj_object = json_object_new();
            json_object_set_int_member(jobj_object, "id", obj_meta->_track_id);
            JsonObject *jobj_box = json_object_new();
            json_object_set_int_member(jobj_box, "startX", obj_meta->_box[0]);
            json_object_set_int_member(jobj_box, "startY", obj_meta->_box[1]);
            json_object_set_int_member(jobj_box, "endX", obj_meta->_box[2]);
            json_object_set_int_member(jobj_box, "endY", obj_meta->_box[3]);
            json_object_set_object_member(jobj_object, "box", jobj_box);
            json_object_set_object_member(jobj_parent, "track", jobj_object);
        } else if (obj_meta->_label != -1) {
            GST_DEBUG("|OBJECT| LabelId: %d, Confidence: %.2f, Box: {%f, %f, "
                      "%f, %f}, Label Name: %s",
                      obj_meta->_label, obj_meta->_confidence,
                      obj_meta->_box[0], obj_meta->_box[1], obj_meta->_box[2],
                      obj_meta->_box[3], obj_meta->_label_name->str);

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

        /* if segment,                        then root -> "segment": {}
         * else if pose, face, track, object, then root -> "objects": [{}, {},
         * ...]
         */
        if (jobj_segment) {
            json_object_set_object_member(jobj_root, "segment", jobj_segment);
        } else {
            json_node_set_object(jnode_object, jobj_parent);
            json_array_add_element(jarray_objects, jnode_object);
        }
    }

    json_node_set_object(jnode_root, jobj_root);

    gchar *json_data = json_to_string(jnode_root, TRUE);

    json_node_free(jnode_root);
    json_object_unref(jobj_root);

#if 0 /* debug purpose */
    if (meta_info->_seq_id <= 10) {
        gchar *file_name;
        file_name = g_strdup_printf("./_mytest/output_%d_%02lu.json",
                                    frame_meta->_stream_id, meta_info->_seq_id);
        GError *error = NULL;
        g_file_set_contents(file_name, json_data, -1, &error);
        if (error != NULL) {
            g_printerr("Error saving file: %s\n", error->message);
            g_error_free(error);
        }
        g_free(file_name);

        file_name = g_strdup_printf("./_mytest/output_%d_%02lu.jpg",
                                    frame_meta->_stream_id, meta_info->_seq_id);
        cv::Mat rgb_image;
        cv::cvtColor(frame_meta->_rgb_surface, rgb_image, cv::COLOR_BGR2RGB);
        cv::imwrite(file_name, rgb_image);
        g_free(file_name);
    }
#endif

    return json_data;
}
