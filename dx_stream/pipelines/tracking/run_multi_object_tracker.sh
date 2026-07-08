#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Model auto-download logic
MODEL_NAME="yolov5-s_640x640_ppu.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found in samples/models. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/blackbox-city-road.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road2.mov"
    "$SRC_DIR/samples/videos/carrierbag.mp4"
    "$SRC_DIR/samples/videos/cctv-city-road2.mov"
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
)

# Default videoconvert pipeline
VIDEOCONVERT_PIPELINE="videoconvert"

# NOTE: If you experience rendering issues (corrupted/distorted video output) on Orange Pi 5 Plus
# with Debian 12, uncomment the lines below to force I420 format conversion.
# See troubleshooting documentation for more details.
# if grep -q "rk3588" /proc/device-tree/compatible 2>/dev/null; then
#     if [ "$(lsb_release -rs)" = "12" ]; then
#         echo "Detected Orange Pi 5 Plus with Debian 12 - using I420 format"
#         VIDEOCONVERT_PIPELINE="videoconvert ! video/x-raw,format=I420"
#     fi
# fi

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! decodebin ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/YoloV5S_PPU/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/postprocess_config.json ! queue ! \
                    dxtracker config-file-path=$SRC_DIR/configs/tracker_config.json ! queue ! \
                    dxosd ! queue ! \
                    $VIDEOCONVERT_PIPELINE ! fpsdisplaysink sync=false
done


