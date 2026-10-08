#!/bin/bash
# Shared settings for the Raspberry Pi camera demos (sourced by run_rpi_camera_*.sh).
#
# Overridable: CAMERA_WIDTH, CAMERA_HEIGHT, CAMERA_FPS, CAMERA_FORMAT (NV12 | RGB | BGR | I420)
# NV12 is the default: libcamerasrc hands it over as dma-buf, which the GLES backend imports without copies.

RPI_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$RPI_SCRIPT_DIR")")
POSTPROCESS_LIB_DIR=/usr/local/share/gstdxstream/lib

CAMERA_WIDTH=${CAMERA_WIDTH:-1280}
CAMERA_HEIGHT=${CAMERA_HEIGHT:-720}
CAMERA_FPS=${CAMERA_FPS:-30}
CAMERA_FORMAT=${CAMERA_FORMAT:-NV12}

if ! grep -qa "Raspberry Pi" /proc/device-tree/model 2>/dev/null; then
    echo "[WARN] This demo is intended for Raspberry Pi (libcamera camera stack)."
fi

if ! gst-inspect-1.0 libcamerasrc >/dev/null 2>&1; then
    echo "[ERROR] GStreamer element 'libcamerasrc' not found."
    echo "        Install it with: sudo apt install gstreamer1.0-libcamera"
    exit 1
fi

# Downloads the model into samples/models if it is missing (same flow as the file-based demos).
ensure_model() {
    MODEL_PATH="$SRC_DIR/samples/models/$1"
    if [ ! -f "$MODEL_PATH" ]; then
        echo "[INFO] $1 not found in samples/models. Downloading..."
        (cd "$SRC_DIR"/.. && ./setup.sh --model="$1")
        if [ ! -f "$MODEL_PATH" ]; then
            echo "[ERROR] Failed to download $1"
            exit 1
        fi
    fi
}

# Array so every gst-launch token stays a separate argument (a single quoted string is not a valid pipeline).
CAMERA_SRC=(libcamerasrc '!' "video/x-raw,format=$CAMERA_FORMAT,width=$CAMERA_WIDTH,height=$CAMERA_HEIGHT,framerate=$CAMERA_FPS/1")
echo "[INFO] Camera: ${CAMERA_WIDTH}x${CAMERA_HEIGHT}@${CAMERA_FPS} $CAMERA_FORMAT"

# Runs gst-launch with the display sink, or prints the measured FPS when no display is available (SSH/headless).
run_camera_pipeline() {
    if [ -n "$WAYLAND_DISPLAY" ] || [ -n "$DISPLAY" ]; then
        gst-launch-1.0 "$@" ! videoconvert ! fpsdisplaysink sync=false
    else
        echo "[INFO] No display found: printing FPS only (Ctrl+C to stop)"
        (
            set -o pipefail
            gst-launch-1.0 -v "$@" ! fpsdisplaysink video-sink=fakesink text-overlay=false sync=false 2>&1 |
                grep --line-buffered -iE "last-message|error|warn|fail" |
                sed -u -E 's/.*last-message = (rendered: .*)/[FPS] \1/'
        )
    fi
}
