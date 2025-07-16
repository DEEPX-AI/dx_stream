#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/bin" ]; then
    rm -rf "$SCRIPT_DIR/bin"
fi

export GST_PLUGIN_PATH="$GST_PLUGIN_PATH:$SCRIPT_DIR/../test_plugin/install"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SCRIPT_DIR/../test_plugin/install"

mkdir -p "$SCRIPT_DIR/bin"

g++ -g -o "$SCRIPT_DIR/bin/test_dxpreprocess" test_dxpreprocess.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxinfer" test_dxinfer.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxpostprocess" test_dxpostprocess.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxosd" test_dxosd.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxtracker" test_dxtracker.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxmsgconv" test_dxmsgconv.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
# g++ -g -o "$SCRIPT_DIR/bin/test_dxmsgbroker" test_dxmsgbroker.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxrate" test_dxrate.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxgather" test_dxgather.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream

cd "$SCRIPT_DIR/bin"

binaries=(
    "test_dxpreprocess"
    "test_dxinfer"
    "test_dxpostprocess"
    "test_dxosd"
    "test_dxtracker"
    "test_dxmsgconv"
    # "test_dxmsgbroker"
    "test_dxrate"
    "test_dxgather"
)

for bin in "${binaries[@]}"; do
    ./$bin
    exit_code=$?

    if [ $exit_code -eq 1 ]; then
        echo "[ERROR] $bin failed with exit code 1" >&2
        exit 1
    fi
done
exit 0
