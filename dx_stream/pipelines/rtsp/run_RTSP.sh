#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

OUTPUT_WIDTH=1280
OUTPUT_HEIGHT=720

PREPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/postprocess_config.json"

# Model auto-download logic
MODEL_NAME="YoloV5S_PPU.dxnn"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
if [ ! -f "$MODEL_PATH" ]; then
    echo "[INFO] $MODEL_NAME not found in samples/models. Downloading..."
    (cd "$SRC_DIR"/.. && ./setup.sh --model="$MODEL_NAME")
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[ERROR] Failed to download $MODEL_NAME"
        exit 1
    fi
fi

RTSP_PUBLIC_URL="rtsp://210.99.70.120:1935"
RTSP_INTERNAL_URL="rtsp://192.168.30.100:8554"
USE_INTERNAL_RTSP=0

for arg in "$@"; do
    case "$arg" in
        --internal-rtsp)
            USE_INTERNAL_RTSP=1
            ;;
    esac
done

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



num_pipelines=4
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
    rtsp_channel=$(( 2 + i ))
    if [ "$USE_INTERNAL_RTSP" -eq 1 ]; then
        formatted_channel=$(printf "%d" "$rtsp_channel")
        uri="${RTSP_INTERNAL_URL}/stream${formatted_channel}"
    else
        formatted_channel=$(printf "%03d" "$rtsp_channel")
        uri="${RTSP_PUBLIC_URL}/live/cctv${formatted_channel}.stream"
    fi
    echo "  Adding pipeline for: ${uri}"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    src_pipe+=" urisourcebin uri=\"${uri}\" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! in.sink_${i}"
    compositor_pipe+=" out.src_${i} ! queue max-size-buffers=2 ! dxscale width=${STREAM_WIDTH} height=${STREAM_HEIGHT} ! queue max-size-buffers=2 ! comp.sink_${i}"
    compositor_props+=" sink_${i}::xpos=${xpos} sink_${i}::ypos=${ypos}"
done

inference_pipe="dxinputselector name=in ! \
                dxpreprocess config-file-path=${PREPROCESS_CONFIG} ! queue max-size-buffers=2 ! \
                dxinfer config-file-path=${INFER_CONFIG} ! queue max-size-buffers=2 ! \
                dxpostprocess config-file-path=${POSTPROCESS_CONFIG} ! queue max-size-buffers=2 ! \
                dxosd ! \
                dxoutputselector name=out"

launch_cmd="gst-launch-1.0 ${src_pipe} ${inference_pipe} ${compositor_pipe} compositor name=comp ${compositor_props} ! $VIDEOCONVERT_PIPELINE ! fpsdisplaysink $VIDEO_SINK_ARGS"
echo "--------------------------------------------------"
echo "Generated gst-launch-1.0 command:"
echo "${launch_cmd}"
echo "--------------------------------------------------"
echo "Launching pipeline..."
eval "${launch_cmd}"

exit 0
