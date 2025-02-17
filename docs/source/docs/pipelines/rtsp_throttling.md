
The following pipeline demonstrates how to process an RTSP video stream in real-time using the YOLOv7 model for object detection, visualize the results, and control the stream's frame rate at 10 FPS:

![](./../../resources/rtsp_throttling.png)

```
gst-launch-1.0 \
urisourcebin uri=rtsp://[ip_address]:[port]/[stream_path] ! \
decodebin ! \
dxpreprocess config-file-path=/path/to/YOLOv7/preprocess_config.json ! \
dxinfer config-file-path=/path/to/YOLOv7/infer_config.json ! \
dxpostprocess config-file-path=/path/to/YOLOv7/postprocess_config.json ! \
dxosd ! \
dxrate framerate=10 throttle=true ! \
fpsdisplaysink sync=true
```

---

### **Explanation**

**Pipeline Overview**:
- This pipeline processes an RTSP video stream, performs object detection using the YOLOv7 model, overlays the results on the video frames, and regulates the frame rate to 10 FPS.

**Element Descriptions**

   - **`urisourcebin`**: Retrieves the video stream from the RTSP source. The `uri` property must be set to the RTSP URL (e.g., `rtsp://[ip_address]:[port]/[stream_path]`).
   - **`decodebin`**: Decodes the video stream from the RTSP source.
   - **`dxpreprocess`**: Applies pre-processing according to the configuration file specified in the `config-file-path`.
   - **`dxinfer`**: Performs inference using the YOLOv7 model. The model configuration file path is specified in `config-file-path`.
   - **`dxpostprocess`**: Post-processes the model's output tensor to extract metadata. The configuration file path is specified in `config-file-path`.
   - **`dxosd`**: Visualizes the detection results, including bounding boxes, class labels, and confidence scores, by overlaying them on the video frames.
   - **`dxrate`**: Controls the stream's frame rate. The `framerate` property sets the target FPS, and the `throttle=true` setting enables NPU throttling to reduce unnecessary computation.
   - **`fpsdisplaysink`**: Displays the video frames with synchronization (`sync=true`) to match the real-time playback timing.

---

### **Usage Notes**

**RTSP URL**

- Replace `[ip_address]`, `[port]`, and `[stream_path]` in the `uri` property with the RTSP stream's address, port, and path.

**Frame Rate Control**

- The `dxrate` element regulates the frame rate to the specified value in the `framerate` property (e.g., 10 FPS in this example).
- Frames exceeding the set FPS are dropped to maintain a consistent frame rate.

**NPU Throttling**

- The `dxrate` element's `throttle=true` setting interacts with the `dxinfer` element to prevent unnecessary NPU computations, optimizing resource usage.

**Quality of Service (QoS)**

- The `fpsdisplaysink` element's `sync=true` setting and `dxrate`'s `throttle=true` enable QoS functionality.
- This allows the pipeline to handle real-time streaming efficiently, even at the cost of dropping buffers to maintain the target FPS.

**Custom Models**

- This pipeline can be adapted for other object detection models or tasks by updating the `config-file-path` properties for `dxpreprocess`, `dxinfer` and `dxpostprocess` to point to the appropriate configuration files.

**Sink Element Options**

- While `fpsdisplaysink` is used in this example, other sink elements like `ximagesink` or `autovideosink` can be used for display, depending on your environment.
