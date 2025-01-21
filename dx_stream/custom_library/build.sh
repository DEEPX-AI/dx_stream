#!/bin/bash

BUILD_TYPE=${1:-release}

echo "Using build type: $BUILD_TYPE"

build_and_install() {
    TARGET_DIR="$1"
    for subdir in "$TARGET_DIR"/*/; do
        echo "Processing directory: $subdir"
        
        cd "$subdir" || exit 1
        meson setup build --buildtype="$BUILD_TYPE"
        meson compile -C build
        sudo meson install -C build
        rm -rf build
        cd - > /dev/null || exit 1
    done
}

build_and_install "./postprocess_library"
build_and_install "./message_convert_library"