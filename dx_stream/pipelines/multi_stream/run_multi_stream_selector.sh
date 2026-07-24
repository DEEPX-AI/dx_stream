#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Model auto-download logic
MODEL_NAME="yolov5-s_640x640_ppu.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found in samples/models. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

OUTPUT_WIDTH=1280
OUTPUT_HEIGHT=720
VIDEO_DIR="${SRC_DIR}/samples/videos/"

PREPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/postprocess_config.json"

# check os version
if [ "$(lsb_release -rs)" = "18.04" ]; then
    echo -e "Using X11 video sink forcely on ubuntu 18.04"
    VIDEO_SINK_ARGS="video-sink=ximagesink"
else
    VIDEO_SINK_ARGS=""
fi

# Default videoconvert pipeline
VIDEOCONVERT_PIPELINE="videoconvert"

# NOTE: If you experience rendering issues (corrupted/distorted video output) on Orange Pi 5 Plus
# with Debian 12, uncomment the lines below to force I420 format conversion.
# See troubleshooting documentation for more details.
# if grep -q "rk3588" /proc/device-tree/compatible 2>/dev/null; then
#     if [ "$(lsb_release -rs)" = "12" ]; then
#         echo "Detected Orange Pi 5 Plus with Debian 12 - using I420 format"
#         VIDEOCONVERT_PIPELINE="videoconvert ! video/x-raw,format=I420"
#     fi
# fi



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

num_pipelines=16
cols=1
while [[ $((cols * cols)) -lt $num_pipelines ]]; do
    cols=$((cols + 1))
done
rows=$(( (num_pipelines + cols - 1) / cols ))

STREAM_WIDTH=$(( OUTPUT_WIDTH / cols ))
STREAM_HEIGHT=$(( OUTPUT_HEIGHT / rows ))

echo "Calculated grid: ${rows} rows x ${cols} columns"
echo "Individual stream size: ${STREAM_WIDTH}x${STREAM_HEIGHT}"
echo "Total output resolution: ${OUTPUT_WIDTH}x${OUTPUT_HEIGHT}"

for i in $(seq 0 $((num_pipelines - 1))); do
    current_video_file="${video_files[$i]}"
    echo "  Adding pipeline for: ${current_video_file}"

    full_video_path=$(readlink -f "${current_video_file}")
    uri="file://${full_video_path}"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    src_pipe+=" urisourcebin uri=\"${uri}\" ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ! in.sink_${i}"
    compositor_pipe+=" out.src_${i} ! queue max-size-buffers=10 ! dxscale width=${STREAM_WIDTH} height=${STREAM_HEIGHT} ! queue max-size-buffers=10 ! comp.sink_${i}"
    compositor_props+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos}"
done

inference_pipe="dxinputselector name=in ! \
                dxpreprocess config-file-path=${PREPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
                dxinfer config-file-path=${INFER_CONFIG} ! queue max-size-buffers=10 ! \
                dxpostprocess config-file-path=${POSTPROCESS_CONFIG} ! queue max-size-buffers=10 ! \
                dxosd ! \
                dxoutputselector name=out"

launch_cmd="gst-launch-1.0 ${src_pipe} ${inference_pipe} ${compositor_pipe} compositor name=comp ${compositor_props} ! $VIDEOCONVERT_PIPELINE ! fpsdisplaysink sync=false $VIDEO_SINK_ARGS"

echo "--------------------------------------------------"
echo "Generated gst-launch-1.0 command:"
echo "${launch_cmd}"
echo "--------------------------------------------------"
echo "Launching pipeline..."
eval "${launch_cmd}"

exit 0
