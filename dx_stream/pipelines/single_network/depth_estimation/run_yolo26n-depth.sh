#!/bin/bash
# YOLO26n-Depth monocular depth estimation pipeline (DX-M1 NPU).
#
#   src -> dxpreprocess -> dxinfer -> dxpostprocess(libpostprocess_yolo26depth.so)
#       -> dxosd (MAGMA depth colormap overlay) -> BGR -> sink
#
# The DxOsd depth path renders on BGR, so we force video/x-raw,format=BGR before
# dxosd. Live view uses fpsdisplaysink; set SAVE=1 (or run without a DISPLAY) to
# dump annotated JPEG frames to ./depth_out/ instead.
set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")"   # -> dx_stream/dx_stream

MODEL_NAME="yolo26n-depth_640x640.dxnn"

# ---- model resolution: samples/models -> dx_app assets (suite) -> compiler out --
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    # walk up to the suite root and try known staged locations
    _d="$SCRIPT_DIR"
    while [ "$_d" != / ]; do
        if [ -f "$_d/dx-runtime/dx_app/assets/models/$MODEL_NAME" ]; then
            MODEL_PATH="$_d/dx-runtime/dx_app/assets/models/$MODEL_NAME"; break
        fi
        _c=$(ls "$_d"/dx-compiler/dx-agent-dev/*yolo26ndepth_compile/yolo26n-depth.dxnn 2>/dev/null | tail -1)
        if [ -n "$_c" ]; then MODEL_PATH="$_c"; break; fi
        _d="$(dirname "$_d")"
    done
fi
if [ ! -f "$MODEL_PATH" ]; then
    echo "[ERROR] $MODEL_NAME not found. Compile it first (dx-compiler yolo26n-depth session)"
    echo "        then copy to: $SRC_DIR/samples/models/$MODEL_NAME"
    exit 1
fi
echo "[INFO] model: $MODEL_PATH"

# ---- input media: arg1 (video or image); else a bundled sample video ----------
INPUT="${1:-}"
if [ -z "$INPUT" ]; then
    for v in "$SRC_DIR"/samples/videos/*.mp4 "$SRC_DIR"/samples/videos/*.mov; do
        [ -f "$v" ] && { INPUT="$v"; break; }
    done
fi
if [ -z "$INPUT" ] || [ ! -f "$INPUT" ]; then
    echo "[ERROR] no input media. Usage: $(basename "$0") <video.mp4|image.jpg>"
    exit 1
fi
echo "[INFO] input: $INPUT"

# ---- source branch: still image (imagefreeze) vs video (decodebin) -------------
ext="${INPUT##*.}"; ext="${ext,,}"
case "$ext" in
    jpg|jpeg|png|bmp) SRC_BRANCH="filesrc location=$INPUT ! decodebin ! imagefreeze ! videoconvert" ;;
    *)                SRC_BRANCH="urisourcebin uri=file://$INPUT ! decodebin ! videoconvert" ;;
esac

# ---- sink branch: display vs headless JPEG dump --------------------------------
if [ "${SAVE:-0}" = "1" ] || [ -z "${DISPLAY:-}" ]; then
    OUT_DIR="${OUT_DIR:-$SCRIPT_DIR/depth_out}"; mkdir -p "$OUT_DIR"
    NUM="${NUM_FRAMES:-30}"
    echo "[INFO] headless: writing up to $NUM frames to $OUT_DIR"
    SINK_BRANCH="identity eos-after=$NUM ! videoconvert ! jpegenc ! multifilesink location=$OUT_DIR/depth_%03d.jpg"
else
    SINK_BRANCH="fpsdisplaysink sync=false"
fi

gst-launch-1.0 -e $SRC_BRANCH ! \
    dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ! \
    queue max-size-buffers=1 ! \
    dxinfer preprocess-id=1 inference-id=1 model-path="$MODEL_PATH" ! \
    queue max-size-buffers=1 ! \
    dxpostprocess inference-id=1 \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolo26depth.so \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    videoconvert ! video/x-raw,format=BGR ! \
    dxosd ! \
    $SINK_BRANCH
