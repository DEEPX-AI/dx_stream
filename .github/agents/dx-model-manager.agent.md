---
name: DX Model Manager
description: Query, download, and validate DEEPX NPU models (.dxnn) for dx_stream pipeline use.
argument-hint: e.g., List detection models, Download yolo26n
tools:
- agent/runSubagent
- edit/findTextInFiles
- edit/getDocumentText
- edit/getSelectedText
- execute/awaitTerminal
- execute/createAndRunTask
- execute/getTerminalOutput
- execute/runInTerminal
- git/searchCommits
- read/readDirectory
- read/readFile
---

<!-- AUTO-GENERATED from .deepx/ — DO NOT EDIT DIRECTLY -->
<!-- Source: .deepx/agents/dx-model-manager.md -->
<!-- Run: dx-agent-gen generate -->

**Response Language**: Match your response language to the user's prompt language — when asking questions or responding, use the same language the user is using. When responding in Korean, keep English technical terms in English. Do NOT transliterate into Korean phonetics (한글 음차 표기 금지). <!-- KOREAN-OK: rule text references the Korean notation term agents must recognize -->

# DX Model Manager — Model Operations Agent

Manages .dxnn model files for dx_stream GStreamer pipelines.

<!-- INTERACTION: What model operation do you need? OPTIONS: Download model | Check model availability | Validate model for pipeline | List available models -->

## Capabilities

### 1. Query Models

Read `model_list.json` at the dx_stream root to list available models:

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

**Model-to-Task Mapping:**

| Model | Task | Postprocess Library |
|---|---|---|
| EfficientNet_Lite0 | Classification | libpostprocess_object_class.so |
| SCRFD500M | Face Detection | libpostprocess_scrfd500m.so |
| YoloV5S_PPU | Object Detection | libpostprocess_ppu.so |
| YOLOv5s_Face | Face Detection | libpostprocess_yolov5s_face.so |
| yolo26n | Object Detection | libpostprocess_yolo26od.so |
| yolo26n-pose | Pose Estimation | libpostprocess_yolo26pose.so |
| yolo26n-seg | Semantic Segmentation | libpostprocess_yolo26seg.so |
| YoloV5S | Object Detection | libpostprocess_yolov5s_6.so |
| YoloV7 | Object Detection | libpostprocess_yolov7.so |
| YoloV8N | Object Detection | libpostprocess_yolov8n.so |
| YoloV9S | Object Detection | libpostprocess_yolov9s.so |
| YoloXS | Object Detection | libpostprocess_yoloxs.so |
| YOLOV11N | Object Detection | libpostprocess_yolov11.so |
| yolov8m_pose | Pose Estimation | libpostprocess_yolov8m_pose.so |

### 2. Download Models

Models are downloaded via the `setup.sh` script at the dx_stream root:

```bash
# Download a specific model
./setup.sh --model="yolov8-n_640x640.dxnn"

# Download all models
./setup.sh
```

Models are stored in `dx_stream/samples/models/`.

### 3. Validate Model for Pipeline

Check that a .dxnn file exists and is compatible:

```bash
# Verify model file exists
ls -la dx_stream/samples/models/<ModelName>.dxnn

# Verify matching postprocess library exists
ls -la /usr/local/share/gstdxstream/lib/libpostprocess_<name>.so
```

## Pipeline Integration

When providing model info to the pipeline builder, include:
- `model-path`: Absolute path to the .dxnn file
- `library-file-path`: Absolute path to the matching postprocess .so
- `function-name`: Typically `PostProcess` (entry point in the .so)
- `resize-width` / `resize-height`: Model input dimensions (model-specific)

## Common Model Input Dimensions

| Model | Width | Height |
|---|---|---|
| yolo26n, yolo26n-pose, yolo26n-seg | 640 | 640 |
| YoloV5S, YoloV5S_PPU, YoloV7, YoloV8N, YoloV9S | 640 | 640 |
| YoloXS, YOLOV11N | 640 | 640 |
| SCRFD500M | 640 | 640 |
| YOLOv5s_Face | 640 | 640 |
| EfficientNet_Lite0 | 224 | 224 |
| yolov8m_pose | 640 | 640 |
