#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")

BUILD_TYPE="release"
SONAR_MODE_ARG=""
NATIVE_FILE_ARG=""
CLEAN_MODE=""
PREFIX="/usr/local"

show_help() {
  echo "Usage: $(basename "$0") [--prefix=PATH] [--type=TYPE] [--help]"
  echo "Example 1): $0"
  echo "Example 2): $0 --prefix=/opt/dx-stream"
  echo "Example 3): $0 --clean"
  echo "Example 4): $0 --type=debug"
  echo "Example 5): $0 --type=Release"
  echo "Options:"
  echo "  [--prefix=PATH] Set installation prefix (default: /usr/local)"
  echo "  [--clean]       Clean build directories before building"
  echo "  [--type=TYPE]   Set build type: Debug/debug or Release/release (default: release)"
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
        --prefix=*)
            PREFIX="${1#*=}"
            echo "Using custom PREFIX: ${PREFIX}"
            ;;
        --clean)
            CLEAN_MODE="--clean"
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

build_and_install() {
    TARGET_DIR="."
    for subdir in "$TARGET_DIR"/*/; do
        echo "Processing directory: $subdir"

        # Skip if meson.build doesn't exist
        if [ ! -f "${subdir}meson.build" ]; then
            echo "Skipping directory (no meson.build): $subdir"
            continue
        fi
        
        cd "$subdir" || exit 1

        if [ "$CLEAN_MODE" == "--clean" ]; then
            echo "Cleaning build directory in $subdir"
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

        cd ./.. || exit 1
    done    
}

build_and_install
