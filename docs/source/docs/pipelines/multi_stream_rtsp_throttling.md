
The following pipeline demonstrates how to process four RTSP video streams simultaneously using the YOLOv7 model for object detection, visualize the results as a tiled display, and regulate the frame rate to 10 FPS:

![](./../../resources/multi_stream_rtsp.png)

```
gst-launch-1.0 \
urisourcebin uri=rtsp://[ip_address]:[port]/[stream_path] ! decodebin ! mux.sink_0 \
urisourcebin uri=rtsp://[ip_address]:[port]/[stream_path] ! decodebin ! mux.sink_1 \
urisourcebin uri=rtsp://[ip_address]:[port]/[stream_path] ! decodebin ! mux.sink_2 \
urisourcebin uri=rtsp://[ip_address]:[port]/[stream_path] ! decodebin ! mux.sink_3 \
dxmuxer name=mux live-source=true ! queue ! \
dxpreprocess config-file-path=/path/to/YOLOv7/preprocess_config.json ! queue ! \
dxinfer config-file-path=/path/to/YOLOv7/inference_config.json ! queue ! \
dxpostprocess config-file-path=/path/to/YOLOv7/postprocess_config.json ! \
dxosd ! queue ! \
dxrate framerate=10 throttle=true ! \
dxtiler config-file-path=/path/to/tiler_config.json ! queue ! \
fpsdisplaysink sync=true
```

---

### **Explanation**

**Pipeline Overview**

- Processes four RTSP streams, performs object detection using the YOLOv7 model, and combines the results into a single tiled display.
- The `dxrate` element regulates the frame rate to 10 FPS, ensuring consistent playback and reduced NPU load.

**Element Descriptions**

- **`urisourcebin`**: Retrieves RTSP video streams. The `uri` property must be set to the RTSP URL for each stream (e.g., `rtsp://[ip_address]:[port]/[stream_path]`).
- **`decodebin`**: Decodes the video streams from the RTSP sources.
- **`dxmuxer`**: Combines multiple streams into a single stream.

    `live-source=true`: Indicates live RTSP sources.

- **`queue`**: Buffers data between elements to enable asynchronous processing.

    Improves pipeline performance by decoupling elements.

    The `max-size-buffers` property controls the maximum number of buffers in the queue.

    The `leaky` property determines how buffers are handled when the queue is full.

- **`dxpreprocess`**: Applies pre-processing according to the configuration file specified in the `config-file-path`.
- **`dxinfer`**: Performs inference using the YOLOv7 model. The model configuration file path is specified in `config-file-path`.
- **`dxpostprocess`**: Post-processes the model's output tensor to extract metadata. The configuration file path is specified in `config-file-path`.
- **`dxosd`**: Visualizes the detection results, including bounding boxes, class labels, and confidence scores, by overlaying them on the video frames.
- **`dxrate`**: Controls the frame rate. The `framerate` property is set to 10 FPS, and `throttle=true` enables NPU throttling to prevent unnecessary computations.
- **`dxtiler`**: Arranges frames from all input streams into a tiled layout for display.
- **`fpsdisplaysink`**: Displays the tiled video frames. The `sync=true` property ensures playback is synchronized with the buffer timestamps.

---

### **Usage Notes**

**Multi-Stream Configuration**

- The `dxmuxer` element combines multiple streams into a single stream.
- The `name` property (e.g., `mux` in this example) must be set, and sink pads must be named `[name].sink_[index]`, where `index` starts at 0 and increments for each stream.

**Asynchronous Processing with `queue`**

- Using `queue` elements between processing stages allows elements to operate asynchronously, improving pipeline performance.
- Configure `queue` properties

    **`max-size-buffers`**: Specifies the maximum number of buffers in the queue.

    **`leaky`**: Controls how the queue handles buffers when full (e.g., drop the oldest buffer or block processing).

**Frame Rate Control**

- The `dxrate` element sets the target frame rate to 10 FPS. Excess frames are dropped to maintain this rate.
- **`throttle=true`** interacts with the `dxinfer` element to enable NPU throttling, reducing computational load and power usage.

**Quality of Service (QoS)**

- The `fpsdisplaysink` element's `sync=true` and `dxrate`'s `throttle=true` enable QoS, ensuring smooth real-time streaming.
- **Note**: QoS may result in buffer drops to maintain real-time playback and frame rate.

**Sink Element Options**

- While `fpsdisplaysink` is used in this example, other sink elements like `ximagesink` or `autovideosink` can be used for display, depending on your environment.
