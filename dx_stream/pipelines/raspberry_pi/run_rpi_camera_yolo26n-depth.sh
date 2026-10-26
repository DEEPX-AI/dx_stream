#!/bin/bash
# Raspberry Pi camera -> Depth Estimation (YOLO26n-Depth)

source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/rpi_camera_common.sh"
ensure_model "yolo26-depth-n_768x768.dxnn"

run_camera_pipeline "${CAMERA_SRC[@]}" ! queue max-size-buffers=1 ! \
    dxpreprocess \
        preprocess-id=1 \
        keep-ratio=false \
        resize-width=768 \
        resize-height=768 ! \
    queue max-size-buffers=1 ! \
    dxinfer \
        preprocess-id=1 \
        inference-id=1 \
        model-path="$MODEL_PATH" ! \
    queue max-size-buffers=1 ! \
    dxpostprocess \
        inference-id=1 \
        library-file-path="$POSTPROCESS_LIB_DIR/libpostprocess_yolo26depth.so" \
        function-name=PostProcess ! \
    queue max-size-buffers=1 ! \
    dxosd ! queue max-size-buffers=1
