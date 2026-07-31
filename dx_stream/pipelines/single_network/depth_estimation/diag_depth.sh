#!/bin/bash
# Diagnostic: isolate WHERE the depth pipeline SIGSEGVs (dxinfer / dxpostprocess / dxosd).
# Runs 3 headless stages with fakesink; each bounded by timeout. Reports per-stage verdict.
set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"   # -> dx_stream/dx_stream

MODEL="$(readlink -f "$SRC_DIR/samples/models" 2>/dev/null)/yolo26n-depth_640x640.dxnn"
VID="${1:-$(readlink -f "$SRC_DIR/samples/videos" 2>/dev/null)/blackbox-city-road.mp4}"
LIB="/usr/local/share/gstdxstream/lib/libpostprocess_yolo26depth.so"
N="${NUM_FRAMES:-10}"
TMO="${TMO:-30}"

echo "MODEL=$MODEL"
echo "VID=$VID"
[ -f "$MODEL" ] || { echo "FATAL: model missing"; exit 1; }
[ -f "$VID" ]   || { echo "FATAL: video missing"; exit 1; }
echo

PRE="uridecodebin uri=file://$VID ! videoconvert ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! queue ! dxinfer preprocess-id=1 inference-id=1 model-path=$MODEL ! queue"

run_stage () {
    local name="$1"; shift
    local tail="$1"
    local log="$SCRIPT_DIR/_diag_${name}.log"
    echo "========== STAGE $name =========="
    stdbuf -oL -eL timeout -s KILL "$TMO" gst-launch-1.0 $PRE ! $tail > "$log" 2>&1
    local rc=$?
    if grep -q "Caught SIGSEGV\|invalid pointer\|Segmentation" "$log"; then
        echo "STAGE $name => CRASH (SIGSEGV/abort)  [rc=$rc]"
    elif [ $rc -eq 137 ] || [ $rc -eq 124 ]; then
        echo "STAGE $name => HANG/timeout-killed  [rc=$rc]"
    elif grep -q "ERROR" "$log"; then
        echo "STAGE $name => ERROR (see log)  [rc=$rc]"
    else
        echo "STAGE $name => OK  [rc=$rc]"
    fi
    tail -4 "$log" | sed 's/^/    /'
    echo
}

# A: inference only (no custom postprocess, no osd)
run_stage A_infer "identity eos-after=$N ! fakesink"
# B: + custom depth postprocess (writes frame_meta->_depth_data)
run_stage B_postproc "dxpostprocess inference-id=1 library-file-path=$LIB function-name=PostProcess ! queue ! identity eos-after=$N ! fakesink"
# C: + dxosd depth colormap render (BGR path)
run_stage C_osd "dxpostprocess inference-id=1 library-file-path=$LIB function-name=PostProcess ! queue ! videoconvert ! video/x-raw,format=BGR ! dxosd ! identity eos-after=$N ! fakesink"

echo "========== SUMMARY =========="
echo "First stage marked CRASH/ERROR/HANG = the culprit boundary."
echo "logs: _diag_A_infer.log _diag_B_postproc.log _diag_C_osd.log"
