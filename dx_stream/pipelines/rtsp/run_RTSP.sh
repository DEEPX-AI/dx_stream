#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

STREAM_WIDTH=640
STREAM_HEIGHT=360
PADDING=5

PREPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/postprocess_config.json"

# check 'vaapidecodebin'
if gst-inspect-1.0 vaapidecodebin &>/dev/null; then
    DECODE_PIPELINE="qtdemux ! vaapidecodebin"
else
    DECODE_PIPELINE="decodebin"
fi

num_pipelines=9
cols=1
while [[ $((cols * cols)) -lt $num_pipelines ]]; do
  cols=$((cols + 1))
done
rows=$(( (num_pipelines + cols - 1) / cols ))

echo "Calculated grid: ${rows} rows x ${cols} columns"
output_width=$(( cols * STREAM_WIDTH + (cols - 1) * PADDING ))
output_height=$(( rows * STREAM_HEIGHT + (rows - 1) * PADDING ))
echo "Estimated output resolution: ${output_width}x${output_height}"

for i in $(seq 0 $((num_pipelines - 1))); do
    rtsp_channel=$(( 2 + i ))
    formatted_channel=$(printf "%03d" "$rtsp_channel")
    uri="rtsp://210.99.70.120:1935/live/cctv${formatted_channel}.stream"
    echo "  Adding pipeline for: ${uri}"
    
    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * (STREAM_WIDTH + PADDING) ))
    ypos=$(( row_idx * (STREAM_HEIGHT + PADDING) ))

    src_pipe=" urisourcebin uri=\"${uri}\" ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ! \
        dxpreprocess config-file-path=${PREPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
        dxinfer config-file-path=${INFER_CONFIG} ! queue max-size-buffers=10 ! \
        dxpostprocess config-file-path=${POSTPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
        dxosd width=${STREAM_WIDTH} height=${STREAM_HEIGHT} "

    pipeline_str+="${src_pipe} ! queue max-size-buffers=10 ! comp.sink_${i}"
    compositor_props+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos} sink_${i}::width=${STREAM_WIDTH} sink_${i}::height=${STREAM_HEIGHT}"
done

launch_cmd="gst-launch-1.0 -e ${pipeline_str} compositor name=comp ${compositor_props} ! videoconvert ! fpsdisplaysink"
echo "--------------------------------------------------"
echo "Generated gst-launch-1.0 command:"
echo "${launch_cmd}"
echo "--------------------------------------------------"
echo "Launching pipeline..."
eval "${launch_cmd}"

exit 0
