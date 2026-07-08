**DxOutputSelector** is a GStreamer element used in multi-channel streaming pipelines. It receives a unified stream from `DxInputSelector` and redistributes the buffers to N output pads based on metadata, such as `stream_id`.

It is the exit boundary of the multi-stream domain — see [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md) for the overall model.

### **Key Features**

**Buffer Routing**  

- Buffers arriving at the single sink pad are routed to the correct `src` pad using the `stream_id` field found in the associated `DXFrameMeta`.  
- This enables seamless demultiplexing of multiple logical video streams within a unified pipeline.  
- If `stream_id` is out of range for the configured number of src pads, the buffer is dropped with a warning and the pipeline continues.

**Event Handling**

Sink-side events arriving from upstream are classified and dispatched in three ways:

- **Domain-wide standard events** (`STREAM_START`, `CAPS=application/x-dxvideoraw`, `SEGMENT`, the final global `EOS`) are consumed at the boundary — the downstream pipeline outside the domain does not receive them. Each output stream's lifecycle is reconstructed from the per-stream wrapped events below.
- **Pipeline-wide events** (`FLUSH_START`, `FLUSH_STOP`, `RECONFIGURE`) are **broadcast to every src pad** so that every downstream branch flushes or reconfigures in sync.
- **Per-stream wrapped events** (`application/x-dx-wrapped-event` from `dxinputselector`, carrying the original per-stream `STREAM_START`, `CAPS=video/x-raw`, `SEGMENT`, `TAG`, `EOS`, `GAP`) are unwrapped and forwarded to the src pad whose index matches the wrapped stream-id. This is how each downstream output sees its own original `video/x-raw` lifecycle.

**Upstream QoS**

QoS events arriving on a src pad are wrapped with the corresponding stream-id and sent upstream as a `CUSTOM_UPSTREAM` event. `dxinputselector` unwraps them and forwards each to the matching sink, so a slow consumer on stream `N` only throttles its own upstream source — not the other streams.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxOutputSelector
```

### **Pad Templates**

**Sink (input, always pad `sink`)**

| **Property** | **Value** |
|---|---|
| Format | `application/x-dxvideoraw` |

**Src (output, request pad `src_%u`)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw` |

### **Properties**

| **Name**      | **Description**    | **Type**  | **Default Value** |
|---------------|--------------------|-----------|--------------------|
| `name`        | Sets the unique name of the DxOutputSelector element.    | String   | `"dxoutputselector0"`   |

!!! note "NOTE" 

    - `DxOutputSelector` requires asynchronous operation between its upstream and downstream elements. To achieve this, a `queue` element must be added to each of its src pads. Failure to do so may result in abnormal pipeline hangs.

    - The `stream_id` from the `DXFrameMeta` of the buffer received on the sink pad is parsed and used as the index of the src pad. If `stream_id` does not match any configured src pad index, the buffer is dropped with a warning and the pipeline continues.

!!! warning "No global EOS downstream"

    By design, `DxOutputSelector` does **not** forward the domain-wide global `EOS` to its src pads. Applications that wait for `EOS` on the bus to terminate the pipeline must not rely on receiving it from the downstream side of `DxOutputSelector`. Use the per-stream wrapped `EOS` (unwrapped to the matching src pad) or trigger termination based on application logic (e.g., a timeout, or `EOS` from another sink).

---
