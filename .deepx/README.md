# .deepx/ — dx_stream Agentic Knowledge Base

> Self-contained knowledge base for AI-assisted development of dx_stream,
> a GStreamer-based video analytics framework for DEEPX NPU accelerators.

## Quick Start

```bash
# Validate a pipeline script
python3 .deepx/scripts/validate_app.py dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh

# Validate .deepx/ integrity
python3 .deepx/scripts/validate_framework.py

# Generate platform configs (.github/, .claude/, .cursor/, .vscode/)
python3 .deepx/scripts/generate_platforms.py --dry-run
```

## Context Routing Table

Based on what the task involves, read **only** the matching rows:

| If the task mentions... | Read these files |
|---|---|
| **Pipeline, detection, classification** | `skills/dx-agentic-stream-build-pipeline.md`, `toolsets/dx-stream-elements.md` |
| **MQTT, Kafka, message broker** | `skills/dx-agentic-stream-build-mqtt-kafka.md`, `toolsets/dx-stream-elements.md` |
| **Multi-model, cascaded, tiled** | `skills/dx-agentic-stream-build-pipeline.md`, `toolsets/dx-stream-metadata.md` |
| **Model, download, registry** | `skills/dx-agentic-stream-model-management.md`, `toolsets/model-registry.md` |
| **Validation, testing** | `skills/dx-agentic-stream-validate.md`, `instructions/testing-patterns.md` |
| **Architecture, structure** | `instructions/architecture.md` |
| **Coding rules, conventions** | `instructions/coding-standards.md` |
| **GStreamer elements, properties** | `toolsets/dx-stream-elements.md` |
| **Metadata, pydxs, DXObjectMeta** | `toolsets/dx-stream-metadata.md` |
| **Orchestration, lifecycle** | `instructions/orchestration.md` |
| **Agent protocols, routing** | `instructions/agent-protocols.md` |
| **Brainstorm, plan, design** | `skills/dx-brainstorm-and-plan.md` |
| **TDD, validation, incremental** | `skills/dx-tdd.md` |
| **Completion, verify, evidence** | `skills/dx-verify-completion.md` |
| **ALWAYS read (every task)** | `memory/common_pitfalls.md`, `instructions/coding-standards.md` |

## Skills

| Command | Description |
|---------|-------------|
| `/dx-agentic-stream-build-pipeline` | Build GStreamer pipeline app (6 categories: single-model, multi-model, cascaded, tiled, parallel, broker) |
| `/dx-agentic-stream-build-mqtt-kafka` | Build MQTT/Kafka message broker pipeline app |
| `/dx-agentic-stream-model-management` | Download, register, and configure .dxnn models |
| `/dx-agentic-stream-validate` | Run pipeline validation checks |
| `/dx-brainstorm-and-plan` | Brainstorm and plan before any code generation (process skill) |
| `/dx-tdd` | Test-driven development — validate each file immediately after creation (process skill) |
| `/dx-verify-completion` | Verify before claiming completion — evidence before assertions, mandatory artifacts enforcement (process skill) |

## Directory Structure

```
.deepx/
  README.md                              # This file — master index
  agents/
    dx-stream-builder.md                 # Master router agent
    dx-pipeline-builder.md               # Pipeline construction specialist
    dx-model-manager.md                  # Model operations agent
    dx-validator.md                      # Pipeline and framework validation agent
  instructions/
    architecture.md                      # dx_stream v2.3.0 architecture overview
    coding-standards.md                  # Pipeline composition rules & conventions
    gstreamer-pipeline.md                # 13 elements + 6 pipeline patterns
    testing-patterns.md                  # GStreamer debug & test patterns
    agent-protocols.md                   # 11 inter-agent protocols
    orchestration.md                     # 5-phase pipeline lifecycle
  skills/
    dx-agentic-stream-build-pipeline.md             # Full pipeline build skill
    dx-agentic-stream-build-mqtt-kafka.md           # Broker integration skill
    dx-agentic-stream-model-management.md               # Model management skill
    dx-agentic-stream-validate.md                       # Pipeline validation skill
    dx-brainstorm-and-plan.md            # Process skill
    dx-tdd.md                            # Process skill
    dx-verify-completion.md              # Process skill
  toolsets/
    dx-stream-elements.md                # Complete 13-element property reference
    dx-stream-metadata.md                # pydxs metadata API reference
    dx-engine-api.md                     # DX-RT inference engine reference
    model-registry.md                    # model_list.json reference (14 models)
  memory/
    MEMORY.md                            # Memory index
    common_pitfalls.md                   # 10 known pitfalls [DX_STREAM] + [UNIVERSAL]
    pipeline_optimization.md             # Performance optimization patterns
    platform_api.md                      # DX-RT platform API reference
  knowledge/
    knowledge_base.yaml                  # Best practices, instructions, recipes (YAML)
  prompts/
    new-stream-pipeline.md               # Prompt template: new pipeline app
    new-mqtt-kafka-app.md                # Prompt template: new broker app
    orchestrated-build.md                # Prompt template: orchestrated multi-step build
  contextual-rules/
    stream-pipelines.md                  # Rules for pipeline development context
    postprocess.md                       # Rules for postprocess library context
    tests.md                             # Rules for test development context
  scripts/
    validate_app.py                      # Pipeline app validator (11 checks)
    validate_framework.py                # .deepx/ integrity checker (8 categories)
    generate_platforms.py                # Platform sync (.github/, .claude/, .cursor/, .vscode/)
  templates/
    copilot-instructions.md              # GitHub Copilot instructions template
```

**Total: 37 files across 11 categories**

## 13 GStreamer Elements

| Element | Category | Purpose |
|---------|----------|---------|
| `DxPreprocess` | Inference | Resize, normalize, color-convert frames for NPU |
| `DxInfer` | Inference | Run .dxnn model inference on NPU |
| `DxPostprocess` | Inference | Decode tensors into detections/classifications |
| `DxTracker` | Analysis | Multi-object tracking (OC-SORT, persistent IDs) |
| `DxOsd` | Display | Draw bounding boxes, labels, keypoints |
| `DxGather` | Routing | N-to-1 merge for secondary mode branches |
| `DxInputSelector` | Routing | N-to-1 round-robin input (multi-stream) |
| `DxOutputSelector` | Routing | 1-to-N output demux (multi-stream) |
| `DxRate` | Rate Control | Frame rate limiting (essential for RTSP) |
| `DxMsgConv` | Messaging | Serialize DXObjectMeta to JSON |
| `DxMsgBroker` | Messaging | Publish JSON to Kafka or MQTT |
| `DxScale` | Transform | Resize video frames |
| `DxConvert` | Transform | Color space conversion |

## 6 Pipeline Categories

| Category | Pattern | Key Elements |
|----------|---------|-------------|
| **Single-model** | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` | Core 3 + OSD |
| **Multi-model** | Chain DxInfer stages with distinct preprocess-ids | Multiple inference |
| **Cascaded** | `DxPostprocess ! tee ! DxPreprocess(secondary) ! DxInfer ! DxGather` | Secondary mode |
| **Tiled** | `DxTile ! DxInfer ! DxDeTile` | High-res tiling |
| **Parallel** | `DxInputSelector ! shared DxInfer ! DxOutputSelector` | Multi-stream |
| **Broker** | `DxPostprocess ! DxMsgConv ! DxMsgBroker` | MQTT/Kafka publish |

## Critical Conventions

1. **Preprocess-ID matching**: DxPreprocess and DxInfer must share the same `preprocess-id`
2. **Queue elements**: Place `queue` between every dx_stream element pair
3. **DxRate for RTSP**: Always insert DxRate after RTSP/variable-rate sources
4. **Absolute paths**: `model-path` and `library-file-path` must be absolute
5. **DxMsgConv before DxMsgBroker**: Always serialize before publishing
6. **DxTracker after DxPostprocess**: Tracker needs DXObjectMeta, not raw tensors
7. **Model auto-download**: Shell scripts must include `setup.sh` download logic
8. **Headless check**: Test `$DISPLAY` before using video sinks

## Supported Models (14)

| Task | Models |
|------|--------|
| Object Detection | YoloV5S, YoloV5S_PPU, yolo26n, YoloV7, YoloV8N, YoloV9S, YoloXS, YOLOV11N |
| Face Detection | SCRFD500M, YOLOv5s_Face |
| Pose Estimation | yolo26n-pose, yolov8m_pose |
| Segmentation | yolo26n-seg |
| Classification | EfficientNet_Lite0 |

## Hardware

| Architecture | Value |
|---|---|
| DX-M1 | `dx_m1` |

## Validation

```bash
# Validate a pipeline script (static + property checks)
python3 .deepx/scripts/validate_app.py <script.sh>

# Validate with smoke test (requires NPU)
python3 .deepx/scripts/validate_app.py <script.sh> --smoke-test

# Validate .deepx/ integrity
python3 .deepx/scripts/validate_framework.py

# Generate platform configs
python3 .deepx/scripts/generate_platforms.py --platforms github claude cursor vscode
```

## Memory

Persistent knowledge in `memory/`. Read at task start, update when learning new
patterns or discovering pitfalls. See `memory/MEMORY.md` for the memory index.
