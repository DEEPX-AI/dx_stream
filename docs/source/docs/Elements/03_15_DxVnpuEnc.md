**DxVnpuEnc** is an element that encodes raw NV12 video frames into H.264 or H.265 bitstreams using the DEEPX VNPU hardware encoder.

!!! note "Build Requirement"

    This element is only available when built with the `--dxvnpu` flag: `./build.sh --dxvnpu`

### **Autoplugging (encodebin)**

DxVnpuEnc's element rank is set dynamically at plugin initialization based on VNPU device availability:

| **Condition** | **Rank** | **Behavior** |
|---|---|---|
| VNPU device present | `GST_RANK_PRIMARY + 1` | Preferred over SW encoders (e.g., x264enc) by encodebin |
| No VNPU device | `GST_RANK_NONE` | Excluded from autoplugging candidates |

When VNPU hardware is available, `encodebin` will automatically select DxVnpuEnc without any explicit configuration.

!!! warning "Property Limitations"

    When autoplugged by encodebin, the `codec` and `bitrate` properties cannot be configured —
    the encoder will use the default values (H.264, 4096 kbps).
    To use custom encoding settings, instantiate `dxvnpuenc` explicitly in the pipeline.

### **Key Features**

**Hardware Encoding**  
Encodes NV12 video frames entirely on the VNPU device, offloading the CPU.

**Codec Selection**  
Supports H.264 (AVC) and H.265 (HEVC) output codecs, configurable via the `codec` property.

**Bitrate Control**  
Target encoding bitrate can be configured via the `bitrate` property (in kbps).

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstVideoEncoder
                         +----GstDxVnpuEnc
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string)NV12` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-h264, stream-format=(string)byte-stream, alignment=(string)au` |
| | `video/x-h265, stream-format=(string)byte-stream, alignment=(string)au` |

### **Properties**

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `codec` | Video codec for encoding. | Enum (h264, h265) | `h264` | - |
| `bitrate` | Target encoding bitrate in kbps. | Unsigned Integer | `4096` | `1 – 100000` |

### **Usage Example**

Encode a decoded video to H.264:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! decodebin ! \
  video/x-raw,format=NV12 ! \
  dxvnpuenc codec=h264 bitrate=8000 ! \
  h264parse ! mp4mux ! filesink location=output.mp4
```

Transcode H.265 to H.264:

```bash
gst-launch-1.0 \
  filesrc location=input.h265 ! h265parse ! \
  dxvnpudec output-format=NV12 ! \
  dxvnpuenc codec=h264 bitrate=4096 ! \
  h264parse ! filesink location=output.h264
```

!!! note "NOTE"

    - DxVnpuEnc accepts **NV12 input only**. Use **DxConvert** to convert other formats to NV12 before encoding.

---
