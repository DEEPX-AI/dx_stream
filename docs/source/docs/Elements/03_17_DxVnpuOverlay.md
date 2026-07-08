**DxVnpuOverlay** is an element that sends bounding-box overlay data to the VNPU device HDMI output via the `OverlayRenderer` API.  
It reads `DXObjectMeta` from upstream (typically from **DxPostprocess**), converts bounding boxes to model-input coordinates, and renders them on the device HDMI display.

!!! note "Build Requirement"

    This element is only available when built with the `--dxvnpu` flag: `./build.sh --dxvnpu`

### **Key Features**

**Device HDMI Overlay**  
Renders bounding boxes directly on the VNPU device HDMI output. Works in conjunction with **DxVnpuPipeline** (with `use-vnpu-hdmi=true`) which outputs video to the same HDMI display.

**Shared OverlayRenderer**  
One `OverlayRenderer` instance is shared per device across all channels. Reference counting ensures proper lifecycle management.

**Coordinate Transform**  
Automatically transforms bounding box coordinates from original image space to model input space, supporting both letterbox (`keep-ratio=true`) and stretch modes.

**Passthrough**  
The element passes buffers through without modification — it only reads metadata and sends overlay commands to the device.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseSink
                         +----GstDxVnpuOverlay
```

### **Pad Templates**

`DxVnpuOverlay` is a sink element — it has a sink pad only, no src pad.

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `application/x-dxtensor` |

### **Properties**

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `model-path` | Path to `.dxnn` model file (used to read input shape for overlay config). | String | `null` | - |
| `keep-ratio` | Maintain aspect ratio for overlay canvas calculation. | Boolean | `true` | - |
| `device-id` | VNPU device ID for OverlayRenderer. `-1` for auto. | Integer | `-1` | `-1 – MAX` |
| `group-count` | Number of overlay groups for HDMI display grid layout. | Integer | `1` | `1 – MAX` |

### **Usage Example**

Used after DxPostprocess in a DxVnpuPipeline chain:

```bash
gst-launch-1.0 \
  dxvnpupipeline name=vp model-path=yolo26-n_640x640.dxnn inference-id=0 \
    device-id=0 use-vnpu-hdmi=true \
  filesrc location=input.mp4 ! parsebin ! vp.sink_0 \
  vp.src_0 ! queue ! \
    dxpostprocess inference-id=0 \
      library-file-path=libpostprocess_yolo26od.so function-name=PostProcess ! \
    dxvnpuoverlay model-path=yolo26-n_640x640.dxnn keep-ratio=true device-id=0 ! \
    fakesink sync=false
```

!!! note "NOTE"

    - DxVnpuOverlay must be used with **DxVnpuPipeline** (`use-vnpu-hdmi=true`) for video to be visible on HDMI.
    - The `model-path` must match the model used in the upstream DxVnpuPipeline for correct coordinate mapping.
    - The element reads `DXObjectMeta` from the buffer metadata — it does not process video frames.

---
