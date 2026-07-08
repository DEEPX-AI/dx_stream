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
    "$SRC_DIR/samples/videos/boat.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road.mp4"
    "$SRC_DIR/samples/videos/cctv-city-road2.mov"
    "$SRC_DIR/samples/videos/dance-group.mov"
)



for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 -e urisourcebin uri=file://$INPUT_VIDEO_PATH ! decodebin ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/preprocess_config.json ! queue max-size-buffers=1 ! \
                    dxinfer config-file-path=$SRC_DIR/configs/YoloV5S_PPU/inference_config.json ! queue max-size-buffers=1 ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/YoloV5S_PPU/postprocess_config.json ! queue max-size-buffers=1 ! \
                    dxosd ! queue max-size-buffers=1 ! \
                    dxmsgconv config-file-path=$SRC_DIR/configs/msgconv_config.json ! queue max-size-buffers=1 ! \
                    dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=test
done
