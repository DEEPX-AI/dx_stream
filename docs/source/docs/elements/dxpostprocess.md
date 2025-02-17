
### **Overview**
DxPostprocess processes the output tensor generated from the DxInfer element and post-processes the result data by creating and modifying DXObjectMeta.

- DxPostprocess receives output tensors from `DxInfer` and performs postprocess using a specified AI model. 

---

### **Key Features**

**Modes of Operation**

   - **Primary Mode** : Create `DXObjectMeta` to `DXFrameMeta` 

   - **Secondary Mode** : Modify `DXObjectMeta` by output tensor from DxInfer

**Custom Postprocessing**

   - Use a custom postprocess library by specifying:
     - `library-file-path`: Path to the library.
     - `function-name`: Name of the custom preprocessing function.

   - Refer to [**Writing Your Own Application**](./../writing_your_own_application.md)

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxPostprocess
```

---

### **Properties**

| **Name**             | **Description**                                                                                      | **Type**             | **Default Value**       |
|-----------------------|------------------------------------------------------------------------------------------------------|----------------------|-------------------------|
| `name`               | Sets the unique name of the DxPreprocess element.                                                   | String               | `"dxpreprocess0"`       |
| `config-file-path`   | Path to the JSON config file containing the element's properties.                                    | String               | `null`                 |
| `inference-id`      | Assigns an ID to the preprocessed input tensor.                                                      | Integer              | `0`                    |
| `secondary-mode`     | Enables Secondary Mode for processing object regions.                                               | Boolean              | `false`                |
| `library-file-path`  | Path to the custom preprocess library, if used.                                                     | String               | `null`                 |
| `function-name`      | Name of the custom preprocessing function to use.                                                   | String               | `null`                 |

---

### **Example JSON Configuration**

```json
{
    "inference_id": 1,
    "library_file_path": "/usr/share/dx-stream/lib/libpostprocess_yolo.so",
    "function_name": "YOLOV5Pose_1"
}
```

---

### **Notes**
- Properties can also be configured using JSON for user convenience.
