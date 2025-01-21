#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/blackbox-city-road.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road2.mov"
    "$SRC_DIR/samples/videos/carrierbag.mp4"
    "$SRC_DIR/samples/videos/cctv-city-road2.mov"
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
)

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! decodebin ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
                    dxtracker config-file-path=$SRC_DIR/configs/tracker_config.json ! queue ! \
                    dxosd ! queue ! \
                    autovideosink sync=false
done


