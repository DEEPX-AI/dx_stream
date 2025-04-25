#!/bin/bash

BUILD_TYPE="release"
SONAR_MODE_ARG=""


show_help() {
  echo "Usage: $(basename "$0") [--debug] [--help]"
  echo "Example 1): $0"
  echo "Example 2): $0 --debug"
  echo "Options:"
  echo "  [--debug]       Build for debug"
  echo "  [--help]        Show this help message"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}


# Parse arguments
for i in "$@"; do
    case "$1" in
        --debug)
            BUILD_TYPE="debug"
            ;;
        --sonar)
            SONAR_MODE_ARG="--sonar"
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help "error"
            ;;
    esac
    shift
done


echo "Using build type: $BUILD_TYPE"
echo "SONAR_MODE_ARG($SONAR_MODE_ARG) is set"

build_and_install() {
    TARGET_DIR="$1"
    for subdir in "$TARGET_DIR"/*/; do
        echo "Processing directory: $subdir"
        
        cd "$subdir" || exit 1
        meson setup build --buildtype="$BUILD_TYPE"
        if [ $? -ne 0 ]; then
            echo -e "Error: meson setup failed"
            exit 1
        fi
        meson compile -C build
        if [ $? -ne 0 ]; then
            echo -e "Error: meson compile failed"
            exit 1
        fi
        sudo meson install -C build
        if [ $? -ne 0 ]; then
            echo -e "Error: meson install failed"
            exit 1
        fi
        if [ ! -n "${SONAR_MODE_ARG}" ]; then
            rm -rf build
        else
            echo -e "Warn: The '--sonar' option is set. So, Skip to remove 'build' directory"
        fi
        cd - > /dev/null || exit 1
    done
}

build_and_install "./postprocess_library"
build_and_install "./message_convert_library"
