**DxInputSelector** is a GStreamer element designed for multi-channel video streaming. It merges frames from multiple input streams into a single synchronized output stream, selecting frames based on Presentation Timestamp (PTS) ordering.

### **Key Features**

**Stream Selection**  

- Among N input streams, DxInputSelector selects the buffer with the smallest PTS and forwards it downstream.  
- This approach ensures that the output stream maintains temporal consistency across input channels.  


**Custom Event Handling**  

- To properly route sticky events received from multiple sink pads, it first sends a custom routing event before forwarding the sticky event downstream.  
- The EOS (End-of-Stream) events received from each sink pad are **not** forwarded downstream directly. instead, they are converted into custom EOS events. This ensures that when EOS is received from one of the N connected input streams, the pipeline does **not** receive a global EOS, and only the EOS for that specific stream is handled.  
- Only after receiving EOS events from all input channels does the element forward a global EOS event downstream.  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxInputSelector
```

### **Properties**  

| **Name**    | **Description**           | **Type**  | **Default Value** |
|-------------|---------------------------|-----------|--------------------|
| `name`     | Sets the unique name of the DxInputSelector element.  | String   | `"dxinputselector0"`   |
| `max-queue-size` | Maximum number of buffers to queue per input stream.  | Unsigned Integer | `2` |
!!! note "NOTE"

- If an incoming buffer does **not** contain `DXFrameMeta`, the element creates a new `DXFrameMeta` using `dx_create_frame_meta()` and assigns the sink pad index as the `stream_id`.
- This metadata tagging is essential for downstream elements that rely on stream identification, such as `DxOutputSelector`.

!!! warning "Downstream Element Restrictions"

    `GstBaseTransform`-based elements that depend on stable caps negotiation (e.g., **DxScale**, **DxConvert**) must **not** be placed directly downstream of DxInputSelector.
    Because DxInputSelector interleaves buffers from multiple streams with potentially different resolutions and color formats, downstream caps would change on every buffer, causing repeated renegotiation, kernel re-creation, and potential caps/buffer mismatch.
    Elements designed to handle per-buffer stream variation via `DXFrameMeta` (e.g., **DxPreprocess**, **DxInfer**, **DxPostprocess**, **DxOsd**) are safe to use downstream.

----
