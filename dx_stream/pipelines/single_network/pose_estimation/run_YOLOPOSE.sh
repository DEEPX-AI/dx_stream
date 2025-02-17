#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
    "$SRC_DIR/samples/videos/snowboard.mp4"
)

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! decodebin ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/postprocess_config.json ! queue ! \
                    dxosd ! queue ! \
                    autovideosink sync=true
done

