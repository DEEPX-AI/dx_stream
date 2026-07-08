**DxScale** is an element that scales video frames to a specified target resolution.  
**DxScale** uses the `VideoTransformFactory` to automatically select the best available hardware-accelerated backend. The selection priority is: V3 DSP > VNPU > RGA > libyuv (software fallback).  

### **Key Features**

**Resolution Scaling**  

- Scales input video frames to the resolution specified by `width` and `height` properties.  
- Supports NV12, I420, RGB, and BGR pixel formats (same format on input and output — scale only, no conversion).  

**Passthrough Mode**  

- When `width` and `height` are both set to `0` (default), the element passes frames through without any modification.  

**Automatic Backend Selection**  

- The `VideoTransformFactory` selects the best available backend automatically:
  1. **V3 DSP** – DEEPX V3 device DSP (when built with `--v3`)
  2. **VNPU** – DEEPX VNPU VideoProcessor (when built with `--dxvnpu`)
  3. **RGA** – Rockchip Raster Graphic Accelerator (auto-detected)
  4. **libyuv** – Software fallback (always available)  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxScale
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

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `width` | Target output width. Set to `0` for passthrough. | Unsigned Integer | `0` | `0 – 8192` |
| `height` | Target output height. Set to `0` for passthrough. | Unsigned Integer | `0` | `0 – 8192` |

### **Usage Example**

Scale an I420 video stream from 1920×1080 to 640×480:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  dxscale width=640 height=480 ! \
  videoconvert ! autovideosink
```

Scale NV12 decoder output before inference:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  dxscale width=640 height=640 ! \
  dxpreprocess preprocess-id=1 ... ! \
  dxinfer ...
```

!!! note "NOTE"

    - `dxscale` does **not** perform color conversion. Input and output must be the same format.
    - For color conversion, use **DxConvert**.
    - Setting both `width` and `height` together is strongly recommended. If only one is set (the other left at `0`), the element does **not** raise an error — it silently falls back to passthrough behavior, which may not match user intent. Set both explicitly when scaling is required.

!!! warning "DxInputSelector Placement"

    `dxscale` must **not** be placed downstream of **DxInputSelector**. DxInputSelector merges multiple streams with potentially different resolutions into a single output, causing caps to change on every buffer. Since `dxscale` is a `GstBaseTransform` element, it requires stable caps for buffer pool allocation and kernel initialization. Placing it after DxInputSelector will cause repeated caps renegotiation, kernel re-creation, and potential caps/buffer mismatch errors.
    Place `dxscale` **before** DxInputSelector (per-stream) or **after** DxOutputSelector (per-stream).

---
