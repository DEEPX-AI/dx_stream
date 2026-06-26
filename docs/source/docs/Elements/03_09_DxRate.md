**DxRate** is an element that adjusts the framerate of a video stream to match a defined target framerate.  
**DxRate** achieves this by dropping or duplicating frames based on the timestamps of incoming buffers.  
**DxRate always recomputes output buffer timestamps based on the target framerate**, producing evenly spaced output PTS regardless of the input PTS pattern. There is no conditional passthrough — even when the input framerate already matches the target, the output PTS is regenerated from the target rate.

### **Key Features**  

**Framerate Adjustment**  

- Ensures the output stream matches the target `framerate` by dropping or duplicating frames.  
- The output buffer's timestamps are adjusted relative to the first input buffer's timestamp.  

**Throttle QoS Event**  

- If `throttle` is set to `true`, a Throttle QoS Event is sent upstream when frames are dropped.  
- **DxInfer** can respond this event t by applying throttling, using the `throttling_delay` value.  
- To enable this function properly, **DxRate must** be placed downstream of **DxInfer** in the pipeline.  

**Framerate and Video Speed**  

- **Framerate** refers to the number of frames per second (FPS) for visual smooth playback.  
- **Video speed** refers to playback speed (e.g., fast-forward), which is a separate concept.  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxRate
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw; application/x-dxvideoraw` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw; application/x-dxvideoraw` |

### **Properties**

| **Name**       | **Description**                                                          | **Type**  | **Default Value** |
|-----------------|--------------------------------------------------------------------------|-----------|--------------------|
| `name`         | Sets the unique name of the DxRate element.                              | String    | `"dxrate0"`        |
| `framerate`    | Sets the target framerate (FPS). This property must be configured.       | Unsigned Integer | `0`                |
| `throttle`     | Determines whether to send Throttle QoS Events upstream on frame drops.  | Boolean   | `false`            |

### **Domain Mode Behavior**

`dxrate` is a dual-mode element (see [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md)).

- **Normal mode** (sink caps = `video/x-raw`): a single rate state controls drop/duplicate decisions for the single stream.
- **Domain mode** (sink caps = `application/x-dxvideoraw`): per-stream rate state. Each `_stream_id` from `DXFrameMeta` has its own first-PTS reference, drop counter and duplicate counter, so framerate adjustment is applied independently per stream.

!!! note "NOTE" 

    - The `framerate` property is mandatory and **must** be explicitly set for the element to function. Setting `framerate=0` results in a `GST_ELEMENT_ERROR` during caps negotiation.

---
