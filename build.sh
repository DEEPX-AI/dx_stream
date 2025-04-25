#!/bin/bash

# Determine the architecture
ARCH=$(uname -m)

# Set the plugin directory based on the architecture
if [ "$ARCH" == "x86_64" ]; then
    GSTREAMER_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
elif [ "$ARCH" == "aarch64" ]; then
    GSTREAMER_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/gstreamer-1.0"
else
    echo "Error: Unsupported architecture '$ARCH'."
    exit 1
fi

# Define variables
WRC=$PWD
BUILD_DIR=builddir
BUILD_TYPE="release"
DEBUG_ARG=""
SONAR_MODE_ARG=""


show_help() {
  echo "Usage: $(basename "$0") [--debug] [--uninstall] [--help]"
  echo "Example 1): $0"
  echo "Example 2): $0 --debug"
  echo "Example 3): $0 --uninstall"
  echo "Options:"
  echo "  [--uninstall]   Remove installed files."
  echo "  [--debug]       Build for debug"
  echo "  [--help]        Show this help message"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}


__print_clean_result() {
    local mode="$1"

    if [[ "${mode}" == "uninstall" ]]; then
        echo "Warn: $TARGET_FILE not found. Was the project installed?"
    else
        echo "Warn: $TARGET_FILE not found. So, skip to remove file."
    fi
}

clean() {
    local mode="$1"     # uninstall | clean

    echo "Starting ${mode} process..."
    TARGET_FILE="/usr/local/lib/libgstdxstream.so"
    if [ -f "$TARGET_FILE" ]; then
        echo "Removing $TARGET_FILE..."
        sudo rm "$TARGET_FILE"
    else
        __print_clean_result "${mode}"
    fi
    
    TARGET_FILE="$GSTREAMER_PLUGIN_DIR/libgstdxstream.so"
    if [ -f "$TARGET_FILE" ]; then
        echo "Removing $TARGET_FILE..."
        sudo rm "$TARGET_FILE"
    else
        __print_clean_result "${mode}"
    fi

    if [ -d "/usr/local/include/dx_stream" ]; then
        echo "Removing /usr/local/include/dx_stream"
        sudo rm -rf /usr/local/include/dx_stream
    else
        __print_clean_result "${mode}"
    fi

    if [ -d "/usr/share/dx-stream" ]; then
        echo "Removing /usr/share/dx-stream"
        sudo rm -rf /usr/share/dx-stream
    else
        __print_clean_result "${mode}"
    fi

    echo "${mode} completed."
}

build() {
    clean "clean"

    echo "Starting build process... build_type(${BUILD_TYPE})"
    cd gst-dxstream-plugin
    meson setup ${BUILD_DIR} --buildtype=${BUILD_TYPE}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson setup failed"
        exit 1
    fi
    meson compile -C ${BUILD_DIR}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson compile failed"
        exit 1
    fi
    echo "Install DX-Stream to $GSTREAMER_PLUGIN_DIR"
    sudo meson install -C ${BUILD_DIR}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson install failed"
        exit 1
    fi

    if [ ! -n "${SONAR_MODE_ARG}" ]; then
        rm -rf ${BUILD_DIR}
    else
        echo -e "Warn: The '--sonar' option is set. So, Skip to remove '${BUILD_DIR}' directory"
    fi
    
    sudo ln -s $GSTREAMER_PLUGIN_DIR/libgstdxstream.so /usr/local/lib/libgstdxstream.so
    sudo ldconfig

    cd $WRC/dx_stream/custom_library
    ./build.sh ${DEBUG_ARG} ${SONAR_MODE_ARG}
    if [ $? -ne 0 ]; then
        echo -e "Error: custom_library build failed"
        exit 1
    fi

    cd $WRC/dx_stream/apps
    ./build.sh ${DEBUG_ARG} ${SONAR_MODE_ARG}
    if [ $? -ne 0 ]; then
        echo -e "Error: apps build failed"
        exit 1
    fi

    cd $WRC/dx_stream/test/test_plugin
    ./build.sh ${DEBUG_ARG} ${SONAR_MODE_ARG}
    if [ $? -ne 0 ]; then
        echo -e "Error: test_plugin build failed"
        exit 1
    fi

    echo "Build completed."
}

# Parse arguments
for i in "$@"; do
    case "$1" in
        --install)
            # nothing to do (for legacy option backward compatibility)
            ;;
        --uninstall)
            clean "uninstall"
            exit 0
            ;;
        --debug)
            BUILD_TYPE="debug"
            DEBUG_ARG="--debug"
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


build

exit 0
