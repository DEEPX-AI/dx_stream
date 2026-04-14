# Skill: Test-Driven Development for dx_stream

> **RIGID skill — Iron Law: No code without a validation check first.**
> Every file you create must be validated immediately. Never batch validations.

## Purpose

Apply a Red-Green-Verify cycle to dx_stream pipeline development. Write the validation
expectation first, create the file, then verify it passes — for every single artifact.

## When to Use

Active during any pipeline build (`/dx-build-pipeline-app`, `/dx-build-mqtt-kafka-app`).
This skill runs alongside the build skill, not instead of it.

---

## The Cycle: Red → Green → Verify

1. **Red:** State what the next file must satisfy (e.g., "pipeline.py must parse without errors")
2. **Green:** Write the file
3. **Verify:** Run the validation check. If it fails, fix immediately before moving on.

Never create the next file until the current file passes its check.

## Validation Order

Validate files in this exact order as they are created:

| # | File | Validation Command | Pass Criteria |
|---|------|--------------------|---------------|
| 1 | `pipeline.py` | `python3 -m py_compile pipeline.py` | Exit code 0 |
| 2 | `pipeline.py` | `python3 pipeline.py --help` | Prints usage, exit code 0 |
| 3 | `run_*.sh` | `bash -n run_*.sh` | Exit code 0 (syntax OK) |
| 4 | `config/*.json` | `python3 -c "import json; json.load(open('<file>'))"` | No JSONDecodeError |
| 5 | `session.json` | `python3 -c "import json; json.load(open('session.json'))"` | No JSONDecodeError |
| 6 | `README.md` | `test -f README.md` | File exists |

Run each check from the session directory (e.g., `dx-agentic-dev/<session>/`).

## Pipeline-Specific Checks

After all files pass basic validation, run these pipeline-level checks:

### 1. preprocess-id Matching

Every `DxPreprocess` must have a `DxInfer` with the same `preprocess-id`.

```bash
# In run_*.sh or pipeline.py, extract and compare:
grep -oP 'dxpreprocess[^!]*preprocess-id=\K\d+' run_*.sh | sort > /tmp/pp_ids
grep -oP 'dxinfer[^!]*preprocess-id=\K\d+' run_*.sh | sort > /tmp/inf_ids
diff /tmp/pp_ids /tmp/inf_ids
# Empty diff = PASS
```

### 2. Queue Placement

Every pair of adjacent DX elements must have a `queue` between them.

```bash
# Check for missing queues (adjacent dx elements without queue separator)
grep -oP '(dxpreprocess|dxinfer|dxpostprocess|dxtracker|dxosd|dxmsgconv|dxmsgbroker)' \
    run_*.sh | head -20
# Manually verify queue exists between each consecutive pair in the pipeline string
```

### 3. Absolute model-path

`DxInfer` `model-path` must be an absolute path (starts with `/` or `$`).

```bash
grep -oP 'model-path=\K\S+' run_*.sh
# Must start with / or $ — relative paths are errors
```

### 4. DxMsgConv Before DxMsgBroker (Broker Pipelines Only)

```bash
# In broker pipelines, dxmsgconv must appear before dxmsgbroker
grep -n 'dxmsgconv\|dxmsgbroker' run_*.sh
# dxmsgconv line number must be less than dxmsgbroker line number
```

### 5. DxRate After RTSP Sources (RTSP Pipelines Only)

```bash
# If pipeline uses rtspsrc or urisourcebin with rtsp://, DxRate must follow
grep -c 'rtsp://' run_*.sh && grep -c 'dxrate' run_*.sh
# If RTSP count > 0, DxRate count must also be > 0
```

## Framework-Level Validation

After all file-level and pipeline-level checks pass, run the framework validator:

```bash
python3 .deepx/scripts/validate_app.py
```

This checks the entire dx_stream project structure. Fix any reported issues before
claiming completion.

## Checklist Summary

Use this as a TodoWrite checklist during builds:

```
- [ ] pipeline.py: py_compile passes
- [ ] pipeline.py: --help prints usage
- [ ] run_*.sh: bash -n syntax OK
- [ ] config/*.json: JSON parse OK
- [ ] session.json: JSON parse OK
- [ ] README.md: exists
- [ ] preprocess-id matching: all pairs consistent
- [ ] Queue placement: queue between every DX element pair
- [ ] model-path: absolute paths only
- [ ] DxMsgConv before DxMsgBroker (broker only)
- [ ] DxRate after RTSP source (RTSP only)
- [ ] Framework validation: validate_app.py passes
```

---

## Anti-Patterns (DO NOT)

- Do NOT create multiple files before validating the first one
- Do NOT skip pipeline-specific checks — they catch the most common runtime failures
- Do NOT treat validation as optional cleanup — it is the core development loop
- Do NOT reference dx_app, factory pattern, 4-variant files, or parent project structure
- Do NOT defer validation to "after everything is done" — validate per-file, immediately
