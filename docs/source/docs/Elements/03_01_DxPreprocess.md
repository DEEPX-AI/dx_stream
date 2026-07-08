**DxPreprocess** is an element that performs input preprocessing on raw video frames, converting them into a format suitable for AI models used by **DxInfer**. Each preprocessed input tensor is assigned an input ID specified by the `preprocess-id` property.  

**DxInfer must** reference this ID in its own configuration to determine which tensor to use for inference.  

Multiple **DxPreprocess** elements can co-exist in the same pipeline, enabling support for multi-input models or customized preprocessing steps for different video streams.  


### **Key Features**

**Color Conversion**  

**DxPreprocess** converts the image color format to `RGB` or `BGR`  based on the `color-format`  property.  

- Default format: `RGB`.  

**Modes of Operation**  

- **Primary Mode** applies preprocessing to the entire frame. If object detection is performed within the same pipeline, **DxPreprocess** will operate in this mode.  
- **Secondary Mode** applies preprocessing to individual object regions detected in this frame. This mode requires object metadata (e.g., from an upstream object detection element).

**Region of Interest (ROI)**  
The Region of Interest (ROI) is defined using the roi property.  

- In **Primary Mode**, the specified ROI area is cropped and preprocessed as a whole.  
- In **Secondary Mode**, only objects that are fully contained within the ROI are selected for preprocessing.  

**Processing Interval**  
The processing interval is controlled by the `interval` property. It skips a specified number of frames before preprocessing the next frame or object. This is useful for reducing processing frequency in resource-constrained environments. 

**Object Filtering in Secondary Mode**  
Objects can be filtered based on the following criteria.  

- **Class ID**: Use the `target-class-id` property to process only objects that match the specified class. Non-matching objects are ignored.  
- **Size** : Use the `min-object-width` and `min-object-height` properties to exclude objects smaller than the defined size.

**Resizing**  
**DxPreprocess** resizes full frames or object regions using the `resize-width` and `resize-height` property.  
If `keep-ratio` is set to `true`, the aspect ratio is preserved by applying padding.  
Padding color is set using the `pad-value` property.

**Custom Preprocessing**  
User-defined preprocessing logic can be implemented by providing:  

- `library-file-path` : Path to the custom shared library (`.so`).  
- `function-name` : Name of the preprocessing function within the library.  

The custom function signature has been updated to include GstBuffer access:
```cpp
extern "C" bool PreProcess(GstBuffer *buf,
                          DXFrameMeta *frame_meta,
                          DXObjectMeta *object_meta,
                          void *output);
```

This allows implementation of customized data handling tailored to specific AI models with direct access to the GStreamer buffer. 

**QoS Handling**  
If the downstream sink element has `sync=true`, input buffers may be dropped based on their timestamps. This helps maintain real-time performance and avoids frame backlog under system load.

**H/W Acceleration**  
DxPreprocess uses the `VideoTransformFactory` to automatically select the best available hardware backend for color conversion and resizing. The selection priority is:

1. **V3 DSP** – DEEPX V3 device DSP (when built with `--v3`)
2. **VNPU** – DEEPX VNPU VideoProcessor (when built with `--dxvnpu`)
3. **RGA** – Rockchip Raster Graphic Accelerator (auto-detected)
4. **libyuv** – Software fallback (always available)

The backend is selected automatically at runtime — no configuration required.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxPreprocess
```

### **Pad Templates**

**Sink (input)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ RGB, I420, NV12 }; application/x-dxvideoraw` |

**Src (output)**

| **Property** | **Value** |
|---|---|
| Format | `video/x-raw, format=(string){ RGB, I420, NV12 }; application/x-dxvideoraw` |

!!! note "Caps `format` vs `color-format` property"

    The sink/src caps `format` specifies the **input video color space** (the raw frame layout that `DxPreprocess` accepts). The `color-format` property below specifies the **output tensor color space** (RGB or BGR) produced for `DxInfer`. They are independent — for example, an NV12 input video can produce a BGR output tensor.

### **Domain Mode Behavior**

`dxpreprocess` is a dual-mode element (see [Multi-Stream Domain](./03_00_Multi_Stream_Domain.md)).

- **Normal mode** (sink caps = `video/x-raw`): frame dimensions are read from caps. `stream_id` is treated as `0`.
- **Domain mode** (sink caps = `application/x-dxvideoraw`): frame dimensions are read from `DXFrameMeta._width/_height`, and `stream_id` from `DXFrameMeta._stream_id`. The element also requests `DXFrameMeta` as a buffer meta during `ALLOCATION` so that upstream pools allocate buffers with the meta attached.
### **Properties**  
This table provides a complete reference to the properties of the **DxPreprocess** element.  

| **Name**     | **Description**     | **Type**      | **Default Value**    |
|--------------|---------------------|---------------|----------------------|
| `name`       | Sets the unique name of the DxPreprocess element.   | String   | `"dxpreprocess0"`  |
| `config-file-path`  | Path to the JSON config file containing the element's properties.   | String    | `null`   |
| `preprocess-id`     | Assigns an ID to the preprocessed input tensor.    | Unsigned Integer  | `0`    |
| `resize-width`       | Specifies the width for resizing.                                                                   | Unsigned Integer     | `0`                    |
| `resize-height`      | Specifies the height for resizing.                                                                  | Unsigned Integer     | `0`                    |
| `keep-ratio`         | Maintains the original aspect ratio during resizing.                                                | Boolean              | `true`                 |
| `pad-value`          | Padding color value for R, G, B pixels during                                             | Unsigned Integer     | `0`                    |
| `color-format`       | Specifies the color format for preprocessing. (`0`: RGB, `1`: BGR)                                  | Unsigned Integer     | `0` (RGB)              |
| `secondary-mode`     | Enables Secondary Mode for processing object regions.                                               | Boolean              | `false`                |
| `target-class-id`    | Filters objects in Secondary Mode by class ID. (`-1` processes all objects).                        | Integer              | `-1`                   |
| `min-object-width`   | Minimum object width for preprocessing in Secondary Mode.                                           | Unsigned Integer     | `0`                    |
| `min-object-height`  | Minimum object height for preprocessing in Secondary Mode.                                          | Unsigned Integer     | `0`                    |
| `roi`                | Defines the ROI (Region of Interest) for preprocessing as a comma-separated string `"x1,y1,x2,y2"`. | String               | `"-1,-1,-1,-1"`        |
| `interval`           | Specifies the interval for preprocessing frames or objects.                                         | Unsigned Integer     | `0`                    |
| `transpose`          | Enables transposing of the output tensor axes.                                                      | Boolean              | `false`                |
| `library-file-path`  | Path to the custom preprocess library, if used.                                                     | String               | `null`                 |
| `function-name`      | Name of the custom preprocessing function to use.                                                   | String               | `null`                 |



### **Example JSON Configuration**

```json
{
    "preprocess_id": 1,
    "resize_width": 640,
    "resize_height": 640,
    "keep_ratio": true
}
```

!!! note "NOTE" 

    - For implementing custom preprocess logic, refer to **Chapter. Writing Your Own Application `“Custom Pre-Process Library Documentation”`**. Custom libraries must follow the error reporting contract described there (no `g_error()` / `abort()`; use `g_warning()` for per-frame skips and `throw std::runtime_error` for permanent errors).
    - All properties can also be configured using a JSON file for enhanced usability and flexibility.

---
