#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Model auto-download logic (deduplicated)
download_model_if_missing() {
    local model_name="$1"
    local model_path="$SRC_DIR/samples/models/$model_name"
    if [ ! -f "$model_path" ]; then
        echo "[INFO] $model_name not found in samples/models. Downloading..."
        (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
        if [ ! -f "$model_path" ]; then
            echo "[ERROR] Failed to download $model_name"
            exit 1
        fi
    fi
}

for model in "YoloV5S_PPU.dxnn" "EfficientNet_Lite0.dxnn" "SCRFD500M.dxnn"; do
    download_model_if_missing "$model"
done

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
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
                    dxpreprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/YoloV5S_PPU/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/postprocess_config.json ! queue ! \
                    dxtracker config-file-path=$SRC_DIR/configs/tracker_config.json ! queue ! \
                    tee name=t \
                    t. ! queue ! \
                    dxpreprocess \
                        preprocess-id=2 \
                        resize-width=224 \
                        resize-height=224 \
                        secondary-mode=true \
                        interval=5 \
                        min-object-width=50 \
                        min-object-height=50 \
                        keep-ratio=false ! \
                    queue max-size-buffers=1 ! \
                    dxinfer \
                        preprocess-id=2 \
                        inference-id=2 \
                        secondary-mode=true \
                        model-path=$SRC_DIR/samples/models/EfficientNet_Lite0.dxnn ! \
                    queue max-size-buffers=1 ! \
                    dxpostprocess \
                        inference-id=2 \
                        secondary-mode=true \
                        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_object_class.so \
                        function-name=PostProcess ! \
                    queue max-size-buffers=1 ! \
                    gather.sink_0 \
                    t. ! queue ! \
                    dxpreprocess \
                        preprocess-id=3 \
                        resize-width=640 \
                        resize-height=640 \
                        keep-ratio=true \
                        pad-value=114 \
                        secondary-mode=true \
                        target-class-id=0 \
                        min-object-width=50 \
                        min-object-height=50 \
                        interval=5 ! \
                    queue max-size-buffers=1 ! \
                    dxinfer \
                        preprocess-id=3 \
                        inference-id=3 \
                        secondary-mode=true \
                        model-path=$SRC_DIR/samples/models/SCRFD500M.dxnn ! \
                    queue max-size-buffers=1 ! \
                    dxpostprocess \
                        inference-id=3 \
                        secondary-mode=true \
                        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_scrfd500m.so \
                        function-name=PostProcess ! \
                    queue max-size-buffers=1 ! \
                    gather.sink_1 \
                    dxgather name=gather ! queue ! \
                    dxosd ! queue ! \
                    $VIDEOCONVERT_PIPELINE ! fpsdisplaysink sync=false $VIDEO_SINK_ARGS
done
