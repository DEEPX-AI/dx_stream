**DxOsd** is a GStreamer element that provides On-Screen Display (OSD) capabilities by overlaying object information onto the original video frame.  
It uses object metadata (Object Meta) passed from upstream elements, such as DxPostprocess or DxTracker, to render visual elements directly on the frame.  

### **Key Features**  

**Draw Inference Results**  

- Draws bounding boxes on video frames  
- Displays class labels and confidence scores  
- Colors boxes by track ID or class  
- Supports segmentation maps, human pose, and facial landmarks  

**H/W Acceleration**  

DXOSD supports hardware acceleration when used with Rockchip RGA. The RGA accelerates the color and format conversions needed during the drawing process, enabling more efficient resource usage.

**Resolution adjustment**

The output buffer of DXOSD can be set to a different resolution than the original buffer. Reducing the output resolution lowers the processing load, enabling more efficient performance.

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstDxOsd
```

### **Properties**  

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|-------------------|
| `name`    | Sets the unique name of the DxOsd element.   | String    | ` "dxosd0" `      |
| `width`    | Sets the width of output buffer             | Integer   | `640`         |
| `height`    | Sets the height of output buffer           | Integer   | `360`         |


**Notes.**  

- DXOSD produces output buffers in BGR format. Therefore, to display the output, use a displaysink that supports this format or insert a videoconvert element to convert it to a suitable format.
- In DXOSD’s output buffers, the `DXFrameMeta` metadata is removed. Therefore, it is **not** suitable for use upstream of elements that require this metadata.
- Visualizations include bounding boxes, class names, confidence scores, and additional data like segmentation maps, poses, or face landmarks, depending on available metadata.  

---
