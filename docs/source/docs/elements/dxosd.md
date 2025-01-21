
DxOsd is an element that visualizes objects on the original video frame.

- DxOsd overlays object metadata (Object Meta) from upstream elements onto the original frame for On-Screen Display (OSD).
- When visualizing object detection results, it displays bounding boxes, class names, and confidence scores.
- If a `track id` is present in the Object Meta, bounding box colors are distinguished by track ID. Otherwise, colors are distinguished by object class type.
- Additionally, DxOsd can display segmentation maps, poses, and face landmarks.

---

### **Hierarchy**

```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstDxOsd
```

---
### **Properties**

| **Name**  | **Description**                              | **Type**  | **Default Value** |
|-----------|----------------------------------------------|-----------|--------------------|
| `name`    | Sets the unique name of the DxOsd element.   | String    | `"dxosd0"`         |

---

### **Notes**
- DxOsd has no configurable properties beyond its `name`.
- Visualizations include bounding boxes, class names, confidence scores, and additional data like segmentation maps, poses, or face landmarks, depending on available metadata.