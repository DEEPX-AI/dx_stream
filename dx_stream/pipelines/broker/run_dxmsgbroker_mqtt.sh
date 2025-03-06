#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

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
                    dxpreprocess config-file-path=$SRC_DIR/configs/Object_Detection/YoloV7/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Object_Detection/YoloV7/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Object_Detection/YoloV7/postprocess_config.json ! queue ! \
                    dxosd ! queue ! \
                    dxmsgconv config-file-path=$SRC_DIR/configs/msgconv_config.json ! queue ! \
                    dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=test
done
