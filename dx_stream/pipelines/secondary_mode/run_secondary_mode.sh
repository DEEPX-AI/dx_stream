#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

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
                    dxpreprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
                    dxtracker config-file-path=$SRC_DIR/configs/tracker_config.json ! queue ! \
                    tee name=t \
                    t. ! queue ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Re-Identification/OSNet/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Re-Identification/OSNet/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Re-Identification/OSNet/postprocess_config.json ! queue ! \
                    gather.sink_0 \
                    t. ! queue ! \
                    dxpreprocess config-file-path=$SRC_DIR/configs/Face_Detection/SCRFD/preprocess_config.json ! queue ! \
                    dxinfer config-file-path=$SRC_DIR/configs/Face_Detection/SCRFD/inference_config.json ! queue ! \
                    dxpostprocess config-file-path=$SRC_DIR/configs/Face_Detection/SCRFD/postprocess_config.json ! queue ! \
                    gather.sink_1 \
                    dxgather name=gather ! queue ! \
                    dxosd width=1280 height=720 ! queue ! \
                    videoconvert ! fpsdisplaysink sync=false $VIDEO_SINK_ARGS
done
