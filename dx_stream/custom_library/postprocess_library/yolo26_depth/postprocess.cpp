// YOLO26n-Depth postprocess for dx_stream.
//
// The model emits a single dense depth map (e.g. output0 [1,1,640,640] float32).
// We min-max normalize the map per-frame to uint8 [0,255] (relative depth) and
// store it in frame_meta->_depth_data; DxOsd::draw_depth applies a MAGMA colormap.
#include "gstdxstream/gst-dxframemeta.hpp"
#include "gstdxstream/gst-dxobjectmeta.hpp"
#include "gstdxstream/dxcommon.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

#include <glib.h>
#include <gst/gst.h>

// Pick the dense depth tensor: prefer a rank-4 tensor with channel dim == 1,
// otherwise fall back to the tensor with the most elements.
static const dxs::DXTensor *select_depth_tensor(const std::vector<dxs::DXTensor> &outputs) {
    const dxs::DXTensor *best = nullptr;
    int64_t best_elems = -1;
    for (const auto &t : outputs) {
        int64_t elems = 1;
        for (auto d : t._shape) elems *= (d > 0 ? d : 1);
        const size_t r = t._shape.size();
        const bool rank4_c1 = (r == 4 && t._shape[1] == 1);
        if (rank4_c1) return &t;  // exact match for [1,1,H,W]
        if (elems > best_elems) { best_elems = elems; best = &t; }
    }
    return best;
}

DX_CUSTOM_EXPORT void PostProcess(GstBuffer *buf,
                                  std::vector<dxs::DXTensor> network_output,
                                  DXFrameMeta *frame_meta,
                                  DXObjectMeta *object_meta) {
    std::ignore = buf;
    std::ignore = object_meta;

    if (network_output.empty()) {
        GST_ERROR("yolo26_depth PostProcess: no network output tensors");
        return;
    }

    const dxs::DXTensor *t = select_depth_tensor(network_output);
    if (t == nullptr || t->_data == nullptr || t->_shape.size() < 2) {
        GST_ERROR("yolo26_depth PostProcess: no usable depth tensor");
        return;
    }

    const size_t r = t->_shape.size();
    const int H = static_cast<int>(t->_shape[r - 2]);
    const int W = static_cast<int>(t->_shape[r - 1]);
    if (H <= 0 || W <= 0) {
        GST_ERROR("yolo26_depth PostProcess: bad depth dims HxW=%dx%d", H, W);
        return;
    }
    const size_t n = static_cast<size_t>(H) * static_cast<size_t>(W);

    // DXNN depth output is float32 (verified: images[1,3,640,640] -> output0 float32).
    const float *src = static_cast<const float *>(t->_data);

    float vmin = std::numeric_limits<float>::max();
    float vmax = std::numeric_limits<float>::lowest();
    for (size_t i = 0; i < n; ++i) {
        const float v = src[i];
        if (v < vmin) vmin = v;
        if (v > vmax) vmax = v;
    }
    const float range = (vmax - vmin);
    const float scale = (range > 1e-6f) ? (255.0f / range) : 0.0f;

    std::vector<unsigned char> depth_u8(n);
    for (size_t i = 0; i < n; ++i) {
        float norm = (src[i] - vmin) * scale;  // 0..255
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 255.0f) norm = 255.0f;
        depth_u8[i] = static_cast<unsigned char>(norm + 0.5f);
    }

    frame_meta->_depth_data = std::move(depth_u8);
    frame_meta->_depth_width = W;
    frame_meta->_depth_height = H;

    GST_DEBUG("yolo26_depth PostProcess: depth %dx%d (min=%.3f max=%.3f)", W, H, vmin, vmax);
}
