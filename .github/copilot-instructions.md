# dx_stream — Copilot Global Instructions

> GStreamer-based video analytics framework for DEEPX NPU accelerators.
> 13 custom GStreamer elements, 6 pipeline categories.

## Response Language

Match your response language to the user's prompt language — when asking questions
or responding, use the same language the user is using.

**Technical term rule**: When responding in Korean, keep English technical terms in
their original English form. Do NOT transliterate English terms into Korean phonetics
(한글 음차 표기 금지). Established Korean loanwords (모델, 서버, 파일, 데이터) are acceptable.

## Recommended Model (Pre-Flight Check)

DX Agentic Development requires strong instruction following and knowledge base
utilization. Before starting any task, check if you are running as one of the
recommended models:

- **Claude Sonnet 4.6** or later
- **Claude Opus 4.6** or later

If you are NOT one of these models (e.g., GPT-4.1, GPT-4o, Gemini, etc.),
output the following notice at the beginning of your first response:

```
══════════════════════════════════════════════════════════════
⚠  DX-AGENTIC-DEV: MODEL NOTICE

DX Agentic Development recommends Claude Sonnet 4.6+ or
Opus 4.6+. Your current model may produce lower quality
in instruction following, knowledge base utilization, and
API accuracy. Please switch to a recommended model.
══════════════════════════════════════════════════════════════
```

After the box notice, proceed with the task normally. This is an informational
warning, not a blocking gate.

### Model Notice Repeat (Non-Recommended Model Only)

If you output the MODEL NOTICE box above (i.e., you are NOT a recommended model),
you MUST also output this shortened reminder **immediately before the actual question
line** that the user needs to answer — NOT at the beginning of the brainstorming flow.

**Timing**: After all file reading, context analysis, and preamble text, right before
the line that contains the `?` (the actual question), insert this reminder:

```
---
⚠ **Non-recommended model** — output quality may be degraded. Recommended: Claude Sonnet 4.6+ / Opus 4.6+
---
```

**Example — WRONG** (repeat scrolls past with the box):
```
[DX-AGENTIC-DEV: START]
══ MODEL NOTICE ══
---  ⚠ Non-recommended model ---     ← TOO EARLY, scrolls past
... (reads files, analyzes context) ...
First question: ...?
```

**Example — CORRECT** (repeat appears right before the question):
```
[DX-AGENTIC-DEV: START]
══ MODEL NOTICE ══
... (reads files, analyzes context) ...
---  ⚠ Non-recommended model ---     ← RIGHT BEFORE the question
First question: ...?
```

Only output this reminder ONCE (before the first question), not before every question.

## Context Routing Table

| If the task mentions... | Read these files |
|---|---|
| **Pipeline, detection, classification** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **MQTT, Kafka, message broker** | `.deepx/skills/dx-build-mqtt-kafka-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **Multi-model, cascaded, tiled** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-metadata.md` |
| **Model, download** | `.deepx/skills/dx-model-management.md` |
| **Validation, testing** | `.deepx/skills/dx-validate.md`, `.deepx/instructions/testing-patterns.md` |
| **ALWAYS read** | `.deepx/memory/common_pitfalls.md`, `.deepx/instructions/coding-standards.md` |

## Skills

| Skill | Description |
|-------|-------------|
| dx-build-pipeline-app | Build GStreamer pipeline (single, multi, cascaded, tiled, parallel, broker) |
| dx-build-mqtt-kafka-app | Build MQTT/Kafka message broker pipeline |
| dx-model-management | Download and configure .dxnn models for pipelines |
| dx-validate | Run pipeline validation checks |
| dx-brainstorm-and-plan | Process: collaborative design session before code generation |
| dx-tdd | Process: test-driven development — validate each file immediately after creation |
| dx-verify-completion | Process: verify before claiming completion — evidence before assertions |

## Interactive Workflow (MUST FOLLOW)

**Always walk through key decisions with the user before building.** This is a HARD GATE.

Before ANY code generation:
1. Ask 2-3 clarifying questions (variant, task type, model)
2. Present a build plan and wait for user approval
3. After generation, validate each file

### Output Isolation
All AI-generated code goes to `dx-agentic-dev/<session_id>/` by default.
Only write to `src/` when explicitly requested by the user.

**Session ID format**: `YYYYMMDD-HHMMSS_<model>_<task>` — the timestamp MUST use the
**system local timezone** (NOT UTC). Use `$(date +%Y%m%d-%H%M%S)` in Bash or
`datetime.now().strftime('%Y%m%d-%H%M%S')` in Python. Do NOT use `date -u`,
`datetime.utcnow()`, or `datetime.now(timezone.utc)`.

## 13 GStreamer Elements

| Element | Purpose |
|---------|---------|
| DxPreprocess | Resize, normalize, color-convert for NPU inference |
| DxInfer | Run .dxnn model inference on NPU |
| DxPostprocess | Decode raw inference tensors |
| DxOsd | Draw bounding boxes, labels, overlays |
| DxRate | Drop excess frames to target FPS |
| DxScale | Resize frames between ROI and secondary inference |
| DxRoiExtract | Extract ROI crops from detection results |
| DxTracker | Multi-object tracking with persistent IDs |
| DxTile | Split frame into tiles for high-res inference |
| DxDeTile | Reassemble tiled results to full-frame |
| DxMsgConv | Serialize results to JSON/protobuf |
| DxMsgBroker | Publish to MQTT or Kafka |
| DxMux | Multiplex multiple streams |

## 6 Pipeline Categories

| Category | Pattern |
|----------|---------|
| Single-model | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` |
| Multi-model | Chain multiple DxInfer stages with distinct preprocess-ids |
| Cascaded | `DxInfer ! DxRoiExtract ! DxScale ! DxInfer` |
| Tiled | `DxTile ! DxInfer ! DxDeTile` |
| Parallel | `DxMux` to combine, shared DxInfer |
| Broker | `DxInfer ! DxMsgConv ! DxMsgBroker` |

## Critical Conventions

1. **preprocess-id matching**: DxPreprocess and DxInfer share the same ID
2. **Queue elements**: `queue` between every processing stage
3. **DxRate for RTSP**: Always rate-limit RTSP sources
4. **Absolute model-path**: DxInfer `model-path` must be absolute
5. **DxMsgConv before DxMsgBroker**: Serialize before publish
6. **Logging**: `logging.getLogger(__name__)` — no `print()`
7. **Mandatory output artifacts**: Every pipeline build session MUST produce these 4 files in the session directory — no exceptions:
   - `session.json` — build metadata (timestamp, model, category, status)
   - `README.md` — how to run, including venv activation (`source ../../venv-dx_stream/bin/activate`), model download, and run commands
   - `pipeline.py` — Python GStreamer pipeline script
   - `run_<app>.sh` — shell wrapper using `gst-launch-1.0`
   Do NOT output the DONE sentinel until all 4 files exist. Run self-verification: `for f in session.json README.md pipeline.py run_*.sh; do [ -f "$f" ] && echo "OK: $f" || echo "MISSING: $f"; done`
8. **x264enc universal rule**: Every `x264enc` in ANY context (shell, Python, pipeline strings) MUST include `bitrate=4000 speed-preset=ultrafast tune=zerolatency` — bare `x264enc` causes deadlocks (pitfall #14)

## Pipeline Template

```
source ! queue ! DxPreprocess preprocess-id=0 ! queue ! DxInfer preprocess-id=0 model-path=/path/to/model.dxnn ! queue ! DxPostprocess ! queue ! DxOsd ! queue ! sink
```

## Quick Reference

```bash
./install.sh                         # Install GStreamer plugin bindings
./setup.sh                           # Download sample models and videos
dxrt-cli -s                          # Verify NPU availability
gst-inspect-1.0 dxinfer              # Verify DxInfer plugin
pytest test/ -m "not npu_required"   # Run unit tests
```

## No Placeholder Code (MANDATORY)

NEVER generate stub/placeholder code. This includes:
- Commented-out imports: `# from dxnn_sdk import InferenceEngine`
- Fake results: `result = np.zeros(...)`
- TODO markers: `# TODO: implement actual inference`
- "Similar to sync version" without actual async implementation

All generated code MUST be functional, using real APIs from the knowledge base.
If the required SDK/API is unknown, read the relevant skill document first.

## Experimental Features — Prohibited

Do NOT offer, suggest, or implement experimental or non-existent features. This includes:
- "웹 기반 비주얼 컴패니언" (web-based visual companion)
- Local URL-based diagram viewers or dashboards
- Any feature requiring the user to open a local URL for visualization
- Any capability that does not exist in the current toolset

**Superpowers brainstorming skill override**: The superpowers `brainstorming` skill
includes a "Visual Companion" step (step 2 in its checklist). This step MUST be
SKIPPED in the DEEPX project. The visual companion does not exist in our environment.
When the brainstorming checklist says "Offer visual companion", skip it and proceed
directly to "Ask clarifying questions" (step 3).

If a feature does not exist, do not pretend it does. Stick to proven, documented
capabilities only.

## Brainstorming — Spec Before Plan (HARD GATE)

When using the superpowers `brainstorming` skill or `/dx-brainstorm-and-plan`:

1. **Spec document is MANDATORY** — Before transitioning to `writing-plans`, a spec
   document MUST be written to `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`.
   Skipping the spec and going directly to plan writing is a violation.
2. **User approval gate is MANDATORY** — After writing the spec, the user MUST review
   and approve it before proceeding to plan writing. Do NOT treat unrelated user
   responses (e.g., answering a different question) as spec approval.
3. **Plan document MUST reference the spec** — The plan header must include a link
   to the approved spec document.
4. **Prefer `/dx-brainstorm-and-plan`** — Use the project-level brainstorming skill
   instead of the generic superpowers `brainstorming` skill. The project-level skill
   has domain-specific questions and pre-flight checks.

## Hardware

| Architecture | Value |
|---|---|
| DX-M1 | `dx_m1` |

## Git Safety — Superpowers Artifacts

**NEVER `git add` or `git commit` files under `docs/superpowers/`.** These are temporary
planning artifacts generated by the superpowers skill system (specs, plans). They are
`.gitignore`d, but some tools may bypass `.gitignore` with `git add -f`. Creating the
files is fine — committing them is forbidden.

## Session Sentinels (MANDATORY for Automated Testing)

When processing a user prompt, output these exact markers for automated session
boundary detection by the test harness:

- **First line of your response**: `[DX-AGENTIC-DEV: START]`
- **Last line after ALL work is complete**: `[DX-AGENTIC-DEV: DONE (output-dir: <relative_path>)]`
  where `<relative_path>` is the session output directory (e.g., `dx-agentic-dev/20260409-143022_yolo26n_detection/`)

Rules:
1. **CRITICAL — Output `[DX-AGENTIC-DEV: START]` as the absolute first line of your
   first response.** This must appear before ANY other text, tool calls, or reasoning.
   Even if the user instructs you to "just proceed" or "use your own judgment",
   the START sentinel is non-negotiable — automated tests WILL fail without it.
2. Output `[DX-AGENTIC-DEV: DONE (output-dir: <path>)]` as the very last line after all work, validation,
   and file generation is complete
3. If you are a **sub-agent** invoked via handoff/routing from a higher-level agent,
   do NOT output these sentinels — only the top-level agent outputs them
4. If the user sends multiple prompts in a session, output START/DONE for each prompt
5. The `output-dir` in DONE must be the relative path from the project root to the
   session output directory. If no files were generated, omit the `(output-dir: ...)` part.
6. **NEVER output DONE after only producing planning artifacts** (specs, plans, design
   documents). DONE means all deliverables are produced — implementation code, scripts,
   configs, and validation results. If you completed a brainstorming or planning phase
   but have not yet implemented the actual code, do NOT output DONE. Instead, proceed
   to implementation or ask the user how to proceed.
7. **Pre-DONE mandatory deliverable check**: Before outputting DONE, verify that all
   mandatory deliverables exist in the session directory. If any mandatory file is
   missing, create it before outputting DONE. Each sub-project defines its own mandatory
   file list in its skill document (e.g., `dx-build-pipeline-app.md` File Creation Checklist).
8. **Session HTML export guidance** (Copilot CLI only): Immediately before the DONE
   sentinel line, output: `To save this session as HTML, type: /share html`
   — this tells the user they can preserve the full conversation. The `/share html`
   command is specific to GitHub Copilot CLI; it does not work in Claude Code,
   Copilot Chat (VS Code), or OpenCode. The test harness (`test.sh`) will automatically
   detect and copy the exported HTML file to the session output directory.

## Plan Output (MANDATORY)

When generating a plan document (e.g., via writing-plans or brainstorming skills),
**always print the full plan content in the conversation output** immediately after
saving the file. Do NOT only mention the file path — the user should be able to
review the plan directly in the prompt without opening a separate file.
