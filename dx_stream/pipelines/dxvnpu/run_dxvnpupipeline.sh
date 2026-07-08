#!/bin/bash
# Example: dxvnpupipeline — VNPU all-in-one pipeline (decode → inference → postprocess → overlay)
# N-channel bitstream → dxvnpupipeline → dxpostprocess → dxvnpuoverlay → HDMI output
#
# Usage:
#   ./run_dxvnpupipeline.sh [channels]
#   Example: ./run_dxvnpupipeline.sh
#            ./run_dxvnpupipeline.sh 4

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

VIDEO_DIR="${SRC_DIR}/samples/videos"
CHANNELS="${1:-2}"
INFER_ID=0
DEVICE_ID=0

# Model auto-download logic
MODEL_NAME="yolo26n.dxnn"
MODEL_PATH="${SRC_DIR}/samples/models/${MODEL_NAME}"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] ${MODEL_NAME} not found in samples/models. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download ${MODEL_NAME}"
        exit 1
    fi
fi

POSTPROCESS_LIB="/usr/local/share/gstdxstream/lib/libpostprocess_yolo26od.so"

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
echo " dxvnpupipeline Example (VNPU Pipeline)"
echo " Channels: ${CHANNELS}"
echo " Available videos: ${#VIDEOS[@]}"
echo " Model: ${MODEL_NAME}"
echo " Device: ${DEVICE_ID}"
echo "============================================"

# Build pipeline
PIPELINE="dxvnpupipeline name=vp \
    model-path=${MODEL_PATH} \
    inference-id=${INFER_ID} \
    keep-ratio=true \
    use-ort=true \
    device-id=${DEVICE_ID} \
    max-hdmi-channels=${CHANNELS} \
    use-vnpu-hdmi=true"

# Add source branches (round-robin video files)
for ((i=0; i<CHANNELS; i++)); do
    VIDEO="${VIDEOS[$((i % ${#VIDEOS[@]}))]}"
    PIPELINE+=" urisourcebin uri=file://${VIDEO} ! parsebin ! vp.sink_${i}"
    echo " Channel ${i}: $(basename "${VIDEO}")"
done

# Add output branches
for ((i=0; i<CHANNELS; i++)); do
    PIPELINE+=" vp.src_${i} ! queue max-size-buffers=1 leaky=downstream !"
    PIPELINE+=" dxpostprocess inference-id=${INFER_ID}"
    PIPELINE+="   library-file-path=${POSTPROCESS_LIB}"
    PIPELINE+="   function-name=PostProcess !"
    PIPELINE+=" dxvnpuoverlay model-path=${MODEL_PATH}"
    PIPELINE+="   keep-ratio=true device-id=${DEVICE_ID}"
done

echo "============================================"

GST_DEBUG=dxvnpupipeline:3,dxvnpuoverlay:3 gst-launch-1.0 ${PIPELINE}
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    echo "[SUCCESS] dxvnpupipeline example completed (${CHANNELS} channels)"
else
    echo "[ERROR] dxvnpupipeline example failed (exit code: $EXIT_CODE)"
fi
exit $EXIT_CODE
