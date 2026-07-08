# dx_stream Testing Patterns

## Pipeline Validation

### GStreamer Debug Levels

Set `GST_DEBUG` to control verbosity:

```bash
# Level 0: No output
# Level 1: ERROR only
# Level 2: WARNING + ERROR
# Level 3: FIXME + WARNING + ERROR
# Level 4: INFO + all above
# Level 5: DEBUG + all above
# Level 6: LOG + all above (very verbose)
# Level 7: TRACE (extremely verbose)

# Global debug level
GST_DEBUG=3 gst-launch-1.0 ...

# Per-element debug
GST_DEBUG=dxinfer:5,dxpostprocess:4 gst-launch-1.0 ...

# Log to file
GST_DEBUG=4 GST_DEBUG_FILE=/tmp/gst-debug.log gst-launch-1.0 ...
```

### Pipeline Parse Test

Validate pipeline syntax without running:

```bash
# Parse-only validation (checks element existence and link compatibility)
gst-launch-1.0 --gst-print-args <pipeline_string> 2>&1

# Verify specific element is registered
gst-inspect-1.0 dxinfer
gst-inspect-1.0 dxpostprocess
gst-inspect-1.0 dxtracker
```

### Element Registration Check

```bash
# List all dx_stream elements
gst-inspect-1.0 | grep dx

# Expected output:
# dxstream: dxpreprocess
# dxstream: dxinfer
# dxstream: dxpostprocess
# dxstream: dxtracker
# dxstream: dxosd
# dxstream: dxgather
# dxstream: dxinputselector
# dxstream: dxoutputselector
# dxstream: dxrate
# dxstream: dxmsgconv
# dxstream: dxmsgbroker
# dxstream: dxscale
# dxstream: dxconvert
```

## GST_TRACERS for Profiling

```bash
# Frame rate tracer
GST_TRACERS="framerate" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Latency tracer
GST_TRACERS="latency" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Buffer flow stats
GST_TRACERS="stats" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Combined tracers
GST_TRACERS="framerate;latency" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...
```

If GstShark is installed (via `install_gstshark.sh`):

```bash
GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;framerate" \
GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...
```

## Model-Specific Test Pipelines

### Object Detection (yolo26n)

```bash
gst-launch-1.0 videotestsrc num-buffers=30 ! video/x-raw,width=640,height=640 ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolo26od.so \
        function-name=PostProcess ! queue ! \
    fakesink
```

### Face Detection (SCRFD500M)

```bash
gst-launch-1.0 videotestsrc num-buffers=30 ! video/x-raw,width=640,height=640 ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_scrfd500m.so \
        function-name=PostProcess ! queue ! \
    fakesink
```

### Pose Estimation (yolo26n-pose)

```bash
gst-launch-1.0 videotestsrc num-buffers=30 ! video/x-raw,width=640,height=640 ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolo26pose.so \
        function-name=PostProcess ! queue ! \
    fakesink
```

### Headless Validation (no display)

```bash
# Use fakesink instead of fpsdisplaysink for headless environments
gst-launch-1.0 ... ! dxosd ! fakesink sync=false

# Or check DISPLAY before choosing sink
if [ -z "$DISPLAY" ]; then
    SINK="fakesink sync=false"
else
    SINK="videoconvert ! fpsdisplaysink sync=false"
fi
```

## Test Script Structure

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"

# Test: Verify pipeline runs without errors
echo "=== Test: Single Network Object Detection ==="
MODEL_PATH="$SRC_DIR/samples/models/yolo26-n_640x640.dxnn"

if [ ! -f "$MODEL_PATH" ]; then
    echo "SKIP: Model not downloaded"
    exit 0
fi

# Run pipeline with limited frames
timeout 30 gst-launch-1.0 \
    videotestsrc num-buffers=10 ! video/x-raw,width=640,height=640,format=RGB ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! \
    dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL_PATH ! queue ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolo26od.so \
        function-name=PostProcess ! queue ! \
    fakesink sync=false

if [ $? -eq 0 ]; then
    echo "PASS"
else
    echo "FAIL"
    exit 1
fi
```

## Python Test with pydxs Probes

```python
#!/usr/bin/env python3
"""Test metadata extraction with pydxs bindings."""

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

import pydxs

Gst.init(None)


def probe_callback(pad, info):
    buffer = info.get_buffer()
    frame_meta = pydxs.DXFrameMeta.from_buffer(buffer)
    if frame_meta:
        for obj in frame_meta.objects:
            print(f"  class={obj.class_id} conf={obj.confidence:.2f} "
                  f"bbox=({obj.x:.0f},{obj.y:.0f},{obj.w:.0f},{obj.h:.0f})")
    return Gst.PadProbeReturn.OK


pipeline = Gst.parse_launch("... ! dxpostprocess name=pp ... ! fakesink")
pp = pipeline.get_by_name("pp")
pp.get_static_pad("src").add_probe(
    Gst.PadProbeType.BUFFER, probe_callback
)

pipeline.set_state(Gst.State.PLAYING)
loop = GLib.MainLoop()
loop.run()
```

## Continuous Integration Patterns

```bash
# 1. Check plugin registration
gst-inspect-1.0 dxinfer > /dev/null 2>&1 || { echo "FAIL: dxinfer not registered"; exit 1; }

# 2. Validate all pipeline scripts parse correctly
for script in dx_stream/pipelines/*/run_*.sh; do
    bash -n "$script" || { echo "FAIL: syntax error in $script"; exit 1; }
done

# 3. Check postprocess libraries exist
for lib in /usr/local/share/gstdxstream/lib/libpostprocess_*.so; do
    nm -D "$lib" | grep -q PostProcess || { echo "FAIL: PostProcess not exported in $lib"; exit 1; }
done
```
