**DxInfer** is an element that performs AI model inference using the **DEEPX** NPU. It processes input tensors received from **DxPreprocess** elements and produces output tensors for downstream processing.  

- **Input tensors:** **DxInfer** receives preprocessed input tensors from **DxPreprocess** and performs inference using a specified AI model.  
- **Output tensors:** Each output tensor is assigned an ID using the `inference-id` property, allowing downstream elements such as **DxPreprocess** to retrieve the correct output.  
- **Model configuration:** The AI model used for inference must be specified using the `model-path` property, which points to a compiled `.dxnn` file.

### **Key Features**

**Input Tensor Management**  
Input tensors are linked to **DxInfer** using the `preprocess-id` property. This ensures that the correct tensor from **DXPreprocess** is used for inference.  

**Output Tensor Management**  
Each output tensor is uniquely defined in the `inference-id` property. This allows downstream elements like **DXPostprocess** to connect to the correct inference output.  

**Pipeline Configuration**  
The recommended pipeline chain is **[DxPreprocess] → [DxInfer] → [DxPostprocess]**.
When using features like `secondary-mode`, the configuration must be consistently applied across all three elements (**DxPreprocess , DxInfer and DxPostprocess**).  

**QoS Handling**  
If the downstream sink element has `sync-true`, input buffers may be dropped based on their timestamps to maintain real-time processing performance.  

**Throttle QoS Events**  
When **DxRate** sends a Throttle QoS Event, **DxInfer** delays inference by the `throttling_delay` interval. This avoids unnecessary NPU computation in low-framerate pipelines and promotes smooth and consistent streaming. 

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

### **Properties**

| **Name**           | **Description**                                                                                      | **Type**  | **Default Value** |
|---------------------|------------------------------------------------------------------------------------------------------|-----------|--------------------|
| `name`             | Sets the unique name of the DxInfer element.                                                        | String    | `"dxinfer0"`       |
| `config-file-path` | Path to the JSON config file containing the element's properties.                                    | String    | `null`             |
| `model-path`       | Path to the `.dxnn` model file used for inference.                                                  | String    | `null`             |
| `preprocess-id`    | Specifies the ID of the input tensor to be used for **inference**.                                       | Integer   | `0`                |
| `inference-id`     | Specifies the ID of the output tensor to be used for **postprocess**.                                                       | Integer   | `0`                |
| `secondary-mode`   | Determines whether to operate in primary mode or secondary mode.                                     | Boolean   | `false`            |


### **Example JSON Configuration**

```json
{
    "preprocess_id": 1,
    "inference_id": 1,
    "model_path" : "./dx_stream/samples/models/YOLOV5S_1.dxnn"
}
```

**Notes.**  

- The pipeline must follow **[DxPreprocess] → [DxInfer] → [DxPostprocess]** for correct and stable operation.  
- All properties can also be configured using a JSON file for enhanced usability and flexibility.  

---
