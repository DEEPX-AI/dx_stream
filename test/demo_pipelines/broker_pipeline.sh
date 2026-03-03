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

# # MQTT
# OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
#     dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/preprocess_config.json ! queue ! \
#     dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/inference_config.json ! queue ! \
#     dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/postprocess_config.json ! queue ! \
#     dxmsgconv config-file-path=$SRC_DIR/dx_stream/configs/msgconv_config.json ! queue ! \
#     dxmsgbroker broker-name=mqtt conn-info=localhost:1883 topic=test 2>&1)

# EXIT_CODE=$?
# check_error "$EXIT_CODE" "$OUTPUT"
# if [ $? -eq 0 ]; then
#     echo "✅ MQTT pipeline test passed"
# else
#     echo "❌ MQTT pipeline test failed"
#     exit 1
# fi

# # Kafka
# export GST_DEBUG=3
# OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
#     dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/preprocess_config.json ! queue ! \
#     dxinfer config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/inference_config.json ! queue ! \
#     dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/Object_Detection/YOLOV5S_4/postprocess_config.json ! queue ! \
#     dxmsgconv config-file-path=$SRC_DIR/dx_stream/configs/msgconv_config.json ! queue ! \
#     dxmsgbroker broker-name=kafka conn-info=localhost:9092 topic=test config=$SRC_DIR/dx_stream/configs/broker_kafka.cfg 2>&1)

# EXIT_CODE=$?
# check_error "$EXIT_CODE" "$OUTPUT"
# if [ $? -eq 0 ]; then
#     echo "✅ Kafka pipeline test passed"
# else
#     echo "❌ Kafka pipeline test failed"
#     echo "Output:"
#     echo "$OUTPUT"
#     exit 1
# fi

exit 0