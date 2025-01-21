#!/bin/bash

BUILD_TYPE=${1:-release}

echo "Using build type: $BUILD_TYPE"

TARGET_DIR="."

for subdir in "$TARGET_DIR"/*/; do
    echo "Processing directory: $subdir"
    
    cd "$subdir" || exit 1

    meson setup build --buildtype="$BUILD_TYPE"
    meson compile -C build
    sudo meson install -C build
    rm -rf build
    cd ./.. || exit 1
done