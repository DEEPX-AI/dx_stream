**DxScale** is an element that scales video frames to a specified target resolution.  
**DxScale** uses the `VideoTransformFactory` to automatically select the best available hardware-accelerated backend (RGA on RK3588) or falls back to the libyuv software backend.  

### **Key Features**

**Resolution Scaling**  

- Scales input video frames to the resolution specified by `width` and `height` properties.  
- Supports NV12, I420, RGB, and BGR pixel formats (same format on input and output — scale only, no conversion).  

**Passthrough Mode**  

- When `width` and `height` are both set to `0` (default), the element passes frames through without any modification.  

**Automatic Backend Selection**  

- On RK3588 platforms with RGA hardware: attempts RGA-accelerated scaling first.  
- Falls back to libyuv software scaling if the hardware backend does not support the source format.  

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
    - Both `width` and `height` must be set together. Setting only one is not supported.

---
