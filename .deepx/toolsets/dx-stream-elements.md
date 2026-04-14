# dx_stream GStreamer Elements Reference

> Primary API reference for all 13 dx_stream GStreamer elements.
> Use this as the definitive source for element properties, pads, and usage patterns.

---

## 1. DxPreprocess

**Description:** Prepares video frames for NPU inference. Handles resize, color conversion,
normalization, and letterbox padding. In secondary mode, crops detected object ROIs.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `preprocess-id` | int | 0 | Unique ID linking to a DxInfer element |
| `resize-width` | int | 0 | Target width for model input |
| `resize-height` | int | 0 | Target height for model input |
| `keep-ratio` | bool | true | Maintain aspect ratio with letterbox padding |
| `pad-value` | int | 0 | Fill value for letterbox padding (0=black, 114=gray) |
| `secondary-mode` | bool | false | Crop per-object ROIs from detected objects |
| `interval` | int | 0 | Process every Nth frame in secondary mode |
| `min-object-width` | int | 0 | Minimum object width to process (secondary) |
| `min-object-height` | int | 0 | Minimum object height to process (secondary) |
| `target-class-id` | int | -1 | Filter objects by class ID (-1 = all classes) |
| `config-file-path` | string | "" | Path to JSON configuration file |

**Pad Templates:**
- Sink: `video/x-raw, format={RGB, BGR, NV12, I420}`
- Source: `video/x-raw`

**Usage Example:**
```bash
# Inline properties
dxpreprocess preprocess-id=1 resize-width=640 resize-height=640

# Config file mode
dxpreprocess config-file-path=/path/to/preprocess_config.json

# Secondary mode (classify detected objects)
dxpreprocess preprocess-id=2 resize-width=224 resize-height=224 \
    secondary-mode=true interval=5 min-object-width=50 min-object-height=50
```

**Common Pitfalls:**
- preprocess-id must match the downstream DxInfer preprocess-id
- Forgetting `keep-ratio=false` for models that require exact dimensions
- Setting `interval=0` in secondary mode processes every frame (high CPU)

---

## 2. DxInfer

**Description:** Executes NPU inference on preprocessed frames using a compiled .dxnn
model file. Attaches DXTensorMeta to the output buffer.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `preprocess-id` | int | 0 | Must match upstream DxPreprocess |
| `inference-id` | int | 0 | Unique ID linking to downstream DxPostprocess |
| `model-path` | string | "" | Absolute path to .dxnn model file |
| `batch-size` | int | 1 | Batch size for inference |
| `secondary-mode` | bool | false | Run inference on per-object ROIs |
| `config-file-path` | string | "" | Path to JSON configuration file |

**Pad Templates:**
- Sink: `video/x-raw`
- Source: `video/x-raw` (with DXTensorMeta attached)

**Usage Example:**
```bash
# Inline properties
dxinfer preprocess-id=1 inference-id=1 model-path=/abs/path/to/YoloV8N.dxnn

# Config file mode
dxinfer config-file-path=/path/to/inference_config.json

# Secondary mode
dxinfer preprocess-id=2 inference-id=2 secondary-mode=true \
    model-path=/abs/path/to/EfficientNet_Lite0.dxnn
```

**Common Pitfalls:**
- model-path must be absolute (not relative)
- preprocess-id mismatch with DxPreprocess causes silent failures
- Using a .dxnn file compiled for a different NPU version

---

## 3. DxPostprocess

**Description:** Decodes raw inference tensor output into structured object metadata
(DXObjectMeta) using a custom shared library (.so).

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `inference-id` | int | 0 | Must match upstream DxInfer |
| `library-file-path` | string | "" | Absolute path to postprocess .so library |
| `function-name` | string | "PostProcess" | Name of the entry function in the .so |
| `secondary-mode` | bool | false | Decode per-object inference results |
| `config-file-path` | string | "" | Path to JSON configuration file |

**Pad Templates:**
- Sink: `video/x-raw` (with DXTensorMeta)
- Source: `video/x-raw` (with DXObjectMeta attached)

**Usage Example:**
```bash
# Inline properties
dxpostprocess inference-id=1 \
    library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so \
    function-name=PostProcess

# Config file mode
dxpostprocess config-file-path=/path/to/postprocess_config.json
```

**Common Pitfalls:**
- library-file-path MUST be absolute
- Using wrong .so for the model (e.g., yolov8n model with yolov5s postprocess)
- Missing function-name when the .so uses a non-default export name

---

## 4. DxTracker

**Description:** Multi-object tracker using OC-SORT algorithm. Assigns persistent track
IDs to detected objects across frames. Operates on DXObjectMeta from DxPostprocess.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | Tracker configuration JSON path |

**Pad Templates:**
- Sink: `video/x-raw` (with DXObjectMeta)
- Source: `video/x-raw` (with track_id populated in DXObjectMeta)

**Usage Example:**
```bash
dxtracker config-file-path=/path/to/tracker_config.json
```

**Common Pitfalls:**
- MUST follow DxPostprocess (needs DXObjectMeta)
- Placing before DxPostprocess has no objects to track
- Missing config file causes default parameters (may not suit all scenarios)

---

## 5. DxOsd

**Description:** On-screen display element. Renders bounding boxes, class labels,
confidence scores, track IDs, and keypoints on video frames.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | OSD configuration JSON path |

**Pad Templates:**
- Sink: `video/x-raw` (with DXObjectMeta)
- Source: `video/x-raw` (with overlays rendered)

**Usage Example:**
```bash
dxosd
dxosd config-file-path=/path/to/osd_config.json
```

**Common Pitfalls:**
- No DXObjectMeta on input = no overlays rendered (but no error)
- Performance overhead on high-resolution streams (consider DxScale first)

---

## 6. DxGather

**Description:** N-to-1 multiplexer that merges multiple secondary inference branches
back into a single stream. Used with `tee` element in secondary mode pipelines.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `num-sources` | int | 2 | Number of input sink pads to create |

**Pad Templates:**
- Sink: `sink_0`, `sink_1`, ..., `sink_{N-1}` (request pads)
- Source: single `src` pad

**Usage Example:**
```bash
# Secondary mode: tee splits, DxGather merges
tee name=t \
t. ! queue ! <secondary_branch_A> ! gather.sink_0 \
t. ! queue ! <secondary_branch_B> ! gather.sink_1 \
dxgather name=gather ! dxosd
```

**Common Pitfalls:**
- num-sources must match actual number of connected sink pads
- Unconnected sink pads cause pipeline hang

---

## 7. DxInputSelector

**Description:** N-to-1 round-robin input selector for time-division multiplexed
inference. Cycles through N input streams, sending one frame at a time to the
inference chain.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| (auto-configured) | | | Sink pads created on demand via pad requests |

**Pad Templates:**
- Sink: `sink_0`, `sink_1`, ..., `sink_{N-1}` (request pads)
- Source: single `src` pad

**Usage Example:**
```bash
urisourcebin uri=rtsp://cam1 ! decodebin ! in.sink_0
urisourcebin uri=rtsp://cam2 ! decodebin ! in.sink_1
dxinputselector name=in ! dxpreprocess ! ... ! dxoutputselector name=out
```

**Common Pitfalls:**
- Must pair with DxOutputSelector for correct frame routing
- Frame ordering depends on round-robin scheduling

---

## 8. DxOutputSelector

**Description:** 1-to-N output demultiplexer. Routes processed frames back to their
original stream index. Paired with DxInputSelector.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| (auto-configured) | | | Source pads created on demand |

**Pad Templates:**
- Sink: single `sink` pad
- Source: `src_0`, `src_1`, ..., `src_{N-1}` (request pads)

**Usage Example:**
```bash
dxoutputselector name=out
out.src_0 ! queue ! dxscale width=640 height=360 ! comp.sink_0
out.src_1 ! queue ! dxscale width=640 height=360 ! comp.sink_1
```

**Common Pitfalls:**
- Number of src pads must match DxInputSelector sink pads
- Unconnected src pads cause frame drops

---

## 9. DxRate

**Description:** Frame rate limiter. Drops excess frames to maintain a maximum frame
rate. Essential for variable-rate sources like RTSP.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `max-rate` | int | 30 | Maximum frames per second to pass through |

**Pad Templates:**
- Sink: `video/x-raw`
- Source: `video/x-raw`

**Usage Example:**
```bash
# Limit RTSP to 15 FPS
urisourcebin uri=rtsp://camera ! decodebin ! dxrate max-rate=15 ! dxpreprocess ...
```

**Common Pitfalls:**
- Without DxRate, RTSP sources can flood the pipeline with buffered frames
- Setting max-rate too low wastes NPU capacity
- Setting max-rate too high negates the rate limiting

---

## 10. DxMsgConv

**Description:** Converts DXObjectMeta from inference results into JSON message format
for publishing to external systems.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | Message format configuration JSON |

**Pad Templates:**
- Sink: `video/x-raw` (with DXObjectMeta)
- Source: `video/x-raw` (with JSON message attached)

**Usage Example:**
```bash
dxmsgconv config-file-path=/path/to/msgconv_config.json
```

**Common Pitfalls:**
- Must receive DXObjectMeta (place after DxPostprocess)
- Missing config file uses default JSON format

---

## 11. DxMsgBroker

**Description:** Publishes JSON messages to an external message broker (Kafka or MQTT).
Terminal element with no source pad.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `broker-name` | string | "" | Broker type: "kafka" or "mqtt" |
| `conn-info` | string | "" | Connection string (host:port) |
| `topic` | string | "" | Topic/channel name to publish to |
| `config` | string | "" | Broker-specific configuration file path |

**Pad Templates:**
- Sink: `video/x-raw`
- Source: none (terminal element)

**Usage Example:**
```bash
# Kafka
dxmsgbroker broker-name=kafka conn-info=localhost:9092 topic=detections \
    config=/path/to/broker_kafka.cfg

# MQTT
dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=detections
```

**Common Pitfalls:**
- Must follow DxMsgConv (expects JSON message format)
- Broker must be running before pipeline starts
- No src pad means nothing can follow DxMsgBroker in the pipeline

---

## 12. DxScale

**Description:** Video scaling element. Resizes video frames to specified dimensions.
Useful for multi-stream compositor grids.

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| `width` | int | 0 | Target width in pixels |
| `height` | int | 0 | Target height in pixels |

**Pad Templates:**
- Sink: `video/x-raw`
- Source: `video/x-raw`

**Usage Example:**
```bash
# Scale for compositor grid
dxscale width=640 height=360
```

**Common Pitfalls:**
- Width/height of 0 passes through unchanged
- Not a replacement for DxPreprocess (no normalization/padding)

---

## 13. DxConvert

**Description:** Color space conversion element. Converts between video formats
(e.g., NV12 to RGB, BGR to I420).

**Properties:**

| Property | Type | Default | Description |
|---|---|---|---|
| (auto-negotiated) | | | Conversion determined by caps negotiation |

**Pad Templates:**
- Sink: `video/x-raw, format={RGB, BGR, NV12, I420, YUY2, ...}`
- Source: `video/x-raw, format={RGB, BGR, NV12, I420, YUY2, ...}`

**Usage Example:**
```bash
# Convert color space for display compatibility
... ! dxconvert ! video/x-raw,format=RGB ! ...
```

**Common Pitfalls:**
- Prefer GStreamer's `videoconvert` for standard conversions
- DxConvert is optimized for hardware-accelerated conversion on supported platforms

---

## Element Ordering Rules

### Valid Pipeline Chains

```
source → DxPreprocess → queue → DxInfer → queue → DxPostprocess → queue → DxOsd → sink
source → DxPreprocess → queue → DxInfer → queue → DxPostprocess → queue → DxTracker → queue → DxOsd → sink
source → ... → DxPostprocess → queue → DxMsgConv → queue → DxMsgBroker
```

### Invalid Orderings

```
DxInfer → DxTracker          # ERROR: DxTracker needs DXObjectMeta from DxPostprocess
DxPostprocess → DxMsgBroker  # ERROR: DxMsgBroker needs JSON from DxMsgConv
DxInfer → DxOsd              # ERROR: DxOsd needs DXObjectMeta from DxPostprocess
```
