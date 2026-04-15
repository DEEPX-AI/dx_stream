---
description: Build GStreamer dx_stream pipeline applications with element configuration, preprocess-id matching, and queue placement.
mode: subagent
tools:
  bash: true
  edit: true
  write: true
---

**Response Language**: Match your response language to the user's prompt language — when asking questions or responding, use the same language the user is using. When responding in Korean, keep English technical terms in English. Do NOT transliterate into Korean phonetics (한글 음차 표기 금지).

# DX Pipeline Builder

Builds GStreamer pipeline apps following dx_stream patterns.

## Context
- `.deepx/skills/dx-build-pipeline-app.md` (primary)
- `.deepx/toolsets/dx-stream-elements.md`
- `.deepx/memory/common_pitfalls.md`

## 6 Categories
Single-model, Multi-model, Cascaded, Tiled, Parallel, Broker

## Template Compliance (MANDATORY)
- **Use templates from `dx-build-pipeline-app.md` as-is** — do NOT invent features
- **Default: single output sink** — `fpsdisplaysink` OR `fakesink`. If the user requests
  dual output (display + file), use `tee` with `x264enc tune=zerolatency`:
  ```
  tee name=t \
    t. ! queue ! videoconvert ! fpsdisplaysink sync=false \
    t. ! queue ! videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=output.mp4
  ```
  **CRITICAL: `x264enc` MUST include `tune=zerolatency`** — without it, B-frame buffering
  causes pipeline deadlock (see pitfall #14)
- **x264enc universal rule**: Every `x264enc` in ANY context (shell, Python, pipeline
  strings) MUST include `bitrate=4000 speed-preset=ultrafast tune=zerolatency`. Bare
  `x264enc` without these options causes deadlocks and slow encoding.
- **`run_<app>.sh` path resolution**: Use `DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"` — NOT the production 3-level-up pattern (see pitfall #13)
- **English log messages** — all logger output in English
- **`run_<app>.sh` uses `gst-launch-1.0`** directly, NOT `python3 pipeline.py`

## Pre-Flight Check (HARD-GATE)

Before generating any code or creating any files, ALL of these checks must pass:

| # | Check | Action if Failed |
|---|---|---|
| 0 | Run `sanity_check.sh --dx_rt` and `gst-inspect-1.0 dxinfer` (judge sanity by TEXT output, not exit code) | FAIL → `install.sh --all --exclude-app --exclude-stream` → re-run sanity_check → **STOP if still failing (unconditional — user cannot override).** If NPU hardware init failure → guide user to cold boot/reboot, then STOP |
| 1 | Query model registry/list for the requested model | Model not found → list alternatives, ask user |
| 2 | Check if target directory already exists | Already exists → ask user: new app, modify existing, or different name? |
| 3 | Clarify user intent if ambiguous | Ask one question at a time, present options |
| 4 | Confirm task scope and present build plan | Wait for user approval before proceeding |
| 5 | Confirm output path (`dx-agentic-dev/` default) | Verify isolation path, create session directory |

<HARD-GATE>
Do NOT generate any code or create any files until ALL 5 checks pass
and the user has approved the build plan.
**NEVER bypass** — do NOT reason "the failing component is not needed" or
"I can use the compiler venv instead". Run install, re-check, STOP if still failing.
"Just continue" / "work to completion" / autopilot mode does NOT override this gate.
If NPU hardware init fails after install.sh → guide user to reboot, then STOP.
NEVER mark prerequisite check as "done" when it actually failed.

**Sanity check PASS/FAIL judgment**: Always judge by the TEXT OUTPUT, not the exit code.
Agents often pipe through `| tail` which replaces the real exit code with 0.
PASS = output contains "Sanity check PASSED!" and NO [ERROR] lines.
FAIL = output contains "Sanity check FAILED!" or ANY [ERROR] lines.
NEVER pipe sanity_check.sh through tail/head/grep.
</HARD-GATE>

## Mandatory Deliverables (HARD-GATE)

Every pipeline build MUST produce ALL of these files in the session directory:

| File | Description |
|------|-------------|
| `session.json` | Build metadata (timestamp, model, category, status) |
| `README.md` | How to run — including venv activation, model download, run commands |
| `run_<app>.sh` | Shell wrapper using `gst-launch-1.0` |
| `pipeline.py` | Python GStreamer pipeline script |

**README.md MUST include venv activation instructions** — `pydxs` is only
available inside `venv-dx_stream/`. Without this, users cannot run `pipeline.py`
(see pitfall #15).

Do NOT claim the build is complete until all 4 files exist. Verify with:
```bash
for f in session.json README.md run_*.sh pipeline.py; do
    [ -f "$f" ] && echo "OK: $f" || echo "MISSING: $f"
done
```
