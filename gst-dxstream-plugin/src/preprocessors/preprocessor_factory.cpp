#include "preprocessor_factory.h"
#include "preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "../transforms/video_transform_factory.hpp"
#include "../transforms/gst_frame_desc.hpp"

std::shared_ptr<Preprocessor> PreprocessorFactory::create_preprocessor(GstDxPreprocess *element) {
    dxt::FrameDesc dst_template = dxt::make_packed_frame_desc(
        nullptr,
        element->_preprocess.width,
        element->_preprocess.height,
        dxt::video_format_from_string(element->_preprocess.color_format));

    dxt::TransformOps ops;
    ops.keep_aspect_ratio = static_cast<bool>(element->_preprocess.keep_ratio);
    ops.padding.enabled   = ops.keep_aspect_ratio;
    ops.padding.pad_r     = element->_preprocess.pad_value;
    ops.padding.pad_g     = element->_preprocess.pad_value;
    ops.padding.pad_b     = element->_preprocess.pad_value;
    ops.interp            = dxt::InterpMethod::BILINEAR;

    auto kernel = dxt::VideoTransformFactory::create(dst_template, ops);
    if (!kernel) {
        GST_ERROR("PreprocessorFactory: failed to create transform kernel");
        return nullptr;
    }
    GST_DEBUG("PreprocessorFactory: using backend '%s'", kernel->backend_name());

    return std::make_shared<Preprocessor>(element, std::move(kernel));
}