#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/bin" ]; then
    rm -rf "$SCRIPT_DIR/bin"
fi

export GST_PLUGIN_PATH="$GST_PLUGIN_PATH:$SCRIPT_DIR/../test_plugin/install"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SCRIPT_DIR/../test_plugin/install"

mkdir -p "$SCRIPT_DIR/bin"

g++ -g -o "$SCRIPT_DIR/bin/test_single_pipeline" test_single_pipeline.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_secondary_pipeline" test_secondary_pipeline.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_multi_pipeline" test_multi_pipeline.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_roi" test_roi.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_interval" test_interval.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream

cd "$SCRIPT_DIR/bin"

LIBRARY_NAME="librga.so"
LIBRARY_PATH=$(ldconfig -p | grep "$LIBRARY_NAME" | grep -oP '=>\s*\K/.*')

binaries=()

if [ -n "$LIBRARY_PATH" ]; then
  FIRST_PATH=$(echo "$LIBRARY_PATH" | head -n 1)
else
  binaries+=(
    "test_single_pipeline"
    "test_secondary_pipeline"
    "test_multi_pipeline"
    "test_roi"
    "test_interval"
)
fi

for bin in "${binaries[@]}"; do
    ./$bin
    exit_code=$?

    if [ $exit_code -eq 1 ]; then
        echo "[ERROR] $bin failed with exit code 1" >&2
        exit 1
    fi
done
exit 0
