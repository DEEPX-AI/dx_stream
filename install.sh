#!/bin/bash

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# DX-Stream Dependencies Installation Script
# Modularized setup using individual component scripts in scripts/setup/

# Project configuration
SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
DX_SRC_DIR=$PWD

echo "DX_SRC_DIR the default one $DX_SRC_DIR"
target_arch=$(uname -m)

# Color environment settings
if [ -f "${PROJECT_ROOT}/scripts/color_env.sh" ]; then
    source ${PROJECT_ROOT}/scripts/color_env.sh
else
    TAG_INFO="ℹ️"
fi

# Push current directory
pushd $SCRIPT_DIR > /dev/null

# Display help information
function help() {
    echo "DX-Stream Dependencies Installation Script"
    echo ""
    echo "Usage: ./install.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "    --help            Show this help message"
    echo "    --wo-gst          Skip GStreamer installation"
    echo "    --wo-opencv       Skip OpenCV installation"
    echo ""
    echo "This script installs all required dependencies for DX-Stream project"
    echo "using modularized setup scripts located in scripts/setup/"
    echo ""
    echo "Installation includes:"
    echo "  1. Essential build tools"
    echo "  2. CMake 3.17+"
    echo "  3. Meson 1.3+"
    echo "  4. GStreamer 1.16.3+ (skippable with --wo-gst)"
    echo "  5. OpenCV 4.2.0+ (skippable with --wo-opencv)"
    echo "  6. Communication libraries"
    echo "  7. libyuv library"
    echo "  8. gstreamer1.0-libcamera (Raspberry Pi only, for the camera demos)"
}

# Main dependency installation function using modular scripts
function install_dx_stream_dep() {
    echo "🚀 DX-Stream Dependencies Installation"
    echo "======================================="
    
    # Update package list
    echo "📦 Updating package lists..."
    sudo apt-get update -qq

    local SETUP_SCRIPTS_DIR="$PROJECT_ROOT/scripts/setup"
    
    if [ ! -d "$SETUP_SCRIPTS_DIR" ]; then
        echo "❌ Setup scripts directory not found: $SETUP_SCRIPTS_DIR"
        exit 1
    fi

    echo "📁 Using modular setup scripts from: $SETUP_SCRIPTS_DIR"

    # 1. Essential build tools
    echo "🔧 Step 1/7: Essential Build Tools"
    source "$SETUP_SCRIPTS_DIR/01_essential_tools.sh" || exit 1
    setup_essential_tools || exit 1

    # 2. CMake setup
    echo "🏗️  Step 2/7: CMake Build System"
    source "$SETUP_SCRIPTS_DIR/02_cmake.sh" || exit 1
    setup_cmake "cmake" "3.17" || exit 1

    # 3. Meson setup
    echo "⚙️  Step 3/7: Meson Build System"
    source "$SETUP_SCRIPTS_DIR/03_meson.sh" || exit 1
    setup_meson "1.3" || exit 1

    # 4. GStreamer setup
    if [ "$SKIP_GSTREAMER" != "true" ]; then
        echo "🎬 Step 4/7: GStreamer Framework"
        source "$SETUP_SCRIPTS_DIR/04_gstreamer.sh" || exit 1
        install_gstreamer_smart || exit 1
    else
        echo "⏭️  Step 4/7: GStreamer Framework (skipped by --wo-gst)"
    fi

    # 5. OpenCV setup
    if [ "$SKIP_OPENCV" != "true" ]; then
        echo "👁️  Step 5/7: OpenCV Library"
        source "$SETUP_SCRIPTS_DIR/05_opencv.sh" || exit 1
        setup_opencv "4.2.0" || exit 1
    else
        echo "⏭️  Step 5/7: OpenCV Library (skipped by --wo-opencv)"
    fi

    # 6. Communication libraries setup
    echo "🔗 Step 6/7: Communication Libraries"
    source "$SETUP_SCRIPTS_DIR/06_communication.sh" || exit 1
    setup_communication_libraries || exit 1

    # 7. libyuv setup
    echo "📺 Step 7/7: libyuv Library"
    source "$SETUP_SCRIPTS_DIR/07_libyuv.sh" || exit 1
    setup_libyuv || exit 1

    # Raspberry Pi camera support (optional, never fails the installation)
    echo "📷 Raspberry Pi Camera Support"
    source "$SETUP_SCRIPTS_DIR/08_rpi_camera.sh" || exit 1
    setup_rpi_camera

    echo "✅ All DX-Stream dependencies installed successfully!"
}

# Display success message
function show_information_message() {
    echo ""
    echo "✅ DX-Stream installation completed successfully!"
}

# Parse command line arguments
SKIP_GSTREAMER=false
SKIP_OPENCV=false

[ $# -gt 0 ] && \
while (( $# )); do
    case "$1" in
        --help) help; exit 0;;
        --wo-gst) SKIP_GSTREAMER=true;;
        --wo-opencv) SKIP_OPENCV=true;;
        *)       echo "❌ Invalid argument: $1"; help; exit 1;;
    esac
    shift
done

# Architecture handling
if [ $target_arch == "arm64" ]; then
    target_arch=aarch64
    echo "ℹ️  Using aarch64 instead of arm64"
fi

# Main execution function
function main() {
    echo "🚀 Starting DX-Stream Dependencies Installation..."
    echo "Target Architecture: $target_arch"
    
    # Display skip information if applicable
    [ "$SKIP_GSTREAMER" = "true" ] && echo "⏭️  GStreamer installation will be skipped (--wo-gst)"
    [ "$SKIP_OPENCV" = "true" ] && echo "⏭️  OpenCV installation will be skipped (--wo-opencv)"
    
    install_dx_stream_dep
    show_information_message
}

# Execute main function
main

# Restore directory
popd > /dev/null
