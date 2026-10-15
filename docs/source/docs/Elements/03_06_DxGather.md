**DxGather** is an element designed to merge multiple branches of a stream that originates from the same source, typically split using a GstTee. It recombines these branches back into a single synchronized output stream, ensuring efficient handling of shared content in multi-branch pipelines.  

This makes **DxGather** ideal for scenarios where duplicated streams need to be reunited after parallel processing (e.g., detection, tracking, or visualization).

### **Key Features**

**PTS-Based Buffer Merging**  

- Merges buffers from multiple sink pads only when their Presentation Timestamps (PTS) match.  
- Ensures that only synchronized frames are merged together.  
- The latest PTS among the sink pads is used as the reference. Buffers with a different (older) PTS are dropped with a warning.  

**Buffer and Metadata Handling**  

- Assumes that buffer content is identical across all input branches.  
- Retains one buffer while others are unreferenced.  
- Merges object metadata (`DXObjectMeta`, matched by `_meta_id`) from all branches into the resulting output buffer. Empty fields are filled from the other branches, and objects that exist only in other branches are added.  
- Frame-level metadata (segmentation map, depth map, user metadata) and object user metadata are not merged; the values of the retained buffer are kept.  

**Source Stream Assumption**  

- Only supports streams split from the same source (e.g., using GstTee).  
- **Not** suitable for merging streams from different sources. Buffers from a different source (`_stream_id`) are dropped with a warning.  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstAggregator
                         +----GstDxGather
```

### **Pad Templates**

**Sink (input, request pad `sink_%u`)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw` |

**Src (output, always pad `src`)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw` |

### **Properties**

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|--------------------|
| `name`    | Sets the unique name of the DxGather element.   | String    | `"dxgather0"`         |

!!! warning "DxInputSelector Placement"

    `dxgather` must **not** be placed downstream of **DxInputSelector** (i.e., it cannot be used inside the multi-stream domain). `dxgather` is designed to merge multiple branches that originate from the *same* source (e.g., split by `tee`), not to demultiplex multiple distinct streams. Inside the domain, buffers carry per-stream `DXFrameMeta` and `dxgather` has no notion of stream-id. See [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md).

---
