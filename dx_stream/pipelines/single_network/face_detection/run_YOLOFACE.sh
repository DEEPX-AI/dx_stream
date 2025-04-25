#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
    "$SRC_DIR/samples/videos/snowboard.mp4"
)

# check 'vaapidecodebin'
if gst-inspect-1.0 vaapidecodebin &>/dev/null; then
    DECODE_PIPELINE="qtdemux ! vaapidecodebin"
else
    DECODE_PIPELINE="decodebin"
fi

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! $DECODE_PIPELINE ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Face_Detection/YOLOV5S_Face/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Face_Detection/YOLOV5S_Face/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Face_Detection/YOLOV5S_Face/postprocess_config.json ! queue ! \
                    dxosd ! queue ! \
                    autovideosink sync=true
done

