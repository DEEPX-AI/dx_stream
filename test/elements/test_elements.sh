#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/bin" ]; then
    rm -rf "$SCRIPT_DIR/bin"
fi

export GST_PLUGIN_PATH="$GST_PLUGIN_PATH:$SCRIPT_DIR/../test_plugin/install"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SCRIPT_DIR/../test_plugin/install"

mkdir -p "$SCRIPT_DIR/bin"

g++ -g -o "$SCRIPT_DIR/bin/test_dxpreprocess" test_dxpreprocess.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxinfer" test_dxinfer.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxpostprocess" test_dxpostprocess.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxosd" test_dxosd.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxtiler" test_dxtiler.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxtracker" test_dxtracker.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxmsgconv" test_dxmsgconv.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
# g++ -g -o "$SCRIPT_DIR/bin/test_dxmsgbroker" test_dxmsgbroker.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxrouter" test_dxrouter.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxmuxer" test_dxmuxer.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxrate" test_dxrate.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
g++ -g -o "$SCRIPT_DIR/bin/test_dxgather" test_dxgather.c $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream

cd "$SCRIPT_DIR/bin"

binaries=(
    "test_dxpreprocess"
    "test_dxinfer"
    "test_dxpostprocess"
    "test_dxosd"
    "test_dxtiler"
    "test_dxtracker"
    "test_dxmsgconv"
    # "test_dxmsgbroker"
    "test_dxrouter"
    "test_dxmuxer"
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
