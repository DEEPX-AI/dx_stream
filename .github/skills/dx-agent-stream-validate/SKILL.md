---
name: dx-agent-stream-validate
description: Pipeline validation for dx_stream
---

<!-- AUTO-GENERATED from .deepx/ — DO NOT EDIT DIRECTLY -->
<!-- Source: .deepx/skills/dx-agent-stream-validate/SKILL.md -->
<!-- Run: dx-agent-gen generate -->

# Skill: dx_stream Pipeline Validation

> **This skill document is sufficient.** Read this FIRST before exploring source code.

## Overview

Validate dx_stream pipeline applications for correctness, completeness, and runtime
readiness. Covers static analysis, property validation, and smoke testing.

## Usage

Invoke with `/dx-agent-stream-validate` to run pipeline validation. Can be used standalone
or as part of the `/dx-agent-runtime-validate` feedback loop.

```bash
# Validate a specific pipeline script
python3 .deepx/scripts/validate_app.py <script.sh>

# Validate .deepx/ framework integrity
python3 .deepx/scripts/validate_framework.py
```

## Validation Levels

### Level 1: Static Analysis (No NPU Required)

Checks that can run without hardware:

```bash
# 1. Shell script syntax
bash -n run_<app>.sh

# 2. Python script syntax
python3 -m py_compile pipeline.py

# 3. GStreamer element registration
gst-inspect-1.0 dxinfer > /dev/null 2>&1

# 4. Postprocess library exists and exports PostProcess
nm -D /usr/local/share/gstdxstream/lib/libpostprocess_xxx.so | grep -q PostProcess

# 5. Path resolution in run_<app>.sh (dx-agent-dev context)
#    Verify that SRC_DIR actually contains samples/models/
#    CRITICAL: dx-agent-dev scripts are 2 levels below dx_stream root,
#    NOT 4 levels like production scripts in dx_stream/pipelines/...
bash -c 'source run_<app>.sh --dry-run 2>/dev/null; [ -d "$SRC_DIR/samples" ]' \
  || echo "FAIL: SRC_DIR does not resolve to dx_stream/dx_stream/"

# 6. x264enc must have tune=zerolatency (prevents pipeline deadlock — pitfall #14)
grep -q 'x264enc' pipeline.py run_<app>.sh 2>/dev/null \
  && { grep -q 'tune=zerolatency' pipeline.py run_<app>.sh 2>/dev/null \
       || echo "FAIL: x264enc found without tune=zerolatency — pipeline will deadlock"; }
```

### Level 2: Property Validation

Check pipeline element properties against known valid values:

```python
#!/usr/bin/env python3
"""Validate dx_stream pipeline properties."""

import re
import sys

REQUIRED_ID_MATCHES = {
    'dxpreprocess': 'preprocess-id',
    'dxinfer': ['preprocess-id', 'inference-id'],
    'dxpostprocess': 'inference-id',
}

REQUIRED_PATHS = {
    'dxinfer': 'model-path',
    'dxpostprocess': 'library-file-path',
}


def extract_element_props(pipeline_str, element_name):
    """Extract properties for a named element from pipeline string."""
    pattern = rf'{element_name}\s+((?:\S+=\S+\s*)*)'
    matches = re.findall(pattern, pipeline_str)
    results = []
    for match in matches:
        props = dict(re.findall(r'(\S+)=(\S+)', match))
        results.append(props)
    return results


def validate_id_matching(pipeline_str):
    """Check preprocess-id and inference-id consistency."""
    errors = []

    preprocess_elements = extract_element_props(pipeline_str, 'dxpreprocess')
    infer_elements = extract_element_props(pipeline_str, 'dxinfer')
    postprocess_elements = extract_element_props(pipeline_str, 'dxpostprocess')

    # Check preprocess-id matching
    for pp in preprocess_elements:
        pp_id = pp.get('preprocess-id', '0')
        matching_infer = [i for i in infer_elements
                          if i.get('preprocess-id', '0') == pp_id]
        if not matching_infer:
            errors.append(
                f"DxPreprocess preprocess-id={pp_id} has no matching DxInfer")

    # Check inference-id matching
    for inf in infer_elements:
        inf_id = inf.get('inference-id', '0')
        matching_post = [p for p in postprocess_elements
                         if p.get('inference-id', '0') == inf_id]
        if not matching_post:
            errors.append(
                f"DxInfer inference-id={inf_id} has no matching DxPostprocess")

    return errors


def validate_queue_placement(pipeline_str):
    """Check that queue elements exist between dx elements."""
    errors = []
    dx_elements = ['dxpreprocess', 'dxinfer', 'dxpostprocess',
                    'dxtracker', 'dxosd', 'dxmsgconv', 'dxmsgbroker']

    # Split by '!' to get element chain
    parts = [p.strip() for p in pipeline_str.split('!')]

    for i in range(len(parts) - 1):
        current = parts[i].split()[0] if parts[i] else ''
        next_elem = parts[i + 1].split()[0] if parts[i + 1] else ''

        if current in dx_elements and next_elem in dx_elements:
            errors.append(
                f"Missing queue between {current} and {next_elem}")

    return errors


def validate_paths(pipeline_str):
    """Check that model-path and library-file-path are absolute."""
    import os
    errors = []

    infer_elements = extract_element_props(pipeline_str, 'dxinfer')
    for inf in infer_elements:
        model_path = inf.get('model-path', '')
        if model_path and not model_path.startswith('$') and not os.path.isabs(model_path):
            errors.append(f"DxInfer model-path is not absolute: {model_path}")

    postprocess_elements = extract_element_props(pipeline_str, 'dxpostprocess')
    for pp in postprocess_elements:
        lib_path = pp.get('library-file-path', '')
        if lib_path and not lib_path.startswith('$') and not os.path.isabs(lib_path):
            errors.append(f"DxPostprocess library-file-path is not absolute: {lib_path}")

    return errors
```

### Level 3: Smoke Test (NPU Required)

```bash
# Run pipeline with test source and limited frames
timeout 30 gst-launch-1.0 \
    videotestsrc num-buffers=10 ! video/x-raw,width=640,height=640,format=RGB ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=$POSTPROCESS_LIB \
        function-name=PostProcess ! queue ! \
    fakesink sync=false

echo "Exit code: $?"
```

### Level 3.5: Headless / Display Fallback Validation (No NPU Required)

Verify the pipeline handles headless environments correctly:

```bash
# 1. pipeline.py has --headless flag or DISPLAY check
grep -qE '(--headless|DISPLAY)' pipeline.py \
  || echo "FAIL: pipeline.py missing --headless flag or DISPLAY check"

# 2. run_<app>.sh has DISPLAY check with fakesink fallback
grep -qE 'DISPLAY.*fakesink' run_<app>.sh \
  || echo "FAIL: run_<app>.sh missing DISPLAY/fakesink fallback"

# 3. Headless smoke test — pipeline should not crash with unset DISPLAY
unset DISPLAY
timeout 10 python3 pipeline.py --input test.mp4 --model model.dxnn \
  --postprocess-lib /usr/local/share/gstdxstream/lib/libpostprocess_xxx.so \
  --headless 2>&1 | grep -qi 'error.*display' \
  && echo "FAIL: pipeline crashes without DISPLAY" \
  || echo "PASS: headless mode works"
```

### Level 4: Performance Validation

```bash
# FPS measurement
GST_TRACERS="framerate" GST_DEBUG=GST_TRACER:7 \
    gst-launch-1.0 ... ! fpsdisplaysink sync=false

# Latency measurement
GST_TRACERS="latency" GST_DEBUG=GST_TRACER:7 \
    gst-launch-1.0 ...
```

## GStreamer Debug Patterns

```bash
# Element-specific debugging
GST_DEBUG=dxinfer:5 gst-launch-1.0 ...        # DxInfer verbose
GST_DEBUG=dxpostprocess:4 gst-launch-1.0 ...   # DxPostprocess info
GST_DEBUG=dxtracker:5 gst-launch-1.0 ...       # DxTracker verbose

# Caps negotiation debugging
GST_DEBUG=GST_CAPS:4 gst-launch-1.0 ...

# All dx_stream elements
GST_DEBUG=dxpreprocess:4,dxinfer:4,dxpostprocess:4,dxtracker:4,dxosd:4 gst-launch-1.0 ...
```

## Required Files Checklist

| File | Required | Description |
|---|---|---|
| `run_<app>.sh` or `pipeline.py` | Yes | Pipeline script |
| `<model>.dxnn` | Yes | Model file |
| `libpostprocess_*.so` | Yes | Postprocess library |
| `preprocess_config.json` | Optional | Preprocess config |
| `inference_config.json` | Optional | Inference config |
| `postprocess_config.json` | Optional | Postprocess config |
| `tracker_config.json` | If tracking | Tracker config |
| `msgconv_config.json` | If broker | Message convert config |
| `broker_*.cfg` | If broker | Broker connection config |

## Common Validation Failures

| Symptom | Cause | Fix |
|---|---|---|
| "No such element" | Plugin not registered | Run `./install.sh && ./build.sh` |
| Pipeline deadlock | Missing queue elements | Add `queue` between dx elements |
| No detections | preprocess-id mismatch | Match IDs across elements |
| Segfault in postprocess | Wrong .so for model | Check library-file-path matches model |
| "Could not link" | Caps incompatibility | Add videoconvert or dxconvert |
| "No such file" in run script | Wrong SRC_DIR path | Use dx-agent-dev path template (pitfall #13) |
| Pipeline deadlocks with x264enc | Missing tune=zerolatency | Add `tune=zerolatency` to x264enc (pitfall #14) |
