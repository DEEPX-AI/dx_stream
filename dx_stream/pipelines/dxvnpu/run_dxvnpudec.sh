#!/bin/bash
# Example: dxvnpudec — VNPU hardware decoder (multi-channel grid display)
# N-channel: H.264/H.265 bitstream → dxvnpudec (scaled output) → compositor → display
#
# Usage:
#   ./run_dxvnpudec.sh [channels]
#   Example: ./run_dxvnpudec.sh
#            ./run_dxvnpudec.sh 4

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

VIDEO_DIR="${SRC_DIR}/samples/videos"
CHANNELS="${1:-2}"

OUTPUT_WIDTH=1280
OUTPUT_HEIGHT=720

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
    echo "[INFO] Using X11 video sink on Ubuntu 18.04"
    VIDEO_SINK_ARGS="video-sink=ximagesink"
else
    VIDEO_SINK_ARGS=""
fi

# Default videoconvert pipeline
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
echo " dxvnpudec Example (VNPU Hardware Decoder)"
echo " Channels: ${CHANNELS}"
echo " Available videos: ${#VIDEOS[@]}"
echo " Grid: ${rows}x${cols} (${STREAM_WIDTH}x${STREAM_HEIGHT} each)"
echo " Output: ${OUTPUT_WIDTH}x${OUTPUT_HEIGHT}"
echo "============================================"

# Build multi-channel pipeline with compositor
PIPELINE=""
COMPOSITOR_PROPS=""

for ((i=0; i<CHANNELS; i++)); do
    VIDEO="${VIDEOS[$((i % ${#VIDEOS[@]}))]}"
    echo " Channel ${i}: $(basename "${VIDEO}")"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    PIPELINE+=" urisourcebin uri=file://${VIDEO} ! parsebin !"
    PIPELINE+=" dxvnpudec output-format=NV12 output-width=${STREAM_WIDTH} output-height=${STREAM_HEIGHT} !"
    PIPELINE+=" queue max-size-buffers=1 ! comp.sink_${i}"

    COMPOSITOR_PROPS+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos}"
done

echo "============================================"

LAUNCH_CMD="gst-launch-1.0 ${PIPELINE} compositor name=comp ${COMPOSITOR_PROPS} ! ${VIDEOCONVERT_PIPELINE} ! fpsdisplaysink sync=true ${VIDEO_SINK_ARGS}"

GST_DEBUG=dxvnpudec:3 eval "${LAUNCH_CMD}"
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "[SUCCESS] dxvnpudec example completed (${CHANNELS} channels)"
else
    echo "[ERROR] dxvnpudec example failed (exit code: $EXIT_CODE)"
fi
exit $EXIT_CODE
