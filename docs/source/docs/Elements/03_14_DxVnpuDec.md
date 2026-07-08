**DxVnpuDec** is an element that decodes H.264/H.265 bitstreams using the DEEPX VNPU hardware decoder.  
It outputs raw video frames in NV12, RGB, or BGR format with optional resolution scaling.

!!! note "Build Requirement"

    This element is only available when built with the `--dxvnpu` flag: `./build.sh --dxvnpu`

### **Autoplugging (decodebin)**

DxVnpuDec's element rank is set dynamically at plugin initialization based on VNPU device availability:

| **Condition** | **Rank** | **Behavior** |
|---|---|---|
| VNPU device present | `GST_RANK_PRIMARY + 1` | Preferred over SW decoders (e.g., avdec_h264) by decodebin |
| No VNPU device | `GST_RANK_NONE` | Excluded from autoplugging candidates |

When VNPU hardware is available, `decodebin` will automatically select DxVnpuDec without any explicit configuration.

```bash
# decodebin automatically selects dxvnpudec on VNPU-equipped systems
gst-launch-1.0 filesrc location=input.mp4 ! qtdemux ! decodebin ! videoconvert ! autovideosink
```

!!! warning "Property Limitations"

    When autoplugged by decodebin, the `output-format`, `output-width`, and `output-height` properties
    cannot be configured — the decoder will always output **NV12 at the original resolution** (the default values).
    These properties are not controllable via downstream caps negotiation either, as the decoder
    resolves them internally from its own property values, not from downstream caps.
    To use custom output settings, instantiate `dxvnpudec` explicitly in the pipeline.

### **Key Features**

**Hardware Decoding**  
Decodes H.264 (AVC) and H.265 (HEVC) bitstreams entirely on the VNPU device, offloading the CPU.

**Output Format Selection**  
The decoded output format can be configured via the `output-format` property. Supported formats: NV12 (default), RGB, BGR.

**Resolution Scaling**  
The decoder can optionally scale the output to a specified resolution using `output-width` and `output-height` properties. When set to `0` (default), the output matches the input resolution.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstVideoDecoder
                         +----GstDxVnpuDec
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-h264, stream-format=(string)byte-stream, alignment=(string)au` |
| | `video/x-h265, stream-format=(string)byte-stream, alignment=(string)au` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ NV12, RGB, BGR }` |

### **Properties**

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `output-format` | Output pixel format from the hardware decoder. | Enum (NV12, RGB, BGR) | `NV12` | - |
| `output-width` | Output width. `0` = same as input. | Integer | `0` | `0 – 8192` |
| `output-height` | Output height. `0` = same as input. | Integer | `0` | `0 – 8192` |

### **Usage Example**

Decode an H.264 file and display:

```bash
gst-launch-1.0 \
  filesrc location=input.h264 ! h264parse ! \
  dxvnpudec output-format=NV12 ! \
  videoconvert ! autovideosink
```

Decode and scale to 640x480 BGR:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! parsebin ! \
  dxvnpudec output-format=BGR output-width=640 output-height=480 ! \
  appsink
```

---
