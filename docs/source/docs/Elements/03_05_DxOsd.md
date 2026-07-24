**DxOsd** is a GStreamer element that provides On-Screen Display (OSD) capabilities by overlaying object information onto the original video frame.  
It uses object metadata (Object Meta) passed from upstream elements, such as DxPostprocess or DxTracker, to render visual elements directly on the frame.  

### **Key Features**  

**Draw Inference Results**  

- Draws bounding boxes on video frames  
- Displays class labels and confidence scores  
- Colors boxes by track ID or class  
- Supports segmentation maps, human pose, and facial landmarks  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxOsd
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ RGB, BGR, NV12, I420 }; application/x-dxvideoraw` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ RGB, BGR, NV12, I420 }; application/x-dxvideoraw` |

### **Domain Mode Behavior**

`dxosd` is a dual-mode element (see [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md)).

- **Normal mode** (sink caps = `video/x-raw`): a single `video_info` is taken from the caps and reused for every buffer.
- **Domain mode** (sink caps = `application/x-dxvideoraw`): `dxosd` reads the per-stream wrapped `CAPS` events emitted by `dxinputselector` (one per stream, carrying the original `video/x-raw` caps and the stream-id) and stores a separate `video_info` per `_stream_id`. For each incoming buffer it picks the matching `video_info` from `DXFrameMeta._stream_id`, so streams with different resolutions or formats are rendered correctly.

### **Properties**  

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|-------------------|
| `name`    | Sets the unique name of the DxOsd element.   | String    | ` "dxosd0" `      |


!!! note "NOTE" 

    - DXOSD supports RGB, BGR, NV12, and I420 input formats. The output format matches the input format (passthrough — no format conversion is applied).
    - In DXOSD's output buffers, the `DXFrameMeta` metadata is preserved and passed to downstream elements.
    - Visualizations include bounding boxes, class names, confidence scores, and additional data like segmentation maps, poses, or face landmarks, depending on available metadata.  

!!! note "Multi-Stream Support"

    DxOsd can be safely placed downstream of **DxInputSelector**. It automatically tracks per-stream video information via wrapped caps events, allowing correct OSD rendering for buffers from different streams with different resolutions and formats.

---
