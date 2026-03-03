#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR=$(dirname "$(dirname "$SCRIPT_DIR")")

# Setup environment for DX-Stream
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR=$(find "${INSTALL_PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs dirname)
    [ -z "$ACTUAL_LIBDIR" ] && ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:${LD_LIBRARY_PATH}"

source "$SCRIPT_DIR/utils.sh"

VIDEO_FILE="$SRC_DIR/dx_stream/samples/videos/dance-group.mov"

if [ -f "$VIDEO_FILE" ]; then
    echo "Input video file found: $VIDEO_FILE"
else
    echo "Error: Input video file not found: $VIDEO_FILE"
    exit 1
fi

cd $SRC_DIR

# multi branch model inference
OUTPUT=$(
    gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
    dxosd width=640 height=360 ! \
    comp.sink_0 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
    dxosd width=640 height=360 ! \
    comp.sink_1 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
    dxosd width=640 height=360 ! \
    comp.sink_2 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
    dxosd width=640 height=360 ! \
    comp.sink_3 \
    compositor name=comp \
    sink_0::xpos=0 sink_0::ypos=0 sink_0::width=640 sink_0::height=360 \
    sink_1::xpos=640 sink_1::ypos=0 sink_1::width=640 sink_1::height=360 \
    sink_2::xpos=0 sink_2::ypos=360 sink_2::width=640 sink_2::height=360 \
    sink_3::xpos=640 sink_3::ypos=360 sink_3::width=640 sink_3::height=360 \
    ! videoconvert ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Multi Branch pipeline test passed"
else
    echo "❌ Multi Branch pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# single branch model inference
OUTPUT=$(
    gst-launch-1.0 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! in.sink_0 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! in.sink_1 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! in.sink_2 \
    urisourcebin uri=file://$VIDEO_FILE ! decodebin ! in.sink_3 \
    dxinputselector name=in ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_3/postprocess_config.json ! queue ! \
    dxoutputselector name=out \
    out.src_0 ! dxosd width=640 height=360 ! comp.sink_0 \
    out.src_1 ! dxosd width=640 height=360 ! comp.sink_1 \
    out.src_2 ! dxosd width=640 height=360 ! comp.sink_2 \
    out.src_3 ! dxosd width=640 height=360 ! comp.sink_3 \
    compositor name=comp \
    sink_0::xpos=0 sink_0::ypos=0 sink_0::width=640 sink_0::height=360 \
    sink_1::xpos=640 sink_1::ypos=0 sink_1::width=640 sink_1::height=360 \
    sink_2::xpos=0 sink_2::ypos=360 sink_2::width=640 sink_2::height=360 \
    sink_3::xpos=640 sink_3::ypos=360 sink_3::width=640 sink_3::height=360 \
    ! videoconvert ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Single Branch pipeline test passed"
else
    echo "❌ Single Branch pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi
