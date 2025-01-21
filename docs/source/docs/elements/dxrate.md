
DxRate is an element that adjusts the stream's framerate to match a target `framerate`.

- It converts the input stream to the specified `framerate` by dropping or duplicating frames based on the buffer's timestamps.
- For streams where no adjustment is necessary, no frames will be dropped or duplicated.
- The timestamps of the output buffers are modified to ensure the target `framerate` is met, with the duration between frames increasing by the interval determined by the framerate.

---

### **Key Features**

**Framerate Adjustment**

- Ensures the output stream matches the target `framerate` by dropping or duplicating frames.
- The output buffer's timestamps are adjusted relative to the first input buffer's timestamp.

**Throttle QoS Event**

- If `throttle` is set to `true`, a Throttle QoS Event is sent upstream when frames are dropped.
- `DxInfer` can use this event to implement throttling based on the `throttling_delay` value provided.
- For throttling to work in `DxInfer`, the `DxRate` element must be placed downstream of `DxInfer` in the pipeline.

**Distinction Between Framerate and Video Speed**

- **Framerate** refers to the number of frames per second (FPS) for smooth playback.
- **Video speed** refers to playback speed (e.g., fast-forward), which is a separate concept.

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----DxRate
```

---
### **Properties**

| **Name**       | **Description**                                                          | **Type**  | **Default Value** |
|-----------------|--------------------------------------------------------------------------|-----------|--------------------|
| `name`         | Sets the unique name of the DxRate element.                              | String    | `"dxrate0"`        |
| `framerate`    | Sets the target framerate (FPS). This property must be configured.       | Integer   | `0`                |
| `throttle`     | Determines whether to send Throttle QoS Events upstream on frame drops.  | Boolean   | `false`            |

---

### **Notes**
- The `framerate` property is mandatory and must be explicitly set for the element to function.
- To enable throttling in `DxInfer`, place `DxRate` downstream of `DxInfer` in the pipeline.
- Ensure not to confuse framerate (FPS for smooth playback) with video speed (playback rate, such as fast-forward).

