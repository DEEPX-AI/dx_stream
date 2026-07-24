#include "preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "./../metadata/gst-dxframemeta.hpp"
#include "./../metadata/gst-dxobjectmeta.hpp"
#include "../transforms/gst_frame_desc.hpp"
#include <algorithm>

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

Preprocessor::Preprocessor(GstDxPreprocess* elem,
                            std::unique_ptr<dxt::TransformKernelPool> kernel_pool)
    : element(elem), kernel_pool_(std::move(kernel_pool)) {
}

// ---------------------------------------------------------------------------
// preprocess — unified pixel transform bridge (replaces per-backend subclasses)
//
// Uses GstSrcFrame RAII wrapper to handle all format branching, DMA-buf
// detection, vmeta/vinfo/heuristic stride resolution, and buffer mapping.
// ---------------------------------------------------------------------------

bool Preprocessor::preprocess(GstBuffer*   buf,
                               DXFrameMeta* frame_meta,
                               uint8_t*     output,
                               cv::Rect*    roi) {
    if (!kernel_pool_) {
        GST_ERROR_OBJECT(element, "Preprocessor: kernel pool not initialised");
        return false;
    }
    if (!output) {
        GST_ERROR_OBJECT(element, "Preprocessor: output pointer is null");
        return false;
    }

    dxt::VideoFormat src_fmt =
        dxt::video_format_from_string(frame_meta->_format.c_str());

    dxt::InputConfig input_cfg{src_fmt, frame_meta->_width, frame_meta->_height};

    // ------------------------------------------------------------------
    // Build src FrameDesc via GstSrcFrame RAII wrapper
    // ------------------------------------------------------------------
    const GstVideoInfo* vinfo_ptr = nullptr;
    {
        auto it = element->_stream.info.find(frame_meta->_stream_id);
        if (it != element->_stream.info.end()) {
            const GstVideoInfo& vinfo = it->second;
            // Guard against stale announce: if a later wrapped CAPS event
            // registered different dimensions than the buffer actually
            // carries, vinfo's stride no longer matches this buffer's real
            // layout. Fall back to tight-packed stride derived from
            // frame_meta instead of reading past the real allocation.
            if (GST_VIDEO_INFO_WIDTH(&vinfo) == frame_meta->_width &&
                GST_VIDEO_INFO_HEIGHT(&vinfo) == frame_meta->_height) {
                vinfo_ptr = &vinfo;
            } else {
                GST_WARNING_OBJECT(element,
                    "Preprocessor: stream %d CAPS %dx%d != buffer %dx%d, "
                    "ignoring stale stream info",
                    frame_meta->_stream_id,
                    GST_VIDEO_INFO_WIDTH(&vinfo), GST_VIDEO_INFO_HEIGHT(&vinfo),
                    frame_meta->_width, frame_meta->_height);
            }
        }
    }

    dxt::GstSrcFrame src(buf, frame_meta->_width, frame_meta->_height,
                          src_fmt, vinfo_ptr);
    if (!src.ok()) {
        GST_ERROR_OBJECT(element, "Preprocessor: failed to map GstBuffer");
        return false;
    }

    // ------------------------------------------------------------------
    // Build dst FrameDesc
    // ------------------------------------------------------------------
    auto dst_fmt = dxt::video_format_from_string(element->_preprocess.color_format);
    auto dst = dxt::make_output_frame_desc(
        output,
        element->_preprocess.width,
        element->_preprocess.height,
        dst_fmt);

    // ------------------------------------------------------------------
    // Dynamic ops: per-call ROI override (secondary mode)
    // ------------------------------------------------------------------
    dxt::DynamicOps dyn;
    dxt::CropRect   crop_rect;
    if (roi && (roi->width != 0 || roi->height != 0)) {
        crop_rect = { roi->x, roi->y, roi->width, roi->height, true };
        dyn.crop_override = &crop_rect;
    }

    // ------------------------------------------------------------------
    // Execute transform (pool handles fallback if primary kernel fails)
    // ------------------------------------------------------------------
    dxt::TransformResult result = kernel_pool_->transform(
        input_cfg, src.desc(), dst,
        frame_meta->_stream_id,
        dyn.crop_override ? &dyn : nullptr);

    return result.success;
}

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
    dxs::DXTensors input_tensors;
    input_tensors.allocate(mem_size);
    dxs::DXTensor t;
    t._name = "input";
    t._shape = shape;
    t._data = input_tensors.data_ptr();
    t._elemSize = 1;
    t._type = dxs::UINT8;
    input_tensors._tensors.push_back(t);

    cv::Rect roi(
        cv::Point(std::max(int(object_meta->_box[0]), 0),
                  std::max(int(object_meta->_box[1]), 0)),
        cv::Point(std::min(int(object_meta->_box[2]), frame_meta->_width),
                  std::min(int(object_meta->_box[3]), frame_meta->_height)));

    bool ret = true;

    cleanup_temp_buffers(frame_meta->_stream_id);

    if (element->_plugin.process_function) {
        try {
            ret = element->_plugin.process_function(buf, frame_meta, object_meta, static_cast<uint8_t*>(input_tensors.data_ptr()));
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(element, "Preprocess custom function threw exception: %s", e.what());
            ret = false;
        } catch (...) {
            GST_ERROR_OBJECT(element, "Preprocess custom function threw unknown exception");
            ret = false;
        }
    } else {
        ret = preprocess(buf, frame_meta, static_cast<uint8_t*>(input_tensors.data_ptr()), &roi);
    }

    if (ret) {
        object_meta->_input_tensors[preprocess_id] = std::move(input_tensors);
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

    if (element->_object_filter.roi[0] != -1) {
        frame_meta->_roi[0] = std::max(element->_object_filter.roi[0], 0);
        frame_meta->_roi[1] = std::max(element->_object_filter.roi[1], 0);
        frame_meta->_roi[2] = std::min(element->_object_filter.roi[2], frame_meta->_width - 1);
        frame_meta->_roi[3] = std::min(element->_object_filter.roi[3], frame_meta->_height - 1);
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
    dxs::DXTensors input_tensors;
    input_tensors.allocate(mem_size);
    dxs::DXTensor t;
    t._name = "input";
    t._shape = shape;
    t._data = input_tensors.data_ptr();
    t._elemSize = 1;
    t._type = dxs::UINT8;
    input_tensors._tensors.push_back(t);

    cv::Rect roi(cv::Point(frame_meta->_roi[0], frame_meta->_roi[1]),
                 cv::Point(frame_meta->_roi[2], frame_meta->_roi[3]));

    uint8_t* input_tensor = static_cast<uint8_t*>(input_tensors.data_ptr());
    if (element->_preprocess.transpose) {
        input_tensor = element->_preprocess.transpose_data.data();
    }

    if (element->_plugin.process_function != nullptr) {
        try {
            if (!element->_plugin.process_function(buf, frame_meta, nullptr, input_tensor)) {
                ret = false;
            }
        } catch (const std::exception &e) {
            GST_ERROR_OBJECT(element, "Preprocess custom function threw exception: %s", e.what());
            ret = false;
        } catch (...) {
            GST_ERROR_OBJECT(element, "Preprocess custom function threw unknown exception");
            ret = false;
        }
    } else {
        if (!preprocess(buf, frame_meta, input_tensor, &roi)) {
            ret = false;
        }
    }

    if (element->_preprocess.transpose) {
        transpose_hwc_to_chw(static_cast<uint8_t*>(input_tensors.data_ptr()), element->_preprocess.transpose_data.data(),
                           element->_preprocess.channel, element->_preprocess.height, element->_preprocess.width);
    }

    if (ret) {
        frame_meta->_input_tensors[element->_preprocess.id] = std::move(input_tensors);
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