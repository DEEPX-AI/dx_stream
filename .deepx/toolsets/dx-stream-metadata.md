# dx_stream Metadata Reference (pydxs)

## Overview

The `pydxs` Python package provides bindings for accessing GStreamer buffer metadata
produced by dx_stream elements. Located at `bindings/python/pydxs/`.

## Metadata Types

### DXFrameMeta

Per-frame metadata attached to every GstBuffer flowing through the pipeline.

**Fields:**

| Field | Type | Description |
|---|---|---|
| `frame_id` | int | Sequential frame counter |
| `timestamp` | int | Frame timestamp in nanoseconds |
| `width` | int | Frame width in pixels |
| `height` | int | Frame height in pixels |
| `objects` | list[DXObjectMeta] | List of detected objects in this frame |

**Python Access:**
```python
import pydxs

def probe_callback(pad, info):
    buffer = info.get_buffer()
    frame_meta = pydxs.DXFrameMeta.from_buffer(buffer)
    if frame_meta:
        print(f"Frame {frame_meta.frame_id}: "
              f"{frame_meta.width}x{frame_meta.height}, "
              f"{len(frame_meta.objects)} objects")
    return Gst.PadProbeReturn.OK
```

### DXObjectMeta

Per-detected-object metadata. One DXObjectMeta per detected object in the frame.

**Fields:**

| Field | Type | Description |
|---|---|---|
| `class_id` | int | Object class index |
| `label` | str | Human-readable class name |
| `confidence` | float | Detection confidence score (0.0-1.0) |
| `x` | float | Bounding box top-left X coordinate |
| `y` | float | Bounding box top-left Y coordinate |
| `w` | float | Bounding box width |
| `h` | float | Bounding box height |
| `track_id` | int | Tracker-assigned persistent ID (-1 if no tracker) |
| `keypoints` | list | Pose keypoints (empty if not pose model) |

**Python Access:**
```python
for obj in frame_meta.objects:
    print(f"  class={obj.class_id} ({obj.label}) "
          f"conf={obj.confidence:.2f} "
          f"bbox=({obj.x:.0f},{obj.y:.0f},{obj.w:.0f},{obj.h:.0f}) "
          f"track_id={obj.track_id}")

    if obj.keypoints:
        for kp in obj.keypoints:
            print(f"    keypoint: ({kp.x:.0f}, {kp.y:.0f}) conf={kp.confidence:.2f}")
```

### DXTensorMeta

Raw inference tensor output, available after DxInfer and before DxPostprocess.
Used for custom postprocessing in Python.

**Fields:**

| Field | Type | Description |
|---|---|---|
| `inference_id` | int | Matching inference-id from DxInfer |
| `num_outputs` | int | Number of output tensors |
| `output_shapes` | list[tuple] | Shape of each output tensor |
| `output_data` | list[numpy.ndarray] | Raw tensor data as numpy arrays |

**Python Access:**
```python
import numpy as np

def tensor_probe(pad, info):
    buffer = info.get_buffer()
    tensor_meta = pydxs.DXTensorMeta.from_buffer(buffer)
    if tensor_meta:
        for i in range(tensor_meta.num_outputs):
            shape = tensor_meta.output_shapes[i]
            data = tensor_meta.output_data[i]
            print(f"  Output {i}: shape={shape}, dtype={data.dtype}")
    return Gst.PadProbeReturn.OK
```

## Probe Callback Patterns

### Adding a Probe to a Pipeline Element

```python
import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib
import pydxs

Gst.init(None)

pipeline_str = (
    'urisourcebin uri=file:///path/video.mp4 ! decodebin ! '
    'dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! '
    'dxinfer preprocess-id=1 inference-id=1 model-path=/path/model.dxnn ! queue ! '
    'dxpostprocess name=postprocess inference-id=1 '
    '    library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so '
    '    function-name=PostProcess ! queue ! '
    'dxosd ! videoconvert ! fpsdisplaysink sync=false'
)

pipeline = Gst.parse_launch(pipeline_str)


def on_detection(pad, info):
    """Probe after DxPostprocess to access detection results."""
    buffer = info.get_buffer()
    frame_meta = pydxs.DXFrameMeta.from_buffer(buffer)

    if frame_meta and frame_meta.objects:
        for obj in frame_meta.objects:
            if obj.confidence > 0.7:
                print(f"High-confidence detection: "
                      f"class={obj.label} conf={obj.confidence:.2f}")

    return Gst.PadProbeReturn.OK


# Attach probe to DxPostprocess src pad
postprocess = pipeline.get_by_name('postprocess')
src_pad = postprocess.get_static_pad('src')
src_pad.add_probe(Gst.PadProbeType.BUFFER, on_detection)

pipeline.set_state(Gst.State.PLAYING)
loop = GLib.MainLoop()

try:
    loop.run()
except KeyboardInterrupt:
    pass
finally:
    pipeline.set_state(Gst.State.NULL)
```

### Counting Detections Per Class

```python
from collections import defaultdict

class_counts = defaultdict(int)

def counting_probe(pad, info):
    buffer = info.get_buffer()
    frame_meta = pydxs.DXFrameMeta.from_buffer(buffer)
    if frame_meta:
        for obj in frame_meta.objects:
            class_counts[obj.label] += 1
    return Gst.PadProbeReturn.OK
```

### Filtering Objects by Region

```python
def region_filter_probe(pad, info):
    """Only print objects in a specific region of interest."""
    ROI_X, ROI_Y, ROI_W, ROI_H = 100, 100, 400, 300

    buffer = info.get_buffer()
    frame_meta = pydxs.DXFrameMeta.from_buffer(buffer)
    if frame_meta:
        for obj in frame_meta.objects:
            cx = obj.x + obj.w / 2
            cy = obj.y + obj.h / 2
            if (ROI_X <= cx <= ROI_X + ROI_W and
                ROI_Y <= cy <= ROI_Y + ROI_H):
                print(f"Object in ROI: {obj.label} at ({obj.x:.0f},{obj.y:.0f})")
    return Gst.PadProbeReturn.OK
```

## Installation

```bash
cd bindings/python/pydxs
pip install -e .
```

## Compatibility

- Requires GStreamer 1.14+
- Python 3.8+
- pydxs is built from C++ bindings via pybind11
- Must have dx_stream GStreamer plugin installed
