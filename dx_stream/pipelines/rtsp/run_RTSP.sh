#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

OUTPUT_WIDTH=1280
OUTPUT_HEIGHT=720

PREPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/preprocess_config.json"
INFER_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/inference_config.json"
POSTPROCESS_CONFIG="${SRC_DIR}/configs/YoloV5S_PPU/postprocess_config.json"

usage() {
    cat <<EOF
Usage: $(basename "$0") --rtsp-url URL [--rtsp-url URL ...]

Multi-channel object detection (YOLOv5s PPU) on RTSP streams.
One channel is created per --rtsp-url, in the given order.

Options:
  --rtsp-url URL   RTSP stream URL (rtsp:// or rtsps://). Repeat for more channels.
  -h, --help       Show this help and exit.

Example:
  $(basename "$0") --rtsp-url rtsp://127.0.0.1:8554/stream1 --rtsp-url rtsp://127.0.0.1:8554/stream2
EOF
}

error() {
    echo "[ERROR] $1" >&2
    usage >&2
    exit 1
}

RTSP_URLS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --rtsp-url)
            [ $# -ge 2 ] || error "--rtsp-url requires a value"
            RTSP_URLS+=("$2")
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        -*)
            error "Unknown option: $1"
            ;;
        *)
            error "Unexpected argument. Use --rtsp-url URL"
            ;;
    esac
done

[ ${#RTSP_URLS[@]} -gt 0 ] || error "At least one --rtsp-url is required"

# URLs are not printed because they may contain credentials.
for url in "${RTSP_URLS[@]}"; do
    case "$url" in
        *[[:space:]]*) error "RTSP URL must not contain whitespace" ;;
        rtsp://?*|rtsps://?*) ;;
        *) error "RTSP URL must start with rtsp:// or rtsps://" ;;
    esac
done

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



num_pipelines=${#RTSP_URLS[@]}
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

src_pipe=()
compositor_pipe=()
compositor_props=()
for i in "${!RTSP_URLS[@]}"; do
    echo "  Adding pipeline: [channel=${i}]"

    row_idx=$(( i / cols ))
    col_idx=$(( i % cols ))
    xpos=$(( col_idx * STREAM_WIDTH ))
    ypos=$(( row_idx * STREAM_HEIGHT ))

    src_pipe+=(urisourcebin "uri=${RTSP_URLS[$i]}" ! queue max-size-buffers=2 ! decodebin ! queue max-size-buffers=2 ! "in.sink_${i}")
    compositor_pipe+=("out.src_${i}" ! queue max-size-buffers=2 ! dxscale "width=${STREAM_WIDTH}" "height=${STREAM_HEIGHT}" ! queue max-size-buffers=2 ! "comp.sink_${i}")
    compositor_props+=("sink_${i}::xpos=${xpos}" "sink_${i}::ypos=${ypos}")
done

echo "[WARN] If the pipeline cannot keep up with the RTSP source rate, frames may be dropped."
echo "Launching pipeline..."
# Arguments are passed as an array (no eval) so URLs are never interpreted by the shell.
# -q hides progress messages, which would print URL credentials; errors are still reported.
exec gst-launch-1.0 -q "${src_pipe[@]}" \
    dxinputselector name=in ! \
    dxpreprocess "config-file-path=${PREPROCESS_CONFIG}" ! queue max-size-buffers=2 ! \
    dxinfer "config-file-path=${INFER_CONFIG}" ! queue max-size-buffers=2 ! \
    dxpostprocess "config-file-path=${POSTPROCESS_CONFIG}" ! queue max-size-buffers=2 ! \
    dxosd ! \
    dxoutputselector name=out \
    "${compositor_pipe[@]}" \
    compositor name=comp "${compositor_props[@]}" ! ${VIDEOCONVERT_PIPELINE} ! fpsdisplaysink ${VIDEO_SINK_ARGS}
