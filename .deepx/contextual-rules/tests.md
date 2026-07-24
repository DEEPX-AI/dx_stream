---
glob: "tests/**"
description: Rules for dx_stream test scripts
---

# Contextual Rules: Tests

These rules apply to all files under `tests/` (and `test/`).

## GStreamer Test Patterns

### Pipeline Validation Test

```bash
#!/bin/bash
set -euo pipefail

# Use videotestsrc for deterministic input (no media files needed)
# Use fakesink for headless execution (no display needed)
# Use num-buffers to limit test duration
# Use timeout to prevent hung tests

timeout 30 gst-launch-1.0 \
    videotestsrc num-buffers=10 ! \
    video/x-raw,width=640,height=640,format=RGB ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=$POSTPROCESS_LIB function-name=PostProcess ! queue ! \
    fakesink sync=false
```

### Element Registration Test

```bash
# Verify all 13 elements are registered
EXPECTED_ELEMENTS=(
    dxpreprocess dxinfer dxpostprocess dxtracker dxosd
    dxgather dxinputselector dxoutputselector dxrate
    dxmsgconv dxmsgbroker dxscale dxconvert
)

for elem in "${EXPECTED_ELEMENTS[@]}"; do
    gst-inspect-1.0 "$elem" > /dev/null 2>&1 || {
        echo "FAIL: $elem not registered"
        exit 1
    }
done
echo "PASS: All 13 elements registered"
```

## GST_DEBUG Levels for Testing

| Level | Use Case |
|---|---|
| 0 | Production (no debug output) |
| 1 | ERROR only — use for CI pass/fail |
| 2 | WARNING + ERROR — use for integration tests |
| 3 | FIXME — use for debugging specific failures |
| 4 | INFO — use for development tracing |
| 5+ | DEBUG/LOG — use for deep element debugging |

```bash
# CI test: errors only
GST_DEBUG=1 gst-launch-1.0 ...

# Debug a specific element
GST_DEBUG=dxinfer:5 gst-launch-1.0 ...
```

## Test Rules

1. **Deterministic Input**: Use `videotestsrc` instead of media files for
   tests that don't require specific visual content.

2. **Headless Output**: Use `fakesink sync=false` for all automated tests.
   Never rely on display availability.

3. **Bounded Duration**: Use `num-buffers=N` on videotestsrc and `timeout`
   command to prevent infinite-running tests.

4. **Exit Code**: Tests must exit with code 0 on success, non-zero on failure.
   Use `set -euo pipefail` for shell scripts.

5. **Model Skip**: If a model file is not downloaded, SKIP the test rather
   than FAIL. Use exit code 0 with a SKIP message.

6. **NPU Skip**: If NPU is not available (`dxrt-cli -s` fails), SKIP tests
   that require inference. Static tests (syntax, registration) should still run.
