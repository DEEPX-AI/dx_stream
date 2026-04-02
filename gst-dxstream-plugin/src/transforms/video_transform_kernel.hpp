#pragma once

// ---------------------------------------------------------------------------
// IVideoTransformKernel — platform-agnostic video transform abstraction
//
// Pure C++ interface: NO GStreamer / GLib headers allowed here.
// GStreamer bridge helpers live in gst_frame_desc.hpp.
//
// Execution order of operations (always):
//   Crop  →  Scale (+ aspect-ratio letterbox)  →  ColorConvert
// ---------------------------------------------------------------------------

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>

namespace dxt {

// ---------------------------------------------------------------------------
// Supported pixel formats (only formats actually used by existing backends)
// ---------------------------------------------------------------------------
enum class VideoFormat {
    I420,   // YUV 4:2:0 planar  (Y / U / V) — libyuv src, V3DSP src
    NV12,   // YUV 4:2:0 semi-planar (Y / UV interleaved) — RGA src (only)
    RGB,    // packed 24-bit RGB
    BGR,    // packed 24-bit BGR
    // Future: NV21, RGBA, BGRA, GRAY8 (add when backend support is verified)
};

// ---------------------------------------------------------------------------
// Memory model
// ---------------------------------------------------------------------------
enum class MemoryType {
    CPU_VIRTUAL,    // Regular uint8_t* pointer
    DMA_BUF,        // Linux DMA-buf fd (RGA zero-copy path)
    // Future: CUDA_DEVICE, VULKAN_IMAGE, OPENCL_BUFFER
};

// ---------------------------------------------------------------------------
// Per-plane descriptor
// ---------------------------------------------------------------------------
struct PlaneDesc {
    uint8_t* data   = nullptr;  // CPU virtual address; nullptr when DMA_BUF-only
    int      stride = 0;        // Row stride in bytes
    int      height = 0;        // Plane height in rows
    size_t   offset = 0;        // Byte offset within the DMA-BUF allocation
};

// ---------------------------------------------------------------------------
// Frame descriptor — non-owning view; caller manages buffer lifetime
// ---------------------------------------------------------------------------
struct FrameDesc {
    int         width       = 0;
    int         height      = 0;
    VideoFormat format      = VideoFormat::NV12;
    MemoryType  memory_type = MemoryType::CPU_VIRTUAL;

    static constexpr int MAX_PLANES = 3;
    PlaneDesc planes[MAX_PLANES];
    int       num_planes = 0;

    // DMA-BUF specific (valid when memory_type == DMA_BUF)
    int    dma_fd   = -1;
    size_t dma_size = 0;

    // Convenience accessors
    uint8_t* luma_data()     const { return planes[0].data; }
    int      luma_stride()   const { return planes[0].stride; }
    uint8_t* chroma_data()   const { return num_planes > 1 ? planes[1].data : nullptr; }
    int      chroma_stride() const { return num_planes > 1 ? planes[1].stride : 0; }
};

// ---------------------------------------------------------------------------
// Crop region (source ROI)
// ---------------------------------------------------------------------------
struct CropRect {
    int  x = 0, y = 0;
    int  w = 0, h = 0;
    bool enabled = false;
};

// ---------------------------------------------------------------------------
// Letterbox / padding configuration
// ---------------------------------------------------------------------------
struct PaddingConfig {
    bool    enabled = false;
    uint8_t pad_r   = 114;  // YOLO default
    uint8_t pad_g   = 114;
    uint8_t pad_b   = 114;
};

// ---------------------------------------------------------------------------
// Interpolation method
// ---------------------------------------------------------------------------
enum class InterpMethod {
    NEAREST,
    BILINEAR,
    // Future: BICUBIC
};

// ---------------------------------------------------------------------------
// Static transform configuration — set once at init()
// ---------------------------------------------------------------------------
struct TransformOps {
    // Crop: source ROI; disabled = full frame
    CropRect crop;

    // Scale with optional letterbox
    bool          keep_aspect_ratio = false;
    InterpMethod  interp            = InterpMethod::BILINEAR;

    // Padding (active only when keep_aspect_ratio == true)
    PaddingConfig padding;

    // Future extension slots (add fields here, no virtual changes needed):
    // RotateConfig   rotate;
    // FlipConfig     flip;
    // NormalizeConfig normalize;  // uint8 → float32
};

// ---------------------------------------------------------------------------
// Per-call dynamic overrides — avoids re-init for secondary mode
// ---------------------------------------------------------------------------
struct DynamicOps {
    // When non-null, overrides the static crop set in TransformOps.
    // Used by dxpreprocess secondary mode: each object has its own bbox ROI.
    const CropRect* crop_override = nullptr;
};

// ---------------------------------------------------------------------------
// Transform result
// ---------------------------------------------------------------------------
struct TransformResult {
    bool success = false;

    // When keep_aspect_ratio is true: the sub-rect within dst that holds
    // actual image content (needed for postprocess coordinate remapping).
    struct {
        int  x = 0, y = 0;
        int  w = 0, h = 0;
        bool valid = false;
    } content_rect;
};

// ---------------------------------------------------------------------------
// Backend capabilities (queried before init to drive factory selection)
// ---------------------------------------------------------------------------
struct BackendCaps {
    const char* name            = nullptr;
    bool        hw_accelerated  = false;
    bool        supports_dma_buf = false;
    int         max_width       = 0;
    int         max_height      = 0;
    std::vector<VideoFormat> src_formats;
    std::vector<VideoFormat> dst_formats;
};

// ---------------------------------------------------------------------------
// Abstract kernel interface
// ---------------------------------------------------------------------------
class IVideoTransformKernel {
public:
    virtual ~IVideoTransformKernel() = default;

    virtual const char* backend_name() const = 0;
    virtual BackendCaps capabilities()  const = 0;

    // One-time setup per stream/element: allocate HW resources, validate config.
    // dst_template carries output dimensions + format (data pointer not required).
    // Returns false if the requested configuration is not supported.
    virtual bool init(const FrameDesc& dst_template, const TransformOps& ops) = 0;

    // Per-frame transform.
    //   src      : fully populated FrameDesc (data ptrs valid, caller manages lifetime)
    //   dst      : caller-allocated output buffer wrapped in FrameDesc
    //   slot_id  : scratch-buffer slot index for multi-stream isolation
    //              (dxpreprocess passes stream_id; single-stream elements pass 0)
    //   dynamic  : optional per-call overrides (nullptr = use static ops from init)
    virtual TransformResult transform(const FrameDesc&  src,
                                      FrameDesc&        dst,
                                      int               slot_id = 0,
                                      const DynamicOps* dynamic  = nullptr) = 0;
};

// ---------------------------------------------------------------------------
// Format utility helpers (pure C++, no GStreamer dependency)
// ---------------------------------------------------------------------------

inline int num_planes_for_format(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::I420: return 3;
        case VideoFormat::NV12: return 2;
        case VideoFormat::RGB:
        case VideoFormat::BGR:  return 1;
    }
    return 1;
}

// Bytes per pixel for packed formats (I420/NV12 return 0 — multi-plane).
inline int bytes_per_pixel(VideoFormat fmt) {
    switch (fmt) {
        case VideoFormat::RGB:
        case VideoFormat::BGR:  return 3;
        case VideoFormat::I420:
        case VideoFormat::NV12: return 0;
    }
    return 0;
}

}  // namespace dxt
