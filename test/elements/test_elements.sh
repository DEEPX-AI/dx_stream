#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/bin" ]; then
    rm -rf "$SCRIPT_DIR/bin"
fi

export GST_PLUGIN_PATH="$GST_PLUGIN_PATH:$SCRIPT_DIR/../test_plugin/install"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SCRIPT_DIR/../test_plugin/install"

mkdir -p "$SCRIPT_DIR/bin"

echo "Building test binaries..."

# Build each test binary and check for compilation errors
g++ -g -o "$SCRIPT_DIR/bin/test_dxpreprocess" test_dxpreprocess.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxpreprocess" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxinfer" test_dxinfer.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxinfer" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxpostprocess" test_dxpostprocess.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxpostprocess" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxosd" test_dxosd.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxosd" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxtracker" test_dxtracker.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxtracker" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxmsgconv" test_dxmsgconv.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxmsgconv" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxrate" test_dxrate.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxrate" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_dxgather" test_dxgather.cpp $(pkg-config --cflags --libs gstreamer-check-1.0) -lgstdxstream
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_dxgather" >&2
    exit 1
fi

echo "All test binaries compiled successfully!"

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
    if [ ! -f "./$bin" ]; then
        echo "[ERROR] Binary $bin not found (compilation may have failed)" >&2
        exit 1
    fi
    
    echo "Running $bin..."
    ./$bin
    exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo "[ERROR] $bin failed with exit code $exit_code" >&2
        exit 1
    fi
    echo "[SUCCESS] $bin completed successfully"
done
exit 0
