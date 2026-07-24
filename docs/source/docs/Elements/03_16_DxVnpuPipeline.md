**DxVnpuPipeline** is an all-in-one multi-channel element that performs decode, process, and inference entirely on the VNPU device.  
It accepts H.264/H.265 bitstreams and outputs inference tensors via `application/x-dxtensor` caps.

!!! note "Build Requirement"

    This element is only available when built with the `--dxvnpu` flag: `./build.sh --dxvnpu`

### **Key Features**

**Multi-Channel Pipeline**  
Supports multiple input/output channels via request pads (`sink_%u` / `src_%u`). Each channel runs an independent decode → process → inference pipeline on the VNPU device.

**Device-Side Processing**  
All processing (decoding, color conversion, resizing, inference) is performed on the VNPU device. The host only receives inference tensor results, minimizing CPU usage and PCIe traffic.

**HDMI Video Output (VO Mode)**  
When `use-vnpu-hdmi=true`, the VNPU device outputs decoded video directly to HDMI. The channel count for the display grid is automatically determined from the number of connected channels.

**Tensor Output**  
Each channel outputs inference tensors as `DXFrameMeta` attached to empty GStreamer buffers. Downstream elements like **DxPostprocess** can process these tensors normally.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxVnpuPipeline
```

### **Pad Templates**

**Sink (input) — Request Pads**

| **Property** | **Value** |
|---|---|
| Name | `sink_%u` |
| Format | `video/x-h264, stream-format=(string)byte-stream, alignment=(string)au` |
| | `video/x-h265, stream-format=(string)byte-stream, alignment=(string)au` |

**Src (output) — Request Pads**

| **Property** | **Value** |
|---|---|
| Name | `src_%u` |
| Format | `application/x-dxtensor` |

### **Properties**

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `model-path` | Path to the `.dxnn` model file for inference. | String | `null` | - |
| `inference-id` | Key for `_output_tensors` map in DXFrameMeta. | Unsigned Integer | `0` | `0 – MAX` |
| `keep-ratio` | Maintain aspect ratio during processor resize. | Boolean | `true` | - |
| `use-ort` | Use ORT runtime for inference. | Boolean | `true` | - |
| `device-id` | VNPU device ID. `-1` for auto round-robin. | Integer | `-1` | `-1 – MAX` |
| `use-vnpu-hdmi` | Enable VNPU device HDMI video output (VO mode). | Boolean | `false` | - |

### **Usage Example**

Single-channel inference pipeline:

```bash
gst-launch-1.0 \
  dxvnpupipeline name=vp model-path=yolo26-n_640x640.dxnn inference-id=0 device-id=0 \
  filesrc location=input.mp4 ! parsebin ! vp.sink_0 \
  vp.src_0 ! dxpostprocess inference-id=0 \
    library-file-path=libpostprocess_yolo26od.so function-name=PostProcess ! \
  fakesink
```

Multi-channel with HDMI output and overlay:

```bash
gst-launch-1.0 \
  dxvnpupipeline name=vp model-path=yolo26-n_640x640.dxnn inference-id=0 \
    device-id=0 use-vnpu-hdmi=true \
  filesrc location=ch0.mp4 ! parsebin ! vp.sink_0 \
  filesrc location=ch1.mp4 ! parsebin ! vp.sink_1 \
  vp.src_0 ! queue ! dxpostprocess inference-id=0 \
    library-file-path=libpostprocess_yolo26od.so function-name=PostProcess ! \
    dxvnpuoverlay model-path=yolo26-n_640x640.dxnn device-id=0 ! fakesink \
  vp.src_1 ! queue ! dxpostprocess inference-id=0 \
    library-file-path=libpostprocess_yolo26od.so function-name=PostProcess ! \
    dxvnpuoverlay model-path=yolo26-n_640x640.dxnn device-id=0 ! fakesink
```

!!! note "NOTE"

    - The element accepts **bitstream input only** (H.264/H.265). For raw video input, use **DxVnpuDec** + **DxInfer** instead.
    - Output buffers contain only metadata (no video frames). Video is displayed via device HDMI when `use-vnpu-hdmi=true`.
    - Downstream of DxVnpuPipeline, use **DxPostprocess** for tensor parsing and **DxVnpuOverlay** for HDMI bbox rendering.

---
