**DxInfer** is an element that performs AI model inference using the **DEEPX** NPU. It processes input tensors received from **DxPreprocess** elements and produces output tensors for downstream processing.  

- **Input tensors:** **DxInfer** receives preprocessed input tensors from **DxPreprocess** and performs inference using a specified AI model.  
- **Output tensors:** Each output tensor is assigned an ID using the `inference-id` property, allowing downstream elements such as **DxPostprocess** to retrieve the correct output.  
- **Model configuration:** The AI model used for inference must be specified using the `model-path` property, which points to a compiled `.dxnn` file.
- **Multiple backends:** **DxInfer** supports multiple inference backends (`dxrt` and `dxvnpu`), selectable via the `backend` property.

### **Key Features**

**Input Tensor Management**  
Input tensors are linked to **DxInfer** using the `preprocess-id` property. This ensures that the correct tensor from **DXPreprocess** is used for inference.  

**Output Tensor Management**  
Each output tensor is uniquely defined in the `inference-id` property. This allows downstream elements like **DXPostprocess** to connect to the correct inference output.  

**Pipeline Configuration**  
The recommended pipeline chain is **[DxPreprocess] → [DxInfer] → [DxPostprocess]**.
When using features like `secondary-mode`, the configuration must be consistently applied across all three elements (**DxPreprocess , DxInfer and DxPostprocess**).  

**QoS Handling**  
If the downstream sink element has `sync=true`, input buffers may be dropped based on their timestamps to maintain real-time processing performance.  

**Throttle QoS Events**  
When **DxRate** sends a Throttle QoS Event, **DxInfer** drops incoming frames until the accumulated time between frames exceeds the `throttling_delay` value. This avoids unnecessary NPU computation in low-framerate pipelines and promotes smooth and consistent streaming. 

**Multiple Backend Support**  
DxInfer supports multiple inference backends, selectable via the `backend` property:

- **auto** (default): Automatically selects the best available backend (priority: `dxrt` > `dxvnpu`).
- **dxrt**: Uses the DEEPX Runtime (DX-RT) backend.
- **dxvnpu**: Uses the DEEPX VNPU backend (requires `--dxvnpu` build flag).

**JSON Configuration**  
All properties can be configured through a JSON file using the `config-file-path` property. This enables reusable, clean, and scalable configuration of inference behavior. 

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxInfer
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

### **Domain Mode Behavior**

`dxinfer` is a dual-mode element (see [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md)).

- **Normal mode** (sink caps = `video/x-raw`): operates on a single stream; the input tensor lookup uses `preprocess-id` as configured.
- **Domain mode** (sink caps = `application/x-dxvideoraw`): per-stream identity is read from `DXFrameMeta._stream_id`. Inference itself is a shared resource — a single NPU/runtime is used across all streams, and LATENCY is reported as a single domain-wide value (no per-stream LATENCY split).

### **Latency Reporting**

`DxInfer` adds its measured inference time (the moving average `avg_latency` of recent inference calls) as the element's own latency contribution when responding to `LATENCY` queries. Downstream sinks therefore see a realistic pipeline latency that includes inference cost.
### **Properties**

| **Name**           | **Description**                                                                                      | **Type**  | **Default Value** |
|---------------------|------------------------------------------------------------------------------------------------------|-----------|--------------------|
| `name`             | Sets the unique name of the DxInfer element.                                                        | String    | `"dxinfer0"`       |
| `config-file-path` | Path to the JSON config file containing the element's properties.                                    | String    | `null`             |
| `model-path`       | Path to the `.dxnn` model file used for inference.                                                  | String    | `null`             |
| `preprocess-id`    | Key of the input tensor in `DXFrameMeta`/`DXObjectMeta` to feed into inference (must match the `preprocess-id` used by the upstream **DxPreprocess**). | Unsigned Integer | `0`                |
| `inference-id`     | Key under which inference output tensors are stored in `DXFrameMeta`/`DXObjectMeta` (the downstream **DxPostprocess** retrieves them by this ID).     | Unsigned Integer | `0`                |
| `secondary-mode`   | Determines whether to operate in primary mode or secondary mode.                                     | Boolean   | `false`            |
| `use-ort`          | Determines whether to use ONNX Runtime (ORT) for inference.                                          | Boolean   | `true`             |
| `backend`          | Selects the inference backend: `auto`, `dxrt`, or `dxvnpu`.                                          | Enum      | `auto`             |


### **Example JSON Configuration**

```json
{
    "preprocess_id": 1,
    "inference_id": 1,
    "model_path" : "./dx_stream/samples/models/YOLOV5S_1.dxnn",
    "backend": "auto"
}
```

!!! note "NOTE" 

    - The pipeline must follow **[DxPreprocess] → [DxInfer] → [DxPostprocess]** for correct and stable operation.  
    - All properties can also be configured using a JSON file for enhanced usability and flexibility.  

---
