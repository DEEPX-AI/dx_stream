
### **Overview**
DxPreprocess is an element that preprocesses raw video frames into a format suitable for AI models used in DxInfer. 

- Each preprocessed input tensor is assigned an ID specified by the `preprocess-id` property.
- DxInfer must explicitly set the `preprocess-id` to specify which input tensor to use for inference.
- Multiple DxPreprocess elements can exist in the pipeline, and the `preprocess-id` links specific input tensors to DxInfer.

---

### **Key Features**

**Color Conversion**

   - Converts the image color format to `RGB`, `BGR` based on the `color-format` property.  
   - Default format: `RGB`.

**Modes of Operation**

   - **Primary Mode**
    Preprocessing applied to the entire frame. 

   - **Secondary Mode**
   
      Preprocessing applied to individual object regions within the frame.  
      Requires object metadata (from upstream object detection).  
      If object detection is performed with DxPreprocess, it will operate in Primary Mode.

**Region of Interest (ROI)**

   - Set using the `roi` property.  
   - In **Primary Mode**, the ROI area is cropped and preprocessed.  
   - In **Secondary Mode**, only objects fully contained within the ROI are preprocessed.

**Processing Interval**

   - Controlled by the `interval` property.  
   - Skips a defined number of frames before preprocessing the next frame or object.

**Object Filtering in Secondary Mode**

   - Objects can be filtered by

     **Class ID**

      Use the `class-id` property to exclude objects that do not match the specified class.

     **Size**

      Use `min-object-width` and `min-object-height` properties to exclude objects smaller than the specified dimensions.

**Resizing**

   - Resizes frames or object regions to `resize-width` and `resize-height`.  
   - Maintains aspect ratio with padding if `keep-ratio` is set to `true`.  
   - Padding color is controlled by the `pad-value` property.

**Custom Preprocessing**

   - Use a custom preprocess library by specifying:
     - `library-file-path`: Path to the library.
     - `function-name`: Name of the custom preprocessing function.

**Memory Management**

   - Preallocates reusable memory blocks for input tensors based on the `pool-size` property.

**QoS Handling**

   - Input buffers may be dropped based on their timestamps when the sink element's `sync` property is `true`.  

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----DxPreprocess
```

---

### **Properties**

| **Name**             | **Description**                                                                                      | **Type**             | **Default Value**       |
|-----------------------|------------------------------------------------------------------------------------------------------|----------------------|-------------------------|
| `name`               | Sets the unique name of the DxPreprocess element.                                                   | String               | `"dxpreprocess0"`       |
| `config-file-path`   | Path to the JSON config file containing the element's properties.                                    | String               | `null`                 |
| `preprocess-id`      | Assigns an ID to the preprocessed input tensor.                                                      | Integer              | `0`                    |
| `resize-width`       | Specifies the width for resizing.                                                                   | Integer              | `0`                    |
| `resize-height`      | Specifies the height for resizing.                                                                  | Integer              | `0`                    |
| `keep-ratio`         | Maintains the original aspect ratio during resizing.                                                | Boolean              | `false`                |
| `pad-value`          | Padding color value for R, G, B pixels during                                             | Integer              | `0`                    |
| `color-format`       | Specifies the color format for preprocessing.                                                       | Enum (`rgb`, `bgr`)  | `0` (`rgb`)            |
| `secondary-mode`     | Enables Secondary Mode for processing object regions.                                               | Boolean              | `false`                |
| `target-class-id`    | Filters objects in Secondary Mode by class ID. (`-1` processes all objects).                        | Integer              | `-1`                   |
| `min-object-width`   | Minimum object width for preprocessing in Secondary Mode.                                           | Integer              | `0`                    |
| `min-object-height`  | Minimum object height for preprocessing in Secondary Mode.                                          | Integer              | `0`                    |
| `roi`                | Defines the ROI (Region of Interest) for preprocessing.                                             | Array of Integers    | `[-1, -1, -1, -1]`     |
| `interval`           | Specifies the interval for preprocessing frames or objects.                                         | Integer              | `0`                    |
| `library-file-path`  | Path to the custom preprocess library, if used.                                                     | String               | `null`                 |
| `function-name`      | Name of the custom preprocessing function to use.                                                   | String               | `null`                 |
| `pool-size`          | Number of preallocated memory blocks for input tensors.                                             | Integer              | `1`                    |

---

### **Example JSON Configuration**

```json
{
    "preprocess_id": 1,
    "resize_width": 640,
    "resize_height": 640,
    "keep_ratio": true
}
```

---

### **Notes**
- For custom preprocess logic, refer to the **Custom Pre-Process Library Documentation**.
- Properties can also be configured using JSON for user convenience.
