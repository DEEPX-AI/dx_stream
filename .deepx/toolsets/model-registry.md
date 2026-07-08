# Model Registry Reference

> **SDK Source of Truth**: `config/model_registry.json`

## Overview

The `model_list.json` file at the dx_stream root is the authoritative registry
of supported .dxnn models for dx_stream v2.3.0.

## Schema

```json
{
    "version": "string",
    "models": ["string"]
}
```

- `version`: Release version using underscores (e.g., "2_4_0" for v2.3.0)
- `models`: Array of .dxnn filenames

## Current Registry (v2.3.0, 14 models)

```json
{
    "version": "2_4_0",
    "models": [
        "efficientnet-lite0_256x256.dxnn",
        "scrfd-500m_640x640.dxnn",
        "yolov5-s_640x640_ppu.dxnn",
        "yolov5-s-face_640x640.dxnn",
        "yolo26-n_640x640.dxnn",
        "yolo26-n-pose_640x640.dxnn",
        "yolo26-n-seg_640x640.dxnn",
        "yolov5-s_640x640.dxnn",
        "yolov7_640x640.dxnn",
        "yolov8-n_640x640.dxnn",
        "yolov9-s_640x640.dxnn",
        "yolox-s_640x640.dxnn",
        "yolo11-n_640x640.dxnn",
        "yolov8-m-pose_640x640.dxnn"
    ]
}
```

## Model Details

### Object Detection Models (8)

| Model | Input Size | Postprocess Library | Notes |
|---|---|---|---|
| YoloV5S_PPU | 640x640 | libpostprocess_ppu.so | Pre/post-processing unit variant |
| yolo26n | 640x640 | libpostprocess_yolo26od.so | YOLO v26 nano |
| YoloV5S | 640x640 | libpostprocess_yolov5s_6.so | YOLOv5 small |
| YoloV7 | 640x640 | libpostprocess_yolov7.so | YOLOv7 |
| YoloV8N | 640x640 | libpostprocess_yolov8n.so | YOLOv8 nano |
| YoloV9S | 640x640 | libpostprocess_yolov9s.so | YOLOv9 small |
| YoloXS | 640x640 | libpostprocess_yoloxs.so | YOLOX small |
| YOLOV11N | 640x640 | libpostprocess_yolov11.so | YOLOv11 nano |

### Face Detection Models (2)

| Model | Input Size | Postprocess Library | Notes |
|---|---|---|---|
| SCRFD500M | 640x640 | libpostprocess_scrfd500m.so | Sample Consistent Ranking Face Detector |
| YOLOv5s_Face | 640x640 | libpostprocess_yolov5s_face.so | YOLOv5 face variant |

### Pose Estimation Models (2)

| Model | Input Size | Postprocess Library | Notes |
|---|---|---|---|
| yolo26n-pose | 640x640 | libpostprocess_yolo26pose.so | 17 COCO keypoints |
| yolov8m_pose | 640x640 | libpostprocess_yolov8m_pose.so | YOLOv8 medium pose |

### Segmentation Models (1)

| Model | Input Size | Postprocess Library | Notes |
|---|---|---|---|
| yolo26n-seg | 640x640 | libpostprocess_yolo26seg.so | Instance segmentation |

### Classification Models (1)

| Model | Input Size | Postprocess Library | Notes |
|---|---|---|---|
| EfficientNet_Lite0 | 224x224 | libpostprocess_object_class.so | 1000-class ImageNet |

## Query Patterns

### Bash: Check if Model Exists

```bash
MODEL_NAME="yolov8-n_640x640.dxnn"
if python3 -c "import json; data=json.load(open('model_list.json')); exit(0 if '$MODEL_NAME' in data['models'] else 1)"; then
    echo "Model $MODEL_NAME is supported"
else
    echo "Model $MODEL_NAME is NOT in registry"
fi
```

### Python: List Models by Task

```python
import json

TASK_MAPPING = {
    'detection': ['YoloV5S_PPU', 'yolo26n', 'YoloV5S', 'YoloV7',
                  'YoloV8N', 'YoloV9S', 'YoloXS', 'YOLOV11N'],
    'face_detection': ['SCRFD500M', 'YOLOv5s_Face'],
    'pose_estimation': ['yolo26n-pose', 'yolov8m_pose'],
    'segmentation': ['yolo26n-seg'],
    'classification': ['EfficientNet_Lite0'],
}

with open('model_list.json') as f:
    registry = json.load(f)

def models_for_task(task):
    """Return .dxnn filenames for a given task."""
    base_names = TASK_MAPPING.get(task, [])
    return [m for m in registry['models']
            if any(m.startswith(b) for b in base_names)]
```

### Python: Get Postprocess Library for Model

```python
POSTPROCESS_MAP = {
    'EfficientNet_Lite0': 'libpostprocess_object_class.so',
    'SCRFD500M': 'libpostprocess_scrfd500m.so',
    'YoloV5S_PPU': 'libpostprocess_ppu.so',
    'YOLOv5s_Face': 'libpostprocess_yolov5s_face.so',
    'yolo26n': 'libpostprocess_yolo26od.so',
    'yolo26n-pose': 'libpostprocess_yolo26pose.so',
    'yolo26n-seg': 'libpostprocess_yolo26seg.so',
    'YoloV5S': 'libpostprocess_yolov5s_6.so',
    'YoloV7': 'libpostprocess_yolov7.so',
    'YoloV8N': 'libpostprocess_yolov8n.so',
    'YoloV9S': 'libpostprocess_yolov9s.so',
    'YoloXS': 'libpostprocess_yoloxs.so',
    'YOLOV11N': 'libpostprocess_yolov11.so',
    'yolov8m_pose': 'libpostprocess_yolov8m_pose.so',
}

def get_postprocess_lib(model_name):
    """Return the postprocess .so filename for a model."""
    base = model_name.replace('.dxnn', '')
    return POSTPROCESS_MAP.get(base, None)
```

## Download URLs

Models are downloaded via `setup.sh` which reads from the DEEPX model server.
The download URL pattern is internal to `setup.sh`.

```bash
# Download specific model
./setup.sh --model="yolov8-n_640x640.dxnn"

# Download all models
./setup.sh

# Storage location
ls dx_stream/samples/models/
```

## Compatibility

- All models in the registry are compiled for the current DX-RT version
- Version mismatch between model_list.json and installed DX-RT may cause
  inference failures
- Check `dpkg -l | grep dx-runtime` for installed DX-RT version
