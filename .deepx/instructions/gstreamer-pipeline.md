# GStreamer Pipeline Composition Guide

## 13 Elements Reference

### DxPreprocess

Prepares video frames for NPU inference: resize, color convert, normalize, letterbox.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `preprocess-id` | int | 0 | Links this preprocessor to a DxInfer element |
| `resize-width` | int | 0 | Target width for model input |
| `resize-height` | int | 0 | Target height for model input |
| `keep-ratio` | bool | true | Maintain aspect ratio (letterbox) |
| `pad-value` | int | 0 | Padding fill value when letterboxing |
| `secondary-mode` | bool | false | Process per-object ROIs instead of full frame |
| `interval` | int | 0 | Process every Nth frame (secondary mode) |
| `min-object-width` | int | 0 | Minimum ROI width to process (secondary) |
| `min-object-height` | int | 0 | Minimum ROI height to process (secondary) |
| `target-class-id` | int | -1 | Only process objects of this class (secondary) |
| `config-file-path` | string | "" | JSON config file path |

**Pads:** sink (video/x-raw) → src (video/x-raw)

### DxInfer

Runs NPU inference on preprocessed frames using a .dxnn model.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `preprocess-id` | int | 0 | Must match upstream DxPreprocess |
| `inference-id` | int | 0 | Links this inference to DxPostprocess |
| `model-path` | string | "" | Absolute path to .dxnn model file |
| `batch-size` | int | 1 | Batch size for inference |
| `secondary-mode` | bool | false | Run on cropped object ROIs |
| `config-file-path` | string | "" | JSON config file path |

**Pads:** sink (video/x-raw) → src (video/x-raw with DXTensorMeta)

### DxPostprocess

Decodes raw inference tensors into structured object metadata using a custom .so library.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `inference-id` | int | 0 | Must match upstream DxInfer |
| `library-file-path` | string | "" | Absolute path to postprocess .so |
| `function-name` | string | "PostProcess" | Entry function name in the .so |
| `secondary-mode` | bool | false | Postprocess per-object results |
| `config-file-path` | string | "" | JSON config file path |

**Pads:** sink (video/x-raw) → src (video/x-raw with DXObjectMeta)

### DxTracker

Multi-object tracker (OC-SORT) that assigns persistent IDs to detected objects.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | Tracker config JSON path |

**Pads:** sink (video/x-raw) → src (video/x-raw)

**Placement:** Must follow DxPostprocess, never DxInfer directly.

### DxOsd

On-screen display: draws bounding boxes, labels, confidence scores, keypoints on frames.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | OSD config JSON path |

**Pads:** sink (video/x-raw) → src (video/x-raw)

### DxGather

N-to-1 multiplexer that merges multiple secondary inference branches back into one stream.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `num-sources` | int | 2 | Number of input sink pads |

**Pads:** sink_0, sink_1, ..., sink_N-1 → src

**Usage:** Used in secondary mode pipelines with `tee` element.

### DxInputSelector

N-to-1 round-robin selector for time-division multiplexed inference on multiple streams.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| (auto-configured) | | | Sink pads created on demand |

**Pads:** sink_0, sink_1, ..., sink_N-1 → src

### DxOutputSelector

1-to-N demultiplexer that routes processed frames back to their original stream.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| (auto-configured) | | | Source pads created on demand |

**Pads:** sink → src_0, src_1, ..., src_N-1

### DxRate

Frame rate limiter. Drops frames to maintain a target rate.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `max-rate` | int | 30 | Maximum frames per second |

**Pads:** sink (video/x-raw) → src (video/x-raw)

**Usage:** Required before DxPreprocess when source is RTSP or other variable-rate input.

### DxMsgConv

Converts DXObjectMeta attached to buffers into JSON message format.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `config-file-path` | string | "" | Message format config JSON |

**Pads:** sink (video/x-raw) → src (video/x-raw)

### DxMsgBroker

Publishes JSON messages from DxMsgConv to an external message broker.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `broker-name` | string | "" | "kafka" or "mqtt" |
| `conn-info` | string | "" | Broker connection string (host:port) |
| `topic` | string | "" | Topic/channel to publish to |
| `config` | string | "" | Broker-specific config file path |

**Pads:** sink (video/x-raw) → (terminal, no src pad)

### DxScale

Video scaling element.

**Key Properties:**
| Property | Type | Default | Description |
|---|---|---|---|
| `width` | int | 0 | Target width |
| `height` | int | 0 | Target height |

**Pads:** sink (video/x-raw) → src (video/x-raw)

### DxConvert

Color space conversion element.

**Pads:** sink (video/x-raw) → src (video/x-raw)

---

## 6 Pipeline Patterns

### Pattern 1: Single Network

```bash
gst-launch-1.0 urisourcebin uri=file:///path/to/video.mp4 ! decodebin ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! \
    queue max-size-buffers=1 ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=/abs/path/to/model.dxnn ! \
    queue max-size-buffers=1 ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    dxosd ! videoconvert ! fpsdisplaysink sync=false
```

### Pattern 2: Multi-Stream (Compositor)

```bash
gst-launch-1.0 -e \
    urisourcebin uri=file:///video1.mp4 ! decodebin ! \
        dxpreprocess config-file-path=preprocess_config.json ! queue ! \
        dxinfer config-file-path=inference_config.json ! queue ! \
        dxpostprocess config-file-path=postprocess_config.json ! queue ! \
        dxosd ! queue ! dxscale width=640 height=360 ! queue ! comp.sink_0 \
    urisourcebin uri=file:///video2.mp4 ! decodebin ! \
        dxpreprocess config-file-path=preprocess_config.json ! queue ! \
        dxinfer config-file-path=inference_config.json ! queue ! \
        dxpostprocess config-file-path=postprocess_config.json ! queue ! \
        dxosd ! queue ! dxscale width=640 height=360 ! queue ! comp.sink_1 \
    compositor name=comp sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 ! \
    videoconvert ! fpsdisplaysink sync=false
```

### Pattern 3: Tracking

```bash
gst-launch-1.0 urisourcebin uri=file:///video.mp4 ! decodebin ! \
    dxpreprocess config-file-path=preprocess_config.json ! queue ! \
    dxinfer config-file-path=inference_config.json ! queue ! \
    dxpostprocess config-file-path=postprocess_config.json ! queue ! \
    dxtracker config-file-path=tracker_config.json ! queue ! \
    dxosd ! queue ! videoconvert ! fpsdisplaysink sync=false
```

### Pattern 4: Secondary Mode

```bash
gst-launch-1.0 urisourcebin uri=file:///video.mp4 ! decodebin ! \
    dxpreprocess config-file-path=primary_preprocess.json ! queue ! \
    dxinfer config-file-path=primary_infer.json ! queue ! \
    dxpostprocess config-file-path=primary_postprocess.json ! queue ! \
    dxtracker config-file-path=tracker_config.json ! queue ! \
    tee name=t \
    t. ! queue ! \
        dxpreprocess preprocess-id=2 resize-width=224 resize-height=224 \
            secondary-mode=true interval=5 min-object-width=50 min-object-height=50 ! \
        queue max-size-buffers=1 ! \
        dxinfer preprocess-id=2 inference-id=2 secondary-mode=true \
            model-path=/path/to/classifier.dxnn ! \
        queue max-size-buffers=1 ! \
        dxpostprocess inference-id=2 secondary-mode=true \
            library-file-path=/path/to/libpostprocess_class.so function-name=PostProcess ! \
        queue max-size-buffers=1 ! \
        gather.sink_0 \
    t. ! queue ! gather.sink_1 \
    dxgather name=gather ! queue ! \
    dxosd ! videoconvert ! fpsdisplaysink sync=false
```

### Pattern 5: RTSP (Input/Output Selector)

```bash
gst-launch-1.0 -e \
    urisourcebin uri=rtsp://cam1 ! queue ! decodebin ! queue ! in.sink_0 \
    urisourcebin uri=rtsp://cam2 ! queue ! decodebin ! queue ! in.sink_1 \
    dxinputselector name=in ! \
        dxpreprocess config-file-path=preprocess.json ! queue ! \
        dxinfer config-file-path=infer.json ! queue ! \
        dxpostprocess config-file-path=postprocess.json ! queue ! \
        dxosd ! \
    dxoutputselector name=out \
    out.src_0 ! queue ! dxscale width=640 height=360 ! queue ! comp.sink_0 \
    out.src_1 ! queue ! dxscale width=640 height=360 ! queue ! comp.sink_1 \
    compositor name=comp sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 ! \
    videoconvert ! fpsdisplaysink sync=false
```

### Pattern 6: Broker (Kafka/MQTT)

```bash
# Kafka
gst-launch-1.0 -e urisourcebin uri=file:///video.mp4 ! decodebin ! \
    dxpreprocess config-file-path=preprocess.json ! queue ! \
    dxinfer config-file-path=infer.json ! queue ! \
    dxpostprocess config-file-path=postprocess.json ! queue ! \
    dxmsgconv config-file-path=msgconv_config.json ! queue ! \
    dxmsgbroker broker-name=kafka conn-info=localhost:9092 topic=detections config=broker_kafka.cfg

# MQTT
gst-launch-1.0 -e urisourcebin uri=file:///video.mp4 ! decodebin ! \
    dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=detections
```

---

## Critical Rules

### Rule 1: preprocess-id / inference-id Matching

DxPreprocess `preprocess-id` MUST equal the downstream DxInfer `preprocess-id`.
DxInfer `inference-id` MUST equal the downstream DxPostprocess `inference-id`.
Mismatches cause silent data routing failures.

### Rule 2: Queue Placement

A `queue` element MUST be placed between every pair of dx_stream elements.
Without queues, GStreamer cannot schedule elements on separate threads,
leading to pipeline deadlocks.

### Rule 3: DxRate for Variable-Rate Sources

RTSP and network sources produce frames at variable rates. Without DxRate,
buffers accumulate in queues causing memory exhaustion and latency spikes.

### Rule 4: DxMsgBroker Must Follow DxMsgConv

DxMsgBroker expects JSON-formatted messages produced by DxMsgConv.
Connecting DxMsgBroker directly to DxPostprocess produces undefined behavior.

### Rule 5: DxTracker Must Follow DxPostprocess

DxTracker operates on DXObjectMeta produced by DxPostprocess.
Placing DxTracker after DxInfer (before postprocess) has no object metadata to track.

### Rule 6: Secondary Mode ROI Processing

In secondary mode, DxPreprocess crops detected object ROIs from the frame.
Set `secondary-mode=true` on DxPreprocess, DxInfer, and DxPostprocess in the
secondary branch. Use `interval` to skip frames and `min-object-width`/`min-object-height`
to filter small objects.

## Custom Postprocess .so Integration

To use a custom postprocess library:

1. Set DxPostprocess `library-file-path` to the absolute .so path
2. Set `function-name` to the exported C function name (typically `PostProcess`)
3. Ensure the .so is built and installed before running the pipeline

```bash
# Verify library exists
ls -la /usr/local/share/gstdxstream/lib/libpostprocess_<name>.so

# Inspect with nm
nm -D /usr/local/share/gstdxstream/lib/libpostprocess_<name>.so | grep PostProcess
```
