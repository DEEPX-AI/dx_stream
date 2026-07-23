**DxConvert** is an element that converts video frames between different color formats.  
**DxConvert** uses the `VideoTransformFactory` to automatically select the best available hardware-accelerated backend. The selection priority is: V3 DSP > RGA > libyuv (software fallback).  

### **Key Features**

**Color Format Conversion**  

- Converts video frames between NV12, I420, RGB, and BGR pixel formats.  
- Supports all 4×4 format combinations (e.g., NV12→RGB, I420→BGR, RGB→NV12, etc.).  

**Same-Format Passthrough**  

- When the input and output formats are identical, the element passes frames through without any modification (in-place).  

**Automatic Backend Selection**  

- The `VideoTransformFactory` selects the best available backend automatically:
  1. **V3 DSP** – DEEPX V3 device DSP (when built with `--v3`)
  2. **RGA** – Rockchip Raster Graphic Accelerator (auto-detected)
  3. **libyuv** – Software fallback (always available)  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxConvert
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ NV12, I420, RGB, BGR }` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ NV12, I420, RGB, BGR }` |

### **Properties**

DxConvert has no configurable properties. The target format is negotiated automatically through GStreamer caps negotiation.

### **Usage Example**

Convert an NV12 stream to RGB:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  video/x-raw,format=NV12 ! \
  dxconvert ! \
  video/x-raw,format=RGB ! \
  autovideosink
```

Convert I420 to BGR before inference:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  video/x-raw,format=I420 ! \
  dxconvert ! \
  video/x-raw,format=BGR ! \
  dxpreprocess preprocess-id=1 ... ! \
  dxinfer ...
```

Combine with DxScale — convert format and resize in a pipeline:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  video/x-raw,format=NV12 ! \
  dxconvert ! \
  video/x-raw,format=RGB ! \
  dxscale width=640 height=480 ! \
  autovideosink
```

!!! note "NOTE"

    - `dxconvert` does **not** perform scaling. Input and output resolutions must match.
    - For video scaling, use **DxScale**.
    - Hardware acceleration is available for NV12→RGB and NV12→BGR on RGA. Other combinations use the libyuv software backend.

!!! warning "DxInputSelector Placement"

    `dxconvert` must **not** be placed downstream of **DxInputSelector**. DxInputSelector merges multiple streams with potentially different color formats into a single output, causing caps to change on every buffer. Since `dxconvert` is a `GstBaseTransform` element, it requires stable caps for buffer pool allocation and kernel initialization. Placing it after DxInputSelector will cause repeated caps renegotiation, kernel re-creation, and potential caps/buffer mismatch errors.
    Place `dxconvert` **before** DxInputSelector (per-stream) or **after** DxOutputSelector (per-stream).

---
