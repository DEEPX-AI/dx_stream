**DxOsd** is a GStreamer element that provides On-Screen Display (OSD) capabilities by overlaying object information onto the original video frame.  
It uses object metadata (Object Meta) passed from upstream elements, such as DxPostprocess or DxTracker, to render visual elements directly on the frame.  

### **Key Features**  

**Draw Inference Results**  

- Draws bounding boxes on video frames  
- Displays class labels and confidence scores  
- Colors boxes by track ID or class  
- Supports segmentation maps, human pose, and facial landmarks  

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxOsd
```

### **Properties**  

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|-------------------|
| `name`    | Sets the unique name of the DxOsd element.   | String    | ` "dxosd0" `      |


!!! note "NOTE" 

    - DXOSD supports RGB, BGR, NV12, and I420 input formats. The output format matches the input format (passthrough — no format conversion is applied).
    - In DXOSD's output buffers, the `DXFrameMeta` metadata is preserved and passed to downstream elements.
    - Visualizations include bounding boxes, class names, confidence scores, and additional data like segmentation maps, poses, or face landmarks, depending on available metadata.  

---
