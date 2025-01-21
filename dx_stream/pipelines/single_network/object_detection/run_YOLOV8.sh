#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/boat.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road.mp4"
    "$SRC_DIR/samples/videos/blackbox-city-road2.mov"
    "$SRC_DIR/samples/videos/carrierbag.mp4"
    "$SRC_DIR/samples/videos/cctv-city-road.mov"
    "$SRC_DIR/samples/videos/cctv-city-road2.mov"
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
    "$SRC_DIR/samples/videos/dogs.mp4"
    "$SRC_DIR/samples/videos/doughnut.mp4"
    "$SRC_DIR/samples/videos/snowboard.mp4"
)

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! decodebin ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV8N/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV8N/inference_config.json ! queue ! \
                    dxosd ! queue ! \
                    fpsdisplaysink sync=false
done
