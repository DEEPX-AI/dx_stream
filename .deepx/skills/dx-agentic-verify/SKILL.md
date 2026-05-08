---
name: dx-agentic-verify
description: Verify work before claiming done
---

# Skill: Verify Before Completion for dx_stream

> **RIGID skill — Iron Law: No completion claims without fresh evidence.**
> Run every check below and confirm output BEFORE saying "done", "complete", or equivalent.

## Purpose

Prevent false completion claims. Before marking any dx_stream pipeline build as done,
execute the full verification gate and produce a completion report with evidence.

## When to Use

Invoke this skill when you are about to claim a pipeline build is complete,
a bug is fixed, or a modification is finished. This applies to all dx_stream work.

---

## Gate Function (5 Steps)

Execute in order. If ANY step fails, stop and fix before proceeding.

### Step 1: File Existence

```bash
SD="dx-agentic-dev/<session_name>"
for f in pipeline.py session.json README.md setup.sh run.sh session.log; do
    test -f "$SD/$f" && echo "OK: $f" || echo "FAIL: $f missing"
done
test -f "$SD"/run_*.sh && echo "OK: run_*.sh" || echo "FAIL: run_*.sh missing"
# Broker pipelines only:
# test -f "$SD/config/broker_"*.cfg && test -f "$SD/config/msgconv_config.json"
```

### Step 2: Syntax Validation

```bash
python3 -m py_compile "$SD/pipeline.py"               # Python syntax
bash -n "$SD"/run_*.sh                                  # Shell syntax
for f in "$SD"/*.json "$SD"/config/*.json; do           # JSON syntax
    [ -f "$f" ] && python3 -c "import json; json.load(open('$f'))" \
        && echo "OK: $f" || echo "FAIL: $f"
done
```

### Step 3: Pipeline Parse Test

```bash
python3 "$SD/pipeline.py" --help    # Must print usage and exit 0
```

If `--help` fails, the pipeline has import errors or broken argument parsing.

### Step 4: Property Validation

```bash
SCRIPT="$SD/run_"*.sh

# preprocess-id matching
PP=$(grep -oP 'dxpreprocess[^!]*preprocess-id=\K\d+' "$SCRIPT" | sort)
INF=$(grep -oP 'dxinfer[^!]*preprocess-id=\K\d+' "$SCRIPT" | sort)
[ "$PP" = "$INF" ] && echo "OK: preprocess-id" || echo "FAIL: preprocess-id mismatch"

# Queue placement — list element sequence, verify no adjacent dx* without queue
grep -oP '(dxpreprocess|dxinfer|dxpostprocess|dxtracker|dxosd|dxmsgconv|dxmsgbroker|queue)' \
    "$SCRIPT" | tr '\n' ' '; echo ""

# Absolute model-path (must start with / or $)
for mp in $(grep -oP 'model-path=\K\S+' "$SCRIPT"); do
    case "$mp" in /*|'$'*) echo "OK: $mp" ;; *) echo "FAIL: relative $mp" ;; esac
done

# Broker only: DxMsgConv before DxMsgBroker
if grep -q 'dxmsgbroker' "$SCRIPT"; then
    CL=$(grep -n 'dxmsgconv' "$SCRIPT" | head -1 | cut -d: -f1)
    BL=$(grep -n 'dxmsgbroker' "$SCRIPT" | head -1 | cut -d: -f1)
    [ "$CL" -lt "$BL" ] 2>/dev/null && echo "OK: conv<broker" || echo "FAIL: conv must precede broker"
fi

# RTSP only: DxRate present
if grep -q 'rtsp://' "$SCRIPT"; then
    grep -q 'dxrate' "$SCRIPT" && echo "OK: DxRate" || echo "FAIL: RTSP without DxRate"
fi
```

### Step 5: Environment & Framework

```bash
gst-inspect-1.0 dxinfer > /dev/null 2>&1 \
    && echo "OK: dxinfer registered" \
    || echo "WARN: dxinfer not registered (run ./install.sh)"

python3 .deepx/scripts/validate_app.py 2>/dev/null \
    && echo "OK: framework validation" \
    || echo "INFO: framework validator unavailable or reported issues"
```

---

## Completion Report Template

After ALL checks pass, produce this report:

```
## Completion Report: <app_name>

**Session:** dx-agentic-dev/<session_dir>/
**Category:** <pipeline_category>  |  **Model:** <model_name>

### Files
- [x] pipeline.py — py_compile OK, --help OK
- [x] run_<app>.sh — bash -n OK
- [x] session.json — JSON OK
- [x] README.md — exists
- [x] setup.sh — bash -n OK
- [x] run.sh — bash -n OK
- [x] session.log — exists, non-empty, contains actual command output
- [x] config/ — JSON OK (if applicable)

### Pipeline Checks
- [x] preprocess-id matching
- [x] Queue placement between DX elements
- [x] model-path absolute
- [x] DxMsgConv before DxMsgBroker (broker only)
- [x] DxRate after RTSP (RTSP only)

### Environment
- [x] dxinfer plugin registered
- [x] Framework validation passed

### How to Run
<exact command from README.md>
```

Only after producing this report with all items checked may you claim completion.

---

## Session Log Rules

The `session.log` file MUST contain **actual command execution output**, not a
hand-crafted summary. Verify by checking:

```bash
# Must exist and be non-empty
[ -s "$SD/session.log" ] && echo "OK: session.log non-empty" || echo "FAIL: session.log missing or empty"

# Must contain at least one timestamped command line (HH:MM:SS $ ...)
grep -qP '^\d{2}:\d{2}:\d{2} \$' "$SD/session.log" \
    && echo "OK: contains timestamped commands" \
    || echo "WARN: no timestamped command output found"
```

If `session.log` is missing or contains only a hand-written summary, go back and
re-capture actual command output before claiming completion.

---

## Anti-Patterns (DO NOT)

- Do NOT claim "done" without running the gate function first
- Do NOT skip property validation — preprocess-id mismatches cause silent failures
- Do NOT substitute "I verified mentally" for actual command output
- Do NOT reference dx_app, C++ apps, factory pattern, or parent project paths
- Do NOT produce a report with unchecked items — every box must have evidence
- Do NOT treat warnings as failures — only FAIL items block completion
