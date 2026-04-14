# dx_stream Coding Standards

## Pipeline Composition Rules

### 1. Preprocess-ID / Inference-ID Matching

Every DxPreprocess element must have a `preprocess-id` that matches the corresponding
DxInfer element's `preprocess-id`. Similarly, DxInfer's `inference-id` must match
the downstream DxPostprocess's `inference-id`.

```bash
# CORRECT — IDs match
dxpreprocess preprocess-id=1 ... ! queue ! \
dxinfer preprocess-id=1 inference-id=1 ... ! queue ! \
dxpostprocess inference-id=1 ...

# WRONG — mismatched preprocess-id
dxpreprocess preprocess-id=1 ... ! queue ! \
dxinfer preprocess-id=2 ...   # ERROR: will not route correctly
```

For secondary mode pipelines, use distinct IDs per branch (e.g., primary=1, secondary_a=2, secondary_b=3).

### 2. Queue Elements Between Stages

Always place a `queue` element between every dx_stream processing element.
Omitting queues causes pipeline deadlocks due to GStreamer threading model.

```bash
# CORRECT
dxpreprocess ... ! queue max-size-buffers=1 ! dxinfer ... ! queue max-size-buffers=1 ! dxpostprocess ...

# WRONG — missing queues
dxpreprocess ... ! dxinfer ... ! dxpostprocess ...   # DEADLOCK RISK
```

Recommended queue sizing:
- Single network: `max-size-buffers=1`
- Multi-stream: `max-size-buffers=10`
- RTSP: `max-size-buffers=2`

### 3. DxRate for Variable-Rate Sources

RTSP and other variable-rate sources must use DxRate to prevent buffer accumulation:

```bash
urisourcebin uri=rtsp://... ! decodebin ! dxrate max-rate=30 ! dxpreprocess ...
```

### 4. Model Path Must Be Absolute

DxInfer `model-path` must be an absolute path or correctly resolved relative to the
script's working directory. Never hardcode relative paths in committed scripts —
use `$SRC_DIR/samples/models/<model>.dxnn` pattern.

### 5. Postprocess Library Path Must Be Absolute

DxPostprocess `library-file-path` must be an absolute path to the .so file.
Standard install location: `/usr/local/share/gstdxstream/lib/lib*.so`

## Custom Postprocess C++ Libraries

### Naming Convention

```
Library directory:  dx_stream/custom_library/postprocess_library/<ModelName>/
Source file:        postprocess_<model>.cpp
Header file:        postprocess_<model>.h (if needed)
Build output:       libpostprocess_<model>.so
Entry function:     PostProcess
```

### Build Pattern (Meson)

```meson
project('postprocess_<model>', 'cpp',
  default_options: ['cpp_std=c++17'])

shared_library('postprocess_<model>',
  'postprocess_<model>.cpp',
  dependencies: [...],
  install: true,
  install_dir: '/usr/local/share/gstdxstream/lib')
```

### Function Signature

```cpp
extern "C" void PostProcess(
    DXInferOutputMeta* output_meta,
    DXObjectMetaList* object_list,
    int width, int height);
```

## Python Pipeline Scripts

### Structure

```python
#!/usr/bin/env python3
"""<description> pipeline for dx_stream."""

import argparse
import logging
import sys

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

logger = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser(description='<pipeline description>')
    parser.add_argument('--input', required=True, help='Input video path or URI')
    parser.add_argument('--model', required=True, help='Path to .dxnn model file')
    parser.add_argument('--postprocess-lib', required=True, help='Path to postprocess .so')
    parser.add_argument('--headless', action='store_true', help='Run without display')
    return parser.parse_args()


def main():
    args = parse_args()
    Gst.init(None)

    pipeline_str = build_pipeline_string(args)
    pipeline = Gst.parse_launch(pipeline_str)

    # Set up bus message handling
    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect('message', on_bus_message)

    pipeline.set_state(Gst.State.PLAYING)

    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        pass
    finally:
        pipeline.set_state(Gst.State.NULL)


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO)
    main()
```

### Logging

Always use Python `logging` module, never `print()` for status messages:

```python
logger = logging.getLogger(__name__)
logger.info("Pipeline started: %s", pipeline_desc)
logger.error("Model not found: %s", model_path)
```

## Shell Script Wrappers (run_*.sh)

### Standard Template

```bash
#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

# Model auto-download
MODEL_NAME="<model>.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

# Pipeline execution
gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO ! decodebin ! \
    dxpreprocess preprocess-id=1 ... ! queue max-size-buffers=1 ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue max-size-buffers=1 ! \
    dxpostprocess inference-id=1 library-file-path=... function-name=PostProcess ! queue max-size-buffers=1 ! \
    dxosd ! videoconvert ! fpsdisplaysink sync=false
```

### Rules for Shell Scripts

1. Always resolve `SCRIPT_DIR` and `SRC_DIR` relative to the script location
2. Always include model auto-download logic
3. Handle Ubuntu 18.04 video sink fallback when needed
4. Use `set -e` for strict error handling in production scripts
5. Quote all paths containing variables

## Convention Checklist

Before submitting any pipeline code, verify:

- [ ] **ID Matching**: preprocess-id on DxPreprocess matches DxInfer; inference-id on DxInfer matches DxPostprocess
- [ ] **Queue Placement**: queue element between every dx_stream element pair
- [ ] **Absolute Paths**: model-path and library-file-path are absolute
- [ ] **Model Download**: Shell scripts include auto-download logic
- [ ] **DxRate for RTSP**: Variable-rate sources have DxRate element
- [ ] **DxTracker Placement**: DxTracker follows DxPostprocess, not DxInfer
- [ ] **Headless Check**: DISPLAY environment variable checked before video sink creation
