#pragma once

// ---------------------------------------------------------------------------
// GStreamer bridge — RAII wrappers and helpers to construct FrameDesc from
// GStreamer types.
//
// This is the ONLY file in transforms/ that may include GStreamer headers.
// Keep all GStreamer-specific logic here, not in the kernel implementations.
// ---------------------------------------------------------------------------

#include "video_transform_kernel.hpp"

#include <gst/gst.h>
#include <gst/video/video.h>

#ifdef HAVE_LIBRGA
#include <gst/allocators/gstdmabuf.h>
#endif

#include <algorithm>
#include <cstring>

namespace dxt {

// ===========================================================================
// Format conversion helpers
// ===========================================================================

inline VideoFormat video_format_from_string(const char* str) {
    if (g_strcmp0(str, "I420") == 0) return VideoFormat::I420;
    if (g_strcmp0(str, "NV12") == 0) return VideoFormat::NV12;
    if (g_strcmp0(str, "RGB")  == 0) return VideoFormat::RGB;
    if (g_strcmp0(str, "BGR")  == 0) return VideoFormat::BGR;
    return VideoFormat::NV12;
}

inline const char* video_format_to_string(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::I420: return "I420";
        case VideoFormat::NV12: return "NV12";
        case VideoFormat::RGB:  return "RGB";
        case VideoFormat::BGR:  return "BGR";
    }
    return "NV12";
}

inline VideoFormat video_format_from_gst(GstVideoFormat fmt) {
    switch (fmt) {
        case GST_VIDEO_FORMAT_I420: return VideoFormat::I420;
        case GST_VIDEO_FORMAT_NV12: return VideoFormat::NV12;
        case GST_VIDEO_FORMAT_RGB:  return VideoFormat::RGB;
        case GST_VIDEO_FORMAT_BGR:  return VideoFormat::BGR;
        default: return VideoFormat::NV12;
    }
}

inline GstVideoFormat gst_format_from_video(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::I420: return GST_VIDEO_FORMAT_I420;
        case VideoFormat::NV12: return GST_VIDEO_FORMAT_NV12;
        case VideoFormat::RGB:  return GST_VIDEO_FORMAT_RGB;
        case VideoFormat::BGR:  return GST_VIDEO_FORMAT_BGR;
    }
    return GST_VIDEO_FORMAT_NV12;
}

// ===========================================================================
// NV12 stride / DMA-buf helpers (used internally and by rga_transform_kernel)
// ===========================================================================

// 16-aligned height stride used by RGA / V4L2 decoders.
inline int rga_hstride(int height) {
    return ((height + 15) / 16) * 16;
}

// Compute NV12 luma byte stride from allocation size.
inline int compute_nv12_actual_stride(GstBuffer* buf, int height, int fallback_width) {
    GstMemory* mem = gst_buffer_peek_memory(buf, 0);
    gsize mem_size = gst_memory_get_sizes(mem, nullptr, nullptr);
    int hstride_val = rga_hstride(height);
    int stride = static_cast<int>((2 * mem_size) / (3 * static_cast<gsize>(hstride_val)));
    return (stride > 0) ? stride : fallback_width;
}

// ===========================================================================
// make_dst_template — for factory init (no data, no GstBuffer)
// ===========================================================================

inline FrameDesc make_dst_template(int w, int h, VideoFormat fmt) {
    FrameDesc desc;
    desc.width       = w;
    desc.height      = h;
    desc.format      = fmt;
    desc.memory_type = MemoryType::CPU_VIRTUAL;
    desc.num_planes  = num_planes_for_format(fmt);

    switch (fmt) {
        case VideoFormat::NV12:
            desc.planes[0] = { nullptr, w, h, 0 };
            desc.planes[1] = { nullptr, w, h / 2, 0 };
            break;
        case VideoFormat::I420:
            desc.planes[0] = { nullptr, w, h, 0 };
            desc.planes[1] = { nullptr, w / 2, h / 2, 0 };
            desc.planes[2] = { nullptr, w / 2, h / 2, 0 };
            break;
        case VideoFormat::RGB:
        case VideoFormat::BGR:
            desc.planes[0] = { nullptr, w * bytes_per_pixel(fmt), h, 0 };
            break;
    }
    return desc;
}

// ===========================================================================
// make_output_frame_desc — for raw pointer outputs (preprocessor tensor, etc.)
// ===========================================================================

inline FrameDesc make_output_frame_desc(uint8_t* data, int w, int h, VideoFormat fmt) {
    FrameDesc desc;
    desc.width        = w;
    desc.height       = h;
    desc.format       = fmt;
    desc.memory_type  = MemoryType::CPU_VIRTUAL;
    desc.num_planes   = num_planes_for_format(fmt);

    switch (fmt) {
        case VideoFormat::NV12:
            desc.planes[0] = { data, w, h, 0 };
            desc.planes[1] = { data ? data + w * h : nullptr, w, h / 2,
                               static_cast<size_t>(w * h) };
            break;
        case VideoFormat::I420: {
            size_t y_size = static_cast<size_t>(w) * h;
            size_t uv_size = static_cast<size_t>(w / 2) * (h / 2);
            desc.planes[0] = { data, w, h, 0 };
            desc.planes[1] = { data ? data + y_size : nullptr, w / 2, h / 2, y_size };
            desc.planes[2] = { data ? data + y_size + uv_size : nullptr,
                               w / 2, h / 2, y_size + uv_size };
            break;
        }
        case VideoFormat::RGB:
        case VideoFormat::BGR:
            desc.planes[0] = { data, w * bytes_per_pixel(fmt), h, 0 };
            break;
    }
    return desc;
}

// ===========================================================================
// Internal: fill plane data pointers from mapped base address
// ===========================================================================

namespace detail {

inline void fill_data_pointers(FrameDesc& desc, uint8_t* base) {
    for (int i = 0; i < desc.num_planes; ++i)
        desc.planes[i].data = base + desc.planes[i].offset;
}

// Build FrameDesc layout from GstBuffer, with vmeta > vinfo > tight-packed priority.
// Data pointers are left nullptr.
inline void build_frame_layout(FrameDesc& desc, GstBuffer* buf,
                               int w, int h, VideoFormat fmt,
                               const GstVideoInfo* vinfo) {
    desc.width       = w;
    desc.height      = h;
    desc.format      = fmt;
    desc.memory_type = MemoryType::CPU_VIRTUAL;
    desc.num_planes  = num_planes_for_format(fmt);
    desc.dma_fd      = -1;
    desc.dma_size    = 0;

    // --- DMA-buf detection (NV12 only) ---
#ifdef HAVE_LIBRGA
    if (fmt == VideoFormat::NV12 && gst_buffer_n_memory(buf) > 0) {
        GstMemory* mem = gst_buffer_peek_memory(buf, 0);
        if (gst_is_dmabuf_memory(mem)) {
            gint fd = gst_dmabuf_memory_get_fd(mem);
            if (fd >= 0) {
                desc.memory_type = MemoryType::DMA_BUF;
                desc.dma_fd      = fd;
                desc.dma_size    = gst_memory_get_sizes(mem, nullptr, nullptr);

                int actual_stride = compute_nv12_actual_stride(buf, h, w);
                int hstride_val   = rga_hstride(h);
                desc.planes[0] = { nullptr, actual_stride, h, 0 };
                desc.planes[1] = { nullptr, actual_stride, h / 2,
                                   static_cast<size_t>(actual_stride) * hstride_val };
                return;  // DMA-buf path complete
            }
        }
    }
#endif

    // --- vmeta > vinfo > tight-packed ---
    GstVideoMeta* vmeta = gst_buffer_get_video_meta(buf);

    switch (fmt) {
        case VideoFormat::NV12:
            if (vmeta) {
                desc.planes[0] = { nullptr, static_cast<int>(vmeta->stride[0]), h,
                                   static_cast<size_t>(vmeta->offset[0]) };
                desc.planes[1] = { nullptr, static_cast<int>(vmeta->stride[1]), h / 2,
                                   static_cast<size_t>(vmeta->offset[1]) };
            } else if (vinfo) {
                desc.planes[0] = { nullptr, GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 0), h,
                                   static_cast<size_t>(GST_VIDEO_INFO_PLANE_OFFSET(vinfo, 0)) };
                desc.planes[1] = { nullptr, GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 1), h / 2,
                                   static_cast<size_t>(GST_VIDEO_INFO_PLANE_OFFSET(vinfo, 1)) };
            } else {
                desc.planes[0] = { nullptr, w, h, 0 };
                desc.planes[1] = { nullptr, w, h / 2, static_cast<size_t>(w) * h };
            }
            break;

        case VideoFormat::I420:
            if (vmeta) {
                desc.planes[0] = { nullptr, static_cast<int>(vmeta->stride[0]), h,
                                   static_cast<size_t>(vmeta->offset[0]) };
                desc.planes[1] = { nullptr, static_cast<int>(vmeta->stride[1]), h / 2,
                                   static_cast<size_t>(vmeta->offset[1]) };
                desc.planes[2] = { nullptr, static_cast<int>(vmeta->stride[2]), h / 2,
                                   static_cast<size_t>(vmeta->offset[2]) };
            } else if (vinfo) {
                for (int i = 0; i < 3; ++i) {
                    desc.planes[i] = { nullptr,
                                       GST_VIDEO_INFO_PLANE_STRIDE(vinfo, i),
                                       (i == 0) ? h : h / 2,
                                       static_cast<size_t>(GST_VIDEO_INFO_PLANE_OFFSET(vinfo, i)) };
                }
            } else {
                size_t y_sz = static_cast<size_t>(w) * h;
                size_t uv_sz = static_cast<size_t>(w / 2) * (h / 2);
                desc.planes[0] = { nullptr, w, h, 0 };
                desc.planes[1] = { nullptr, w / 2, h / 2, y_sz };
                desc.planes[2] = { nullptr, w / 2, h / 2, y_sz + uv_sz };
            }
            break;

        case VideoFormat::RGB:
        case VideoFormat::BGR: {
            int bpp = bytes_per_pixel(fmt);
            if (vmeta) {
                desc.planes[0] = { nullptr, static_cast<int>(vmeta->stride[0]), h,
                                   static_cast<size_t>(vmeta->offset[0]) };
            } else if (vinfo) {
                desc.planes[0] = { nullptr, GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 0), h,
                                   static_cast<size_t>(GST_VIDEO_INFO_PLANE_OFFSET(vinfo, 0)) };
            } else {
                desc.planes[0] = { nullptr, w * bpp, h, 0 };
            }
            break;
        }
    }
}

}  // namespace detail

// ===========================================================================
// GstSrcFrame — RAII source buffer wrapper
//
// Builds FrameDesc (format-aware, vmeta>vinfo>heuristic, DMA-buf detection)
// and maps the GstBuffer for read access.
//
// DMA-buf: always maps anyway so that CPU data pointers are valid (required
// for RGA → libyuv runtime fallback). dma_fd is preserved in the desc.
// Future optimization: lazy map via ensure_cpu_mapped().
// ===========================================================================

class GstSrcFrame {
public:
    // From GstVideoInfo (dxscale, dxconvert)
    GstSrcFrame(GstBuffer* buf, const GstVideoInfo& vinfo)
        : buf_(buf) {
        int w = GST_VIDEO_INFO_WIDTH(&vinfo);
        int h = GST_VIDEO_INFO_HEIGHT(&vinfo);
        VideoFormat fmt = video_format_from_gst(GST_VIDEO_INFO_FORMAT(&vinfo));
        detail::build_frame_layout(desc_, buf, w, h, fmt, &vinfo);
        do_map();
    }

    // From explicit params (preprocessor — DXFrameMeta)
    GstSrcFrame(GstBuffer* buf, int w, int h, VideoFormat fmt,
                const GstVideoInfo* vinfo = nullptr)
        : buf_(buf) {
        detail::build_frame_layout(desc_, buf, w, h, fmt, vinfo);
        do_map();
    }

    ~GstSrcFrame() {
        if (mapped_)
            gst_buffer_unmap(buf_, &map_);
    }

    // Non-copyable
    GstSrcFrame(const GstSrcFrame&) = delete;
    GstSrcFrame& operator=(const GstSrcFrame&) = delete;

    bool ok() const { return mapped_; }
    const FrameDesc& desc() const { return desc_; }
    FrameDesc& desc() { return desc_; }

private:
    void do_map() {
        map_ = GST_MAP_INFO_INIT;
        if (gst_buffer_map(buf_, &map_, GST_MAP_READ)) {
            mapped_ = true;
            detail::fill_data_pointers(desc_, map_.data);
        }
    }

    GstBuffer* buf_;
    GstMapInfo map_{};
    FrameDesc  desc_{};
    bool       mapped_ = false;
};

// ===========================================================================
// GstDstFrame — RAII destination buffer wrapper
//
// Maps the output GstBuffer for write and builds a FrameDesc from GstVideoInfo.
// ===========================================================================

class GstDstFrame {
public:
    GstDstFrame(GstBuffer* buf, const GstVideoInfo& vinfo)
        : buf_(buf) {
        int w = GST_VIDEO_INFO_WIDTH(&vinfo);
        int h = GST_VIDEO_INFO_HEIGHT(&vinfo);
        VideoFormat fmt = video_format_from_gst(GST_VIDEO_INFO_FORMAT(&vinfo));

        desc_.width       = w;
        desc_.height      = h;
        desc_.format      = fmt;
        desc_.memory_type = MemoryType::CPU_VIRTUAL;
        desc_.num_planes  = num_planes_for_format(fmt);

        int n = desc_.num_planes;
        for (int i = 0; i < n; ++i) {
            desc_.planes[i].stride = GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, i);
            desc_.planes[i].height = (i == 0) ? h : h / 2;
            desc_.planes[i].offset = GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, i);
        }
        // For packed formats, plane 0 height is h (already set above)

        map_ = GST_MAP_INFO_INIT;
        if (gst_buffer_map(buf_, &map_, GST_MAP_WRITE)) {
            mapped_ = true;
            detail::fill_data_pointers(desc_, map_.data);
        }
    }

    ~GstDstFrame() {
        if (mapped_)
            gst_buffer_unmap(buf_, &map_);
    }

    GstDstFrame(const GstDstFrame&) = delete;
    GstDstFrame& operator=(const GstDstFrame&) = delete;

    bool ok() const { return mapped_; }
    const FrameDesc& desc() const { return desc_; }
    FrameDesc& desc() { return desc_; }

private:
    GstBuffer* buf_;
    GstMapInfo map_{};
    FrameDesc  desc_{};
    bool       mapped_ = false;
};

// ===========================================================================
// gst_copy_video_frame — plane-aware passthrough copy
//
// Used by dxscale (same-size) and dxconvert (same-format) passthrough paths.
// Handles stride differences between HW decoder and tight-packed output.
// ===========================================================================

inline GstFlowReturn gst_copy_video_frame(GstBuffer* inbuf, GstBuffer* outbuf,
                                           const GstVideoInfo& src_info,
                                           const GstVideoInfo& dst_info) {
    GstMapInfo pin = GST_MAP_INFO_INIT, pout = GST_MAP_INFO_INIT;
    if (!gst_buffer_map(inbuf, &pin, GST_MAP_READ))
        return GST_FLOW_ERROR;
    if (!gst_buffer_map(outbuf, &pout, GST_MAP_WRITE)) {
        gst_buffer_unmap(inbuf, &pin);
        return GST_FLOW_ERROR;
    }

    GstVideoFormat fmt = GST_VIDEO_INFO_FORMAT(&src_info);
    int width  = GST_VIDEO_INFO_WIDTH(&src_info);
    int height = GST_VIDEO_INFO_HEIGHT(&src_info);

    if (fmt == GST_VIDEO_FORMAT_NV12 || fmt == GST_VIDEO_FORMAT_I420) {
        GstVideoMeta* vmeta = gst_buffer_get_video_meta(inbuf);
        int n_planes = (fmt == GST_VIDEO_FORMAT_NV12) ? 2 : 3;

        for (int p = 0; p < n_planes; ++p) {
            int src_stride = vmeta ? static_cast<int>(vmeta->stride[p])
                                   : GST_VIDEO_INFO_PLANE_STRIDE(&src_info, p);
            size_t src_off = vmeta ? vmeta->offset[p]
                                   : GST_VIDEO_INFO_PLANE_OFFSET(&src_info, p);
            int dst_stride = GST_VIDEO_INFO_PLANE_STRIDE(&dst_info, p);
            size_t dst_off = GST_VIDEO_INFO_PLANE_OFFSET(&dst_info, p);
            int plane_h = (p == 0) ? height : height / 2;
            int row_bytes;
            if (fmt == GST_VIDEO_FORMAT_NV12)
                row_bytes = (p == 0) ? width : width;
            else
                row_bytes = (p == 0) ? width : (width + 1) / 2;

            for (int row = 0; row < plane_h; ++row) {
                std::memcpy(pout.data + dst_off + row * dst_stride,
                            pin.data  + src_off + row * src_stride,
                            row_bytes);
            }
        }
    } else {
        std::memcpy(pout.data, pin.data, std::min(pin.size, pout.size));
    }

    gst_buffer_unmap(outbuf, &pout);
    gst_buffer_unmap(inbuf, &pin);
    // Copy only timestamps and flags — GstBaseTransform handles meta copying
    gst_buffer_copy_into(outbuf, inbuf,
        static_cast<GstBufferCopyFlags>(GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS),
        0, -1);
    return GST_FLOW_OK;
}

}  // namespace dxt
