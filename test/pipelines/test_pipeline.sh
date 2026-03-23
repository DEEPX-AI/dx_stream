#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/bin" ]; then
    rm -rf "$SCRIPT_DIR/bin"
fi

# Setup environment for DX-Stream
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

# Find the actual libdir
if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR=$(find "${INSTALL_PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs dirname)
    [ -z "$ACTUAL_LIBDIR" ] && ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi

export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:$SCRIPT_DIR/../test_plugin/install:${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:$SCRIPT_DIR/../test_plugin/install:${LD_LIBRARY_PATH}"

mkdir -p "$SCRIPT_DIR/bin"

echo "Building pipeline test binaries..."

# Build each test binary and check for compilation errors
g++ -g -o "$SCRIPT_DIR/bin/test_single_pipeline" test_single_pipeline.cpp $(pkg-config --cflags --libs gstreamer-check-1.0 gstdxstream) -ldxrt
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_single_pipeline" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_secondary_pipeline" test_secondary_pipeline.cpp $(pkg-config --cflags --libs gstreamer-check-1.0 gstdxstream) -ldxrt
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_secondary_pipeline" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_multi_pipeline" test_multi_pipeline.cpp $(pkg-config --cflags --libs gstreamer-check-1.0 gstdxstream) -ldxrt
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_multi_pipeline" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_roi" test_roi.cpp $(pkg-config --cflags --libs gstreamer-check-1.0 gstdxstream) -ldxrt
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_roi" >&2
    exit 1
fi

g++ -g -o "$SCRIPT_DIR/bin/test_interval" test_interval.cpp $(pkg-config --cflags --libs gstreamer-check-1.0 gstdxstream) -ldxrt
if [ $? -ne 0 ]; then
    echo "[ERROR] Failed to compile test_interval" >&2
    exit 1
fi

echo "All pipeline test binaries compiled successfully!"

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
