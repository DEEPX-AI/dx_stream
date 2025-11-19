#!/bin/bash

# OpenCV setup for DX-Stream
# This script handles OpenCV installation with smart version checking and source build fallback

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Check video backend availability (GStreamer or FFmpeg)
check_video_backend() {
    local has_gstreamer=false
    local has_ffmpeg=false
    
    if pkg-config --exists gstreamer-1.0; then
        local gst_version=$(pkg-config --modversion gstreamer-1.0)
        print_message "success" "GStreamer v$gst_version detected - will be used for video I/O"
        has_gstreamer=true
    fi
    
    if command -v ffmpeg >/dev/null 2>&1; then
        local ffmpeg_version=$(ffmpeg -version 2>&1 | head -n1 | awk '{print $3}' | cut -d'-' -f1)
        if [ -n "$ffmpeg_version" ]; then
            print_message "success" "FFmpeg v$ffmpeg_version detected - available as fallback"
            has_ffmpeg=true
        fi
    fi
    
    if [ "$has_gstreamer" = false ] && [ "$has_ffmpeg" = false ]; then
        print_message "warning" "Neither GStreamer nor FFmpeg found!"
        print_message "warning" "OpenCV will be built with minimal video support"
        print_message "info" "Consider installing GStreamer or FFmpeg for full functionality"
        echo "minimal"
    elif [ "$has_gstreamer" = true ]; then
        echo "gstreamer"
    elif [ "$has_ffmpeg" = true ]; then
        echo "ffmpeg"
    fi
}

# Install OpenCV build dependencies
install_opencv_build_dependencies() {
    print_message "install" "Installing OpenCV build dependencies..."
    
    sudo apt-get update
    sudo apt-get install -y \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        libavcodec-dev \
        libavformat-dev \
        libswscale-dev \
        libxvidcore-dev \
        libavutil-dev \
        libtbb-dev \
        libeigen3-dev \
        libx264-dev \
        libv4l-dev \
        v4l-utils \
        libgtk2.0-dev \
        libopenexr-dev \
        unzip wget
}

# Build OpenCV from source
build_opencv_from_source() {
    local version=${1:-"4.5.5"}
    
    print_message "build" "Building OpenCV from source (version: ${version})"
    
    # Check video backend availability
    local backend=$(check_video_backend)
    
    # Install build dependencies
    install_opencv_build_dependencies
    
    local build_dir="$DX_SRC_DIR/util"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Download OpenCV source
    print_message "info" "Downloading OpenCV ${version} source..."
    if [ ! -f "opencv-${version}.zip" ]; then
        if ! wget --tries=1 --timeout=30 --connect-timeout=10 -O "opencv-${version}.zip" "https://github.com/opencv/opencv/archive/${version}.zip"; then
            print_message "error" "Failed to download OpenCV. Exiting."
            exit 1
        fi
        
        print_message "search" "Verifying OpenCV download integrity..."
        if ! unzip -t "opencv-${version}.zip" >/dev/null 2>&1; then
            print_message "error" "OpenCV download is corrupted. Exiting."
            rm -f "opencv-${version}.zip"
            exit 1
        fi
    fi
    
    if [ ! -f "opencv_contrib-${version}.zip" ]; then
        if ! wget --tries=1 --timeout=30 --connect-timeout=10 -O "opencv_contrib-${version}.zip" "https://github.com/opencv/opencv_contrib/archive/${version}.zip"; then
            print_message "error" "Failed to download OpenCV contrib. Exiting."
            exit 1
        fi
        
        print_message "search" "Verifying OpenCV contrib download integrity..."
        if ! unzip -t "opencv_contrib-${version}.zip" >/dev/null 2>&1; then
            print_message "error" "OpenCV contrib download is corrupted. Exiting."
            rm -f "opencv_contrib-${version}.zip"
            exit 1
        fi
    fi
    
    print_message "info" "Extracting source archives..."
    unzip -oq "opencv-${version}.zip"
    unzip -oq "opencv_contrib-${version}.zip"
    
    cd "opencv-${version}"
    mkdir -p build && cd build
    
    # Configure CMake options based on available backend
    local cmake_opts=(
        -D CMAKE_BUILD_TYPE=RELEASE
        -D CMAKE_INSTALL_PREFIX=/usr/local
        -D OPENCV_EXTRA_MODULES_PATH="../../opencv_contrib-${version}/modules"
        -D BUILD_LIST="imgcodecs,imgproc,core,highgui,videoio"
        -D WITH_TBB=ON
        -D WITH_V4L=ON
        -D WITH_GTK=ON
        -D BUILD_EXAMPLES=OFF
        -D BUILD_TESTS=OFF
        -D BUILD_PERF_TESTS=OFF
        -D OPENCV_GENERATE_PKGCONFIG=ON
        -D WITH_CUDA=OFF
        -D WITH_1394=OFF
    )
    
    # Set video backend options
    case "$backend" in
        "gstreamer")
            print_message "info" "Configuring with GStreamer support"
            cmake_opts+=(-D WITH_GSTREAMER=ON -D WITH_FFMPEG=OFF)
            ;;
        "ffmpeg")
            print_message "info" "Configuring with FFmpeg support"
            cmake_opts+=(-D WITH_GSTREAMER=OFF -D WITH_FFMPEG=ON)
            ;;
        "minimal")
            print_message "warning" "Configuring with minimal video support (no GStreamer/FFmpeg)"
            cmake_opts+=(-D WITH_GSTREAMER=OFF -D WITH_FFMPEG=OFF)
            ;;
    esac
    
    print_message "build" "Configuring OpenCV with CMake..."
    cmake "${cmake_opts[@]}" ..
    
    if [ $? -ne 0 ]; then
        print_message "error" "CMake configuration failed"
        return 1
    fi
    
    print_message "build" "Compiling OpenCV..."
    make -j$(($(nproc) / 2))
    
    if [ $? -ne 0 ]; then
        print_message "error" "OpenCV compilation failed"
        return 1
    fi
    
    print_message "install" "Installing OpenCV..."
    sudo make install
    
    if [ $? -ne 0 ]; then
        print_message "error" "OpenCV installation failed"
        return 1
    fi
    
    sudo ldconfig
    
    # Cleanup
    cd "$build_dir"
    rm -rf "opencv-${version}" "opencv_contrib-${version}"
    
    print_message "success" "OpenCV ${version} installation completed!"
    
    # Verify installation
    if pkg-config --exists opencv4; then
        local installed_version=$(pkg-config --modversion opencv4)
        print_message "success" "OpenCV ${installed_version} ready"
    fi
}

# Setup OpenCV with smart version checking
setup_opencv() {
    local required_version=${1:-"4.2.0"}
    
    print_message "search" "Checking OpenCV installation..."
    
    # Check if OpenCV is already installed and meets requirements
    local current_version=""
    local opencv_ok=false
    
    # Ensure pkg-config can find locally installed packages
    local arch=$(dpkg --print-architecture 2>/dev/null || uname -m)
    export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib/${arch}-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/lib/${arch}-linux-gnu/pkgconfig:${PKG_CONFIG_PATH}"
    
    # Check via pkg-config (opencv4 or opencv)
    if pkg-config --exists opencv4; then
        current_version=$(pkg-config --modversion opencv4 2>/dev/null)
    elif pkg-config --exists opencv; then
        current_version=$(pkg-config --modversion opencv 2>/dev/null)
    fi
    
    if [ -n "$current_version" ]; then
        if [ "$(printf '%s\n%s' "$required_version" "$current_version" | sort -V | head -n1)" = "$required_version" ]; then
            print_message "success" "OpenCV v$current_version meets requirement (>= $required_version)"
            return 0
        else
            print_message "warning" "OpenCV v$current_version below requirement ($required_version)"
        fi
    else
        print_message "warning" "OpenCV not found"
    fi
    
    # Check APT availability
    print_message "search" "Checking APT OpenCV availability..."
    local apt_version=$(apt-cache policy libopencv-dev 2>/dev/null | awk '/Candidate:/ {print $2}' | cut -d'+' -f1 | cut -d'-' -f1)
    
    if [ -n "$apt_version" ] && [ "$apt_version" != "(none)" ]; then
        # Compare versions
        if [ "$(printf '%s\n%s' "$required_version" "$apt_version" | sort -V | head -n1)" = "$required_version" ]; then
            print_message "success" "APT version $apt_version meets requirement (>= $required_version)"
            print_message "install" "Installing OpenCV via APT..."
            sudo apt-get install -y libopencv-dev 2>&1 | grep -v "Note, selecting"
            
            # Verify APT installation
            if pkg-config --exists opencv4 || pkg-config --exists opencv; then
                local installed_version=$(pkg-config --modversion opencv4 2>/dev/null || pkg-config --modversion opencv 2>/dev/null)
                print_message "success" "OpenCV ${installed_version} installed via APT"
                return 0
            fi
        else
            print_message "warning" "APT version $apt_version below requirement ($required_version)"
        fi
    else
        print_message "warning" "OpenCV not available in APT"
    fi
    
    # Fallback to source build
    print_message "warning" "Building OpenCV from source..."
    build_opencv_from_source "4.5.5"
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_opencv "${1:-4.2.0}"
fi