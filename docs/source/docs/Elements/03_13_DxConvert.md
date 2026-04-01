**DxConvert** is an element that converts video frames between different color formats.  
**DxConvert** uses the `VideoTransformFactory` to automatically select the best available hardware-accelerated backend (RGA on RK3588) or falls back to the libyuv software backend.  

### **Key Features**

**Color Format Conversion**  

- Converts video frames between NV12, I420, RGB, and BGR pixel formats.  
- Supports all 4×4 format combinations (e.g., NV12→RGB, I420→BGR, RGB→NV12, etc.).  

**Same-Format Passthrough**  

- When the input and output formats are identical, the element passes frames through without any modification (in-place).  

**Automatic Backend Selection**  

- On RK3588 platforms with RGA hardware: attempts RGA-accelerated conversion first (currently supports NV12→RGB and NV12→BGR).  
- Falls back to libyuv software conversion if the hardware backend does not support the source format or format combination.  

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
    - On RK3588, RGA hardware acceleration is available for NV12→RGB and NV12→BGR conversions. Other combinations use the libyuv software backend.

---
