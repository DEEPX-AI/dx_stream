#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")

BUILD_TYPE="release"
SONAR_MODE_ARG=""
NATIVE_FILE_ARG=""
V3_MODE=""
CLEAN_MODE=""
PREFIX="/usr/local"

show_help() {
  echo "Usage: $(basename "$0") [--prefix=PATH] [--clean] [--type=TYPE] [--help]"
  echo "Example 1): $0"
  echo "Example 2): $0 --prefix=/opt/dx-stream"
  echo "Example 3): $0 --clean"
  echo "Example 4): $0 --type=debug"
  echo "Example 5): $0 --type=Release"
  echo "Example 6): $0 --v3"
  echo "Example 7): $0 --uninstall"
  echo "Options:"
  echo "  [--prefix=PATH] Set installation prefix (default: /usr/local)"
  echo "  [--clean]       Clean build directories before building"
  echo "  [--type=TYPE]   Set build type: Debug/debug or Release/release (default: release)"
  echo "  [--v3]          Build for DEEPX V3 Standalone Device (skip Host installation)."
  echo "  [--uninstall]   Uninstall previously installed libraries."
  echo "  [--help]        Show this help message"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}

uninstall() {
    TARGET_DIR="$1"
    for subdir in "$TARGET_DIR"/*/; do
        cd "$subdir" || exit 1
        if [ -d "builddir" ]; then
            rm -rf builddir
        else
            echo "Warn: builddir not found in $subdir. So, skip uninstall."
        fi
        cd - > /dev/null || exit 1
    done
}

# Parse arguments
for i in "$@"; do
    case "$1" in
        --prefix=*)
            PREFIX="${1#*=}"
            echo "Using custom PREFIX: ${PREFIX}"
            ;;
        --type=*)
            BUILD_TYPE_INPUT="${1#*=}"
            # Convert to lowercase for validation
            BUILD_TYPE_LOWER=$(echo "$BUILD_TYPE_INPUT" | tr '[:upper:]' '[:lower:]')
            
            # Validate: only 'debug' or 'release' allowed
            if [[ "$BUILD_TYPE_LOWER" != "debug" && "$BUILD_TYPE_LOWER" != "release" ]]; then
                echo "Error: Invalid build type '$BUILD_TYPE_INPUT'"
                echo "Allowed values: Debug, debug, Release, release"
                exit 1
            fi
            
            # Use lowercase for meson
            BUILD_TYPE="$BUILD_TYPE_LOWER"
            ;;
        --clean)
            CLEAN_MODE="--clean"
            ;;
        --v3)
            V3_MODE="--v3"
            ;;
        --sonar)
            SONAR_MODE_ARG="--sonar"
            ;;
        --uninstall)
            uninstall "./postprocess_library"
            uninstall "./message_convert_library"
            exit 0
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
        # Skip if meson.build doesn't exist
        if [ ! -f "${subdir}meson.build" ]; then
            echo "Skipping directory (no meson.build): $subdir"
            continue
        fi
        
        echo "Processing directory: $subdir"
        
        cd "$subdir" || exit 1

        if [ "$CLEAN_MODE" == "--clean" ]; then
            echo "Cleaning build directory..."
            rm -rf builddir
        fi

        # Setup meson with cache handling
        if [ -d "builddir" ] && [ "$CLEAN_MODE" != "--clean" ]; then
            echo "Reconfiguring existing build directory..."
            meson setup builddir --reconfigure --prefix="${PREFIX}" --buildtype="$BUILD_TYPE"
        else
            echo "Setting up fresh build directory..."
            meson setup builddir --wipe --prefix="${PREFIX}" --buildtype="$BUILD_TYPE"
        fi
        if [ $? -ne 0 ]; then
            echo -e "Error: meson setup failed"
            exit 1
        fi

        meson compile -C builddir
        if [ $? -ne 0 ]; then
            echo -e "Error: meson compile failed"
            exit 1
        fi

        yes | meson install -C builddir
        if [ $? -ne 0 ]; then
            echo -e "Error: meson install failed"
            exit 1
        fi

        cd - > /dev/null || exit 1
    done
}

build_and_install "./postprocess_library"

if [ "$V3_MODE" != "--v3" ]; then
    build_and_install "./message_convert_library"
fi
