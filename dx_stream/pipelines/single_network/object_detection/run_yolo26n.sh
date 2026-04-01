#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

# Model auto-download logic
MODEL_NAME="yolo26n.dxnn"
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
    "$SRC_DIR/samples/videos/boat.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road.mp4"
    "$SRC_DIR/samples/videos/cctv-city-road2.mov"
    "$SRC_DIR/samples/videos/dance-group.mov"
)

if [ "$(lsb_release -rs)" = "18.04" ]; then
    echo -e "Using X11 video sink forcely on ubuntu 18.04"
    VIDEO_SINK_ARGS="video-sink=ximagesink"
else
    VIDEO_SINK_ARGS=""
fi

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
                    dxpreprocess \
                        preprocess-id=1 \
                        resize-width=640 \
                        resize-height=640 ! \
                    queue max-size-buffers=1 ! \
                    dxinfer \
                        preprocess-id=1 \
                        inference-id=1 \
                        model-path=$MODEL_PATH ! \
                    queue max-size-buffers=1 ! \
                    dxpostprocess \
                        inference-id=1 \
                        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_yolo26od.so \
                        function-name=PostProcess ! \
                    queue max-size-buffers=1 ! \
                    dxosd ! \
                    $VIDEOCONVERT_PIPELINE ! fpsdisplaysink sync=false $VIDEO_SINK_ARGS
done
