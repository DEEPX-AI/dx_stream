#!/bin/bash
# Example: dxvnpuenc — VNPU hardware encoder
# N-channel: video → dxvnpudec (NV12) → dxvnpuenc → file/fakesink
#
# Usage:
#   ./run_dxvnpuenc.sh [channels] [h264|h265] [bitrate_kbps]
#   Example: ./run_dxvnpuenc.sh
#            ./run_dxvnpuenc.sh 4
#            ./run_dxvnpuenc.sh 2 h265
#            ./run_dxvnpuenc.sh 4 h264 8000

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

VIDEO_DIR="${SRC_DIR}/samples/videos"
OUTPUT_DIR="${SCRIPT_DIR}/output"
CHANNELS="${1:-1}"
CODEC="${2:-h264}"
BITRATE="${3:-4096}"

# Collect video files
VIDEOS=()
for f in "${VIDEO_DIR}"/*.mp4 "${VIDEO_DIR}"/*.mov; do
    [ -f "$f" ] && VIDEOS+=("$f")
done

if [ ${#VIDEOS[@]} -eq 0 ]; then
    echo "[ERROR] No video files found in ${VIDEO_DIR}"
    exit 1
fi

echo "============================================"
echo " dxvnpuenc Example (VNPU Hardware Encoder)"
echo " Channels: ${CHANNELS}"
echo " Available videos: ${#VIDEOS[@]}"
echo " Codec: ${CODEC}"
echo " Bitrate: ${BITRATE} kbps"
echo "============================================"

if [ "$CHANNELS" -eq 1 ]; then
    # Single channel: decode → encode → save to file
    VIDEO="${VIDEOS[0]}"
    echo " Channel 0: $(basename "${VIDEO}")"

    mkdir -p "${OUTPUT_DIR}"
    OUTPUT_FILE="${OUTPUT_DIR}/output.${CODEC}"
    echo " Output: ${OUTPUT_FILE}"
    echo "============================================"

    gst-launch-1.0 -v \
        urisourcebin uri=file://${VIDEO} ! parsebin ! \
        dxvnpudec output-format=NV12 ! \
        dxvnpuenc codec=${CODEC} bitrate=${BITRATE} ! \
        filesink location=${OUTPUT_FILE}
else
    # Multi-channel: decode → encode → fakesink
    PIPELINE=""
    for ((i=0; i<CHANNELS; i++)); do
        VIDEO="${VIDEOS[$((i % ${#VIDEOS[@]}))]}"
        echo " Channel ${i}: $(basename "${VIDEO}")"
        if [ -n "$PIPELINE" ]; then
            PIPELINE+=" "
        fi
        PIPELINE+="urisourcebin uri=file://${VIDEO} ! parsebin ! dxvnpudec output-format=NV12 ! dxvnpuenc codec=${CODEC} bitrate=${BITRATE} ! fakesink sync=false"
    done
    echo "============================================"

    GST_DEBUG=dxvnpuenc:3,dxvnpudec:3 gst-launch-1.0 ${PIPELINE}
fi

EXIT_CODE=$?
if [ $EXIT_CODE -eq 0 ]; then
    echo "[SUCCESS] dxvnpuenc example completed (${CHANNELS} channels, codec=${CODEC})"
else
    echo "[ERROR] dxvnpuenc example failed (exit code: $EXIT_CODE)"
fi
exit $EXIT_CODE
