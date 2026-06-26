**DxInputSelector** is a GStreamer element designed for multi-channel video streaming. It merges frames from multiple input streams into a single synchronized output stream, selecting frames based on Presentation Timestamp (PTS) ordering.

It is the entry boundary of the multi-stream domain — its src caps are `application/x-dxvideoraw`. See [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md) for the overall model.

### **Key Features**

**Stream Selection**  

- Among N input streams, DxInputSelector selects the buffer with the smallest PTS and forwards it downstream.  
- This approach ensures that the output stream maintains temporal consistency across input channels.  


**Event Handling**

- After all input sink pads have received their first sticky events, `DxInputSelector` emits a single domain `STREAM_START`, a `CAPS` event with `application/x-dxvideoraw`, and a `SEGMENT` event downstream — these are the L1A events that drive the merged chain.
- The original per-stream sticky events (`STREAM_START`, `CAPS=video/x-raw`, `SEGMENT`, `TAG`, `GAP`) and per-stream `EOS` are wrapped as `application/x-dx-wrapped-event` (a `CUSTOM_DOWNSTREAM` event) tagged with the stream-id, and emitted alongside the L1A events. Downstream elements that need per-stream lifecycle information (e.g., `DxOsd`, `DxOutputSelector`) read them from the wrap.
- `EOS` received on a single sink does **not** propagate downstream immediately. It is wrapped as a per-stream `EOS`. Only after every sink has received `EOS` is a single global `EOS` emitted downstream.
- Upstream `QoS`/`RECONFIGURE` events arriving as `CUSTOM_UPSTREAM` wraps from `DxOutputSelector` are unwrapped and routed to the corresponding sink pad's upstream peer.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstAggregator
                         +----GstDxInputSelector
```

### **Pad Templates**

**Sink (input, request pad `sink_%u`)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw` |

**Src (output, always pad `src`)**

| **Property** | **Value** |
|---|---|
| Format | `application/x-dxvideoraw` |

### **Properties**  

| **Name**    | **Description**           | **Type**  | **Default Value** |
|-------------|---------------------------|-----------|--------------------|
| `name`     | Sets the unique name of the DxInputSelector element.  | String   | `"dxinputselector0"`   |
| `max-queue-size` | Maximum number of buffers to queue per input stream.  | Unsigned Integer | `2` |

!!! note "NOTE"  

    - If an incoming buffer does **not** contain `DXFrameMeta`, the element creates a new `DXFrameMeta` using `dx_create_frame_meta()` and assigns the sink pad index as the `stream_id`.  
    - This metadata tagging is essential for downstream elements that rely on stream identification, such as `DxOutputSelector`.  
    - When two input streams share the same framerate, their buffers can interleave by up to **50%** in the merged output (i.e., a buffer from one stream may arrive several PTS positions ahead or behind its strict interleave position before the other stream catches up). This is expected behavior of PTS-based selection.

!!! warning "Downstream Element Restrictions"  

    `GstBaseTransform`-based elements that depend on stable caps negotiation (e.g., **DxScale**, **DxConvert**) must **not** be placed directly downstream of DxInputSelector. The reason is that the src caps of DxInputSelector are `application/x-dxvideoraw`, not `video/x-raw` — standard GStreamer elements (`videoconvert`, `videoscale`, `compositor`, …) will fail caps negotiation.  
    Elements designed for the domain (`DxPreprocess`, `DxInfer`, `DxPostprocess`, `DxTracker`, `DxOsd`, `DxRate`, `DxMsgConv`, `DxMsgBroker`) accept `application/x-dxvideoraw` and read per-buffer dimensions/stream-id from `DXFrameMeta`.  
    See [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md) for the placement matrix.

----
