**DxVnpuDec** is an element that decodes H.264/H.265 bitstreams using the DEEPX VNPU hardware decoder.
It outputs host-memory NV12 frames at the coded resolution.

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

### **Key Features**

**Hardware Decoding**  
Decodes H.264 (AVC) and H.265 (HEVC) bitstreams entirely on the VNPU device, offloading the CPU.

**Output Format**
The decoder always outputs NV12. Use `dxconvert` or `dxscale` downstream for conversion or scaling.

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
| Format | `video/x-raw, format=(string)NV12` |

### **Properties**

| **Name** | **Description** | **Type** | **Default Value** | **Range** |
|---|---|---|---|---|
| `device-id` | VNPU device index. `-1` selects automatically. | Integer | `-1` | `-1 – 127` |

### **Usage Example**

Decode an H.264 file and display:

```bash
gst-launch-1.0 \
  filesrc location=input.h264 ! h264parse ! \
  dxvnpudec ! \
  videoconvert ! autovideosink
```

Decode on a specific VNPU device:

```bash
gst-launch-1.0 \
  filesrc location=input.mp4 ! parsebin ! \
  dxvnpudec device-id=0 ! \
  appsink
```

---
