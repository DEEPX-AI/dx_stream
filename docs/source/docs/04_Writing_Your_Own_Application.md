This chapter  describes how to integrate a custom AI model and implement user-defined logic within the **DX-STREAM** pipeline. It assumes that your model has already been **Differences in Post-Processing Logic Based on Inference Mode**  

**Primary Mode**  

- Inference is performed on the entire frame.  
- Postprocessing is responsible for creating new objects (`DXObjectMeta`) based on the model's output.  
- Use `dx_create_object_meta(buf)` to create new objects.
- These new objects are then added to the associated `DXFrameMeta`.  

**Secondary Mode**  

- Inference is performed per object, based on existing `DXObjectMeta` in buffer.  
- Postprocessing is applied to modify or enrich existing object metadata.  
- The `DxObjectMeta` structure contains the input object information, which is passed to the postprocess function for update or enhancement.  

**API Migration Notes:**  

- The `frame_meta->_buf` member has been removed to eliminate circular references.
- All functions now receive `GstBuffer *buf` as the first parameter for direct buffer access.
- Custom libraries must be updated to use the new function signatures.to `.dxnn` format using **DX-COM**. For details on model compilation, refer to **DX-COM User Manual**.  

This guide focuses on how to configure and integrate custom logic into the **DX-STREAM** pipeline using modular elements such as **DxPreprocess, DxInfer,** and **DxPostprocess**.

## Custom Library for Model Inference  

![](./../resources/04_01_writing_your_own_application.png)

The **DX-STREAM** inference pipeline is composed of the following elements.  

**DxPreprocess**  

- Allocates `DXFrameMeta` based on the `GstBuffer` received from upstream.  
- Performs the preprocessing algorithm as defined by elements properties.  
- For custom preprocessing algorithms, a **Custom Pre-Process Library** can be built and integrated.  
- See the **dxpreprocess** section in the Elements documentation for details.  

**DxPostprocess**  

- Receives the input tensor created by `dxpreprocess`.  
- Performs inference using the `dxinfer` element (**DX-RT**).  
- Access the output tensor from `dxinfer` and executes the custom postprocessing algorithm defined in a custom library.  
- A custom postprocessing implementation is required for each model.  
- Example libraries for common vision tasks can be found in `dx_stream/custom_library/postprocess_library`.  

### **Writing Custom Pre-Process Function**
For models requiring additional preprocessing beyond the default functionality, you can implement a **Custom Pre-Process Function** using a user-defined library.  

#### **Implementation Example**  
```cpp
extern "C" bool CustomPreprocessFunc(GstBuffer *buf,
                                   DXFrameMeta *frame_meta,
                                   DXObjectMeta *object_meta,
                                   void* input_tensor) 
{
    // Preprocessing logic
    return true;
}
```

**GstBuffer**  

- Direct access to the GStreamer buffer containing the frame data.  
- Replaces the previous indirect access through `frame_meta->_buf`.  

**DXFrameMeta**  

- Contains frame-level metadata such as dimensions, format, and stream information.  
- No longer contains the `_buf` member - buffer access is provided through the first parameter.  

**DXObjectMeta**  

- In **Secondary Mode**, metadata for each object is passed to the function.  
- In **Primary Mode**, no object metadata is available. (`nullptr`)  

**input_tensor**

- The address of the input tensor generated through user-defined preprocessing.
- It is pre-allocated based on the input tensor size specified by the `dxpreprocess` property and passed to the user. Therefore, users **must not** free or reallocate this memory.

#### **Library Integration** 
To build the custom object library, use a `meson.build` file and compile as follows.

```
gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

gstreamer_plugin_dir = gst_dep.get_pkgconfig_variable('pluginsdir')

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L' + gstreamer_plugin_dir, '-lgstdxstream'],
)


libcustompreproc = shared_library('custompreproc', 
    'preprocess.cpp',
    dependencies: [gst_dep, dx_stream_dep],
    install: true,
    install_dir: plugins_install_dir + '/lib'
)
```

Specify the library path and function name in the JSON configuration file for `dxpreprocess` as follows.
```
{
    "library_file_path": "./install/gstreamer-1.0/lib/libcustompreproc.so",
    "function_name": "CustomPreprocessFunc"
}
```

### **Writing Custom Post-Process Function**  
Postprocessing is essential for interpreting and converting the model’s output tensor into meaningful results. To do this, a custom post-process library **must** be implemented to match your model’s architecture and output format. 

#### **Output Tensor Parsing**  
To check the structure of the output tensor, use the following command. This prints the tensor shape for each output.  

```
$ parse_model -m YOLOv7.dxnn

Example output:

outputs:
  onnx::Reshape_491, FLOAT, [1, 80, 80, 256]
  onnx::Reshape_525, FLOAT, [1, 40, 40, 256]
  onnx::Reshape_559, FLOAT, [1, 20, 20, 256]
```

The example shows three blobs with NHWC dimensions. Use this information to implement the custom postprocessing logic.

#### **Implementation Example**  

```cpp
extern "C" void YOLOV7(GstBuffer *buf,
                       std::vector<dxs::DXTensor> network_output,
                       DXFrameMeta *frame_meta,
                       DXObjectMeta *object_meta)
{
    // Convert output tensor to bounding box information
    
    // Example of creating new object metadata:
    DXObjectMeta *obj_meta = dx_create_object_meta(buf);
    // ... populate object metadata ...
}
```

**Function Parameters:**

- **GstBuffer \*buf**: Direct access to the GStreamer buffer containing frame data
- **std::vector<dxs::DXTensor> network_output**: Output tensors from the inference engine
- **DXFrameMeta \*frame_meta**: Frame-level metadata (dimensions, format, etc.)
- **DXObjectMeta \*object_meta**: Object-level metadata (in Secondary Mode) or nullptr (in Primary Mode)

#### **Library Integration**  
Build the custom library using a `meson.build` script.

```
project('postprocess_yolov5s', 'cpp', version : '1.0.0', license : 'LGPL', default_options: ['cpp_std=c++11'])

gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

gstreamer_plugin_dir = gst_dep.get_pkgconfig_variable('pluginsdir')

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L' + gstreamer_plugin_dir, '-lgstdxstream'],
)

opencv_dep = dependency('opencv4', required: true)

yolo_postprocess_lib = shared_library('postprocess_yolo', 
    'postprocess.cpp',
    dependencies: [opencv_dep, gst_dep, dx_stream_dep, dxrt_dep],
    install: true,
    install_dir: '/usr/share/dx-stream/lib'
)
```

Specify the library path and function name in the JSON configuration file for `dxpostprocess` as follows.  

```
{
    "library_file_path": "./install/gstreamer-1.0/lib/libyolo_postprocess.so",
    "function_name": "yolo_post_process"
}
```

### **Differences in Post-Processing Logic Based on Inference Mode**  

**Primary Mode**  

- Inference is performed on the entire frame.  
- Postprocessing is responsible for creating new objects (`DXObjectMeta`) based on the model’s output.  
- These new objects are then added to the associated `DXFrameMeta`.  

**Secondary Mode**  

- Inference is performed per object, based on existing metadata.  
- Postprocessing is applied to modify or enrich existing object metadata.  
- The `DxObjectMeta` structure contains the input object information, which is passed to the postprocess function for update or enhancement.  

---

## Custom Message Convert Library  

Custom message conversion in **DX-STREAM** requires implementing a user-defined library that converts inference metadata into the desired message format (typically JSON).

The library converts comprehensive object detection metadata including:
- **Object Detection**: label_id, track_id, confidence, name, bounding box
- **Body Features**: extracted body feature vectors for re-identification  
- **Segmentation**: pixel-level classification maps with height, width, and data
- **Pose Estimation**: 17 keypoints with coordinates (kx, ky) and confidence scores (ks)
- **Face Detection**: landmarks, face bounding box, confidence, and face feature vectors

#### **Functions to Implement**  
Your custom library **must** define the following three functions.  

- `dxmsg_create_context`: Initializes the message conversion context  
- `dxmsg_delete_context`: Deletes and releases all resources associated with the context  
- `dxmsg_convert_payload`: Converts the metadata into the target message format  

#### **Implementation Example**  

The custom library implementation consists of the main interface functions and helper functions for JSON conversion:

```cpp
#include "dx_msgconvl_priv.hpp"

extern "C" DxMsgContext *dxmsg_create_context() {
    DxMsgContext *context = g_new0(DxMsgContext, 1);
    context->_priv_data = (void *)dxcontext_create_contextPriv();
    return context;
}

extern "C" void dxmsg_delete_context(DxMsgContext *context) {
    g_return_if_fail(context != nullptr);
    dxcontext_delete_contextPriv((DxMsgContextPriv *)context->_priv_data);
    g_free(context);
}

extern "C" DxMsgPayload *dxmsg_convert_payload(DxMsgContext *context, 
                                              GstDxMsgMetaInfo *meta_info) {
    DxMsgPayload *payload = g_new0(DxMsgPayload, 1);
    if (!payload) {
        g_warning("Failed to allocate DxMsgPayload");
        return nullptr;
    }

    gchar *json_data = dxpayload_convert_to_json(context, meta_info);
    if (json_data == nullptr) {
        g_warning("dxpayload_convert_to_json returned null");
        g_free(payload);
        return nullptr;
    }

    payload->_size = strlen(json_data);
    payload->_data = json_data;
    return payload;
}
```

**Helper Functions Implementation:**

The core JSON conversion logic is implemented in helper functions:

```cpp
// Private context management
DxMsgContextPriv *dxcontext_create_contextPriv(void);
void dxcontext_delete_contextPriv(DxMsgContextPriv *contextPriv);

// Main JSON conversion function
gchar *dxpayload_convert_to_json(DxMsgContext *context, GstDxMsgMetaInfo *meta_info);
```

The `dxpayload_convert_to_json` function processes the metadata and generates the final JSON string using json-glib library functions. The returned JSON data is automatically freed by the DxMsgConv element after transmission.
```

#### **JSON Output Example**

The example `dxpayload_convert_to_json` function implementation generates structured JSON messages by processing metadata from `DXFrameMeta` and `DXObjectMeta` structures. The function uses json-glib library to construct the JSON output:

**JSON Structure Overview:**
```cpp
// Create root JSON object with frame metadata
json_object_set_int_member(jobj_root, "streamId", frame_meta->_stream_id);
json_object_set_int_member(jobj_root, "seqId", meta_info->_seq_id);
json_object_set_int_member(jobj_root, "width", frame_meta->_width);
json_object_set_int_member(jobj_root, "height", frame_meta->_height);

// Process each object in the frame
for (size_t i = 0; i < object_length; i++) {
    DXObjectMeta *obj_meta = (DXObjectMeta *)g_list_nth_data(frame_meta->_object_meta_list, i);
    add_object_meta_to_json(jarray_objects, obj_meta);
}
```

**Complete JSON Output Format:**

```json
{
  "streamId": 0,
  "seqId": 123,
  "width": 1920,
  "height": 1080,
  "objects": [
    {
      "object": {
        "label_id": 1,
        "track_id": 42,
        "confidence": 0.87,
        "name": "person",
        "box": {
          "startX": 300.0,
          "startY": 400.0,
          "endX": 500.0,
          "endY": 600.0
        },
        "body_feature": [0.321, 0.654, 0.987],
        "segment": {
          "height": 1080,
          "width": 1920,
          "data": 140712345678912
        },
        "pose": {
          "keypoints": [
            {"kx": 100.5, "ky": 200.3, "ks": 0.8},
            {"kx": 105.2, "ky": 205.7, "ks": 0.9}
          ]
        },
        "face": {
          "landmark": [
            {"x": 150.2, "y": 180.5},
            {"x": 155.8, "y": 185.3}
          ],
          "box": {
            "startX": 100.0,
            "startY": 150.0,
            "endX": 200.0,
            "endY": 250.0
          },
          "confidence": 0.95,
          "face_feature": [0.123, 0.456, 0.789]
        }
      }
    }
  ]
}
```

**JSON Structure Explanation:**

- **Frame-level metadata**: `streamId`, `seqId`, `width`, `height` are extracted from `DXFrameMeta`
- **Object-level metadata**: Each `DXObjectMeta` from the frame's object list is processed to create:
  - **Object Detection**: `label_id` (_label), `track_id` (_track_id), `confidence` (_confidence), `name` (_label_name->str), `box` (_box[4])
  - **Body Features**: `body_feature` array from _body_feature vector (if available)
  - **Segmentation**: `segment` object with height, width, and data pointer from _seg_cls_map (if available)
  - **Pose Estimation**: `pose` object with 17 keypoints from _keypoints vector, each with kx, ky, ks values (if available)
  - **Face Detection**: `face` object with landmarks from _face_landmarks, face bounding box from _face_box, confidence from _face_confidence, and face features from _face_feature (if available)

**Data Type Handling:**
- Coordinates and confidence values are stored as double precision floating-point
- Feature vectors are converted to JSON arrays of double values
- Integer values (dimensions, IDs) remain as integers
- Memory addresses (like segmentation data pointer) are cast to integer representation

**Note:** Fields like `segment`, `pose`, and `face` are only included if the corresponding metadata is available from upstream elements.
```

#### **Library Integration**  

Build the custom message convert library with proper dependencies:

```meson
gst_dep = dependency('gstreamer-1.0', version : '>=1.16.3',
    required : true, fallback : ['gstreamer', 'gst_dep'])

gstreamer_plugin_dir = gst_dep.get_pkgconfig_variable('pluginsdir')

dx_stream_dep = declare_dependency(
    include_directories : include_directories('/usr/local/include/dx_stream'),
    link_args : ['-L' + gstreamer_plugin_dir, '-lgstdxstream'],
)

json_glib_dep = dependency('json-glib-1.0', required: true)

custom_msgconv_lib = shared_library('dx_msgconvl', 
    ['dx_msgconvl.cpp', 'dx_msgconvl_priv.cpp'],
    dependencies: [gst_dep, dx_stream_dep, json_glib_dep],
    include_directories: [include_directories('.')],
    install: true,
    install_dir: '/opt/dx_stream/msgconv/lib'
)
```

**Required Dependencies:**
- **gstreamer-1.0**: Core GStreamer framework
- **dx_stream**: DX-STREAM metadata and type definitions
- **json-glib-1.0**: JSON processing library for structured output generation

**Usage in Pipeline:**
```bash
dxmsgconv library-file-path=/opt/dx_stream/msgconv/lib/libdx_msgconvl.so
```

**Note:** The `config-file-path` property is no longer required as configuration parsing has been removed from the library implementation.

---
