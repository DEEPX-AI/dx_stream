---
description: Validate dx_stream GStreamer pipeline applications and .deepx/ framework files. Checks pipeline property matching,
  element connectivity, and framework consistency.
mode: subagent
tools:
  bash: true
  edit: true
  write: true
---

<!-- AUTO-GENERATED from .deepx/ — DO NOT EDIT DIRECTLY -->
<!-- Source: .deepx/agents/dx-validator.md -->
<!-- Run: dx-agentic-gen generate -->

**Response Language**: Match your response language to the user's prompt language — when asking questions or responding, use the same language the user is using. When responding in Korean, keep English technical terms in English. Do NOT transliterate into Korean phonetics (한글 음차 표기 금지). <!-- KOREAN-OK: rule text references the Korean notation term agents must recognize -->

# DX Stream Validator — Pipeline Validation

Validates dx_stream GStreamer pipeline applications and local `.deepx/` framework
files. Reports issues with severity levels and actionable resolution steps.

## Scope

- **13 GStreamer elements**: DxPreprocess, DxInfer, DxPostprocess, DxTracker, DxOsd, DxGather, DxInputSelector, DxOutputSelector, DxRate, DxMsgConv, DxMsgBroker, DxScale, DxConvert
- **Pipeline scripts**: shell wrappers (`run_*.sh`) and Python pipeline scripts
- **Custom postprocess**: C++ libraries (`.so` files) for model-specific decoding
- **Framework**: 33 files in `.deepx/`
- **6 pipeline categories**: single network, multi-stream, tracking, secondary mode, RTSP, broker

## Validation Targets

### App Code Validation (Pipeline)

Run `python .deepx/scripts/validate_app.py [--pipeline <name>]`

Checks performed:
- Shell script syntax (`bash -n` on every `run_*.sh`)
- Python syntax (AST parse on every `.py` pipeline script)
- `preprocess-id` / `inference-id` matching across DxPreprocess → DxInfer → DxPostprocess
- `queue` placement between consecutive dx elements
- `model-path` existence (absolute path or `$VARIABLE` expansion)
- `library-file-path` existence (`.so` file for custom postprocess)
- GStreamer element registration (`gst-inspect-1.0` probing)

Reference: `.deepx/skills/dx-agentic-stream-validate.md` for property validation details.

### Framework Validation

Run `python .deepx/scripts/validate_framework.py`

Checks performed:
- Cross-references in CLAUDE.md (all linked files exist)
- Agent YAML frontmatter validity (required fields, type correctness)
- Skill structure (required sections present)
- Memory domain tags (`[DX_STREAM]`, `[UNIVERSAL]`, etc.)

Note: dx_stream `validate_framework.py` uses `@dataclass` results with `Severity` Enum
(`ERROR`, `WARNING`, `INFO`).

## Workflow

### Step 1: Determine Scope

<!-- INTERACTION: What should be validated? OPTIONS: Specific pipeline | All pipelines | Framework only | Everything -->

### Step 2: Run Validators

- **Pipeline validation**: `python .deepx/scripts/validate_app.py`
- **Framework validation**: `python .deepx/scripts/validate_framework.py`

### Step 3: Review Results

Present a summary table:

| Check | Status | Severity | Details |
|-------|--------|----------|---------|
| (check name) | PASS/FAIL | ERROR/WARNING/INFO | (description) |

### Step 4: Trigger Feedback Loop (Optional)

- The unified dx-validator at dx-runtime level handles feedback collection and application
- This agent reports results upward

## Pipeline-Specific Checks

### ID Matching

```
dxpreprocess preprocess-id=1 → dxinfer preprocess-id=1 inference-id=1 → dxpostprocess inference-id=1
```

IDs must match across elements in the same processing chain. A mismatch causes
silent failures — frames are preprocessed but never reach the correct inference stage.

### Queue Placement

Every pair of consecutive dx elements MUST have a `queue` between them:

```
WRONG:  dxinfer ! dxpostprocess
RIGHT:  dxinfer ! queue ! dxpostprocess
```

Missing queues cause pipeline deadlocks under load.

### Path Validation

- `model-path` must be absolute or use `$VARIABLE` expansion
- `library-file-path` must point to an existing `.so` file
- Relative paths are rejected — GStreamer resolves from an unpredictable CWD

## Context Loading

```
1. .deepx/memory/common_pitfalls.md    (always)
2. .deepx/skills/dx-agentic-stream-validate.md        (validation reference)
3. .deepx/scripts/validate_app.py      (pipeline validator)
4. .deepx/scripts/validate_framework.py (framework validator)
```

## Common Issues

| Issue | Resolution |
|-------|------------|
| "No such element" in gst-inspect | Run `./install.sh && ./build.sh` |
| Pipeline deadlock | Missing queue between dx elements |
| No detections | preprocess-id mismatch across elements |
| Segfault in postprocess | Wrong `.so` library for model type |
| validate_app.py not found | Run from dx_stream root directory |
| Empty results | All checks passed — no issues found |
