#!/bin/bash
# Example: dxinfer (dxvnpu backend) — VNPU inference per channel
# N-channel: source → decodebin → dxpreprocess → dxinfer(dxvnpu) →
#            dxpostprocess → dxosd → dxscale → compositor → display
#
# Usage:
#   ./run_dxvnpuinf.sh [channels]
#   Example: ./run_dxvnpuinf.sh
#            ./run_dxvnpuinf.sh 4

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

VIDEO_DIR="${SRC_DIR}/samples/videos"
CHANNELS="${1:-4}"

OUTPUT_WIDTH=1280
OUTPUT_HEIGHT=720

# Model auto-download logic
MODEL_NAME="YoloV5S_PPU.dxnn"
MODEL_PATH="${SRC_DIR}/samples/models/${MODEL_NAME}"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] ${MODEL_NAME} not found in samples/models. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download ${MODEL_NAME}"
        exit 1
    fi
fi

# Collect video files
VIDEOS=()
for f in "${VIDEO_DIR}"/*.mp4 "${VIDEO_DIR}"/*.mov; do
    [ -f "$f" ] && VIDEOS+=("$f")
done

if [ ${#VIDEOS[@]} -eq 0 ]; then
    echo "[ERROR] No video files found in ${VIDEO_DIR}"
    exit 1
fi

# Check OS version for video sink
if [ "$(lsb_release -rs 2>/dev/null)" = "18.04" ]; then
    VIDEO_SINK_ARGS="video-sink=ximagesink"
else
    VIDEO_SINK_ARGS=""
fi

VIDEOCONVERT_PIPELINE="videoconvert"

# Calculate grid layout
cols=1
while [ $((cols * cols)) -lt "$CHANNELS" ]; do
    cols=$((cols + 1))
done
rows=$(( (CHANNELS + cols - 1) / cols ))

STREAM_WIDTH=$(( (OUTPUT_WIDTH / cols) / 2 * 2 ))
STREAM_HEIGHT=$(( (OUTPUT_HEIGHT / rows) / 2 * 2 ))

echo "============================================"
echo " dxinfer (dxvnpu backend)"
echo " Channels: ${CHANNELS}"
echo " Available videos: ${#VIDEOS[@]}"
echo " Model: ${MODEL_NAME}"
echo " Grid: ${rows}x${cols} (${STREAM_WIDTH}x${STREAM_HEIGHT} each)"
echo " Output: ${OUTPUT_WIDTH}x${OUTPUT_HEIGHT}"
echo "============================================"

# Build per-channel pipelines
PIPELINE=""
COMPOSITOR_PROPS=""
for ((i=0; i<CHANNELS; i++)); do
    VIDEO="${VIDEOS[$((i % ${#VIDEOS[@]}))]}"
    echo " Channel ${i}: $(basename "${VIDEO}")"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    PIPELINE+=" urisourcebin uri=file://${VIDEO} ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 !"
    PIPELINE+=" dxpreprocess config-file-path=${SRC_DIR}/configs/YoloV5S_PPU/preprocess_config.json ! queue max-size-buffers=10 !"
    PIPELINE+=" dxinfer config-file-path=${SRC_DIR}/configs/YoloV5S_PPU/inference_config.json ! queue max-size-buffers=10 !"
    PIPELINE+=" dxpostprocess config-file-path=${SRC_DIR}/configs/YoloV5S_PPU/postprocess_config.json ! queue max-size-buffers=10 !"
    PIPELINE+=" dxosd ! queue max-size-buffers=10 ! dxscale width=${STREAM_WIDTH} height=${STREAM_HEIGHT} ! queue max-size-buffers=10 ! comp.sink_${i}"

    COMPOSITOR_PROPS+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos}"
done

echo "============================================"

LAUNCH_CMD="GST_DEBUG=dxscale:5 gst-launch-1.0 -e ${PIPELINE} compositor name=comp ${COMPOSITOR_PROPS} ! ${VIDEOCONVERT_PIPELINE} ! fpsdisplaysink sync=false ${VIDEO_SINK_ARGS}"

echo "[CMD] ${LAUNCH_CMD}"
echo "============================================"

GST_DEBUG=dxinfer:3 eval "${LAUNCH_CMD}"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "[SUCCESS] dxinfer(dxvnpu) example completed (${CHANNELS} channels)"
else
    echo "[ERROR] dxinfer(dxvnpu) example failed (exit code: $EXIT_CODE)"
fi
exit $EXIT_CODE


