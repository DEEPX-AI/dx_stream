#include "preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "gst-dxmeta.hpp"
#include <algorithm>

bool Preprocessor::check_object(DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {
    if (element->_target_class_id != -1 &&
        object_meta->_label != element->_target_class_id) {
        return false;
    }

    if (frame_meta->_roi[0] != -1 &&
        !check_object_roi(object_meta->_box, frame_meta->_roi)) {
        return false;
    }

    if (object_meta->_box[2] - object_meta->_box[0] < element->_min_object_width ||
        object_meta->_box[3] - object_meta->_box[1] <
            element->_min_object_height) {
        return false;
    }

    if (object_meta->_track_id != -1) {
        if (element->_track_cnt[frame_meta->_stream_id].count(
                object_meta->_track_id) > 0) {
            element->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] +=
                1;
        } else {
            element->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] =
                0;
        }

        if (element->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] <
            static_cast<int>(element->_interval)) {
            return false;
        }

        element->_track_cnt[frame_meta->_stream_id][object_meta->_track_id] = 0;
    } else {
        if (element->_cnt[frame_meta->_stream_id] < element->_interval) {
            return false;
        }
    }
    return true;
}

bool Preprocessor::check_object_roi(float *box, int *roi) {
    if (int(box[0]) < roi[0])
        return false;
    if (int(box[1]) < roi[1])
        return false;
    if (int(box[2]) > roi[2])
        return false;
    if (int(box[3]) > roi[3])
        return false;
    return true;
}

bool Preprocessor::check_primary_interval(GstBuffer *buf) {
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(element, "Failed to get DXFrameMeta from GstBuffer");
        return false;
    }
    auto iter = element->_cnt.find(frame_meta->_stream_id);
    if (iter == element->_cnt.end()) {
        element->_cnt[frame_meta->_stream_id] = 0;
    }
    if (element->_secondary_mode) {
        return false;
    }
    if (element->_cnt[frame_meta->_stream_id] < element->_interval) {
        element->_cnt[frame_meta->_stream_id] += 1;
        return true;
    }
    element->_cnt[frame_meta->_stream_id] = 0;
    return false;
}

void Preprocessor::check_frame_meta(GstBuffer *buf) {
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        if (!gst_buffer_is_writable(buf)) {
            buf = gst_buffer_make_writable(buf);
        }
        frame_meta = (DXFrameMeta *)gst_buffer_add_meta(buf, DX_FRAME_META_INFO,
                                                        nullptr);

        GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(element);
        GstCaps *caps = gst_pad_get_current_caps(sinkpad);
        GstStructure *s = gst_caps_get_structure(caps, 0);
        frame_meta->_name = gst_structure_get_name(s);
        frame_meta->_format = gst_structure_get_string(s, "format");
        gst_structure_get_int(s, "width", &frame_meta->_width);
        gst_structure_get_int(s, "height", &frame_meta->_height);
        gint num, denom;
        gst_structure_get_fraction(s, "framerate", &num, &denom);
        frame_meta->_frame_rate = (gfloat)num / (gfloat)denom;
        frame_meta->_stream_id = 0;
        gst_caps_unref(caps);
    }
}

void Preprocessor::cleanup_temp_buffers(int stream_id) {
    if (element->_crop_frame[stream_id]) {
        free(element->_crop_frame[stream_id]);
        element->_crop_frame[stream_id] = nullptr;
    }
    if (element->_resized_frame[stream_id]) {
        free(element->_resized_frame[stream_id]);
        element->_resized_frame[stream_id] = nullptr;
    }
    if (element->_convert_frame[stream_id]) {
        free(element->_convert_frame[stream_id]);
        element->_convert_frame[stream_id] = nullptr;
    }
}

bool Preprocessor::process_object(GstBuffer *buf, DXFrameMeta *frame_meta, DXObjectMeta *object_meta, int &preprocess_id) {
    if (object_meta->_input_tensors.find(preprocess_id) !=
        object_meta->_input_tensors.end()) {
        GST_ERROR_OBJECT(element, "Preprocess ID %d already exists in the object meta. "
                          "check your pipeline", preprocess_id);
        return false;
    }

    if (!check_object(frame_meta, object_meta)) {
        return false;
    }

    dxs::DXTensors tensors;
    tensors._mem_size =
        element->_resize_height * element->_resize_width * element->_input_channel;
    tensors._data = malloc(tensors._mem_size);
    dxs::DXTensor tensor;
    tensor._type = dxs::DataType::UINT8;
    tensor._name = "input";
    tensor._shape.push_back(element->_resize_height);
    tensor._shape.push_back(element->_resize_width);
    tensor._shape.push_back(element->_input_channel);
    tensor._elemSize = 1;
    tensors._tensors.push_back(tensor);

    cv::Rect roi(
        cv::Point(std::max(int(object_meta->_box[0]), 0),
                  std::max(int(object_meta->_box[1]), 0)),
        cv::Point(std::min(int(object_meta->_box[2]), frame_meta->_width),
                  std::min(int(object_meta->_box[3]), frame_meta->_height)));

    bool ret = true;

    cleanup_temp_buffers(frame_meta->_stream_id);

    if (element->_process_function) {
        ret = element->_process_function(buf, frame_meta, object_meta, tensors._data);
    } else {
        ret = preprocess(buf, frame_meta, tensors._data, &roi);
    }

    if (ret) {
        object_meta->_input_tensors[preprocess_id] = tensors;
    } else {
        free(tensors._data);
        tensors._data = nullptr;
    }
    return ret;
}

bool Preprocessor::secondary_process(GstBuffer *buf) {
    if (check_primary_interval(buf)) {
        return true;
    }
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(element, "Failed to get DXFrameMeta from GstBuffer");
        return false;
    }

    if (element->_track_cnt.count(frame_meta->_stream_id) == 0) {
        element->_track_cnt[frame_meta->_stream_id] = std::map<int, int>();
    }

    int objects_size = g_list_length(frame_meta->_object_meta_list);
    int preprocess_id = element->_preprocess_id;

    for (int o = 0; o < objects_size; o++) {
        DXObjectMeta *object_meta =
            (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, o);
        process_object(buf, frame_meta, object_meta, preprocess_id);
    }

    if (element->_cnt[frame_meta->_stream_id] < element->_interval) {
        element->_cnt[frame_meta->_stream_id] += 1;
    } else {
        element->_cnt[frame_meta->_stream_id] = 0;
    }
    return true;
}

bool Preprocessor::primary_process(GstBuffer *buf) {
    if (check_primary_interval(buf)) {
        return true;
    }
    DXFrameMeta *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(element, "Failed to get DXFrameMeta from GstBuffer");
        return false;
    }
    bool ret = true;
    if (element->_roi[0] != -1) {
        frame_meta->_roi[0] = std::max(element->_roi[0], 0);
        frame_meta->_roi[1] = std::max(element->_roi[1], 0);
        frame_meta->_roi[2] = std::min(element->_roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(element->_roi[3], frame_meta->_height - 1);
    }

    if (frame_meta->_input_tensors.find(element->_preprocess_id) !=
        frame_meta->_input_tensors.end()) {
        GST_ERROR_OBJECT(element, "Preprocess ID %d already exists in the frame meta. "
                          "check your pipeline", element->_preprocess_id);
        ret = false;
    }

    dxs::DXTensors tensors;
    tensors._mem_size =
        element->_resize_height * element->_resize_width * element->_input_channel;
    tensors._data = malloc(tensors._mem_size);
    dxs::DXTensor tensor;
    tensor._type = dxs::DataType::UINT8;
    tensor._name = "input";
    tensor._shape.push_back(element->_resize_height);
    tensor._shape.push_back(element->_resize_width);
    tensor._shape.push_back(element->_input_channel);
    tensor._elemSize = 1;
    tensors._tensors.push_back(tensor);

    cv::Rect roi(cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                 cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));

    void* input_tensor = tensors._data;
    if (element->_transpose) {
        input_tensor = element->_transpose_data;
    }

    if (element->_process_function != nullptr) {
        if (!element->_process_function(buf, frame_meta, nullptr, input_tensor)) {
            ret = false;
        }
    } else {
        if (!preprocess(buf, frame_meta, input_tensor, &roi)) {
            ret = false;
        }
    }

    if (element->_transpose) {
        for (guint c = 0; c < element->_input_channel; c++) {
            for (guint h = 0; h < element->_resize_height; h++) {
                for (guint w = 0; w < element->_resize_width; w++) {
                    int chw_idx = c * (element->_resize_height * element->_resize_width) + h * element->_resize_width + w;
                    int hwc_idx = h * (element->_resize_width * element->_input_channel) + w * element->_input_channel + c;
                    ((uint8_t *)tensors._data)[chw_idx] = ((uint8_t *)element->_transpose_data)[hwc_idx];
                }
            }
        }
    }

    if (ret) {
        frame_meta->_input_tensors[element->_preprocess_id] = tensors;
    } else {
        free(tensors._data);
        tensors._data = nullptr;
    }
    return ret;
}