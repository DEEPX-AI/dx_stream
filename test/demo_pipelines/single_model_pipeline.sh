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

# Object Detection
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/postprocess_config.json ! queue ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Object Detection pipeline test passed"
else
    echo "❌ Object Detection pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# Face Detection
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Face_Detection/YOLOV5S_Face/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Face_Detection/YOLOV5S_Face/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Face_Detection/YOLOV5S_Face/postprocess_config.json ! queue ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Face Detection pipeline test passed"
else
    echo "❌ Face Detection pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# Pose Estimation
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Pose_Estimation/YOLOV5Pose640_1/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Pose_Estimation/YOLOV5Pose640_1/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Pose_Estimation/YOLOV5Pose640_1/postprocess_config.json ! queue ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Pose Estimation pipeline test passed"
else
    echo "❌ Pose Estimation pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# Segmentation
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Segmentation/DeepLabV3PlusMobileNetV2_2/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Segmentation/DeepLabV3PlusMobileNetV2_2/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Segmentation/DeepLabV3PlusMobileNetV2_2/postprocess_config.json ! queue ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Segmentation pipeline test passed"
else
    echo "❌ Segmentation pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# Tracking
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/postprocess_config.json ! queue ! \
    dxtracker config-file-path=$SRC_DIR/dx_stream/configs/tracker_config.json ! queue ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Multi Object Tracking pipeline test passed"
else
    echo "❌ Multi Object Tracking pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

exit 0