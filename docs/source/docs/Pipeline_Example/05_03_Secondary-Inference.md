This section describes the secondary mode inference pipeline, which performs object-level inference based on objects detected in the primary mode. A single input stream is processed through primary inference for object detection, followed by tracking.  

The tracked objects are then passed to secondary inference, where additional tasks such as face detection are performed on each detected object, with the results stored in each object's metadata.  

Finally, the results are rendered for visualization.

![](./../../resources/05_03_secondary_pipeline.png)

The pipeline in the figure is defined in 
`dx_stream/dx_stream/pipelines/secondary_mode/run_secondary_mode.sh` and can be used as a reference for execution.

### **Explanation**  

**Pipeline Flow**  

1. **Object Detection**: Primary inference detects objects in the input stream
2. **Tracker**: Assigns unique IDs to detected objects and tracks them across frames
3. **Secondary Face Detection**: Performs face detection on each tracked object

### **Usage Notes**  

**Configure file setting** 

- This pipeline runs multiple models sequentially. Be careful to configure each model's preprocess_id and inference_id properly to avoid unexpected behavior.

**Secondary post process**

- When implementing a custom post-processing function for secondary inference mode, modify the provided DXObjectMeta directly. Removing or re-allocating the metadata may lead to unintended behavior.

---
