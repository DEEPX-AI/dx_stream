**DXGather** is an element designed to merge multiple branches of a stream that originates from the same source, typically split using a GstTee. It recombines these branches back into a single synchronized output stream, ensuring efficient handling of shared content in multi-branch pipelines.  

This makes **DXGather** ideal for scenarios where duplicated streams need to be reunited after parallel processing (e.g., detection, tracking, or visualization).

### **Key Features**

**PTS-Based Buffer Merging**  

- Merges buffers from multiple sink pads only when their Presentation Timestamps (PTS) match.  
- Ensures that only synchronized frames are merged together.  

**Buffer and Metadata Handling**  

- Assumes that buffer content is identical across all input branches.  
- Retains one buffer while others are unreferenced.  
- Merges metadata from all branches into the resulting output buffer.  

**Source Stream Assumption**  

- Only supports streams split from the same source (e.g., using GstTee).  
- **Not** suitable for merging streams from different sources.  

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
