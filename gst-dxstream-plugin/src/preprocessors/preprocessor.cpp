#include "preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include <algorithm>

bool Preprocessor::check_object(const DXFrameMeta *frame_meta, DXObjectMeta *object_meta) {
    if (element->_object_filter.target_class_id != -1 &&
        object_meta->_label != element->_object_filter.target_class_id) {
        return false;
    }

    if (frame_meta->_roi[0] != -1 &&
        !check_object_roi(object_meta->_box.data(), frame_meta->_roi)) {
        return false;
    }

    if (object_meta->_box[2] - object_meta->_box[0] < (float)element->_object_filter.min_width ||
        object_meta->_box[3] - object_meta->_box[1] <
            (float)element->_object_filter.min_height) {
        return false;
    }

    if (object_meta->_track_id != -1) {
        if (element->_frame_ctrl.track_cnt[frame_meta->_stream_id].count(
                object_meta->_track_id) > 0) {
            element->_frame_ctrl.track_cnt[frame_meta->_stream_id][object_meta->_track_id] +=
                1;
        } else {
            element->_frame_ctrl.track_cnt[frame_meta->_stream_id][object_meta->_track_id] =
                1;
        }

        if (element->_frame_ctrl.track_cnt[frame_meta->_stream_id][object_meta->_track_id] <
            static_cast<int>(element->_frame_ctrl.interval)) {
            return false;
        }

        element->_frame_ctrl.track_cnt[frame_meta->_stream_id][object_meta->_track_id] = 0;
    } else {
        if (element->_frame_ctrl.cnt[frame_meta->_stream_id] < element->_frame_ctrl.interval) {
            return false;
        }
    }
    return true;
}

bool Preprocessor::check_object_roi(const float *box, const int *roi) const {
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
    const auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        GST_ERROR_OBJECT(element, "Failed to get DXFrameMeta from GstBuffer");
        return false;
    }
    auto iter = element->_frame_ctrl.cnt.find(frame_meta->_stream_id);
    if (iter == element->_frame_ctrl.cnt.end()) {
        element->_frame_ctrl.cnt[frame_meta->_stream_id] = 0;
    }
    if (element->_object_filter.secondary_mode) {
        return false;
    }
    if (element->_frame_ctrl.cnt[frame_meta->_stream_id] < element->_frame_ctrl.interval) {
        element->_frame_ctrl.cnt[frame_meta->_stream_id] += 1;
        return true;
    }
    element->_frame_ctrl.cnt[frame_meta->_stream_id] = 0;
    return false;
}

GstBuffer* Preprocessor::check_frame_meta(GstBuffer *buf) {
    auto *frame_meta = dx_get_frame_meta(buf);
    if (!frame_meta) {
        buf = dx_create_frame_meta(buf);
        frame_meta = dx_get_frame_meta(buf);

        GstPad *sinkpad = GST_BASE_TRANSFORM_SINK_PAD(element);
        GstCaps *caps = gst_pad_get_current_caps(sinkpad);
        const GstStructure *s = gst_caps_get_structure(caps, 0);
        frame_meta->_name = gst_structure_get_name(s);
        frame_meta->_format = gst_structure_get_string(s, "format");
        gst_structure_get_int(s, "width", &frame_meta->_width);
        gst_structure_get_int(s, "height", &frame_meta->_height);
        gint num;
        gint denom;
        gst_structure_get_fraction(s, "framerate", &num, &denom);
        frame_meta->_frame_rate = (gfloat)num / (gfloat)denom;
        frame_meta->_stream_id = 0;
        gst_caps_unref(caps);
    }
    return buf;
}

void Preprocessor::cleanup_temp_buffers(int stream_id) {
    // Vectors automatically manage memory, just clear them to free memory
    if (element->_buffers.crop.find(stream_id) != element->_buffers.crop.end()) {
        element->_buffers.crop[stream_id].clear();
        element->_buffers.crop[stream_id].shrink_to_fit();
    }
    if (element->_buffers.resized.find(stream_id) != element->_buffers.resized.end()) {
        element->_buffers.resized[stream_id].clear();
        element->_buffers.resized[stream_id].shrink_to_fit();
    }
    if (element->_buffers.convert.find(stream_id) != element->_buffers.convert.end()) {
        element->_buffers.convert[stream_id].clear();
        element->_buffers.convert[stream_id].shrink_to_fit();
    }
}

bool Preprocessor::process_object(GstBuffer *buf, DXFrameMeta *frame_meta, DXObjectMeta *object_meta, const int &preprocess_id) {
    if (object_meta->_input_tensors.find(preprocess_id) !=
        object_meta->_input_tensors.end()) {
        GST_ERROR_OBJECT(element, "Preprocess ID %d already exists in the object meta. "
                          "check your pipeline", preprocess_id);
        return false;
    }

    if (!check_object(frame_meta, object_meta)) {
        return false;
    }

    size_t mem_size = element->_preprocess.height * element->_preprocess.width * element->_preprocess.channel;
    std::vector<int64_t> shape = {
        static_cast<int64_t>(element->_preprocess.height),
        static_cast<int64_t>(element->_preprocess.width),
        static_cast<int64_t>(element->_preprocess.channel)
    };
    dxs::InputBuffer input_buffer = dxs::InputBuffer::allocate(mem_size, shape, "input");
    dxs::InputBuffers input_buffers;
    input_buffers.push_back(input_buffer);

    cv::Rect roi(
        cv::Point(std::max(int(object_meta->_box[0]), 0),
                  std::max(int(object_meta->_box[1]), 0)),
        cv::Point(std::min(int(object_meta->_box[2]), frame_meta->_width),
                  std::min(int(object_meta->_box[3]), frame_meta->_height)));

    bool ret = true;

    cleanup_temp_buffers(frame_meta->_stream_id);

    if (element->_plugin.process_function) {
        ret = element->_plugin.process_function(buf, frame_meta, object_meta, input_buffer.data.get());
    } else {
        ret = preprocess(buf, frame_meta, input_buffer.data.get(), &roi);
    }

    if (ret) {
        object_meta->_input_tensors[preprocess_id] = input_buffers;
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

    if (element->_frame_ctrl.track_cnt.count(frame_meta->_stream_id) == 0) {
        element->_frame_ctrl.track_cnt[frame_meta->_stream_id] = std::map<int, int>();
    }

    size_t objects_size = frame_meta->_object_meta_list.size();
    int preprocess_id = element->_preprocess.id;

    for (size_t o = 0; o < objects_size; o++) {
        DXObjectMeta *object_meta = frame_meta->_object_meta_list[o];
        process_object(buf, frame_meta, object_meta, preprocess_id);
    }

    if (element->_frame_ctrl.cnt[frame_meta->_stream_id] < element->_frame_ctrl.interval) {
        element->_frame_ctrl.cnt[frame_meta->_stream_id] += 1;
    } else {
        element->_frame_ctrl.cnt[frame_meta->_stream_id] = 0;
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
    if (element->_object_filter.roi[0] != -1) {
        frame_meta->_roi[0] = std::max(element->_object_filter.roi[0], 0);
        frame_meta->_roi[1] = std::max(element->_object_filter.roi[1], 0);
        frame_meta->_roi[2] = std::min(element->_object_filter.roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(element->_object_filter.roi[3], frame_meta->_height - 1);
    }

    if (frame_meta->_input_tensors.find(element->_preprocess.id) !=
        frame_meta->_input_tensors.end()) {
        GST_ERROR_OBJECT(element, "Preprocess ID %d already exists in the frame meta. "
                          "check your pipeline", element->_preprocess.id);
        ret = false;
    }

    size_t mem_size = element->_preprocess.height * element->_preprocess.width * element->_preprocess.channel;
    std::vector<int64_t> shape = {
        static_cast<int64_t>(element->_preprocess.height),
        static_cast<int64_t>(element->_preprocess.width),
        static_cast<int64_t>(element->_preprocess.channel)
    };
    dxs::InputBuffer input_buffer = dxs::InputBuffer::allocate(mem_size, shape, "input");
    dxs::InputBuffers input_buffers;
    input_buffers.push_back(input_buffer);

    cv::Rect roi(cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                 cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));

    uint8_t* input_tensor = input_buffer.data.get();
    if (element->_preprocess.transpose) {
        input_tensor = element->_preprocess.transpose_data.data();
    }

    if (element->_plugin.process_function != nullptr) {
        if (!element->_plugin.process_function(buf, frame_meta, nullptr, input_tensor)) {
            ret = false;
        }
    } else {
        if (!preprocess(buf, frame_meta, input_tensor, &roi)) {
            ret = false;
        }
    }

    if (element->_preprocess.transpose) {
        transpose_hwc_to_chw(input_buffer.data.get(), element->_preprocess.transpose_data.data(),
                           element->_preprocess.channel, element->_preprocess.height, element->_preprocess.width);
    }

    if (ret) {
        frame_meta->_input_tensors[element->_preprocess.id] = input_buffers;
    }
    return ret;
}

void Preprocessor::transpose_hwc_to_chw(uint8_t* output, const uint8_t* input, guint channels, guint height, guint width) const {
    for (guint c = 0; c < channels; c++) {
        for (guint h = 0; h < height; h++) {
            for (guint w = 0; w < width; w++) {
                int chw_idx = c * (height * width) + h * width + w;
                int hwc_idx = h * (width * channels) + w * channels + c;
                output[chw_idx] = input[hwc_idx];
            }
        }
    }
}