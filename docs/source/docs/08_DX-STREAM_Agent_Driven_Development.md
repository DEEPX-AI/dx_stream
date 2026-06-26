# DX-STREAM Agent-Driven Development Guide

## Overview

This guide describes how to use DEEPX agent-driven development (dx-agent-dev) to build GStreamer pipeline applications with **dx_stream** and DEEPX NPU accelerators. Agents handle pipeline construction, model management, and validation so you can go from a natural-language request to a working pipeline script in minutes.

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

#### General SWE Process

| Skill | Description |
|-------|-------------|
| `dx-swe-brainstorm` | Brainstorm and plan before any pipeline generation |
| `dx-swe-tdd` | Test-driven development — validate each file immediately after creation |
| `dx-swe-verify` | Verify before claiming completion — evidence before assertions |
| `dx-swe-writing-plans` | Write implementation plans from specs or requirements |
| `dx-swe-executing-plans` | Execute a written implementation plan with review checkpoints |
| `dx-swe-debugging` | Systematic debugging — diagnose before proposing fixes |
| `dx-swe-parallel-agents` | Dispatch 2+ independent tasks to parallel agents |
| `dx-swe-subagent-dev` | Execute implementation plans with independent sub-agent tasks |
| `dx-swe-receiving-review` | Receive and process code review feedback with technical rigor |
| `dx-swe-requesting-review` | Request code review to verify work meets requirements |
| `dx-skill-router` | Route to the appropriate skill based on task classification |

#### DEEPX Build

| Skill | Description |
|-------|-------------|
| `dx-agent-stream-build-pipeline` | Build a GStreamer pipeline across 6 categories: single-model, multi-model, cascaded, tiled, parallel, broker |
| `dx-agent-stream-build-mqtt-kafka` | Build an MQTT or Kafka message broker pipeline for event publishing |
| `dx-agent-stream-model-management` | Download and configure `.dxnn` models for target NPU architecture |
| `dx-agent-stream-validate` | Run pipeline validation checks (syntax, properties, element order) |

---

## Supported AI Tools

dx_stream agent-driven development works with four AI coding tools. Each auto-loads
the knowledge base through its own configuration.

| Tool | Config Files | Agents Available |
|---|---|---|
| **Claude Code** | `CLAUDE.md` | All 4 agents via context routing |
| **GitHub Copilot** | `.github/copilot-instructions.md`, 4 agents in `.github/agents/`, 16 skills in `.github/skills/`, 2 instructions in `.github/instructions/` | `@dx-stream-builder`, `@dx-pipeline-builder`, `@dx-model-manager`, `@dx-validator` |
| **Cursor** | `.cursor/rules/dx-stream.mdc` (always), `dx-model-manager.mdc`, `dx-pipeline-builder.mdc`, `dx-stream-builder.mdc`, `dx-validator.mdc`, `stream-pipelines.mdc`, `tests.mdc`, 16 `skill-*.mdc` files (23 total) | Free-form with auto-applied rules |
| **OpenCode** | `AGENTS.md`, `opencode.json`, 4 agents in `.opencode/agents/`, 16 skills in `.deepx/skills/` | `@dx-stream-builder` or `/dx-agent-stream-build-pipeline` |

### Copilot File-Specific Instructions

| Glob Pattern | Injected Instruction | Content |
|---|---|---|
| `**/pipeline/**`, `**/pipelines/**`, `**/*pipeline*.py` | `stream-pipelines.instructions.md` | preprocess-id matching, queue placement, DxRate for RTSP, element ordering |
| `test/**` | `tests.instructions.md` | pytest patterns, pipeline fixtures, mock elements |

### OpenCode Skills (Slash Commands)

**General SWE Process**

| Slash Command | Description |
|---|---|
| `/dx-swe-brainstorm` | Brainstorm and plan before pipeline generation |
| `/dx-swe-tdd` | Test-driven development with incremental validation |
| `/dx-swe-verify` | Verify completion with evidence before assertions |
| `/dx-swe-writing-plans` | Write implementation plans from specs |
| `/dx-swe-executing-plans` | Execute a written implementation plan |
| `/dx-swe-debugging` | Systematic debugging before proposing fixes |
| `/dx-swe-parallel-agents` | Dispatch 2+ independent tasks to parallel agents |
| `/dx-swe-subagent-dev` | Execute plans with independent sub-agent tasks |
| `/dx-swe-receiving-review` | Process code review feedback |
| `/dx-swe-requesting-review` | Request code review |
| `/dx-skill-router` | Route to the appropriate skill |

**DEEPX Build**

| Slash Command | Description |
|---|---|
| `/dx-agent-stream-build-pipeline` | Build a GStreamer pipeline across 6 categories |
| `/dx-agent-stream-build-mqtt-kafka` | Build an MQTT/Kafka broker pipeline |
| `/dx-agent-stream-model-management` | Download and configure .dxnn models |
| `/dx-agent-stream-validate` | Run pipeline validation checks |

### Platform File Loading Reference

Each AI coding agent auto-loads different configuration files at the dx_stream level.

#### Auto-Loaded Files

| File | Auto-loaded by | Loading |
|------|----------------|---------|
| `.github/copilot-instructions.md` | Copilot Chat/CLI | Auto |
| `CLAUDE.md` | Claude Code | Auto |
| `AGENTS.md` + `opencode.json` | OpenCode | Auto |
| `AGENTS.md` + `.codex/skills/dx-codex-identity/SKILL.md` | Codex CLI | Auto |
| `.cursor/rules/dx-stream.mdc` | Cursor | Auto |

#### Agent Files (Manual @mention)

| Agent | Copilot (`@mention`) | OpenCode (`@mention`) | Claude Code (`@mention`) |
|-------|------|---------|---------|
| `dx-stream-builder` | `.github/agents/dx-stream-builder.agent.md` | `.opencode/agents/dx-stream-builder.md` | `.claude/agents/dx-stream-builder.md` |
| `dx-pipeline-builder` | `.github/agents/dx-pipeline-builder.agent.md` | `.opencode/agents/dx-pipeline-builder.md` | `.claude/agents/dx-pipeline-builder.md` |
| `dx-model-manager` | `.github/agents/dx-model-manager.agent.md` | `.opencode/agents/dx-model-manager.md` | `.claude/agents/dx-model-manager.md` |
| `dx-validator` | `.github/agents/dx-validator.agent.md` | `.opencode/agents/dx-validator.md` | `.claude/agents/dx-validator.md` |

#### Skill Files (All Platforms — `/slash-command`)

| Skill | File |
|-------|------|
| `/dx-swe-brainstorm` | `.deepx/skills/dx-swe-brainstorm/SKILL.md` |
| `/dx-agent-stream-build-mqtt-kafka` | `.deepx/skills/dx-agent-stream-build-mqtt-kafka/SKILL.md` |
| `/dx-agent-stream-build-pipeline` | `.deepx/skills/dx-agent-stream-build-pipeline/SKILL.md` |
| `/dx-swe-parallel-agents` | `.deepx/skills/dx-swe-parallel-agents/SKILL.md` |
| `/dx-swe-executing-plans` | `.deepx/skills/dx-swe-executing-plans/SKILL.md` |
| `/dx-agent-stream-model-management` | `.deepx/skills/dx-agent-stream-model-management/SKILL.md` |
| `/dx-swe-receiving-review` | `.deepx/skills/dx-swe-receiving-review/SKILL.md` |
| `/dx-swe-requesting-review` | `.deepx/skills/dx-swe-requesting-review/SKILL.md` |
| `/dx-skill-router` | `.deepx/skills/dx-skill-router/SKILL.md` |
| `/dx-swe-subagent-dev` | `.deepx/skills/dx-swe-subagent-dev/SKILL.md` |
| `/dx-swe-debugging` | `.deepx/skills/dx-swe-debugging/SKILL.md` |
| `/dx-swe-tdd` | `.deepx/skills/dx-swe-tdd/SKILL.md` |
| `/dx-agent-stream-validate` | `.deepx/skills/dx-validate/SKILL.md` |
| `/dx-swe-verify` | `.deepx/skills/dx-swe-verify/SKILL.md` |
| `/dx-swe-writing-plans` | `.deepx/skills/dx-swe-writing-plans/SKILL.md` |

#### Shared Knowledge Base (`.deepx/`)

The `.deepx/` directory is the canonical source for all agent-driven development content.
Platform-specific files (`.claude/`, `.github/`, `.cursor/`, `.opencode/`) are generated
from `.deepx/` by `dx-agent-gen generate --repo dx-runtime/dx_stream`. Never edit
generated files directly — update `.deepx/` and regenerate.

| Directory | Files | Description |
|-----------|-------|-------------|
| `.deepx/agents/` | 4 files (`dx-stream-builder.md`, `dx-pipeline-builder.md`, `dx-model-manager.md`, `dx-validator.md`) | Authoritative agent definitions |
| `.deepx/skills/` | 16 directories (one per skill, each containing `SKILL.md`) | Detailed skill workflows |
| `.deepx/toolsets/` | 4 files | GStreamer elements and API references |
| `.deepx/instructions/` | 6 files | Coding standards and workflow rules |
| `.deepx/memory/` | 4 files | Persistent knowledge — pitfalls and session memory |
| `.deepx/templates/` | Template files | Code generation templates |
| `.deepx/knowledge/` | Knowledge files | Domain-specific knowledge bases |
| `.deepx/contextual-rules/` | Rule files | Context-dependent rules for agents |
| `.deepx/prompts/` | Prompt files | Reusable prompt fragments |
| `.deepx/scripts/` | Script files | Validation and utility scripts |

#### Claude Code Agents (`.claude/agents/`)

| Agent | File |
|-------|------|
| `dx-model-manager` | `.claude/agents/dx-model-manager.md` |
| `dx-pipeline-builder` | `.claude/agents/dx-pipeline-builder.md` |
| `dx-stream-builder` | `.claude/agents/dx-stream-builder.md` |
| `dx-validator` | `.claude/agents/dx-validator.md` |

#### GitHub Copilot Skills (`.github/skills/`)

16 skill directories (inline copies generated by `dx-agent-gen`), one per skill matching `.deepx/skills/`.

---

## Generation Pipeline

All platform-specific configuration files are generated from the canonical `.deepx/`
source by the `dx-agent-gen` tool:

```bash
dx-agent-gen generate --repo dx-runtime/dx_stream
```

This generates:
- `.claude/agents/` — Claude Code agent files
- `.github/agents/`, `.github/skills/`, `.github/instructions/` — GitHub Copilot files
- `.cursor/rules/` — Cursor rule files (23 total)
- `.opencode/agents/` — OpenCode agent files
- `CLAUDE.md`, `AGENTS.md`, `opencode.json` — Platform entry points

**Never edit generated files directly.** Update `.deepx/` and regenerate.

---

## User Scenarios

### Scenario 1: Build a Detection Pipeline with Tracking

**Prompt:**

```
"Build an object detection pipeline with yolo26n and tracking on RTSP camera"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. `CLAUDE.md` routes to `dx-agent-stream-build-pipeline` skill. Asks about RTSP URL, display preferences, and tracker type, then generates the pipeline with DxRate → DxPreprocess → DxInfer → DxTracker → DxOsd chain. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + tracking", hands off to `dx-pipeline-builder`, runs `dx-validator` checks. |
| **Cursor** | Type the prompt directly. `dx-stream.mdc` (always loaded) provides the 13-element catalog. `stream-pipelines.mdc` activates for pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

### Scenario 2: Build an MQTT Broker Pipeline

**Prompt:**

```
"Build a pipeline that detects people and publishes events to MQTT"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-agent-stream-build-mqtt-kafka` skill. Generates a pipeline ending with `DxPostprocess ! DxMsgConv ! DxMsgBroker`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-mqtt-kafka` skill directly. |

### Scenario 3: Multi-Model Cascaded Pipeline

**Prompt:**

```
"Build a cascaded pipeline: first detect people, then classify their actions"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Generates cascaded pattern: `DxInfer (primary) → DxPostprocess → DxTracker → tee → DxPreprocess(secondary-mode=true) → DxInfer(secondary-mode=true) → DxGather`. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

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
| **Claude Code** | Type the prompt directly. Routes to `dx-agent-stream-build-pipeline` with pose estimation model. Generates pipeline with keypoint overlay via `DxOsd`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + pose", hands off to `dx-pipeline-builder`. |
| **Cursor** | Type the prompt directly. `stream-pipelines.mdc` activates for generated pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

### Scenario 6: Build a Tiled High-Resolution Pipeline

**Prompt:**

```
"Build a tiled detection pipeline for 4K input with yolo26n"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-agent-stream-build-pipeline` with tiled category. Generates `DxTile → DxInfer → DxDeTile` pattern for high-resolution input. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

### Scenario 7: Build a Multi-Stream Parallel Pipeline

**Prompt:**

```
"Build a parallel pipeline processing 4 RTSP cameras with yolo26n"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-agent-stream-build-pipeline` with parallel category. Generates `DxInputSelector`+`DxOutputSelector` for shared inference, or independent sub-pipelines per stream. |
| **GitHub Copilot** | `@dx-pipeline-builder` followed by the prompt. |
| **Cursor** | Type the prompt directly. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

### Scenario 8: Build a Segmentation Pipeline

**Prompt:**

```
"Build a segmentation pipeline with yolo26n-seg for road scene analysis"
```

| Tool | How to Use |
|---|---|
| **Claude Code** | Type the prompt directly. Routes to `dx-agent-stream-build-pipeline` with segmentation model. Generates pipeline with per-pixel mask overlay via `DxOsd`. |
| **GitHub Copilot** | `@dx-stream-builder` followed by the prompt. Classifies as "single-model + segmentation". |
| **Cursor** | Type the prompt directly. `stream-pipelines.mdc` activates for generated pipeline files. |
| **OpenCode** | `@dx-stream-builder` followed by the prompt, or `/dx-agent-stream-build-pipeline` skill directly. |

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

By default, AI-generated pipeline code is placed in the `dx-agent-dev/` isolation
directory to prevent conflicts with existing scripts.

### Default Output (dx-agent-dev/)

```
dx-agent-dev/<session_id>/
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
| `[DX-AGENT-DEV: START]` | First line of the agent's response |
| `[DX-AGENT-DEV: DONE (output-dir: <relative_path>)]` | Last line after all work is complete. `<relative_path>` is the session output directory relative to the project root. If no files were generated, omit the `(output-dir: ...)` part. |

Sub-agents invoked via handoff do not output sentinels — only the top-level agent does.

Rules:
1. **CRITICAL** — Output `[DX-AGENT-DEV: START]` as the absolute first line of the first response, before any other text. This is non-negotiable even if the user says to proceed autonomously.
2. Output `[DX-AGENT-DEV: DONE (output-dir: <path>)]` as the very last line after all work is complete.
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
