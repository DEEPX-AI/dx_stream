#include "transform_kernel_base.hpp"
#include <algorithm>

namespace dxt {

// ---------------------------------------------------------------------------
// init — default implementation; subclass may call this after HW setup
// ---------------------------------------------------------------------------

bool TransformKernelBase::init(const FrameDesc& dst_template, const TransformOps& ops) {
    dst_template_ = dst_template;
    ops_          = ops;
    initialized_  = true;
    return true;
}

// ---------------------------------------------------------------------------
// effective_crop
// ---------------------------------------------------------------------------

CropRect TransformKernelBase::effective_crop(const FrameDesc&  src,
                                              const DynamicOps* dynamic) const {
    const CropRect* cr = nullptr;

    if (dynamic && dynamic->crop_override && dynamic->crop_override->enabled) {
        cr = dynamic->crop_override;
    } else if (ops_.crop.enabled) {
        cr = &ops_.crop;
    }

    if (!cr) {
        return CropRect{ 0, 0, src.width, src.height, false };
    }

    int x = cr->x, y = cr->y, w = cr->w, h = cr->h;
    x = std::max(x, 0);
    y = std::max(y, 0);
    if (x + w > src.width)  w = src.width  - x;
    if (y + h > src.height) h = src.height - y;

    return CropRect{ x, y, w, h, true };
}

// ---------------------------------------------------------------------------
// compute_dst_rect
// ---------------------------------------------------------------------------

void TransformKernelBase::compute_dst_rect(int src_w, int src_h,
                                            int& dst_x, int& dst_y,
                                            int& content_w, int& content_h) const {
    const int out_w = dst_template_.width;
    const int out_h = dst_template_.height;

    if (!ops_.keep_aspect_ratio) {
        dst_x     = 0;
        dst_y     = 0;
        content_w = out_w;
        content_h = out_h;
        return;
    }

    float ratio_dst = static_cast<float>(out_w) / out_h;
    float ratio_src = static_cast<float>(src_w) / src_h;
    int new_w, new_h;
    if (ratio_src < ratio_dst) {
        new_h = out_h;
        new_w = static_cast<int>(new_h * ratio_src);
    } else {
        new_w = out_w;
        new_h = static_cast<int>(new_w / ratio_src);
    }

    dst_x     = (out_w - new_w) / 2;
    dst_y     = (out_h - new_h) / 2;
    content_w = new_w;
    content_h = new_h;
}

}  // namespace dxt
