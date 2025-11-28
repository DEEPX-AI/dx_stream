**DxRate** is an element that adjusts the framerate of a video stream to match a defined target framerate.  
**DxRate** archives this by dropping or duplicating frames based on the timestamps of incoming buffers. If the input stream already matches the desired framerate, no frames are altered.  
Otherwise, **DxRate** modifies the output buffer timestamps to maintain consistent intervals, ensuring the smooth and accurate playback at the specified framerate.  

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
                         +----DxRate
```

### **Properties**

| **Name**       | **Description**                                                          | **Type**  | **Default Value** |
|-----------------|--------------------------------------------------------------------------|-----------|--------------------|
| `name`         | Sets the unique name of the DxRate element.                              | String    | `"dxrate0"`        |
| `framerate`    | Sets the target framerate (FPS). This property must be configured.       | Integer   | `0`                |
| `throttle`     | Determines whether to send Throttle QoS Events upstream on frame drops.  | Boolean   | `false`            |

!!! note "NOTE" 

    - The `framerate` property is mandatory and **must** be explicitly set for the element to function.

---
