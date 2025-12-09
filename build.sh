#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
VENV_PATH=${PROJECT_ROOT}/venv-dx_stream

# Determine the architecture
ARCH=$(uname -m)

# Set the plugin directory based on pkg-config
GSTREAMER_PLUGIN_DIR=$(pkg-config --variable=pluginsdir gstreamer-1.0 2>/dev/null)

if [ -z "$GSTREAMER_PLUGIN_DIR" ]; then
    echo "Warning: Could not determine GStreamer plugin directory from pkg-config."
    echo "Falling back to architecture-based path..."
    
    # Fallback to architecture-based path
    if [ "$ARCH" == "x86_64" ]; then
        GSTREAMER_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
    elif [ "$ARCH" == "aarch64" ]; then
        GSTREAMER_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/gstreamer-1.0"
    else
        echo "Error: Unsupported architecture '$ARCH'."
        exit 1
    fi
fi

echo "Using GStreamer plugin directory: $GSTREAMER_PLUGIN_DIR"

# Define variables
WRC=$PWD
BUILD_DIR=builddir
BUILD_TYPE="release"
DEBUG_ARG=""
SONAR_MODE_ARG=""
V3_MODE=""

show_help() {
  echo "Usage: $(basename "$0") [--v3] [--debug] [--uninstall] [--help]"
  echo "Example 1): $0"
  echo "Example 2): $0 --v3"
  echo "Example 3): $0 --debug"
  echo "Example 4): $0 --uninstall"
  echo "Options:"
  echo "  [--v3]          Build for DEEPX V3 Standalone Device (skip Host installation)."
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
    
    # Clean local install directory
    if [ -d "${PROJECT_ROOT}/install" ]; then
        echo "Removing ${PROJECT_ROOT}/install"
        rm -rf "${PROJECT_ROOT}/install"
    else
        __print_clean_result "${mode}"
    fi
    
    # Also clean system-wide installations (for backward compatibility)
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

    V3_OPTION=""
    if [ "$V3_MODE" == "--v3" ]; then
        V3_OPTION="-Dv3_mode=true"
        echo "Building in V3 mode..."
    fi

    echo "Starting build process... build_type(${BUILD_TYPE})"
    cd gst-dxstream-plugin
    meson setup ${BUILD_DIR} --buildtype=${BUILD_TYPE} --prefix=${PROJECT_ROOT}/install ${V3_OPTION}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson setup failed"
        exit 1
    fi
    meson compile -C ${BUILD_DIR}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson compile failed"
        exit 1
    fi
    echo "Install DX-Stream to ${PROJECT_ROOT}/install"
    yes | meson install -C ${BUILD_DIR}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson install failed"
        exit 1
    fi

    if [ ! -n "${SONAR_MODE_ARG}" ]; then
        rm -rf ${BUILD_DIR}
    else
        echo -e "Warn: The '--sonar' option is set. So, Skip to remove '${BUILD_DIR}' directory"
    fi

    cd $WRC/dx_stream/custom_library
    ./build.sh ${DEBUG_ARG} ${SONAR_MODE_ARG} ${V3_MODE}
    if [ $? -ne 0 ]; then
        echo -e "Error: custom_library build failed"
        exit 1
    fi

    if [ "$V3_MODE" == "--v3" ]; then
        echo "Skipping Application build in V3 mode."
        echo "Skipping test_plugin build in V3 mode."
        echo "Skipping Installation step in V3 mode."
        return 0
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

    # Merge compile_commands.json files for SonarLint (only if sonar mode is enabled)
    if [ -n "${SONAR_MODE_ARG}" ]; then
        echo "Merging compile_commands.json files for SonarLint..."
        cd $WRC/dx_stream
        python3 scripts/merge_compile_commands.py
        if [ $? -eq 0 ]; then
            echo "Successfully merged compile_commands.json files"
        else
            echo "Warning: Failed to merge compile_commands.json files"
        fi
    fi

    echo "Build completed."
}

install() {
    if [ ! -f "${PROJECT_ROOT}/install/lib/gstreamer-1.0/libgstdxstream.so" ]; then
        echo -e "Error: ${PROJECT_ROOT}/install/lib/gstreamer-1.0/libgstdxstream.so Not found. Please run 'build.sh' first to build the project, then try again."
        exit 1
    fi
    echo "Installing libgstdxstream.so to $GSTREAMER_PLUGIN_DIR"
    sudo cp ${PROJECT_ROOT}/install/lib/gstreamer-1.0/libgstdxstream.so $GSTREAMER_PLUGIN_DIR/libgstdxstream.so
    sudo ln -s $GSTREAMER_PLUGIN_DIR/libgstdxstream.so /usr/local/lib/libgstdxstream.so
    sudo ldconfig

    if [ ! -d "${PROJECT_ROOT}/install/include/dx_stream" ]; then
        echo -e "Error: ${PROJECT_ROOT}/install/include/dx_stream Not found. Please run 'build.sh' first to build the project, then try again."
        exit 1
    fi
    echo "Installing header files to /usr/local/include/dx_stream"
    sudo cp -r ${PROJECT_ROOT}/install/include/dx_stream /usr/local/include

    # Install PostProcess models
    if [ ! -d "/usr/share/dx-stream/lib" ]; then
        sudo mkdir -p /usr/share/dx-stream/lib
    fi

    if [ ! -d "${PROJECT_ROOT}/install/lib/postprocess" ]; then
        echo -e "Error: ${PROJECT_ROOT}/install/lib/postprocess Not found. Please run 'build.sh' first to build the project, then try again."
        exit 1
    fi
    echo "Installing PostProcess models to /usr/share/dx-stream/lib"
    sudo cp -r ${PROJECT_ROOT}/install/lib/postprocess/* /usr/share/dx-stream/lib

    if [ ! -d "${PROJECT_ROOT}/install/lib/msgcomvert" ]; then
        echo -w "Error: ${PROJECT_ROOT}/install/lib/msgcomvert Not found. Please run 'build.sh' first to build the project, then try again."
        exit 1
    fi
    echo "Installing MsgComvert models to /usr/share/dx-stream/lib"
    sudo cp -r ${PROJECT_ROOT}/install/lib/msgcomvert/* /usr/share/dx-stream/lib

    # Install dx_stream applications
    if [ ! -d "/usr/share/dx-stream/bin" ]; then
        sudo mkdir -p /usr/share/dx-stream/bin
    fi
    if [ ! -d "${PROJECT_ROOT}/install/bin" ]; then
        echo -e "Error: ${PROJECT_ROOT}/install/bin Not found. Please run 'build.sh' first to build the project, then try again."
        exit 1
    fi
    echo "Installing dx_stream applications to /usr/share/dx-stream/bin"
    sudo cp -r ${PROJECT_ROOT}/install/bin/* /usr/share/dx-stream/bin
    sudo chmod +x /usr/share/dx-stream/bin/*

    exit 0
}

activate_venv() {
    # activate venv
    source ${VENV_PATH}/bin/activate
    if [ $? -ne 0 ]; then
        echo -e "[ERROR] Failed to activate venv. Please run 'install.sh' first to set up the venv environment, then try again."
        exit 1
    fi
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
        --v3)
            V3_MODE="--v3"
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

# activate_venv
build
install

exit 0
