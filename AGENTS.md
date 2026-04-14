# dx_stream — Claude Code Entry Point

> Self-contained entry point for dx_stream GStreamer pipeline development.

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

## Shared Knowledge

All skills, instructions, toolsets, and memory live in `.deepx/`.
Read `.deepx/README.md` for the complete index.

## Quick Reference

```bash
./install.sh                        # Install GStreamer plugin bindings
./setup.sh                          # Download sample models and videos
dxrt-cli -s                         # Verify NPU availability
gst-inspect-1.0 dxinfer             # Verify DxInfer plugin is registered
pytest test/ -m "not npu_required"  # Run unit tests (no NPU)
pytest test/ -m npu_required        # Run NPU integration tests
```

## Skills

| Command | Description |
|---------|-------------|
| /dx-build-pipeline-app | Build GStreamer pipeline app (6 categories: single-model, multi-model, cascaded, tiled, parallel, broker) |
| /dx-build-mqtt-kafka-app | Build MQTT/Kafka message broker pipeline app |
| /dx-model-management | Download and configure .dxnn models for pipelines |
| /dx-validate | Run pipeline validation checks |
| /dx-validate-and-fix | Full feedback loop: validate, collect, approve, apply, verify |
| /dx-brainstorm-and-plan | Brainstorm and plan before any code generation |
| /dx-tdd | Test-driven development — validate each file immediately after creation |
| /dx-verify-completion | Verify before claiming completion — evidence before assertions |

## Critical Conventions

1. **preprocess-id matching**: Every `DxPreprocess` / `DxInfer` pair must share the same `preprocess-id` value
2. **Queue elements**: Place `queue` between every GStreamer processing stage
3. **DxRate for RTSP**: Always insert `DxRate rate=<fps>` after RTSP sources to limit frame ingestion
4. **Absolute model-path**: `DxInfer` `model-path` property must be an absolute filesystem path
5. **DxMsgConv before DxMsgBroker**: Always serialize metadata with `DxMsgConv` before `DxMsgBroker`
6. **Logging**: `logging.getLogger(__name__)` — no bare `print()`
7. **Skill doc is sufficient**: Do NOT read source code unless skill is insufficient
8. **No hardcoded model paths**: Use variables or config for model path resolution
9. **Model list**: Query `model_list.json` for model download URLs and expected paths
10. **PPU model auto-detection**: Auto-detect PPU models by checking model name `_ppu` suffix, `model_list.json` postprocess library, or compiler session context. PPU pipelines omit `DxPostprocess` or use a pass-through library.
11. **Existing pipeline search**: Before generating a new pipeline, search `pipelines/` and `run_*.sh` for existing examples. If found, ask user: (a) explain-only, or (b) create new based on existing. Never silently skip or overwrite.
12. **PPU pipeline generation is MANDATORY**: If the compiled .dxnn model is PPU, the agent MUST generate a working pipeline example.
13. **Mandatory output artifacts**: Every pipeline build session MUST produce: `pipeline.py`, `run_<app>.sh`, `session.json`, `README.md`, `setup.sh`, `run.sh`, `session.log`. The `session.log` must contain actual command output (never a hand-written summary). Run self-verification before presenting the final report.

## Context Routing Table

| Task mentions... | Read these files |
|---|---|
| **Pipeline, detection, classification** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **MQTT, Kafka, message broker** | `.deepx/skills/dx-build-mqtt-kafka-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **Multi-model, cascaded, tiled** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-metadata.md` |
| **Model, download** | `.deepx/skills/dx-model-management.md` |
| **Validation, testing** | `.deepx/skills/dx-validate.md`, `.deepx/instructions/testing-patterns.md` |
| **Validation, feedback, fix** | `.deepx/skills/dx-validate.md` (cross-project: `dx-runtime/.deepx/skills/dx-validate-and-fix.md`) |
| **Brainstorm, plan, design** | `.deepx/skills/dx-brainstorm-and-plan.md` |
| **TDD, validation, incremental** | `.deepx/skills/dx-tdd.md` |
| **Completion, verify, evidence** | `.deepx/skills/dx-verify-completion.md` |
| **ALWAYS read (every task)** | `.deepx/memory/common_pitfalls.md`, `.deepx/instructions/coding-standards.md` |

## 13 GStreamer Elements

| Element | Purpose |
|---------|---------|
| `DxPreprocess` | Resize, normalize, color-convert input frames for NPU inference |
| `DxInfer` | Run .dxnn model inference on the NPU |
| `DxPostprocess` | Decode raw inference tensors into detections/classifications |
| `DxOsd` | Draw bounding boxes, labels, and overlays on frames |
| `DxRate` | Drop excess frames to match target inference FPS |
| `DxScale` | Resize frames (used between ROI extraction and secondary inference) |
| `DxRoiExtract` | Extract ROI crops from primary detection results |
| `DxTracker` | Multi-object tracking (assign persistent IDs) |
| `DxTile` | Split frame into tiles for higher-resolution inference |
| `DxDeTile` | Reassemble tiled inference results into full-frame coordinates |
| `DxMsgConv` | Serialize inference results into wire format (JSON/protobuf) |
| `DxMsgBroker` | Publish serialized messages to MQTT or Kafka |
| `DxMux` | Multiplex multiple streams into a single pipeline |

## 6 Pipeline Categories

| Category | Description | Key Pattern |
|----------|-------------|-------------|
| **Single-model** | One model, one stream | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` |
| **Multi-model** | Multiple models, sequential | Chain multiple DxInfer stages with distinct preprocess-ids |
| **Cascaded** | Primary detection feeds secondary classification | `DxInfer ! DxRoiExtract ! DxScale ! DxInfer` |
| **Tiled** | High-res input split into tiles | `DxTile ! DxInfer ! DxDeTile` |
| **Parallel** | Multiple streams processed in parallel | `DxMux` to combine, shared DxInfer |
| **Broker** | Publish results to MQTT/Kafka | `DxInfer ! DxMsgConv ! DxMsgBroker` |

## Pipeline Template

```
source ! queue ! DxPreprocess preprocess-id=0 ! queue ! DxInfer preprocess-id=0 model-path=/path/to/model.dxnn ! queue ! DxPostprocess ! queue ! DxOsd ! queue ! sink
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

## Memory

Persistent knowledge in `.deepx/memory/`. Read at task start, update when learning.

## Git Operations — User Handles

Do NOT ask about git branch operations (merge, PR, push, cleanup) at the end of
work. The user will handle all git operations themselves. Never present options
like "merge to main", "create PR", or "delete branch" — just finish the task.

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
