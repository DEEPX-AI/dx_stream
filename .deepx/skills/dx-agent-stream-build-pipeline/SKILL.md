---
name: dx-agent-stream-build-pipeline
description: Build GStreamer pipeline application for dx_stream
---

# Skill: Build dx_stream Pipeline Application

> **This skill document is sufficient.** Read this FIRST before exploring source code.
> Only read source files if this document lacks the specific information you need.

## Overview

Build a GStreamer pipeline application for real-time video processing on DEEPX NPU.
This skill covers all 6 pipeline categories with complete templates and patterns.

## Output Isolation (MUST FOLLOW)

All AI-generated pipeline applications MUST be created under `dx-agent-dev/`, NOT
in production directories. This prevents accidental modification of existing pipelines.

### Session Directory

```
dx-agent-dev/<YYYYMMDD-HHMMSS>_<model>_<pipeline_category>/
├── session.json          # Build metadata
├── README.md             # How to run this pipeline
├── pipeline.py           # Python pipeline script
├── run_<app>.sh           # Shell script wrapper
└── config/               # Pipeline-specific configs (optional)
```

### session.json Template

```jsonc
{
  "session_id": "<YYYYMMDD-HHMMSS>_<agent>_<model>_<category>",
  "created_at": "<ISO 8601 with local timezone — use datetime.now().astimezone().isoformat(timespec='seconds')>",
  "model": "<DX inference model name — e.g. 'yolo26n'>",
  // ⚠ HARD ERROR: This is the DXNN model name deployed (e.g., "yolo26n", "EfficientNet_Lite0").
  // NOT the AI agent name. Writing "claude-sonnet-4.6", "gpt-4.1", or any AI model name
  // here will cause test_session_json_model_is_dx_model to FAIL.
  "pipeline_category": "<single_model|multi_model|cascaded|tiled|parallel|broker> (use underscores, NOT hyphens)",
  "task": "<Object Detection|Pose Estimation|Segmentation|Classification>",
  "postprocess_lib": "<libpostprocess_xxx.so>",
  "tracker": "<dxtracker|none>",
  "input_size": "<WxH e.g. 640x640>",
  "status": "complete",
  "notes": "<any relevant notes>"
}
```

**`session_id` MUST contain the agent identifier (R81).** Format: `YYYYMMDD-HHMMSS_<agent>_<model>_<task>`
where `<agent>` is one of: `claude`, `codex`, `copilot`, `cursor`, `opencode`.

```jsonc
// CORRECT:  "session_id": "20260429-155626_copilot_yolo26n_cascaded"
// WRONG:    "session_id": "20260429-155626_yolo26n_cascaded"  ← missing agent identifier
```

**`created_at` MUST include timezone offset.** Do NOT use `datetime.now().strftime(...)` without appending
timezone — downstream ISO 8601 parsers will reject timestamps without an offset.

```python
# Correct — includes +09:00 or equivalent local offset:
"created_at": datetime.now().astimezone().isoformat(timespec='seconds')

# Wrong — missing timezone:
"created_at": datetime.now().strftime('%Y-%m-%dT%H:%M:%S')
```

### When to Use Production Path

Only create files in production directories when the user EXPLICITLY says:
- "Add this to the production codebase"
- "Create this in the examples directory"
- "Make this a permanent addition"

Default behavior: ALWAYS use `dx-agent-dev/`.

## Usage

Invoke with `/dx-agent-stream-build-pipeline` or ask the dx-stream-builder agent.
Specify the pipeline category (single-model, multi-model, cascaded, tiled,
parallel, or broker), the model name, and the input source type.

## Template Compliance (MANDATORY)

The templates in this document are battle-tested against the actual dx_stream
runtime. Generated code MUST follow these templates exactly.

### Rules

1. **Follow the template structure** — use the template as the baseline for correctness.
   User-requested additions and modifications are allowed. Do NOT add unrequested
   features (no custom logging format, no FPS overlay unless asked)
2. **Default: single output sink** — `fpsdisplaysink` OR `fakesink`. If the user
   requests file output, replace the display sink with
   `videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=...`
3. **`--output` / `tee` is RECOMMENDED for detection pipelines** — Detection
   pipelines SHOULD include the `--output` argument in `parse_args()` and the `tee`
   code path for simultaneous display+recording, unless the user explicitly excludes
   it. When included, use the `tee name=t` pattern:
   ```
   tee name=t \
     t. ! queue ! videoconvert ! fpsdisplaysink sync=false \
     t. ! queue ! videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=output.mp4
   ```
   **CRITICAL: `x264enc` MUST include `tune=zerolatency`** — see Pitfall #14
4. **English log messages** — all `logger.info`/`logger.error` messages in English,
   matching the style of existing examples
5. **`--headless` flag** — always include it in `pipeline.py`. Additionally check
   `DISPLAY` env as fallback (see template)
7. **`run_<app>.sh` MUST delegate to `pipeline.py`** — the shell wrapper MUST invoke
   `python pipeline.py` with `--input` and `--model` arguments. Embedding `gst-launch-1.0`
   inline in `run_<app>.sh` is PROHIBITED (see Anti-Patterns below). The test
   `test_run_script_invokes_pipeline` enforces this and will fail on inline gst-launch.
8. **Path resolution** — `run_<app>.sh` uses `DX_STREAM_ROOT` (2 levels up from
   `dx-agent-dev/<session>/`), NOT the production 3-level-up pattern.
   See the `run_<app>.sh` template below for the exact code

### Anti-Patterns (NEVER Do)

- Using `x264enc` without `tune=zerolatency` (see Pitfall #14)
- Using relative model paths in `dxinfer model-path=` (must be absolute)
- Copying `SRC_DIR` calculation from production scripts in `dx_stream/pipelines/`
- Writing log messages in Korean or other non-English languages
- Adding unrequested features that complicate the pipeline (e.g., FPS overlay, auto-recording)
- **Embedding `gst-launch-1.0` inline in `run_<app>.sh`** — PROHIBITED. The run script
  MUST delegate to `python pipeline.py`, not call `gst-launch-1.0` directly. Example of
  the correct pattern:
  ```bash
  # CORRECT — delegates to pipeline.py:
  source "$DX_STREAM_ROOT/venv-dx_stream/bin/activate"
  python pipeline.py --input "$INPUT_VIDEO" --model "$MODEL_PATH" \
      --postprocess-lib /usr/local/share/gstdxstream/lib/{POSTPROCESS_LIB}

  # ANTI-PATTERN (PROHIBITED) — inline gst-launch embedding:
  gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO ! decodebin ! dxpreprocess ...
  ```
- **Omitting `dxrate` after RTSP decodebin** — always add `dxrate max-rate=30` in the
  RTSP source branch. Example: `urisourcebin uri=rtsp://... ! decodebin ! dxrate max-rate=30 ! dxpreprocess ...`
- **`created_at` without timezone offset** — always use `datetime.now().astimezone().isoformat(timespec='seconds')`

## Phase 0: Prerequisites Check

Before starting the build workflow, verify:

> **Working directory note**: All Phase 0 commands assume you are in the `dx_stream/` root.
> The sanity check script lives TWO levels up: `../../scripts/sanity_check.sh`
> (NOT `scripts/sanity_check.sh` — that path does not exist from `dx_stream/`).
> NEVER pipe through `| tail` or `| head` — they mask the exit code and may hide failures.

1. **dx-runtime**: `bash ../../scripts/sanity_check.sh --dx_rt`
   - ← **MANDATORY: run from `dx_stream/` root with the `../../` prefix. Do NOT truncate the path.**
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
source → dxpreprocess → queue → dxinfer → queue → dxpostprocess → queue → dxtracker → queue → dxosd → sink
```

> **DxTracker is RECOMMENDED for object-detection pipelines.** Pipelines whose
> task is object detection (yolo*, ssd, scrfd, efficient-det, etc.) SHOULD include
> `dxtracker` between `dxpostprocess` and `dxosd` by default. Use the config file
> `../../dx_stream/configs/tracker_config.json` (relative to the session directory).
> The user may explicitly exclude tracker if not needed for their use case.

---

## Template: pipeline.py (Python GStreamer Pipeline)

```python
#!/usr/bin/env python3
"""dx_stream pipeline: {TASK} with {MODEL}."""

import argparse
import logging
import os
from pathlib import Path
import sys

try:
    import pydxs  # noqa: F401 — confirms dx_stream venv is active
except ImportError:
    print("Error: pydxs not found. Activate the dx_stream venv first:", file=sys.stderr)
    print("  source ../../venv-dx_stream/bin/activate", file=sys.stderr)
    sys.exit(1)

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
        # Always add DxRate after decodebin to cap frame ingestion rate (convention #3)
        return f'urisourcebin uri={input_path} ! decodebin ! dxrate max-rate=30'
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

    # Tracker is mandatory for detection pipelines — always include with a resolved default path
    _default_tracker_cfg = str(
        (Path(__file__).resolve().parent / "../../dx_stream/configs/tracker_config.json").resolve()
    )
    tracker_config_path = args.tracker_config or _default_tracker_cfg
    tracker = f'queue max-size-buffers=1 ! dxtracker config-file-path={tracker_config_path} !'

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

> **CRITICAL — Delegation Rule**: `run_<app>.sh` MUST invoke `python pipeline.py`.
> Do NOT embed `gst-launch-1.0` inline — this fails `test_run_script_invokes_pipeline`.
>
> **CRITICAL — Path Resolution**: This script lives in `dx-agent-dev/<session>/`,
> which is **2 levels below** `dx_stream/` root. Do NOT copy the `SRC_DIR` calculation
> from production scripts (`dx_stream/pipelines/...`) — those are 4 levels deep.
> Always use the pattern below.

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ─── Path Resolution (dx-agent-dev context) ─────────────────────
# This script is at: dx_stream/dx-agent-dev/<session>/run_<app>.sh
# dx_stream root is 2 levels up.
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"

# ─── Virtual Environment ───────────────────────────────────────────
VENV_DIR="$DX_STREAM_ROOT/venv-dx_stream"
if [ -f "$VENV_DIR/bin/activate" ]; then
    # shellcheck source=/dev/null
    source "$VENV_DIR/bin/activate"
else
    echo "[WARN] dx_stream venv not found at $VENV_DIR — pydxs may be unavailable"
fi

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

# ─── Run pipeline.py ──────────────────────────────────────────────
# MANDATORY: delegate to pipeline.py — do NOT embed gst-launch-1.0 inline
POSTPROCESS_LIB="/usr/local/share/gstdxstream/lib/{POSTPROCESS_LIB}"
python pipeline.py \
    --input "$INPUT_VIDEO" \
    --model "$MODEL_PATH" \
    --postprocess-lib "$POSTPROCESS_LIB"
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

## Cascaded Pipeline Requirements (pipeline_category: cascaded)

When `pipeline_category` is `cascaded`, the following are **MANDATORY** in addition
to the standard single-model requirements. The E2E tests verify all of these:

1. **`secondary-mode=true`** — set this property on the `dxpreprocess`, `dxinfer`,
   and `dxpostprocess` elements that form the secondary inference stage. When
   `secondary-mode=true`, `DxPreprocess` automatically extracts and preprocesses
   individual object regions detected by the primary stage (ROI extraction is built in).
   The test `test_pipeline_has_secondary_mode` checks for its presence.

2. **Two `DxInfer` stages** — one for primary detection (e.g., yolo26n), one for
   secondary classification (e.g., EfficientNet_Lite0).
   The test `test_pipeline_has_two_inference_stages` verifies this.

3. **`pipeline_category: "cascaded"`** in `session.json` — the test
   `test_session_json_pipeline_category_is_cascaded` checks for this exact value.

4. **`tee` + `dxgather`** — branch the stream after `dxtracker` using GStreamer `tee`,
   run secondary inference on each branch with `secondary-mode=true`, then merge
   results with `dxgather` before `dxosd`.

### Cascaded Pipeline Pattern

```
source → dxpreprocess → dxinfer → dxpostprocess [primary detection]
       → dxtracker
       → tee name=t
         t. → queue ! dxpreprocess(secondary-mode=true, preprocess-id=2)
                    ! queue ! dxinfer(secondary-mode=true, preprocess-id=2, inference-id=2)
                    ! queue ! dxpostprocess(secondary-mode=true, inference-id=2)
                    ! queue ! gather.sink_0
       → dxgather name=gather → dxosd → sink
```

ROI extraction for secondary inference is handled **automatically** by `DxPreprocess`
when `secondary-mode=true` — no separate ROI extraction element is required or exists.

> **Reference implementation**: `dx_stream/pipelines/secondary_mode/run_secondary_mode.sh`

### Cascaded session.json Example

> **`model` field — CORRECT vs WRONG (R76):**
>
> ```
> "model": "yolo26n"           ← CORRECT — the DXNN model name being deployed
> "model": "claude-sonnet-4.6" ← WRONG — AI agent's own model identifier
> "model": "gpt-4.1"           ← WRONG — AI agent's own model identifier
> "model": "gemini-pro"        ← WRONG — AI agent's own model identifier
> ```
>
> The `model` field MUST contain the DX inference model name (e.g. `yolo26n`,
> `EfficientNet_Lite0`), NOT the name of the AI coding assistant running this session.

```jsonc
{
  "session_id": "YYYYMMDD-HHMMSS_<model>_cascaded",
  "created_at": "<ISO-8601-with-timezone>",
  "model": "yolo26n",
  // ⚠ HARD ERROR: This is the DXNN model name deployed (e.g., "yolo26n", "EfficientNet_Lite0").
  // NOT the AI agent name. Writing "claude-sonnet-4.6", "gpt-4.1", or any AI model name
  // here will cause test_session_json_model_is_dx_model to FAIL.
  "pipeline_category": "cascaded",
  "task": "Object Detection + Classification",
  "secondary_model": "EfficientNet_Lite0",
  "postprocess_lib": "<libpostprocess_xxx.so>",
  "secondary_postprocess_lib": "<libpostprocess_yyy.so>",
  "tracker": "<dxtracker|none>",
  "input_size": "640x640",
  "secondary_input_size": "224x224",
  "status": "complete",
  "notes": "Primary: yolo26n detection; Secondary: EfficientNet_Lite0 classification"
}
```

**The following fields are REQUIRED in every cascaded `session.json`:**
- `created_at` — ISO-8601 timestamp with local timezone
- `status` — must be `"complete"` when the session finishes
- `postprocess_lib` — shared object path for the primary model's postprocessor
- `secondary_postprocess_lib` — shared object path for the secondary model's postprocessor
- `tracker` — tracker element name (e.g. `"dxtracker"`) or `"none"` if not used

Omitting any of these fields produces an incomplete cascaded `session.json`.

> **CRITICAL**: `pipeline_category` MUST be `"cascaded"` — NOT `"single_model"`.
> Never copy `pipeline_category: "single_model"` from a single-model template for
> cascaded sessions. The test `test_session_json_pipeline_category_is_cascaded`
> will fail if this field contains any value other than `"cascaded"`.

### Pre-DONE Checklist (cascaded) — R58

Before outputting DONE for a cascaded session, verify ALL of the following exist:

- [ ] `pipeline.py` — verify `secondary-mode=true` and two `DxInfer` stages are present
- [ ] `run_cascaded.sh` (or `run_<app>.sh`) — verify it delegates to `pipeline.py`
- [ ] `session.json` — verify `pipeline_category="cascaded"` and `secondary_model` field are present
- [ ] `README.md` — **MANDATORY** (`test_readme_md_exists` WILL FAIL without it; do NOT skip)
- [ ] `setup.sh` — verify present (`test_setup_sh_exists` WILL FAIL without it)
- [ ] `run.sh` — verify present
- [ ] `session.log` — actual command output (never a hand-written summary)

> **R58 NOTE**: Cascaded sessions in fast autopilot mode frequently omit README.md
> and setup.sh because the agent focuses on pipeline.py and forgets documentation.
> These files are tested explicitly — a cascaded build without README.md or setup.sh
> is **incomplete** and MUST be corrected before outputting DONE.

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
- [ ] `session.json session_id` contains agent identifier (`claude`/`codex`/`copilot`/`cursor`/`opencode`) — R81

### README.md Required Sections (HARD-GATE)

Every `README.md` MUST contain ALL of the following sections. Missing sections
are a test failure (`test_readme_has_pipeline_diagram`, `test_readme_has_files_section`):

- [ ] **Prerequisites** — venv activation, GStreamer plugin check (`gst-inspect-1.0 dxinfer`), model download
- [ ] **How to Run** — `bash run_<app>.sh`, `python3 pipeline.py`, headless mode (`--headless`)
- [ ] **Pipeline Diagram** — ASCII `→` chain showing element flow (e.g., `source → DxPreprocess → DxInfer → DxPostprocess → DxOsd → sink`)
- [ ] **Configuration Table** — model name, task, input size, postprocess library
- [ ] **Files Table** — lists all generated artifacts: `pipeline.py`, `run_<app>.sh`, `session.json`, `README.md`, `setup.sh`, `run.sh`, `session.log`

### Conditional Files (create if applicable)

- [ ] Model downloaded: `dx_stream/samples/models/<model>.dxnn`
- [ ] Postprocess library: exists at `/usr/local/share/gstdxstream/lib/`
- [ ] Config files (if using config-file-path mode): `dx_stream/configs/<Model>/`
- [ ] Tracker config (if tracking): `dx_stream/configs/tracker_config.json`
- [ ] Broker config (if broker): `dx_stream/configs/broker_kafka.cfg` or `broker_mqtt.cfg`

### Verification Step

After creating all files, run this self-check:

```bash
SESSION_DIR="dx-agent-dev/<YYYYMMDD-HHMMSS>_<model>_<category>"
for f in session.json README.md run_*.sh pipeline.py; do
    [ -f "$SESSION_DIR/$f" ] && echo "OK: $f" || echo "MISSING: $f"
done

# model-path: accept absolute path (/...) or $VAR reference ($...)
MP=$(grep -oP 'model-path=\K\S+' "$SESSION_DIR"/run_*.sh 2>/dev/null | head -n1)
if [ -n "$MP" ]; then
    echo "$MP" | grep -qE '^[/$]' \
        && echo "model-path OK: $MP" \
        || { echo "model-path FAIL: $MP — must be absolute path or \$VAR reference"; exit 1; }
fi
```

If any mandatory file shows `MISSING`, create it before completing the build.

### session.log required content

`session.log` MUST contain output from **both** of the following stages, not just pipeline
runtime output. A pipeline that finishes in under 2 seconds produces only 4 lines of
Python logger output — that passes content checks, but pre-execution verification lines
make the log self-documenting and useful for debugging:

1. **Pre-execution checks** (at minimum one of):
   - Model path check: `ls -la "$MODEL_PATH"`
   - GStreamer element check: `gst-inspect-1.0 dxinfer 2>&1 | head -3`
   - Element factory count: `gst-inspect-1.0 dxinfer | grep -c factory`
2. **Pipeline execution output** — all stdout/stderr from `python pipeline.py`

Collect both stages with a single compound redirect:

```bash
{
  echo "=== Pre-execution checks ==="
  ls -la "$MODEL_PATH" 2>&1 || echo "[WARN] model path not found"
  gst-inspect-1.0 dxinfer 2>&1 | head -3
  echo "=== Pipeline execution ==="
  python pipeline.py --input "$INPUT_VIDEO" --model "$MODEL_PATH" \
      --postprocess-lib "$POSTPROCESS_LIB" 2>&1
} | tee session.log
```

> **IMPORTANT (R84)**: If your session.log contains only validation output (validate_app, gst-inspect, py_compile) and no GStreamer runtime lines, you have NOT completed the Verification Step — `python pipeline.py` has not been run.

> ⛔ **MANDATORY EXECUTION — HARD GATE (R72)**
> Running `gst-inspect-1.0` or the run-script validator is **NOT sufficient** to complete the session.
> The pipeline **MUST** be launched with `python pipeline.py ... 2>&1 | tee -a session.log`.
> If `session.log` does not contain the word `"Pipeline"` after this step, the session is **INCOMPLETE**.
> `test_session_log_has_pipeline_execution_evidence` **WILL FAIL** if this step is skipped.
> Completing validation and stopping WITHOUT running `python pipeline.py` is a **PROHIBITED anti-pattern** —
> validator output (`Validation: PASS`, `syntax: Shell syntax OK`) does NOT satisfy this gate.

> **MANDATORY — Explicit pipeline execution (R68):** `python pipeline.py` MUST be
> launched as a **separate, explicitly labelled step** in session.log. Running ONLY
> dx-stream's run-script validator (e.g., `validate_run_sh.py` or similar tools)
> without also running `python pipeline.py` is PROHIBITED. Validator output such as
> `[INFO] syntax: Shell syntax OK` and `Validation: PASS | 0 error(s)` does NOT
> contain GStreamer `Pipeline` output. `test_session_log_has_meaningful_content` WILL
> FAIL if the session.log contains only validator output. Use a separate tee block
> after any pre-checks:
>
> ```bash
> # MANDATORY: Explicit pipeline execution — MUST be a distinct step
> # Paste this output into the conversation to prove execution before DONE (R84)
> # Expected output must contain 'Pipeline' or 'PLAYING' or 'End of stream'
> # If output shows only import errors or is empty → fix pipeline.py first
> echo "=== pipeline execution ===" | tee -a session.log
> timeout 30 python pipeline.py \
>   --input "$INPUT_VIDEO" \
>   --model "$MODEL_PATH" \
>   --postprocess-lib "$POSTPROCESS_LIB" \
>   --headless \
>   2>&1 | tee -a session.log || true
>
> # Verify GStreamer output is present (session.log must contain "Pipeline")
> grep -q "Pipeline" session.log \
>   && echo "[OK] Pipeline keyword confirmed in session.log" | tee -a session.log \
>   || echo "[WARNING] Pipeline keyword not found — pipeline may not have executed" | tee -a session.log
> echo "=== pipeline.py execution verified ===" | tee -a session.log
> ```

This ensures `session.log` has substantive content (≥10 non-empty lines) regardless of
how quickly the pipeline finishes, satisfying `test_session_log_has_meaningful_content`.

> **Portable grep idiom (MANDATORY in all in-session verification scripts)**:
> Do NOT use `rg` (ripgrep) directly — it is not universally installed and its
> absence produces `rg: command not found` noise in `session.log`. Always use
> `grep` or the portable fallback:
> ```bash
> RG=$(command -v rg 2>/dev/null || echo grep)
> $RG "queue max-size-buffers" pipeline.py
> ```

#### Cascaded verification in session.log (R64)

For **cascaded** pipelines, add this self-check immediately before launching the
pipeline, and log the result to `session.log`:

```bash
grep -q "secondary-mode=true" pipeline.py \
  && echo "[OK] secondary-mode=true found in pipeline.py" | tee -a session.log \
  || { echo "[FAIL] secondary-mode=true missing — secondary stage not configured" | tee -a session.log; exit 1; }
```

This converts `session.log` into a self-verifying artifact: if `[OK] secondary-mode=true found`
appears in the log, it confirms the secondary inference stage is correctly configured.

#### ⛔ CASCADED HARD GATE — Pre-DONE self-check (R83)

**You MUST run these checks and show the `[MANDATORY-ALL-OK]` line in the conversation before outputting `[DX-AGENT-DEV: DONE ...]`. A DONE sentinel emitted without `[MANDATORY-ALL-OK]` evidence is INVALID.**

Before outputting `[DX-AGENT-DEV: DONE ...]`, run this compound self-check and paste the output into the conversation. If any check produces `[MANDATORY-FAIL]`, the session is **INCOMPLETE** — do NOT output DONE until the issue is resolved:

```bash
# ⛔ CASCADED MANDATORY CHECK — paste output into conversation before DONE (R83)
echo "=== Cascaded mandatory checks ===" | tee -a session.log
grep -q "secondary-mode=true" pipeline.py \
  && echo "[MANDATORY-OK] secondary-mode=true present in pipeline.py" | tee -a session.log \
  || { echo "[MANDATORY-FAIL] secondary-mode=true ABSENT — secondary stage not configured" | tee -a session.log; exit 1; }
grep -q "pipeline.py" run_cascaded*.sh run_*.sh 2>/dev/null \
  && echo "[MANDATORY-OK] run script delegates to pipeline.py" | tee -a session.log \
  || { echo "[MANDATORY-FAIL] run script uses inline gst-launch — do NOT emit DONE" | tee -a session.log; exit 1; }
grep -qE "(--output|tee name=)" pipeline.py \
  && echo "[MANDATORY-OK] output recording present" | tee -a session.log \
  || { echo "[MANDATORY-FAIL] --output/tee recording absent — do NOT emit DONE" | tee -a session.log; exit 1; }
echo "[MANDATORY-ALL-OK] All cascaded checks passed" | tee -a session.log
```

This check catches the three most common cascaded regressions:
- Missing `secondary-mode=true` (secondary inference stage not properly configured)
- Inline `gst-launch-1.0` in run script (agent wrote self-contained launcher instead of delegating)
- Missing `--output`/`tee` recording (agent omitted the output recording code path)

## setup.sh Template (MANDATORY)

> **CRITICAL**: `setup.sh` must be runnable standalone. A user must be able to `cd` into
> the session directory and run `./setup.sh` without manually activating any venv first.

```bash
#!/bin/bash
# Environment setup for <Model> <Category> Pipeline
# Generated by DX Agent-Driven Dev

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
# Generated by DX Agent-Driven Dev

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- Activate venv ---
source "$SCRIPT_DIR/setup.sh" 2>/dev/null || true

# --- Default paths ---
# Model (absolute path required by GStreamer)
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"
DEFAULT_MODEL="${SRC_DIR}/samples/models/{MODEL}.dxnn"
# Video input
DEFAULT_VIDEO="${SRC_DIR}/samples/videos/{VIDEO}"

MODEL="${1:-$DEFAULT_MODEL}"
VIDEO="${2:-$DEFAULT_VIDEO}"

if [ ! -f "$MODEL" ]; then
    echo "[ERROR] Model not found: $MODEL"
    echo "Usage: bash run.sh [model_path] [video_path]"
    echo ""
    echo "Model locations:"
    echo "  dx_stream: ${SRC_DIR}/samples/models/{MODEL}.dxnn"
    echo "  dx_app:    ${DX_STREAM_ROOT}/../dx_app/assets/models/{MODEL}.dxnn"
    exit 1
fi

echo "[INFO] Model: $MODEL"
echo "[INFO] Video: $VIDEO"
bash "$SCRIPT_DIR/run_{APP_NAME}.sh" "$VIDEO"
```
