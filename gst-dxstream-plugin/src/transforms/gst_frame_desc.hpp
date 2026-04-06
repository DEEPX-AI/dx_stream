#pragma once

// ---------------------------------------------------------------------------
// GStreamer bridge — helpers to construct FrameDesc from GStreamer types.
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

namespace dxt {

// ---------------------------------------------------------------------------
// VideoFormat helpers
// ---------------------------------------------------------------------------

inline VideoFormat video_format_from_string(const char* str) {
    if (g_strcmp0(str, "I420") == 0) return VideoFormat::I420;
    if (g_strcmp0(str, "NV12") == 0) return VideoFormat::NV12;
    if (g_strcmp0(str, "RGB")  == 0) return VideoFormat::RGB;
    if (g_strcmp0(str, "BGR")  == 0) return VideoFormat::BGR;
    return VideoFormat::NV12;  // safe default
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

// num_planes_for_format() and bytes_per_pixel() moved to
// video_transform_kernel.hpp (pure C++ utility, no GStreamer dependency)

// ---------------------------------------------------------------------------
// NV12 stride helpers
//
// Rockchip decoders may output buffers where the physical stride (alignment)
// differs from the frame width.  We derive the real stride from the memory
// size rather than from GstVideoInfo, which can be unreliable for NV12.
// ---------------------------------------------------------------------------

// Compute 16-aligned height stride used by RGA / V4L2 decoders.
inline int rga_hstride(int height) {
    return ((height + 15) / 16) * 16;
}

// Compute actual NV12 luma byte stride from buffer allocation size.
// NV12 layout: Y plane = stride * hstride bytes, UV plane = stride * hstride/2 bytes
//   => total = stride * hstride * 3/2
//   => stride = (2 * total) / (3 * hstride)
inline int compute_nv12_actual_stride(GstBuffer* buf, int height, int fallback_width) {
    GstMemory* mem = gst_buffer_peek_memory(buf, 0);
    gsize mem_size = gst_memory_get_sizes(mem, nullptr, nullptr);
    int hstride_val = rga_hstride(height);
    int stride = static_cast<int>((2 * mem_size) / (3 * static_cast<gsize>(hstride_val)));
    return (stride > 0) ? stride : fallback_width;
}

// ---------------------------------------------------------------------------
// make_nv12_frame_desc
//
// Build a FrameDesc for a NV12 GstBuffer coming from a video decoder.
//
// Two paths based on memory type:
//   DMA-BUF path: buffer allocation size heuristic for stride/offset.
//                  RK3588 HW decoders may report unreliable GstVideoMeta
//                  stride for DMA-buf buffers. Early return after computation.
//   CPU path:      GstVideoMeta > GstVideoInfo > tight-packed fallback.
//
// GstVideoMeta is buffer-specific metadata attached by HW decoders; it
// preserves the true stride/offset even after gst_buffer_make_writable().
// GstVideoInfo is derived from caps negotiation and may not reflect padding
// (e.g. RK3588 16-row alignment).
//
// vinfo: optional GstVideoInfo pointer used as an intermediate fallback.
//
// For CPU_VIRTUAL path: planes[0/1].data are LEFT NULLPTR — caller must
// gst_buffer_map(), set planes[i].data = mapped_ptr + planes[i].offset,
// then call gst_buffer_unmap() after the transform.
// For DMA_BUF path: kernel uses dma_fd directly; data pointers stay nullptr.
// ---------------------------------------------------------------------------
inline FrameDesc make_nv12_frame_desc(GstBuffer* buf, int width, int height,
                                      const GstVideoInfo* vinfo = nullptr) {
    FrameDesc desc;
    desc.width      = width;
    desc.height     = height;
    desc.format     = VideoFormat::NV12;
    desc.num_planes = 2;

    // --- Step 1: Detect DMA-buf for zero-copy RGA path ---
    // FrameDesc defaults: memory_type = CPU_VIRTUAL, dma_fd = -1.
    // Only overwritten when a valid DMA-buf fd is found.
#ifdef HAVE_LIBRGA
    {
        GstMemory* mem = gst_buffer_peek_memory(buf, 0);
        if (gst_is_dmabuf_memory(mem)) {
            gint fd = gst_dmabuf_memory_get_fd(mem);
            if (fd >= 0) {
                desc.memory_type = MemoryType::DMA_BUF;
                desc.dma_fd      = fd;
                desc.dma_size    = gst_memory_get_sizes(mem, nullptr, nullptr);

                // RK3588/RGA fallback: HW decoder buffers may use 16-row-aligned
                // physical height; derive real stride from buffer allocation size.
                int actual_stride = compute_nv12_actual_stride(buf, height, width);
                int hstride_val   = rga_hstride(height);
                desc.planes[0].stride = actual_stride;
                desc.planes[0].height = height;
                desc.planes[0].offset = 0;
                desc.planes[0].data   = nullptr;
                desc.planes[1].stride = actual_stride;
                desc.planes[1].height = height / 2;
                desc.planes[1].offset = static_cast<size_t>(actual_stride) * hstride_val;
                desc.planes[1].data   = nullptr;
                
                return desc;
            }
        }
    }
#endif

    // --- Step 2: Determine plane layout ---
    // Priority: GstVideoMeta  (buffer-specific, from HW decoder / upstream)
    //         > GstVideoInfo   (caps-negotiated, from SW decode)
    //         > size heuristic (RGA 16-row-aligned) / tight-packed fallback
    GstVideoMeta *vmeta = gst_buffer_get_video_meta(buf);
    if (vmeta) {
        desc.planes[0].stride = vmeta->stride[0];
        desc.planes[0].height = height;
        desc.planes[0].offset = vmeta->offset[0];
        desc.planes[0].data   = nullptr;
        desc.planes[1].stride = vmeta->stride[1];
        desc.planes[1].height = height / 2;
        desc.planes[1].offset = vmeta->offset[1];
        desc.planes[1].data   = nullptr;
    } else if (vinfo != nullptr) {
        desc.planes[0].stride = GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 0);
        desc.planes[0].height = height;
        desc.planes[0].offset = GST_VIDEO_INFO_PLANE_OFFSET(vinfo, 0);
        desc.planes[0].data   = nullptr;
        desc.planes[1].stride = GST_VIDEO_INFO_PLANE_STRIDE(vinfo, 1);
        desc.planes[1].height = height / 2;
        desc.planes[1].offset = GST_VIDEO_INFO_PLANE_OFFSET(vinfo, 1);
        desc.planes[1].data   = nullptr;
    } else {
        desc.planes[0].stride = width;
        desc.planes[0].height = height;
        desc.planes[0].offset = 0;
        desc.planes[0].data   = nullptr;
        desc.planes[1].stride = width;
        desc.planes[1].height = height / 2;
        desc.planes[1].offset = static_cast<size_t>(width) * height;
        desc.planes[1].data   = nullptr;
    }

    return desc;
}

// ---------------------------------------------------------------------------
// make_packed_frame_desc
//
// Build a FrameDesc for a packed RGB/BGR buffer (e.g., dxs::InputBuffer data
// pointer, or a GstVideoFrame plane).
// ---------------------------------------------------------------------------
inline FrameDesc make_packed_frame_desc(uint8_t* data,
                                        int      width,
                                        int      height,
                                        VideoFormat fmt) {
    FrameDesc desc;
    desc.width        = width;
    desc.height       = height;
    desc.format       = fmt;
    desc.memory_type  = MemoryType::CPU_VIRTUAL;
    desc.num_planes   = 1;
    desc.dma_fd       = -1;

    desc.planes[0].data   = data;
    desc.planes[0].stride = width * bytes_per_pixel(fmt);
    desc.planes[0].height = height;
    desc.planes[0].offset = 0;

    return desc;
}

// ---------------------------------------------------------------------------
// make_i420_frame_desc
//
// Build a FrameDesc for an I420 buffer from GstVideoInfo (dxpreprocess stream).
// Strides and offsets are taken from GstVideoInfo which is authoritative for
// the I420 case (decoder output is packed or has explicit stride from caps).
// data pointer is LEFT NULLPTR — caller maps/unmaps GstBuffer.
// ---------------------------------------------------------------------------
inline FrameDesc make_i420_frame_desc(const GstVideoInfo& vinfo) {
    FrameDesc desc;
    desc.width      = GST_VIDEO_INFO_WIDTH(&vinfo);
    desc.height     = GST_VIDEO_INFO_HEIGHT(&vinfo);
    desc.format     = VideoFormat::I420;
    desc.memory_type = MemoryType::CPU_VIRTUAL;
    desc.num_planes = 3;
    desc.dma_fd     = -1;

    for (int i = 0; i < 3; ++i) {
        desc.planes[i].data   = nullptr;  // caller fills after map
        desc.planes[i].stride = GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, i);
        desc.planes[i].height = (i == 0) ? desc.height : desc.height / 2;
        desc.planes[i].offset = GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, i);
    }

    return desc;
}

}  // namespace dxt
