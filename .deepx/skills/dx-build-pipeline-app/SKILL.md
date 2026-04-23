---
name: dx-build-pipeline-app
description: Build GStreamer pipeline application for dx_stream
---

# Skill: Build dx_stream Pipeline Application

> **This skill document is sufficient.** Read this FIRST before exploring source code.
> Only read source files if this document lacks the specific information you need.

## Overview

Build a GStreamer pipeline application for real-time video processing on DEEPX NPU.
This skill covers all 6 pipeline categories with complete templates and patterns.

## Output Isolation (MUST FOLLOW)

All AI-generated pipeline applications MUST be created under `dx-agentic-dev/`, NOT
in production directories. This prevents accidental modification of existing pipelines.

### Session Directory

```
dx-agentic-dev/<YYYYMMDD-HHMMSS>_<model>_<pipeline_category>/
├── session.json          # Build metadata
├── README.md             # How to run this pipeline
├── pipeline.py           # Python pipeline script
├── run_<app>.sh           # Shell script wrapper
└── config/               # Pipeline-specific configs (optional)
```

### session.json Template

```json
{
  "session_id": "<YYYYMMDD-HHMMSS>_<model>_<category>",
  "created_at": "<ISO 8601 timestamp>",
  "model": "<model_name>",
  "pipeline_category": "<single_model|multi_model|cascaded|tiled|parallel|broker>",
  "status": "complete",
  "notes": "<any relevant notes>"
}
```

### When to Use Production Path

Only create files in production directories when the user EXPLICITLY says:
- "Add this to the production codebase"
- "Create this in the examples directory"
- "Make this a permanent addition"

Default behavior: ALWAYS use `dx-agentic-dev/`.

## Usage

Invoke with `/dx-build-pipeline-app` or ask the dx-stream-builder agent.
Specify the pipeline category (single-model, multi-model, cascaded, tiled,
parallel, or broker), the model name, and the input source type.

## Template Compliance (MANDATORY)

The templates in this document are battle-tested against the actual dx_stream
runtime. Generated code MUST follow these templates exactly.

### Rules

1. **Use the template as-is** — do NOT invent features not in the template
   (no custom logging format, no unrequested features)
2. **Default: single output sink** — `fpsdisplaysink` OR `fakesink`. If the user
   requests file output, replace the display sink with
   `videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=...`
3. **Dual output (tee) is allowed** — if the user requests both display AND file
   recording, use `tee`:
   ```
   tee name=t \
     t. ! queue ! videoconvert ! fpsdisplaysink sync=false \
     t. ! queue ! videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=output.mp4
   ```
   **CRITICAL: `x264enc` MUST include `tune=zerolatency`** — without it,
   B-frame buffering causes pipeline deadlock (see pitfall #14)
4. **English log messages** — all `logger.info`/`logger.error` messages in English,
   matching the style of existing examples
5. **x264enc universal rule** — every occurrence of `x264enc` in ANY context
   (shell scripts, Python code, pipeline strings) MUST include all three options:
   `bitrate=4000 speed-preset=ultrafast tune=zerolatency`. Bare `x264enc` without
   these options causes slow encoding and B-frame deadlocks (pitfall #14)
6. **`--headless` flag** — always include it in `pipeline.py`. Additionally check
   `DISPLAY` env as fallback (see template)
7. **`run_<app>.sh` uses `gst-launch-1.0` directly** — the shell wrapper invokes
   `gst-launch-1.0`, NOT `python3 pipeline.py`. `pipeline.py` is for programmatic use
8. **Path resolution** — `run_<app>.sh` uses `DX_STREAM_ROOT` (2 levels up from
   `dx-agentic-dev/<session>/`), NOT the production 3-level-up pattern.
   See the `run_<app>.sh` template below for the exact code

### Anti-Patterns (NEVER Do)

- Using `x264enc` without `tune=zerolatency` (causes deadlock — pitfall #14)
- Using bare `x264enc` in Python pipeline string building (e.g., `f"x264enc ! mp4mux"`)
  — always use `f"x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! ..."`
- Using relative model paths in `dxinfer model-path=` (must be absolute)
- Copying `SRC_DIR` calculation from production scripts in `dx_stream/pipelines/`
- Writing log messages in Korean or other non-English languages
- Adding features the user did not request (e.g., FPS overlay, auto-recording)

## Phase 0: Prerequisites Check

Before starting the build workflow, verify:

1. **dx-runtime**: `bash ../../scripts/sanity_check.sh --dx_rt`
   - FAIL → `bash ../../install.sh --all --exclude-app --exclude-stream --skip-uninstall --venv-reuse`
   - Re-run sanity_check.sh — must PASS after install
   - **If still failing → STOP (unconditional).** User instructions to continue do NOT override this.
     If NPU hardware init failure ("Device initialization failed"): tell the user a cold boot /
     system reboot is required, then STOP. NEVER proceed with code generation while sanity check is failing.
     NEVER mark this check as "done" when it actually failed.
2. **GStreamer plugins**: `gst-inspect-1.0 dxinfer` — FAIL → `./install.sh && ./build.sh`
3. **Postprocess libraries**: `ls /usr/local/share/gstdxstream/lib/libpostprocess_*.so`
4. **Python venv (for pipeline.py)**: The `pydxs` module is only available inside
   the dx_stream virtual environment. Verify it:
   ```bash
   source venv-dx_stream/bin/activate && python3 -c "import pydxs; print('pydxs OK')"
   ```
   - FAIL → `./install.sh && ./build.sh` (creates venv-dx_stream and installs pydxs)
   - If venv exists but `pydxs` not importable → `source venv-dx_stream/bin/activate && pip install -e .`
   - **IMPORTANT**: The README.md template MUST document this venv activation step
     so users know to activate it before running `pipeline.py` (see pitfall #15)

## Prerequisites

- DX-RT installed and NPU detected (`dxrt-cli -s`)
- GStreamer plugin registered (`gst-inspect-1.0 dxinfer`)
- Model file downloaded (`./setup.sh --model="<model>.dxnn"`)

## Quick Reference

### Supported Models (14)

| Model | Task | Input Size | Postprocess .so |
|---|---|---|---|
| EfficientNet_Lite0 | Classification | 224x224 | libpostprocess_object_class.so |
| SCRFD500M | Face Detection | 640x640 | libpostprocess_scrfd500m.so |
| YoloV5S_PPU | Object Detection | 640x640 | libpostprocess_ppu.so |
| YOLOv5s_Face | Face Detection | 640x640 | libpostprocess_yolov5s_face.so |
| yolo26n | Object Detection | 640x640 | libpostprocess_yolo26od.so |
| yolo26n-pose | Pose Estimation | 640x640 | libpostprocess_yolo26pose.so |
| yolo26n-seg | Segmentation | 640x640 | libpostprocess_yolo26seg.so |
| YoloV5S | Object Detection | 640x640 | libpostprocess_yolov5s_6.so |
| YoloV7 | Object Detection | 640x640 | libpostprocess_yolov7.so |
| YoloV8N | Object Detection | 640x640 | libpostprocess_yolov8n.so |
| YoloV9S | Object Detection | 640x640 | libpostprocess_yolov9s.so |
| YoloXS | Object Detection | 640x640 | libpostprocess_yoloxs.so |
| YOLOV11N | Object Detection | 640x640 | libpostprocess_yolov11.so |
| yolov8m_pose | Pose Estimation | 640x640 | libpostprocess_yolov8m_pose.so |

### Core Pipeline Pattern

```
source → dxpreprocess → queue → dxinfer → queue → dxpostprocess → queue → [dxtracker → queue →] dxosd → sink
```

---

## Template: pipeline.py (Python GStreamer Pipeline)

```python
#!/usr/bin/env python3
"""dx_stream pipeline: {TASK} with {MODEL}."""

import argparse
import logging
import os
import sys

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

logger = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser(
        description='{TASK} pipeline using {MODEL} on DEEPX NPU')
    parser.add_argument('--input', required=True,
                        help='Input source: file path, USB device, or RTSP URI')
    parser.add_argument('--model', required=True,
                        help='Path to .dxnn model file')
    parser.add_argument('--postprocess-lib', required=True,
                        help='Path to postprocess .so library')
    parser.add_argument('--function-name', default='PostProcess',
                        help='Postprocess function name (default: PostProcess)')
    parser.add_argument('--resize-width', type=int, default=640,
                        help='Preprocessing resize width')
    parser.add_argument('--resize-height', type=int, default=640,
                        help='Preprocessing resize height')
    parser.add_argument('--tracker-config', default=None,
                        help='Tracker config JSON path (enables tracking)')
    parser.add_argument('--headless', action='store_true',
                        help='Run without display (use fakesink)')
    parser.add_argument('--output', default=None,
                        help='Output MP4 file path (enables file recording)')
    return parser.parse_args()


def build_source_string(input_path):
    """Build source element string from input specification."""
    if input_path.startswith('rtsp://'):
        return f'urisourcebin uri={input_path} ! decodebin'
    elif input_path.startswith('/dev/video'):
        return f'v4l2src device={input_path} ! videoconvert'
    elif input_path == 'usb':
        return 'v4l2src ! videoconvert'
    else:
        uri = input_path if input_path.startswith('file://') else f'file://{input_path}'
        return f'urisourcebin uri={uri} ! decodebin'


def build_pipeline_string(args):
    """Compose the full GStreamer pipeline string."""
    source = build_source_string(args.input)

    # Inference chain
    preprocess = (f'dxpreprocess preprocess-id=1 '
                  f'resize-width={args.resize_width} '
                  f'resize-height={args.resize_height}')
    infer = (f'dxinfer preprocess-id=1 inference-id=1 '
             f'model-path={args.model}')
    postprocess = (f'dxpostprocess inference-id=1 '
                   f'library-file-path={args.postprocess_lib} '
                   f'function-name={args.function_name}')

    # Optional tracker
    tracker = ''
    if args.tracker_config:
        tracker = f'queue max-size-buffers=1 ! dxtracker config-file-path={args.tracker_config} !'

    # Sink
    file_enc = 'videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux'
    if args.output:
        if args.headless or not os.environ.get('DISPLAY'):
            sink = f'{file_enc} ! filesink location={args.output}'
        else:
            sink = (f'tee name=t '
                    f't. ! queue max-size-buffers=1 ! videoconvert ! fpsdisplaysink sync=false '
                    f't. ! queue max-size-buffers=1 ! {file_enc} ! filesink location={args.output}')
    elif args.headless or not os.environ.get('DISPLAY'):
        sink = 'fakesink sync=false'
    else:
        sink = 'videoconvert ! fpsdisplaysink sync=false'

    return (f'{source} ! '
            f'{preprocess} ! queue max-size-buffers=1 ! '
            f'{infer} ! queue max-size-buffers=1 ! '
            f'{postprocess} ! {tracker} '
            f'queue max-size-buffers=1 ! dxosd ! {sink}')


def on_bus_message(bus, message, loop):
    """Handle GStreamer bus messages."""
    msg_type = message.type
    if msg_type == Gst.MessageType.EOS:
        logger.info("End of stream")
        loop.quit()
    elif msg_type == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        logger.error("Pipeline error: %s\n%s", err.message, debug)
        loop.quit()
    elif msg_type == Gst.MessageType.WARNING:
        warn, debug = message.parse_warning()
        logger.warning("Pipeline warning: %s\n%s", warn.message, debug)
    return True


def main():
    args = parse_args()
    Gst.init(None)

    pipeline_str = build_pipeline_string(args)
    logger.info("Pipeline: %s", pipeline_str)

    try:
        pipeline = Gst.parse_launch(pipeline_str)
    except GLib.Error as e:
        logger.error("Failed to parse pipeline: %s", e.message)
        sys.exit(1)

    loop = GLib.MainLoop()

    bus = pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect('message', on_bus_message, loop)

    pipeline.set_state(Gst.State.PLAYING)
    logger.info("Pipeline running. Press Ctrl+C to stop.")

    try:
        loop.run()
    except KeyboardInterrupt:
        logger.info("Interrupted by user")
    finally:
        pipeline.set_state(Gst.State.NULL)
        logger.info("Pipeline stopped")


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')
    main()
```

---

## Template: run_<app>.sh (Shell Script Wrapper)

> **CRITICAL — Path Resolution**: This script lives in `dx-agentic-dev/<session>/`,
> which is **2 levels below** `dx_stream/` root. Do NOT copy the `SRC_DIR` calculation
> from production scripts (`dx_stream/pipelines/...`) — those are 4 levels deep.
> Always use the pattern below.

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ─── Path Resolution (dx-agentic-dev context) ─────────────────────
# This script is at: dx_stream/dx-agentic-dev/<session>/run_<app>.sh
# dx_stream root is 2 levels up. Samples live under dx_stream/dx_stream/samples/.
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"

# ─── Model Auto-Download ───────────────────────────────────────────
MODEL_NAME="{MODEL}.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found in samples/models. Downloading..."
    (cd "$DX_STREAM_ROOT" && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

# ─── Input Video ───────────────────────────────────────────────────
# Task-aware sample video: select based on model's vision task
# Object Detection: dogs.mp4, blackbox-city-road.mp4
# Face Detection / Pose Estimation: use video with people
# General fallback: boat.mp4
INPUT_VIDEO="${1:-$SRC_DIR/samples/videos/boat.mp4}"
if [ ! -f "$INPUT_VIDEO" ]; then
    echo "[ERROR] Input video not found: $INPUT_VIDEO"
    exit 1
fi

# ─── Video Sink Detection ─────────────────────────────────────────
if [ -z "${DISPLAY:-}" ]; then
    SINK="fakesink sync=false"
    echo "[INFO] No DISPLAY detected, using fakesink"
else
    VIDEOCONVERT_PIPELINE="videoconvert"
    SINK="$VIDEOCONVERT_PIPELINE ! fpsdisplaysink sync=false"
fi

# ─── Pipeline ──────────────────────────────────────────────────────
gst-launch-1.0 \
    urisourcebin uri=file://$INPUT_VIDEO ! decodebin ! \
    dxpreprocess \
        preprocess-id=1 \
        resize-width={WIDTH} \
        resize-height={HEIGHT} ! \
    queue max-size-buffers=1 ! \
    dxinfer \
        preprocess-id=1 \
        inference-id=1 \
        model-path=$MODEL_PATH ! \
    queue max-size-buffers=1 ! \
    dxpostprocess \
        inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/{POSTPROCESS_LIB} \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    dxosd ! $SINK
```

---

## Template: README.md (Session Documentation)

> Every session directory MUST include a README.md. This is the primary entry point
> for anyone who opens the directory — without it, users cannot run the pipeline.

````markdown
# {TASK} Pipeline — {MODEL}

> Generated by dx_stream Pipeline Builder on {DATE}.

## Prerequisites

### 1. dx-runtime and dx_stream

```bash
# Verify NPU is available
dxrt-cli -s

# Verify GStreamer plugin is registered
gst-inspect-1.0 dxinfer
```

If `dxinfer` is not found, install dx_stream:

```bash
cd ../../        # Navigate to dx_stream root
./install.sh && ./build.sh
```

### 2. Python Virtual Environment (required for pipeline.py)

The `pipeline.py` script imports `pydxs`, which is only available inside
the dx_stream virtual environment.

```bash
# Activate the virtual environment (from dx_stream root)
source ../../venv-dx_stream/bin/activate

# Verify pydxs is importable
python3 -c "import pydxs; print('pydxs OK')"
```

### 3. Model Download

```bash
# Auto-download (from dx_stream root)
cd ../../
./setup.sh --model="{MODEL}.dxnn"
```

## How to Run

> **RULE**: Never use `/path/to/<model>.dxnn` or `/path/to/video.mp4` in generated
> run commands, README examples, or run.sh scripts. Always use **real paths** resolved
> from the session directory or from known sample/model locations.

### Option A: Shell script (recommended)

```bash
# Display output (requires X11/Wayland)
bash run_{APP_NAME}.sh

# With custom input video (use actual sample video path)
bash run_{APP_NAME}.sh ../../assets/videos/dogs.mp4
```

### Option B: Python script

```bash
# Activate venv first!
source ../../venv-dx_stream/bin/activate

# Use real paths — resolve model to absolute path for GStreamer
MODEL_PATH="$(realpath ../../samples/models/{MODEL}.dxnn)"
python3 pipeline.py \
    --input ../../assets/videos/dogs.mp4 \
    --model "$MODEL_PATH" \
    --postprocess-lib /usr/local/share/gstdxstream/lib/{POSTPROCESS_LIB}
```

### Model Path Resolution (GStreamer requires absolute paths)

GStreamer `model-path` property requires an **absolute filesystem path**. In run scripts:
1. Check `samples/models/<model>.dxnn` from dx_stream root
2. Check `../dx_app/assets/models/<model>.dxnn` for shared models
3. Use `realpath` to convert relative to absolute: `MODEL="$(realpath <relative_path>)"`

### Headless Mode (SSH / no display)

```bash
python3 pipeline.py --input video.mp4 --model model.dxnn \
    --postprocess-lib lib.so --headless
```

## Files

| File | Description |
|------|-------------|
| `session.json` | Build metadata (timestamp, model, category) |
| `README.md` | This file |
| `pipeline.py` | Python GStreamer pipeline script |
| `run_{APP_NAME}.sh` | Shell wrapper using `gst-launch-1.0` |

## Pipeline Diagram

```
source → DxPreprocess → DxInfer → DxPostprocess → DxOsd → sink
```
````

---

## Template: Custom Postprocess Library

### meson.build

```meson
project('postprocess_{model_lower}', 'cpp',
  version: '1.0.0',
  default_options: ['cpp_std=c++17'])

dxstream_dep = dependency('gstdxstream', required: true)

shared_library('postprocess_{model_lower}',
  'postprocess_{model_lower}.cpp',
  dependencies: [dxstream_dep],
  install: true,
  install_dir: '/usr/local/share/gstdxstream/lib')
```

### postprocess_{model_lower}.cpp

```cpp
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>

// Include dx_stream postprocess headers
#include "dx_stream_postprocess.h"

static constexpr float CONFIDENCE_THRESHOLD = 0.5f;
static constexpr float NMS_THRESHOLD = 0.45f;

struct Detection {
    float x, y, w, h;
    float confidence;
    int class_id;
};

static std::vector<Detection> decode_outputs(
    const float* output_data, int output_size,
    int input_width, int input_height) {
    std::vector<Detection> detections;
    // Model-specific decoding logic here
    return detections;
}

static void nms(std::vector<Detection>& detections, float threshold) {
    // Non-maximum suppression implementation
    std::sort(detections.begin(), detections.end(),
        [](const Detection& a, const Detection& b) {
            return a.confidence > b.confidence;
        });
    // IoU-based filtering
}

extern "C" void PostProcess(
    DXInferOutputMeta* output_meta,
    DXObjectMetaList* object_list,
    int width, int height) {

    const float* output = output_meta->output_data;
    int output_size = output_meta->output_size;

    auto detections = decode_outputs(output, output_size, width, height);
    nms(detections, NMS_THRESHOLD);

    for (const auto& det : detections) {
        if (det.confidence < CONFIDENCE_THRESHOLD) continue;

        DXObjectMeta obj = {};
        obj.x = det.x;
        obj.y = det.y;
        obj.w = det.w;
        obj.h = det.h;
        obj.confidence = det.confidence;
        obj.class_id = det.class_id;
        object_list->push_back(obj);
    }
}
```

### Build Instructions

```bash
cd dx_stream/custom_library/postprocess_library/<ModelName>/
meson setup builddir
ninja -C builddir
sudo ninja -C builddir install
```

---

## Pipeline Patterns by Category

### Single Network

Simplest pattern. One source, one model, display output.

```bash
gst-launch-1.0 urisourcebin uri=file:///path/video.mp4 ! decodebin ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! \
    queue max-size-buffers=1 ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=/path/model.dxnn ! \
    queue max-size-buffers=1 ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_xxx.so \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    dxosd ! videoconvert ! fpsdisplaysink sync=false
```

### Multi-Stream (Compositor Grid)

Multiple sources, each with its own inference chain, composited into a grid.

```bash
# Each stream: source ! preprocess ! infer ! postprocess ! osd ! dxscale ! comp.sink_N
# Compositor merges all streams with sink_N::xpos/ypos positioning
```

### Tracking

Add DxTracker after DxPostprocess for persistent object IDs.

```bash
... ! dxpostprocess ... ! queue ! \
    dxtracker config-file-path=tracker_config.json ! queue ! \
    dxosd ! ...
```

### Secondary Mode (Cascade)

Primary detector → tee → secondary branches → DxGather → OSD.
Use distinct preprocess-id/inference-id per branch.

### RTSP (Input/Output Selector)

DxInputSelector round-robins N RTSP sources through one inference chain.
DxOutputSelector routes results back to per-stream display.

### Broker

DxMsgConv + DxMsgBroker at pipeline end publishes results to Kafka/MQTT.

---

## preprocess-id / inference-id Matching Guide

```
DxPreprocess(preprocess-id=1) → DxInfer(preprocess-id=1, inference-id=1) → DxPostprocess(inference-id=1)
DxPreprocess(preprocess-id=2) → DxInfer(preprocess-id=2, inference-id=2) → DxPostprocess(inference-id=2)
DxPreprocess(preprocess-id=3) → DxInfer(preprocess-id=3, inference-id=3) → DxPostprocess(inference-id=3)
```

Rules:
- Each DxPreprocess `preprocess-id` must match exactly one DxInfer `preprocess-id`
- Each DxInfer `inference-id` must match exactly one DxPostprocess `inference-id`
- In single network pipelines, use preprocess-id=1, inference-id=1
- In secondary mode, use sequential IDs: primary=1, secondary_a=2, secondary_b=3

---

## File Creation Checklist (MANDATORY HARD-GATE)

<HARD-GATE>
Before claiming the build is complete, ALL mandatory files MUST exist in the
session directory. If ANY mandatory file is missing, the build is NOT complete.
Do NOT skip this checklist. Do NOT treat it as optional.
</HARD-GATE>

### Mandatory Files (MUST create — no exceptions)

- [ ] `session.json` — build metadata (use template in "session.json Template" above)
- [ ] `README.md` — how to run this pipeline (use template in "README.md Template" below)
- [ ] `run_<app>.sh` — shell script wrapper using `gst-launch-1.0` (use template above)
- [ ] `pipeline.py` — Python GStreamer pipeline script (use template above)
- [ ] `setup.sh` — environment setup (**MUST** detect/activate venv — see setup.sh template below)
- [ ] `run.sh` — one-command launcher (**MUST** use real model/video paths — see run.sh template below)
- [ ] `session.log` — actual command output (captured via tee, NOT a summary)

### Conditional Files (create if applicable)

- [ ] Model downloaded: `dx_stream/samples/models/<model>.dxnn`
- [ ] Postprocess library: exists at `/usr/local/share/gstdxstream/lib/`
- [ ] Config files (if using config-file-path mode): `dx_stream/configs/<Model>/`
- [ ] Tracker config (if tracking): `dx_stream/configs/tracker_config.json`
- [ ] Broker config (if broker): `dx_stream/configs/broker_kafka.cfg` or `broker_mqtt.cfg`

### Verification Step

After creating all files, run this self-check:

```bash
SESSION_DIR="dx-agentic-dev/<YYYYMMDD-HHMMSS>_<model>_<category>"
for f in session.json README.md run_*.sh pipeline.py; do
    [ -f "$SESSION_DIR/$f" ] && echo "OK: $f" || echo "MISSING: $f"
done
```

If any mandatory file shows `MISSING`, create it before completing the build.

## setup.sh Template (MANDATORY)

> **CRITICAL**: `setup.sh` must be runnable standalone. A user must be able to `cd` into
> the session directory and run `./setup.sh` without manually activating any venv first.

```bash
#!/bin/bash
# Environment setup for <Model> <Category> Pipeline
# Generated by DX Agentic Dev

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- 1. Virtual environment detection & activation ---
# Search upward for dx_stream or dx-runtime shared venv
STREAM_VENV=""
RUNTIME_VENV=""
_search="$SCRIPT_DIR"
for _i in 1 2 3 4 5; do
    _search="$(dirname "$_search")"
    [ -d "$_search/venv-dx_stream" ] && STREAM_VENV="$_search/venv-dx_stream" && break
    [ -d "$_search/venv-dx-runtime" ] && RUNTIME_VENV="$_search/venv-dx-runtime"
done

LOCAL_VENV="$SCRIPT_DIR/.venv"

if [ -n "$STREAM_VENV" ]; then
    echo "[INFO] Activating dx_stream venv: $STREAM_VENV"
    source "$STREAM_VENV/bin/activate"
elif [ -n "$RUNTIME_VENV" ]; then
    echo "[INFO] Activating dx-runtime venv: $RUNTIME_VENV"
    source "$RUNTIME_VENV/bin/activate"
elif [ -d "$LOCAL_VENV" ]; then
    echo "[INFO] Activating local venv: $LOCAL_VENV"
    source "$LOCAL_VENV/bin/activate"
else
    echo "[INFO] Creating local venv at $LOCAL_VENV ..."
    python3 -m venv "$LOCAL_VENV"
    source "$LOCAL_VENV/bin/activate"
    pip install --upgrade pip
fi

# --- 2. Verify GStreamer plugins ---
gst-inspect-1.0 dxinfer >/dev/null 2>&1 && echo "[OK] DxInfer plugin available" || {
    echo "[WARN] DxInfer plugin not found. Run: cd $(cd "$SCRIPT_DIR/../.." && pwd) && ./install.sh"
}

# --- 3. Download model (if not present) ---
MODEL_NAME="{MODEL}"
MODEL_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)/samples/models"
if [ ! -f "$MODEL_DIR/${MODEL_NAME}.dxnn" ]; then
    echo "[INFO] Downloading model: $MODEL_NAME"
    (cd "$SCRIPT_DIR/../.." && ./setup.sh --model="${MODEL_NAME}.dxnn") || {
        echo "[WARN] Model download failed. Download manually."
    }
fi

echo "[INFO] Setup complete. Run: bash run.sh"
```

## run.sh Template (MANDATORY)

> **CRITICAL**: `run.sh` must include **real, working paths** — never `/path/to/` placeholders.

```bash
#!/bin/bash
# One-command pipeline launcher for <Model> <Category> Pipeline
# Generated by DX Agentic Dev

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- Activate venv ---
source "$SCRIPT_DIR/setup.sh" 2>/dev/null || true

# --- Default paths ---
# Model (absolute path required by GStreamer)
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DEFAULT_MODEL="${DX_STREAM_ROOT}/samples/models/{MODEL}.dxnn"
# Video input
DEFAULT_VIDEO="${DX_STREAM_ROOT}/samples/videos/{VIDEO}"

MODEL="${1:-$DEFAULT_MODEL}"
VIDEO="${2:-$DEFAULT_VIDEO}"

if [ ! -f "$MODEL" ]; then
    echo "[ERROR] Model not found: $MODEL"
    echo "Usage: bash run.sh [model_path] [video_path]"
    echo ""
    echo "Model locations:"
    echo "  dx_stream: ${DX_STREAM_ROOT}/samples/models/{MODEL}.dxnn"
    echo "  dx_app:    ${DX_STREAM_ROOT}/../dx_app/assets/models/{MODEL}.dxnn"
    exit 1
fi

echo "[INFO] Model: $MODEL"
echo "[INFO] Video: $VIDEO"
bash "$SCRIPT_DIR/run_{APP_NAME}.sh" "$VIDEO"
```
