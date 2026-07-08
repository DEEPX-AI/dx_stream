#include "preprocessor_factory.h"
#include "preprocessor.h"
#include "gst-dxpreprocess.hpp"
#include "../transforms/transform_kernel_pool.hpp"
#include "../transforms/gst_frame_desc.hpp"

#define GST_CAT_DEFAULT transform_kernel_cat
GST_DEBUG_CATEGORY_EXTERN(transform_kernel_cat);

std::shared_ptr<Preprocessor> PreprocessorFactory::create_preprocessor(GstDxPreprocess *element) {

    dxt::FrameDesc dst_template = dxt::make_output_frame_desc(
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

    auto pool = std::make_unique<dxt::TransformKernelPool>(
        dst_template, ops,
        /*require_dynamic_input=*/static_cast<bool>(element->_object_filter.secondary_mode));
    GST_DEBUG("PreprocessorFactory: kernel pool created");

    return std::make_shared<Preprocessor>(element, std::move(pool));
}