#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

STREAM_WIDTH=640
STREAM_HEIGHT=360
PADDING=5
VIDEO_DIR="${SRC_DIR}/samples/videos/"

PREPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/Object_Detection/YOLOV5S_3/postprocess_config.json"

# check 'vaapidecodebin'
if gst-inspect-1.0 vaapidecodebin &>/dev/null; then
    DECODE_PIPELINE="qtdemux ! vaapidecodebin"
else
    DECODE_PIPELINE="decodebin"
fi

echo "Searching for video files (including .mov) in ${VIDEO_DIR}..."
video_files=()
while IFS= read -r -d $'\0'; do
    video_files+=("$REPLY")
done < <(find "$VIDEO_DIR" -maxdepth 1 -type f \( -name "*.mp4" -o -name "*.mov" -o -name "*.avi" -o -name "*.mkv" \) -print0 | sort -z)

num_available=${#video_files[@]}

if [[ $num_available -eq 0 ]]; then
   echo "Error: No video files found in $VIDEO_DIR with specified extensions."
   exit 1
fi
echo "Found $num_available video files."

num_pipelines=$num_pipelines_req
if [[ $num_pipelines -gt $num_available ]]; then
  echo "Warning: Requested $num_pipelines pipelines, but only $num_available video files found."
  echo "Using $num_available pipelines instead."
  num_pipelines=$num_available
fi

num_pipelines=4
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
    current_video_file="${video_files[$i]}"
    echo "  Adding pipeline for: ${current_video_file}"

    full_video_path=$(readlink -f "${current_video_file}")
    uri="file://${full_video_path}"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * (STREAM_WIDTH + PADDING) ))
    ypos=$(( row_idx * (STREAM_HEIGHT + PADDING) ))

    src_pipe+=" urisourcebin uri=\"${uri}\" ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ! in.sink_${i}"
    compositor_pipe+=" out.src_${i} ! queue max-size-buffers=10 ! dxosd width=${STREAM_WIDTH} height=${STREAM_HEIGHT} ! comp.sink_${i}"
    compositor_props+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos} sink_${i}::width=${STREAM_WIDTH} sink_${i}::height=${STREAM_HEIGHT}"
done

inference_pipe="dxinputselector name=in ! \
                dxpreprocess config-file-path=${PREPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
                dxinfer config-file-path=${INFER_CONFIG} ! queue max-size-buffers=10 ! \
                dxpostprocess config-file-path=${POSTPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
                dxoutputselector name=out"

launch_cmd="gst-launch-1.0 -e ${src_pipe} ${inference_pipe} ${compositor_pipe} compositor name=comp ${compositor_props} ! videoconvert ! fpsdisplaysink"

echo "--------------------------------------------------"
echo "Generated gst-launch-1.0 command:"
echo "${launch_cmd}"
echo "--------------------------------------------------"
echo "Launching pipeline..."
eval "${launch_cmd}"

exit 0