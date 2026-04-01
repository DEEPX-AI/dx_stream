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

# object detection + face recognition
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/postprocess_config.json ! queue ! \
    dxpreprocess \
        preprocess-id=3 \
        resize-width=640 \
        resize-height=640 \
        keep-ratio=true \
        pad-value=114 \
        secondary-mode=true \
        target-class-id=0 \
        min-object-width=50 \
        min-object-height=50 \
        interval=5 ! \
    queue max-size-buffers=1 ! \
    dxinfer \
        preprocess-id=3 \
        inference-id=3 \
        secondary-mode=true \
        model-path=$SRC_DIR/dx_stream/samples/models/SCRFD500M.dxnn ! \
    queue max-size-buffers=1 ! \
    dxpostprocess \
        inference-id=3 \
        secondary-mode=true \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_scrfd500m.so \
        function-name=PostProcess ! \
    dxosd ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Secondary pipeline test passed"
else
    echo "❌ Secondary pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

# gather
OUTPUT=$(gst-launch-1.0 urisourcebin uri=file://$VIDEO_FILE ! decodebin ! \
    dxpreprocess config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/preprocess_config.json ! queue ! \
    dxinfer config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/inference_config.json ! queue ! \
    dxpostprocess config-file-path=$SRC_DIR/dx_stream/configs/YoloV5S_PPU/postprocess_config.json ! queue ! \
    dxtracker config-file-path=$SRC_DIR/dx_stream/configs/tracker_config.json ! queue ! \
    tee name=t \
    t. ! queue ! \
    dxpreprocess \
        preprocess-id=2 \
        resize-width=224 \
        resize-height=224 \
        secondary-mode=true \
        interval=5 \
        min-object-width=50 \
        min-object-height=50 \
        keep-ratio=false ! \
    queue max-size-buffers=1 ! \
    dxinfer \
        preprocess-id=2 \
        inference-id=2 \
        secondary-mode=true \
        model-path=$SRC_DIR/dx_stream/samples/models/EfficientNet_Lite0.dxnn ! \
    queue max-size-buffers=1 ! \
    dxpostprocess \
        inference-id=2 \
        secondary-mode=true \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_object_class.so \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    gather.sink_0 \
    t. ! queue ! \
    dxpreprocess \
        preprocess-id=3 \
        resize-width=640 \
        resize-height=640 \
        keep-ratio=true \
        pad-value=114 \
        secondary-mode=true \
        target-class-id=0 \
        min-object-width=50 \
        min-object-height=50 \
        interval=5 ! \
    queue max-size-buffers=1 ! \
    dxinfer \
        preprocess-id=3 \
        inference-id=3 \
        secondary-mode=true \
        model-path=$SRC_DIR/dx_stream/samples/models/SCRFD500M.dxnn ! \
    queue max-size-buffers=1 ! \
    dxpostprocess \
        inference-id=3 \
        secondary-mode=true \
        library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_scrfd500m.so \
        function-name=PostProcess ! \
    gather.sink_1 \
    dxgather name=gather ! queue ! \
    dxosd ! queue ! \
    fakesink 2>&1)

EXIT_CODE=$?
check_error "$EXIT_CODE" "$OUTPUT"
if [ $? -eq 0 ]; then
    echo "✅ Gather pipeline test passed"
else
    echo "❌ Gather pipeline test failed"
    echo "$OUTPUT"
    exit 1
fi

exit 0