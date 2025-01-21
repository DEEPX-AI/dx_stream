
The DXGather element is designed to merge streams originating from the same source, which have been split using a GstTee element, into a single stream.

**PTS-Based Buffer Merging** 

- The DXGather element compares incoming streams from multiple sink pads and merges buffers with identical PTS values into a single buffer.

**Buffer and Metadata Handling**

- During the merging process, the original buffer data is assumed to be identical across the incoming streams. One of the buffers is unreferenced (unref), and their respective metadata is compared and merged into the resulting buffer.

**Source Stream Assumption**

- The DXGather element expects the upstream streams to originate from the same source, split using a GstTee element. It is not suitable for merging streams from different sources. For merging streams from distinct sources, the [DXMuxer](./dxmuxer.md) element should be used.

This design ensures efficient handling of identical source streams, making DXGather an optimal choice for specific stream merging scenarios in GStreamer pipelines.

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxGather
```

---
### **Properties**

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|--------------------|
| `name`    | Sets the unique name of the DxGather element.   | String    | `"dxgather0"`         |

---
