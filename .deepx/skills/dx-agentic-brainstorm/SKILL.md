---
name: dx-agentic-brainstorm
description: Brainstorm and plan before implementation
---

# Skill: Brainstorm and Plan for dx_stream

> **RIGID skill — HARD-GATE: No code generation without an approved plan.**
> Do not write pipeline.py, run_*.sh, or any config file until the plan is approved.

## Purpose

Explore requirements, validate feasibility, and produce a concrete build plan before
generating any dx_stream GStreamer pipeline application.

## When to Use

Invoke this skill BEFORE any pipeline build task. This includes `/dx-agentic-stream-build-pipeline`
and `/dx-agentic-stream-build-mqtt-kafka`. Skip only if the user explicitly says "just build it".

---

## Phase 1: Context Check

Run these checks silently before asking the user anything:

```bash
# 1. Query available models
python3 -c "import json; print(json.dumps(json.load(open('model_list.json'))['models'], indent=2))"

# 2. List existing pipelines for reuse/reference
ls pipelines/ 2>/dev/null || echo "No pipelines/ directory"

# 3. Verify GStreamer plugin registration
gst-inspect-1.0 dxinfer > /dev/null 2>&1 && echo "OK: dxinfer registered" || echo "WARN: dxinfer not found"
```

If the user's requested model is not in `model_list.json`, stop and report it.
If a similar pipeline already exists in `pipelines/`, ask: (a) explain existing, or (b) create new.

## Phase 2: Key Decisions (ask the user)

Ask 2-3 targeted questions covering:

| Decision | Options | Default |
|----------|---------|---------|
| **Pipeline category** | single-model, multi-model, cascaded, tiled, parallel, broker | single-model |
| **Model(s)** | Any from `model_list.json` (14 supported) | YoloV8N |
| **Input source** | video file, USB camera, RTSP stream | video file |
| **Output** | display, headless (fakesink), file, broker (MQTT/Kafka) | display |
| **Tracking** | yes / no | no |
| **Tiling** | yes / no (for high-res input) | no |
| **Multi-stream** | yes / no (DxInputSelector+DxOutputSelector for shared inference, or independent sub-pipelines) | no |

Skip decisions that are already clear from the user's request.

## Phase 3: Build Plan

Produce a structured plan using this template:

```
## Build Plan: <app_name>

**Category:** <pipeline_category>
**Model:** <model_name> (<task>, <input_size>)
**Input:** <source_type>
**Output:** <sink_type>

### Files to Create

dx-agentic-dev/<session_dir>/
├── session.json          # Session metadata
├── README.md             # Summary, quick start
├── pipeline.py           # Python pipeline application
├── run_<app>.sh          # Shell script with GStreamer pipeline
├── setup.sh              # Environment setup (model download, plugin check)
├── run.sh                # One-command launcher
├── session.log           # Actual command output (appended during build)
└── config/               # (if needed)
    ├── tracker_config.json      # (if tracking)
    ├── broker_kafka.cfg         # (if broker)
    └── msgconv_config.json      # (if broker)

### Pipeline Pattern

<source> ! queue ! DxPreprocess preprocess-id=1 ! queue !
DxInfer preprocess-id=1 model-path=<abs_path> ! queue !
DxPostprocess ! queue ! [DxTracker !] DxOsd ! <sink>

### Pre-Flight Checklist

- [ ] GStreamer plugin registered: `gst-inspect-1.0 dxinfer`
- [ ] Model available: `samples/models/<model>.dxnn`
- [ ] Postprocess .so exists: `/usr/local/share/gstdxstream/lib/<lib>.so`
- [ ] Input source accessible (file exists / USB connected / RTSP reachable)
```

Present the plan and wait for user approval. Do NOT proceed to code generation
until the user confirms or says "looks good" / "go ahead" / equivalent.

## Phase 4: Route to Build Skill

After approval, invoke the appropriate build skill:

| Pipeline type | Route to |
|---------------|----------|
| single-model, multi-model, cascaded, tiled, parallel | `/dx-agentic-stream-build-pipeline` |
| broker (MQTT/Kafka) | `/dx-agentic-stream-build-mqtt-kafka` |

Pass the approved plan context to the build skill.

---

## Anti-Patterns (DO NOT)

- Do NOT generate any .py, .sh, or .json files before plan approval
- Do NOT assume a pipeline category — always confirm with the user
- Do NOT skip the pre-flight checklist
- Do NOT reference dx_app, factory pattern, Python inference apps, or C++ code
- Do NOT use paths outside the dx_stream project root
- Do NOT hardcode `/dev/video0` — use `usb` or let the user specify
