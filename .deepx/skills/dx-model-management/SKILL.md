---
name: dx-model-management
description: Model management for dx_stream pipelines
---

# Skill: dx_stream Model Management

> **This skill document is sufficient.** Read this FIRST before exploring source code.

## Overview

Manage .dxnn model files for dx_stream GStreamer pipelines. Query the model registry,
download models, and validate compatibility with pipeline elements.

## Usage

Invoke with `/dx-model-management` or ask the dx-model-manager agent directly.

```bash
# Check if a model is available
# Download a specific model
# Validate a .dxnn file for pipeline use
```

## model_list.json Schema

Located at the dx_stream root:

```json
{
    "version": "2_3_0",
    "models": [
        "EfficientNet_Lite0.dxnn",
        "SCRFD500M.dxnn",
        "YoloV5S_PPU.dxnn",
        "YOLOv5s_Face.dxnn",
        "yolo26n.dxnn",
        "yolo26n-pose.dxnn",
        "yolo26n-seg.dxnn",
        "YoloV5S.dxnn",
        "YoloV7.dxnn",
        "YoloV8N.dxnn",
        "YoloV9S.dxnn",
        "YoloXS.dxnn",
        "YOLOV11N.dxnn",
        "yolov8m_pose.dxnn"
    ]
}
```

**Version:** Matches dx_stream release version (2.3.0 → "2_3_0")
**Models:** Array of .dxnn filenames (14 models total)

## 14 Supported Models

| # | Model Name | Task | Input Size | Postprocess Library |
|---|---|---|---|---|
| 1 | EfficientNet_Lite0 | Classification | 224x224 | libpostprocess_object_class.so |
| 2 | SCRFD500M | Face Detection | 640x640 | libpostprocess_scrfd500m.so |
| 3 | YoloV5S_PPU | Object Detection | 640x640 | libpostprocess_ppu.so |
| 4 | YOLOv5s_Face | Face Detection | 640x640 | libpostprocess_yolov5s_face.so |
| 5 | yolo26n | Object Detection | 640x640 | libpostprocess_yolo26od.so |
| 6 | yolo26n-pose | Pose Estimation | 640x640 | libpostprocess_yolo26pose.so |
| 7 | yolo26n-seg | Segmentation | 640x640 | libpostprocess_yolo26seg.so |
| 8 | YoloV5S | Object Detection | 640x640 | libpostprocess_yolov5s_6.so |
| 9 | YoloV7 | Object Detection | 640x640 | libpostprocess_yolov7.so |
| 10 | YoloV8N | Object Detection | 640x640 | libpostprocess_yolov8n.so |
| 11 | YoloV9S | Object Detection | 640x640 | libpostprocess_yolov9s.so |
| 12 | YoloXS | Object Detection | 640x640 | libpostprocess_yoloxs.so |
| 13 | YOLOV11N | Object Detection | 640x640 | libpostprocess_yolov11.so |
| 14 | yolov8m_pose | Pose Estimation | 640x640 | libpostprocess_yolov8m_pose.so |

## Query Patterns

### By Task

```python
import json

with open('model_list.json') as f:
    data = json.load(f)

# Detection models
detection_models = [
    'YoloV5S_PPU', 'yolo26n', 'YoloV5S', 'YoloV7',
    'YoloV8N', 'YoloV9S', 'YoloXS', 'YOLOV11N'
]

# Face detection models
face_models = ['SCRFD500M', 'YOLOv5s_Face']

# Pose estimation models
pose_models = ['yolo26n-pose', 'yolov8m_pose']

# Segmentation models
seg_models = ['yolo26n-seg']

# Classification models
class_models = ['EfficientNet_Lite0']
```

### Check Model Availability

```bash
# Check if model is in registry
python3 -c "
import json, sys
with open('model_list.json') as f:
    models = json.load(f)['models']
name = sys.argv[1]
match = [m for m in models if name in m]
print(f'Found: {match}' if match else f'Not found: {name}')
" "YoloV8N"
```

## Download Models

```bash
# Download specific model
./setup.sh --model="YoloV8N.dxnn"

# Download all models
./setup.sh

# Models are stored at:
# dx_stream/samples/models/<ModelName>.dxnn
```

## Validate .dxnn for Pipeline Use

```bash
# 1. Check model file exists
ls -la dx_stream/samples/models/YoloV8N.dxnn

# 2. Check matching postprocess library
ls -la /usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so

# 3. Verify PostProcess function is exported
nm -D /usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so | grep PostProcess

# 4. Test with a minimal pipeline
gst-launch-1.0 videotestsrc num-buffers=5 ! video/x-raw,width=640,height=640 ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 \
        model-path=$(pwd)/dx_stream/samples/models/YoloV8N.dxnn ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolov8n.so \
        function-name=PostProcess ! queue ! \
    fakesink
```
