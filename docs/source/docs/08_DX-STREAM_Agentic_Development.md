# DX-STREAM Agentic Development Guide

## Overview

This guide describes how to use AI-powered agentic development to build GStreamer pipeline applications with **dx_stream** and DEEPX NPU accelerators. Agents handle pipeline construction, model management, and validation so you can go from a natural-language request to a working pipeline script in minutes.

---

## Agents

Four agents collaborate to build, configure, and validate dx_stream pipelines.

| Agent | Description | Routes To |
|---|---|---|
| `dx-stream-builder` | Master router — classifies the pipeline type from the user request and dispatches to the appropriate specialist agent | `dx-pipeline-builder`, `dx-model-manager` |
| `dx-pipeline-builder` | Builds GStreamer pipeline apps across 6 categories (single-model, multi-model, cascaded, tiled, parallel, broker) | — |
| `dx-model-manager` | Downloads and configures `.dxnn` models for use in pipelines | — |
| `dx-validator` | Validates generated pipeline scripts and `.deepx/` framework integrity | — |

### Routing Flow

```
User Request
    └─▶ dx-stream-builder (classify & route)
            ├─▶ dx-pipeline-builder (generate pipeline)
            ├─▶ dx-model-manager   (resolve models)
            └─▶ dx-validator       (validate output)
```

---

## Skills

| Skill | Description |
|---|---|
| `dx-build-pipeline-app` | Build a GStreamer pipeline across 6 categories: single-model, multi-model, cascaded, tiled, parallel, broker |
| `dx-build-mqtt-kafka-app` | Build an MQTT or Kafka message broker pipeline for event publishing |
| `dx-model-management` | Download and configure `.dxnn` models for target NPU architecture |
| `dx-validate` | Run pipeline validation checks (syntax, properties, element order) |
| `dx-validate-and-fix` | *(shared, from dx-runtime)* Full feedback loop — validate, diagnose, fix, re-validate |
| `dx-brainstorm-and-plan` | Brainstorm and plan before any pipeline generation (process skill) |
| `dx-tdd` | Test-driven development — validate each file immediately after creation (process skill) |
| `dx-verify-completion` | Verify before claiming completion — evidence before assertions (process skill) |

---

## Supported AI Tools

dx_stream agentic development works with four AI coding tools. Each auto-loads
the knowledge base through its own configuration.

| Tool | Config Files | Agents Available |
|---|---|---|
| **Claude Code** | `CLAUDE.md` | All 4 agents via context routing |
| **GitHub Copilot** | `.github/copilot-instructions.md`, 4 agents in `.github/agents/`, 2 instructions in `.github/instructions/` | `@dx-stream-builder`, `@dx-pipeline-builder`, `@dx-model-manager`, `@dx-validator` |
| **Cursor** | `.cursor/rules/dx-stream.mdc` (always), 2 glob rules (`stream-pipelines`, `tests`) | Free-form with auto-applied rules |
| **OpenCode** | `AGENTS.md`, `opencode.json`, 4 agents in `.opencode/agents/`, 4 skills in `.opencode/skills/` | `@dx-stream-builder` or `/dx-build-pipeline-app` |

### Copilot File-Specific Instructions

| Glob Pattern | Injected Instruction | Content |
|---|---|---|
| `**/pipeline/**`, `**/pipelines/**`, `**/*pipeline*.py` | `stream-pipelines.instructions.md` | preprocess-id matching, queue placement, DxRate for RTSP, element ordering |
| `test/**` | `tests.instructions.md` | pytest patterns, pipeline fixtures, mock elements |

### OpenCode Skills (Slash Commands)

| Slash Command | Description |
|---|---|
| `/dx-build-pipeline-app` | Build a GStreamer pipeline across 6 categories |
| `/dx-build-mqtt-kafka-app` | Build an MQTT/Kafka broker pipeline |
| `/dx-model-management` | Download and configure .dxnn models |
| `/dx-validate` | Run pipeline validation checks |
| `/dx-brainstorm-and-plan` | Brainstorm and plan before pipeline generation |
| `/dx-tdd` | Test-driven development with incremental validation |
| `/dx-verify-completion` | Verify completion with evidence before assertions |

---

## User Scenarios

### Scenario 1: Build a Detection Pipeline with Tracking

**Prompt:**

```
"Build an object detection pipeline with yolo26n and tracking on RTSP camera"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. `CLAUDE.md` routes to `dx-build-pipeline-app` skill. Asks about RTSP URL, display preferences, and tracker type, then generates the pipeline with DxRate → DxPreprocess → DxInfer → DxTracker → DxOsd chain. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + tracking", hands off to `dx-pipeline-builder`, runs `dx-validator` checks. |
| **Cursor** | Type the prompt directly. `dx-stream.mdc` (always loaded) provides the 13-element catalog. `stream-pipelines.mdc` activates for pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

### Scenario 2: Build an MQTT Broker Pipeline

**Prompt:**

```
"Build a pipeline that detects people and publishes events to MQTT"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-build-mqtt-kafka-app` skill. Generates a pipeline ending with `DxPostprocess ! DxMsgConv ! DxMsgBroker`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-mqtt-kafka-app` skill directly. |

### Scenario 3: Multi-Model Cascaded Pipeline

**Prompt:**

```
"Build a cascaded pipeline: first detect people, then classify their actions"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Generates cascaded pattern: `DxInfer (primary) → DxRoiExtract → DxScale → DxInfer (secondary)`. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

### Scenario 4: Validate a Pipeline

**Prompt:**

```
"Validate the pipeline I just created"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | `@dx-validator` followed by the prompt. |
| **GitHub Copilot** | `@dx-validator` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-validator` followed by the prompt, or run manually: `python .deepx/scripts/validate_app.py <script.sh>` |

### Scenario 5: Build a Pose Estimation Pipeline

**Prompt:**

```
"Build a pose estimation pipeline with yolo26n-pose on USB camera"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-build-pipeline-app` with pose estimation model. Generates pipeline with keypoint overlay via `DxOsd`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + pose", hands off to `dx-pipeline-builder`. |
| **Cursor** | Type the prompt directly. `stream-pipelines.mdc` activates for generated pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

### Scenario 6: Build a Tiled High-Resolution Pipeline

**Prompt:**

```
"Build a tiled detection pipeline for 4K input with yolo26n"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-build-pipeline-app` with tiled category. Generates `DxTile → DxInfer → DxDeTile` pattern for high-resolution input. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

### Scenario 7: Build a Multi-Stream Parallel Pipeline

**Prompt:**

```
"Build a parallel pipeline processing 4 RTSP cameras with yolo26n"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-build-pipeline-app` with parallel category. Generates `DxMux` to merge 4 sources, shared `DxInfer` for efficient NPU utilization. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

### Scenario 8: Build a Segmentation Pipeline

**Prompt:**

```
"Build a segmentation pipeline with yolo26n-seg for road scene analysis"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-build-pipeline-app` with segmentation model. Generates pipeline with per-pixel mask overlay via `DxOsd`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + segmentation". |
| **Cursor** | Type the prompt directly. `stream-pipelines.mdc` activates for generated pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-build-pipeline-app` skill directly. |

---

## Quick Start

Invoke the master router with a natural-language description:

```
@dx-stream-builder "object detection pipeline with yolo26n and tracking on RTSP camera"
```

The agent will:

1. **Ask clarifying questions** — pipeline category, source type, model variant, output sink
2. **Present a pipeline plan** — element chain, properties, model path
3. **Route to `dx-pipeline-builder`** — generate the pipeline script
4. **Resolve the model** — download or locate the `.dxnn` file via `dx-model-manager`
5. **Validate and report** — run `dx-validator` and return results

---

## What Gets Created

By default, AI-generated pipeline code is placed in the `dx-agentic-dev/` isolation
directory to prevent conflicts with existing scripts.

### Default Output (dx-agentic-dev/)

```
dx-agentic-dev/<session_id>/
├── README.md              # Session metadata and run instructions
├── session.json           # Machine-readable session config
└── {pipeline_name}.py     # Generated pipeline script
```

Session ID format: `YYYYMMDD-HHMMSS_model_task` (e.g., `20260403-150045_yolo26n_tracking`).

### Production Output

When you explicitly request production placement, files are written to the standard
pipeline directory.

---

## Pipeline Categories

dx_stream supports six pipeline categories. Each follows a distinct GStreamer element pattern.

| Category | Pattern | Key Elements |
|---|---|---|
| **Single-model** | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` | Core inference trio + on-screen display |
| **Multi-model** | Chain multiple `DxInfer` stages, each with a distinct `preprocess-id` | Multiple inference passes in sequence |
| **Cascaded** | Primary `DxInfer` → `DxRoiExtract` → `DxScale` → Secondary `DxInfer` | ROI extraction feeds a second model |
| **Tiled** | `DxTile ! DxInfer ! DxDeTile` | Split high-res frames into tiles for inference |
| **Parallel** | `DxMux` to merge multiple source streams into one pipeline | Multi-stream ingest and processing |
| **Broker** | `DxPostprocess ! DxMsgConv ! DxMsgBroker` | Serialize detections and publish to MQTT/Kafka |

---

## GStreamer Elements Reference

dx_stream provides 13 custom GStreamer elements for NPU-accelerated pipelines.

| Element | Purpose |
|---|---|
| `DxPreprocess` | Resize, normalize, and color-convert frames for model input |
| `DxInfer` | Run a `.dxnn` model on the DEEPX NPU |
| `DxPostprocess` | Decode raw tensors into structured detection/classification results |
| `DxTracker` | Multi-object tracking (assign persistent IDs across frames) |
| `DxOsd` | Draw bounding boxes, labels, and overlays on frames |
| `DxGather` | N-to-1 merge — collect buffers from multiple branches |
| `DxInputSelector` | N-to-1 round-robin input selection |
| `DxOutputSelector` | 1-to-N demux — route buffers to one of N output pads |
| `DxRate` | Limit frame rate (essential for RTSP sources) |
| `DxMsgConv` | Serialize detection metadata to JSON |
| `DxMsgBroker` | Publish serialized messages to MQTT or Kafka |
| `DxScale` | Resize frames to a target resolution |
| `DxConvert` | Color space conversion between formats |

---

## Pipeline-Specific Validation Rules

Agents enforce these rules when generating and validating pipelines:

1. **`preprocess-id` matching** — `DxPreprocess` and `DxInfer` must share the same `preprocess-id` value. A mismatch causes silent inference failures.
2. **Queue placement** — Insert a `queue` element between every processing stage to prevent pipeline deadlocks.
3. **`DxRate` for RTSP** — Always insert `DxRate` immediately after RTSP sources to prevent frame flooding and buffer overruns.
4. **`DxMsgConv` before `DxMsgBroker`** — Always serialize metadata with `DxMsgConv` before publishing with `DxMsgBroker`.
5. **Absolute model paths** — The `model-path` property on `DxInfer` must be an absolute filesystem path. Relative paths cause runtime errors.

---

## Validation Commands

```bash
# Validate a pipeline script (static checks)
python .deepx/scripts/validate_app.py <script.sh>

# Validate with smoke test (requires NPU hardware)
python .deepx/scripts/validate_app.py <script.sh> --smoke-test

# Check .deepx/ framework integrity
python .deepx/scripts/validate_framework.py
```

Validation checks include:
- Element property completeness
- `preprocess-id` consistency across `DxPreprocess` / `DxInfer` pairs
- Queue placement between processing stages
- Model file existence at the specified `model-path`
- Broker element ordering (`DxMsgConv` before `DxMsgBroker`)

---

## Session Sentinels

Agents output fixed markers at the start and end of each task for automated testing:

| Marker | When |
|---|---|
| `[DX-AGENTIC-DEV: START]` | First line of the agent's response |
| `[DX-AGENTIC-DEV: DONE (output-dir: <relative_path>)]` | Last line after all work is complete. `<relative_path>` is the session output directory relative to the project root. If no files were generated, omit the `(output-dir: ...)` part. |

Sub-agents invoked via handoff do not output sentinels — only the top-level agent does.

Rules:
1. **CRITICAL** — Output `[DX-AGENTIC-DEV: START]` as the absolute first line of the first response, before any other text. This is non-negotiable even if the user says to proceed autonomously.
2. Output `[DX-AGENTIC-DEV: DONE (output-dir: <path>)]` as the very last line after all work is complete.
3. Sub-agents invoked via handoff do not output sentinels.
4. If the user sends multiple prompts in a session, output START/DONE for each prompt.
5. The `output-dir` in DONE must be the relative path from the project root to the session output directory.
6. **Never output DONE after only producing planning artifacts** (specs, plans, design documents). DONE means all deliverables are produced — implementation code, scripts, configs, and validation results.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Inference produces no detections | `preprocess-id` mismatch between `DxPreprocess` and `DxInfer` | Ensure both elements share the same `preprocess-id` value |
| Pipeline deadlocks or stalls | Missing `queue` elements between processing stages | Add a `queue` element between every pair of processing elements |
| RTSP stream drops frames | No frame rate limiting after source | Insert `DxRate` immediately after the RTSP source element |
| `model-path` not found at runtime | Relative path used for `model-path` property | Use an absolute path (e.g., `/opt/deepx/models/yolo26n.dxnn`) |
| `DxInfer` plugin not registered | Plugin not installed or `GST_PLUGIN_PATH` not set | Run `gst-inspect-1.0 dxinfer` to verify; check `GST_PLUGIN_PATH` |
| Broker pipeline sends empty messages | `DxMsgConv` missing before `DxMsgBroker` | Add `DxMsgConv` to serialize metadata before the broker element |
