#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

OUTPUT_WIDTH=1440
OUTPUT_HEIGHT=810

PREPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/postprocess_config.json"

RTSP_PUBLIC_URL="rtsp://210.99.70.120:1935"
# RTSP_INTERNAL_URL="rtsp://192.168.30.11:8554"

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

num_pipelines=9
cols=3
rows=3

STREAM_WIDTH=$(( OUTPUT_WIDTH / cols ))
STREAM_HEIGHT=$(( OUTPUT_HEIGHT / rows ))

echo "Calculated grid: ${rows} rows x ${cols} columns"
echo "Individual stream size: ${STREAM_WIDTH}x${STREAM_HEIGHT}"
echo "Total output resolution: ${OUTPUT_WIDTH}x${OUTPUT_HEIGHT}"

for i in $(seq 0 $((num_pipelines - 1))); do
    rtsp_channel=$(( 2 + i ))
    formatted_channel=$(printf "%03d" "$rtsp_channel")
    uri="${RTSP_PUBLIC_URL}/live/cctv${formatted_channel}.stream"
    # formatted_channel=$(printf "%d" "$rtsp_channel")
    # uri="${RTSP_INTERNAL_URL}/stream${formatted_channel}"
    echo "  Adding pipeline for: ${uri}"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    src_pipe=" urisourcebin uri=\"${uri}\" ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ! \
        dxpreprocess config-file-path=${PREPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
        dxinfer config-file-path=${INFER_CONFIG} ! queue max-size-buffers=10 ! \
        dxpostprocess config-file-path=${POSTPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
        dxosd width=${STREAM_WIDTH} height=${STREAM_HEIGHT} "

    pipeline_str+="${src_pipe} ! queue max-size-buffers=10 ! comp.sink_${i}"
    compositor_props+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos} sink_${i}::width=${STREAM_WIDTH} sink_${i}::height=${STREAM_HEIGHT}"
done

launch_cmd="gst-launch-1.0 -e ${pipeline_str} compositor name=comp ${compositor_props} ! videoconvert ! fpsdisplaysink $VIDEO_SINK_ARGS"
echo "--------------------------------------------------"
echo "Generated gst-launch-1.0 command:"
echo "${launch_cmd}"
echo "--------------------------------------------------"
echo "Launching pipeline..."
eval "${launch_cmd}"

exit 0