#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# check 'vaapidecodebin'
if gst-inspect-1.0 vaapidecodebin &>/dev/null; then
    DECODE_PIPELINE="qtdemux ! vaapidecodebin"
else
    DECODE_PIPELINE="decodebin"
fi

gst-launch-1.0 urisourcebin uri=rtsp://210.99.70.120:1935/live/cctv010.stream ! $DECODE_PIPELINE ! mux.sink_0 \
                urisourcebin uri=rtsp://210.99.70.120:1935/live/cctv002.stream ! $DECODE_PIPELINE ! mux.sink_1 \
                urisourcebin uri=rtsp://210.99.70.120:1935/live/cctv003.stream ! $DECODE_PIPELINE ! mux.sink_2 \
                urisourcebin uri=rtsp://210.99.70.120:1935/live/cctv004.stream ! $DECODE_PIPELINE ! mux.sink_3 \
                dxmuxer name=mux live-source=true ! queue ! \
                dxpreprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
                dxinfer config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
                dxpostprocess config-file-path=$SRC_DIR/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
                dxosd ! queue ! \
                dxtiler config-file-path=$SRC_DIR/configs/tiler_config.json ! queue  ! \
                videoconvert ! queue ! fpsdisplaysink sync=true
