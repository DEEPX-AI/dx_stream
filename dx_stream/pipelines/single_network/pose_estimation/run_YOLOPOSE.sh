#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")

INPUT_VIDEO_PATH_LIST=(
    "$SRC_DIR/samples/videos/dance-group.mov"
    "$SRC_DIR/samples/videos/dance-group2.mov"
    "$SRC_DIR/samples/videos/dance-solo.mov"
    "$SRC_DIR/samples/videos/snowboard.mp4"
)

if [ "$(lsb_release -rs)" = "18.04" ]; then
    echo -e "Using X11 video sink forcely on ubuntu 18.04"
    VIDEO_SINK_ARGS="video-sink=ximagesink"
else
    VIDEO_SINK_ARGS=""
fi

# check 'vaapidecodebin'
if gst-inspect-1.0 vaapidecodebin &>/dev/null; then
    DECODE_PIPELINE="qtdemux ! vaapidecodebin"
else
    DECODE_PIPELINE="decodebin"
fi

for INPUT_VIDEO_PATH in "${INPUT_VIDEO_PATH_LIST[@]}"; do
    gst-launch-1.0 urisourcebin uri=file://$INPUT_VIDEO_PATH ! $DECODE_PIPELINE ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Pose_Estimation/YOLOV5Pose640_1/postprocess_config.json ! queue ! \
                    dxosd width=1280 height=720 ! queue ! \
                    videoconvert ! autovideosink sync=true $VIDEO_SINK_ARGS
done

