# dx_stream v2.3.0 Architecture

## Overview

dx_stream is a GStreamer-based video analytics framework for DEEPX NPU accelerators.
It provides 13 custom GStreamer elements that compose into pipelines for real-time
inference on video streams. All NPU inference is handled transparently through the
DxInfer element, while pre/post-processing, tracking, display, and message brokering
are handled by dedicated elements.

## GStreamer Plugin Architecture

All 13 elements are registered as a single GStreamer plugin (`gstdxstream`):

```
gst-dxstream-plugin/
  src/
    gst-dxstream.cpp          # Plugin registration entry point
    gst-dxpreprocess.cpp       # DxPreprocess element
    gst-dxinfer.cpp            # DxInfer element (NPU inference)
    gst-dxpostprocess.cpp      # DxPostprocess element
    gst-dxtracker.cpp          # DxTracker element (OC-SORT)
    gst-dxosd.cpp              # DxOsd element (on-screen display)
    gst-dxgather.cpp           # DxGather element (N:1 mux)
    gst-dxinputselector.cpp    # DxInputSelector (N:1 round-robin)
    gst-dxoutputselector.cpp   # DxOutputSelector (1:N demux)
    gst-dxrate.cpp             # DxRate element (frame rate control)
    gst-dxmsgconv.cpp          # DxMsgConv (metadata to JSON)
    gst-dxmsgbroker.cpp        # DxMsgBroker (publish to broker)
    gst-dxscale.cpp            # DxScale element (video scaling)
    gst-dxconvert.cpp          # DxConvert element (color conversion)
```

### Element Categories

| Category | Elements | Purpose |
|---|---|---|
| **Inference** | DxPreprocess, DxInfer, DxPostprocess | Core inference pipeline |
| **Analysis** | DxTracker | Multi-object tracking (OC-SORT) |
| **Display** | DxOsd | Bounding box, label, keypoint overlay |
| **Routing** | DxGather, DxInputSelector, DxOutputSelector | Multi-stream muxing/demuxing |
| **Rate Control** | DxRate | Frame rate limiting for variable-rate sources |
| **Messaging** | DxMsgConv, DxMsgBroker | Inference result publishing |
| **Transform** | DxScale, DxConvert | Video scaling and color conversion |

## Pipeline Categories

dx_stream provides 6 pipeline patterns:

### 1. Single Network (`pipelines/single_network/`)
One model, one input stream. Sub-categories by vision task:
- `object_detection/` — YOLO family detectors
- `face_detection/` — SCRFD500M, YOLOv5s_Face
- `pose_estimation/` — yolo26n-pose, yolov8m_pose
- `semantic_segmentation/` — yolo26n-seg

### 2. Multi-Stream (`pipelines/multi_stream/`)
Multiple input sources processed independently, displayed in a compositor grid.
Uses DxScale to resize each stream before compositing.

### 3. Tracking (`pipelines/tracking/`)
Detection + DxTracker for persistent object IDs across frames.
DxTracker must follow DxPostprocess in the pipeline.

### 4. Secondary Mode (`pipelines/secondary_mode/`)
Cascade inference: primary detector finds objects, then secondary models
run on cropped ROIs. Uses `tee` to split, DxGather to merge results.
DxPreprocess in secondary branches uses `secondary-mode=true`.

### 5. RTSP (`pipelines/rtsp/`)
Network camera input with DxInputSelector/DxOutputSelector for
time-division multiplexed inference across multiple streams.
DxRate recommended for variable-rate RTSP sources.

### 6. Broker (`pipelines/broker/`)
Inference results published to Kafka or MQTT via DxMsgConv + DxMsgBroker chain.

## Custom Postprocess Libraries

17 C++ shared libraries in `dx_stream/custom_library/postprocess_library/`:

```
CLIP/                    → libpostprocess_clip.so
Object_Classification/   → libpostprocess_object_class.so
PPU/                     → libpostprocess_ppu.so
SCRFD500M/               → libpostprocess_scrfd500m.so
yolo26_cls/              → libpostprocess_yolo26cls.so
yolo26_obb/              → libpostprocess_yolo26obb.so
yolo26_od/               → libpostprocess_yolo26od.so
yolo26_pose/             → libpostprocess_yolo26pose.so
yolo26_seg/              → libpostprocess_yolo26seg.so
YOLOV11/                 → libpostprocess_yolov11.so
YoloV5S/                 → libpostprocess_yolov5s_6.so
YOLOv5s_Face/            → libpostprocess_yolov5s_face.so
YoloV7/                  → libpostprocess_yolov7.so
yolov8m_pose/            → libpostprocess_yolov8m_pose.so
YoloV8N/                 → libpostprocess_yolov8n.so
YoloV9S/                 → libpostprocess_yolov9s.so
YoloXS/                  → libpostprocess_yoloxs.so
```

Each library:
- Built with Meson (`meson.build`)
- Exports a `PostProcess` function as the entry point
- Installed to `/usr/local/share/gstdxstream/lib/`
- Referenced by DxPostprocess via `library-file-path` and `function-name`

## Python Bindings (pydxs)

The `pydxs` package (`bindings/python/pydxs/`) provides Python access to
GStreamer buffer metadata:

- **DXFrameMeta** — Per-frame metadata attached to GstBuffer
- **DXObjectMeta** — Per-detected-object metadata (bbox, class, confidence)
- **DXTensorMeta** — Raw tensor output from DxInfer (for custom processing)

Usage: Add a GStreamer pad probe on a pipeline element's src pad, then
extract metadata from the buffer using pydxs bindings.

## Message Broker Integration

Two broker backends:
- **Kafka** (`brokers/dx_msgbrokerl_kafka.cpp`) — Connects via librdkafka
- **MQTT** (`brokers/dx_msgbrokerl_mqtt.cpp`) — Connects via Paho MQTT

Pipeline pattern: `... ! dxpostprocess ! dxmsgconv ! dxmsgbroker broker-name=kafka|mqtt`

DxMsgConv converts DXObjectMeta to JSON. DxMsgBroker publishes to the configured topic.

## Configuration System

Pipeline elements can be configured via:
1. **Inline properties** — Set directly on gst-launch-1.0 command line
2. **JSON config files** — Referenced via `config-file-path` property

Config file locations: `dx_stream/configs/<ModelName>/`
- `preprocess_config.json` — DxPreprocess settings
- `inference_config.json` — DxInfer settings (model-path, batch-size)
- `postprocess_config.json` — DxPostprocess settings (library, function)
- `tracker_config.json` — DxTracker settings (at configs root)
- `msgconv_config.json` — DxMsgConv settings (at configs root)
- `broker_kafka.cfg` / `broker_mqtt.cfg` — Broker connection configs

## Directory Structure

```
dx_stream/
  model_list.json              # 14 supported models
  setup.sh                     # Model/media download
  build.sh                     # Build all libraries
  install.sh                   # Install GStreamer plugin
  dx_stream/
    pipelines/                 # 6 categories of pipeline scripts
    custom_library/            # Postprocess + message convert .so
    configs/                   # JSON config files
    apps/                      # Broker consumer examples (kafka, mqtt)
  gst-dxstream-plugin/         # GStreamer element C++ source
  bindings/python/pydxs/       # Python metadata bindings
  test/                        # Test scripts
  .deepx/                      # Agent-Driven knowledge base (this directory)
```
