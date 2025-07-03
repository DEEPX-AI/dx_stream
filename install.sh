#!/bin/bash
# dependencies install script in host

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
VENV_PATH=${PROJECT_ROOT}/venv-dx_stream

cmd=()
DX_SRC_DIR=$PWD
echo "DX_SRC_DIR the default one $DX_SRC_DIR"
target_arch=$(uname -m)
build_type='Release'

# color env settings
source ${PROJECT_ROOT}/scripts/color_env.sh

pushd $SCRIPT_DIR

function help()
{
    echo "./install.sh"
    echo "    --help            show this help"
}

# Version comparison: returns 0 if $1 < $2
version_lt() {
    dpkg --compare-versions "$1" lt "$2"
}

# Install package via apt
install_apt_pkgs() {
    local pacakges=$1
    echo "Installing $pkgs from apt..."
    sudo apt-get update
    sudo apt-get install -y "$pkgs"
}

# Function to get installed CMake version
get_cmake_version() {
    cmake --version 2>/dev/null | head -n1 | awk '{print $3}'
}

# Build CMake from source
build_cmake_from_source() {
    local version=$1
    echo "Building CMake $version from source..."
    sudo apt-get update
    sudo apt-get install -y build-essential libssl-dev wget

    cd ${PROJECT_ROOT}/util
    rm -rf "cmake-$version" "cmake-$version.tar.gz"
    wget "https://github.com/Kitware/CMake/releases/download/v$version/cmake-$version.tar.gz"
    tar -zxvf "cmake-$version.tar.gz"
    cd "cmake-$version"

    ./bootstrap -- -DCMAKE_BUILD_TYPE:STRING=Release
    make -j$(nproc)
    sudo make install
    sudo ldconfig

    echo "CMake $(cmake --version | head -n1) installed from source!"
}

setup_cmake() {
    local package_name=$1
    local required_version=$2
    local installed_version=$(get_cmake_version || echo "")
    
    local build_needed=0
    local install_needed=0
    if [ -z "$installed_version" ]; then
        echo "${package_name} is not installed. Source build is required."
        install_needed=1

        local apt_prebuilt_version=$(get_apt_prebuilt_version "${package_name}")

        if version_lt "$apt_prebuilt_version" "$required_version"; then
            echo "${package_name} apt prebuilt version: $apt_prebuilt_version (required: $required_version). Source build is required."
            build_needed=1
        else
            echo "${package_name} apt prebuilt version: $apt_prebuilt_version (meets requirement). No source build needed."
            install_needed=1
            
        fi
    else
        if version_lt "$installed_version" "$required_version"; then
            echo "Installed ${package_name} version: $installed_version (required: $required_version). Source build is required."
            build_needed=1
        else
            echo "Installed ${package_name} version: $installed_version (meets requirement). No source build needed."
        fi
    fi

    if [ $build_needed -eq 1 ]; then
        if [ -n "$installed_version" ]; then
            remove_apt_package "$package_name"
        fi
        build_cmake_from_source "$required_version"
    elif [ $install_needed -eq 1 ]; then
        install_apt_pkgs "$package_name"
    fi
}

# Install Meson
install_meson() {
    UBUNTU_VERSION=$(lsb_release -rs)
    
    if [ "$UBUNTU_VERSION" = "24.04" ]; then
        echo "Ubuntu 24.04 detected. Installing Meson using --break-system-packages..."
        python3 -m pip install --break-system-packages meson==1.3
    elif [ "$UBUNTU_VERSION" = "22.04" ] || [ "$UBUNTU_VERSION" = "20.04" ]; then
        echo "Installing Meson using system Python..."
        python3 -m pip install meson==1.3
    elif [ "$UBUNTU_VERSION" = "18.04" ]; then
        echo "Installing Meson using Python 3.7..."
        sudo add-apt-repository ppa:deadsnakes/ppa -y
        sudo apt-get update
        sudo apt-get install -y python3.7 python3.7-dev python3.7-distutils
        curl -sSL https://bootstrap.pypa.io/pip/3.7/get-pip.py -o get-pip.py
        python3.7 get-pip.py
        python3.7 -m pip install meson==1.3
        rm -rf get-pip.py
    else
        echo "Unsupported Ubuntu version: $UBUNTU_VERSION"
        exit 1
    fi
}

# Get installed version
get_apt_prebuilt_version() {
    local package_name=$1
    dpkg-query -W -f='${Version}\n' ${package_name} 2>/dev/null | head -n1
}

# Compare versions: returns 0 if $1 < $2
version_lt() {
    dpkg --compare-versions "$1" lt "$2"
}

# Remove package via apt
remove_apt_package() {
    local pkg=$1
    echo "Removing apt package: $pkg"
    sudo apt-get remove --purge -y "$pkg" || true
    sudo apt-get autoremove -y
    sudo ldconfig
}

function install_opencv()
{
    local suggestion_version="4.5.5"
    
    echo "Installing OpenCV dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        wget \
        unzip \
        libjpeg-dev \
        libpng-dev \
        libtiff-dev \
        ffmpeg \
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
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        libgtk2.0-dev \
        libopenexr-dev
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install dependencies." >&2
        exit 1
    fi

    mkdir -p "$PROJECT_ROOT/util"
    cd "$PROJECT_ROOT/util"

    if [ ! -f "opencv-${suggestion_version}.zip" ]; then
        wget -O "opencv-${suggestion_version}.zip" "https://github.com/opencv/opencv/archive/${suggestion_version}.zip"
    fi
    if [ ! -f "opencv_contrib-${suggestion_version}.zip" ]; then
        wget -O "opencv_contrib-${suggestion_version}.zip" "https://github.com/opencv/opencv_contrib/archive/${suggestion_version}.zip"
    fi

    unzip -oq "opencv-${suggestion_version}.zip"
    unzip -oq "opencv_contrib-${suggestion_version}.zip"

    cd "opencv-${suggestion_version}"
    mkdir -p build && cd build
    
    cmake \
        -D CMAKE_BUILD_TYPE=RELEASE \
        -D CMAKE_INSTALL_PREFIX=/usr/local \
        -D OPENCV_EXTRA_MODULES_PATH="../../opencv_contrib-${suggestion_version}/modules" \
        -D BUILD_LIST="imgcodecs,imgproc,core,highgui,videoio" \
        -D WITH_TBB=ON \
        -D WITH_V4L=ON \
        -D WITH_FFMPEG=OFF \
        -D WITH_GSTREAMER=ON \
        -D WITH_GTK=ON \
        -D BUILD_EXAMPLES=OFF \
        -D BUILD_TESTS=OFF \
        -D BUILD_PERF_TESTS=OFF \
        -D OPENCV_GENERATE_PKGCONFIG=ON \
        -D WITH_CUDA=OFF \
        -D WITH_1394=OFF \
        ..

    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to configure CMake." >&2
        exit 1
    fi

    make -j$(($(nproc) / 2))
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to compile OpenCV." >&2
        exit 1
    fi

    sudo make install
    
    if [ $? -ne 0 ]; then
        echo "ERROR: Failed to install OpenCV." >&2
        exit 1
    fi

    sudo ldconfig
    
    echo "OpenCV ${suggestion_version} installation completed successfully."
}

function install_ffmpeg_from_source() {
  local ffmpeg_version=${1:-"4.4"}  # 기본값으로 4.4 사용
  
  echo "🚀 FFmpeg build from source (version: ${ffmpeg_version})"
  
  echo "📦 Install FFmpeg build dependencies..."
  sudo apt-get update
  sudo apt-get install -y \
    build-essential git wget pkg-config \
    libx264-dev libx265-dev libnuma-dev libvpx-dev \
    libmp3lame-dev libopus-dev libvorbis-dev libtheora-dev \
    libass-dev libfreetype6-dev libfontconfig1-dev libfribidi-dev \
    libgnutls28-dev libz-dev libbz2-dev liblzma-dev

  local build_dir="${PROJECT_ROOT}/util/ffmpeg_build"
  mkdir -p "$build_dir"
  cd "$build_dir"
  
  echo "📥 Downloading FFmpeg ${ffmpeg_version} source..."
  # Remove any existing corrupted files
  rm -f "ffmpeg-${ffmpeg_version}.tar.bz2"*
  wget "https://ffmpeg.org/releases/ffmpeg-${ffmpeg_version}.tar.bz2"
  
  # Verify download integrity
  echo "🔍 Verifying download integrity..."
  if ! bzip2 -t "ffmpeg-${ffmpeg_version}.tar.bz2"; then
    echo "❌ Download corrupted. Retrying..."
    rm -f "ffmpeg-${ffmpeg_version}.tar.bz2"
    wget "https://ffmpeg.org/releases/ffmpeg-${ffmpeg_version}.tar.bz2"
    if ! bzip2 -t "ffmpeg-${ffmpeg_version}.tar.bz2"; then
      echo "❌ Download still corrupted. Exiting."
      exit 1
    fi
  fi
  
  tar -xf "ffmpeg-${ffmpeg_version}.tar.bz2"
  cd "ffmpeg-${ffmpeg_version}"
  
  echo "🔨 Configuring FFmpeg..."
  ./configure \
    --prefix=/usr/local \
    --enable-gpl \
    --enable-libx264 \
    --enable-libx265 \
    --enable-libvpx \
    --enable-libmp3lame \
    --enable-libopus \
    --enable-libvorbis \
    --enable-libtheora \
    --enable-shared \
    --enable-static
  
  echo "🔨 Building FFmpeg..."
  make -j$(nproc)
  sudo make install
  sudo ldconfig
  
  echo "✅ FFmpeg build completed!"
  ffmpeg -version
  
  # Clean up
  cd ..
  rm -rf "ffmpeg-${ffmpeg_version}"*
}

function install_gstreamer_from_source() {
  local gstreamer_version=${1:-"1.16.3"}  # 기본값으로 1.16.3 사용
  
  set -e

  echo "🚀 GStreamer build from source (version: ${gstreamer_version})"
  
  echo "📦 Install build tools and dependencies..."
  sudo apt-get update
  sudo apt-get install -y \
    build-essential git python3-pip ninja-build flex bison nasm pkg-config \
    libx264-dev libx265-dev libnuma-dev libvpx-dev libfdk-aac-dev \
    libmp3lame-dev libopus-dev \
    libglib2.0-dev libgirepository1.0-dev libcairo2-dev libpango1.0-dev \
    libgdk-pixbuf2.0-dev libatk1.0-dev libgtk-3-dev

  # Check if FFmpeg is installed and has sufficient version
  if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "FFmpeg not found. Installing from source..."
    install_ffmpeg_from_source "4.4"
  else
    local ffmpeg_version=$(ffmpeg -version | head -n1 | awk '{print $3}' | cut -d'-' -f1)
    local required_ffmpeg_version="4.0"
    
    if [ "$(printf '%s\n' "$required_ffmpeg_version" "$ffmpeg_version" | sort -V | head -n1)" != "$required_ffmpeg_version" ]; then
      echo "FFmpeg version $ffmpeg_version is below required version $required_ffmpeg_version. Installing from source..."
      install_ffmpeg_from_source "4.4"
    else
      echo "FFmpeg version $ffmpeg_version meets requirement (>= $required_ffmpeg_version)"
    fi
  fi

  export PATH="$HOME/.local/bin:$PATH"

  local build_dir="${PROJECT_ROOT}/util/gstreamer_conditional_build"
  local min_version="${gstreamer_version}"
  local modules=(
    "gstreamer"
    "gst-plugins-base"
    "gst-plugins-good"
    "gst-plugins-bad"
    "gst-libav"
  )
  
  if [ -d "$build_dir" ]; then
    echo "🧹 Cleaning up existing build directory: $build_dir"
    rm -rf "$build_dir"
  fi

  mkdir -p "$build_dir"
  cd "$build_dir"
  echo "📂 Manage source code in [ $build_dir ] directory"
  
  for module in "${modules[@]}"; do
    echo ""
    echo "▶️ Check [ ${module} ] module..."

    local pkg_name=""
    case "$module" in
      gstreamer)        pkg_name="gstreamer-1.0" ;;
      gst-plugins-base) pkg_name="gstreamer-plugins-base-1.0" ;;
      gst-plugins-good) pkg_name="gstreamer-plugins-good-1.0" ;;
      gst-plugins-bad)  pkg_name="gstreamer-plugins-bad-1.0" ;;
      gst-libav)        pkg_name="gstreamer-libav-1.0" ;;
    esac

    local installed_version
    installed_version=$(pkg-config --modversion "$pkg_name" 2>/dev/null || echo "")
    
    # Also check if the library files exist
    local lib_exists=false
    case "$module" in
      gstreamer)        [ -f "/usr/local/lib/libgstreamer-1.0.so" ] && lib_exists=true ;;
      gst-plugins-base) [ -f "/usr/local/lib/libgstbase-1.0.so" ] && lib_exists=true ;;
      gst-plugins-good) [ -f "/usr/local/lib/libgstgood-1.0.so" ] && lib_exists=true ;;
      gst-plugins-bad)  [ -f "/usr/local/lib/libgstbad-1.0.so" ] && lib_exists=true ;;
      gst-libav)        [ -f "/usr/local/lib/libgstlibav-1.0.so" ] && lib_exists=true ;;
    esac

    if [ -n "$installed_version" ] && [ "$lib_exists" = "true" ]; then
      local highest_version
      highest_version=$(printf '%s\n%s' "$min_version" "$installed_version" | sort -V | tail -n1)
      
      if [ "$highest_version" = "$installed_version" ]; then
        echo "✅ Skip: installed version(${installed_version}) is greater than or equal to minimum version(${min_version})"
        continue
      else
        echo "⚠️ Build needed: installed version(${installed_version}) is less than minimum version(${min_version})"
      fi
    else
      echo "ℹ️ Build needed: [ ${module} ] is not installed (pkg-config: ${installed_version:-"not found"}, lib: $([ "$lib_exists" = "true" ] && echo "exists" || echo "not found"))"
    fi

    echo "Build..."
    echo "📥 Downloading ${module} ${min_version} source archive..."
    
    # Download source archive for the specific module
    local download_url=""
    case "$module" in
      gstreamer)        download_url="https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-${min_version}.tar.xz" ;;
      gst-plugins-base) download_url="https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-${min_version}.tar.xz" ;;
      gst-plugins-good) download_url="https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${min_version}.tar.xz" ;;
      gst-plugins-bad)  download_url="https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-${min_version}.tar.xz" ;;
      gst-libav)        download_url="https://gstreamer.freedesktop.org/src/gst-libav/gst-libav-${min_version}.tar.xz" ;;
    esac
    
    wget "$download_url"
    
    echo "📦 Extracting source archive..."
    local archive_name=$(basename "$download_url")
    tar -xf "$archive_name"
    
    # Get the extracted directory name
    local extracted_dir=""
    case "$module" in
      gstreamer)        extracted_dir="gstreamer-${min_version}" ;;
      gst-plugins-base) extracted_dir="gst-plugins-base-${min_version}" ;;
      gst-plugins-good) extracted_dir="gst-plugins-good-${min_version}" ;;
      gst-plugins-bad)  extracted_dir="gst-plugins-bad-${min_version}" ;;
      gst-libav)        extracted_dir="gst-libav-${min_version}" ;;
    esac
    
    cd "$extracted_dir"
    meson setup build --prefix=/usr/local
    ninja -C build
    sudo ninja -C build install
    cd ..
    
    # Clean up downloaded archive
    rm -f "$archive_name"
    
    # Update pkg-config cache after each module installation
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
  done

  echo ""
  echo "🔗 Update system library cache..."
  sudo ldconfig

  echo "✅ GStreamer build completed!"
  gst-inspect-1.0 --version
  
  set +e
}

function install_dx_stream_dep() {
    echo "Install DX-Stream Build Dependencies"

    UBUNTU_VERSION=$(lsb_release -rs)

    sudo apt-get update

    # Build tools
    sudo apt-get install -y apt-utils software-properties-common cmake build-essential make zlib1g-dev libcurl4-openssl-dev git curl wget tar zip unzip ninja-build

    local required_cmake_version="3.17.0"
    setup_cmake "cmake" "${required_cmake_version}"

    # Install Meson (already handled by install_meson function)
    if ! command -v meson >/dev/null 2>&1; then
        echo "Meson not found. Installing via pip..."
        install_meson
    fi

    # Python development tools
    sudo apt-get install -y python3-dev python3-setuptools

    # Development libraries and headers
    # sudo apt-get install -y libjpeg-dev libtiff5-dev libpng-dev libavcodec-dev libavformat-dev libswscale-dev libxvidcore-dev libx264-dev libxine2-dev

    # GStreamer development
    # Check GStreamer version and install accordingly
    GSTREAMER_VERSION=$(apt-cache show libgstreamer1.0-dev | grep Version | head -1 | cut -d' ' -f2)
    GSTREAMER_VERSION_CLEAN=$(echo "$GSTREAMER_VERSION" | cut -d'-' -f1)
    MIN_GSTREAMER_VERSION="1.16.3"

    # Check if apt version meets minimum requirement
    if [ -n "$GSTREAMER_VERSION_CLEAN" ]; then
        if [ "$(printf '%s\n' "$MIN_GSTREAMER_VERSION" "$GSTREAMER_VERSION_CLEAN" | sort -V | head -n1)" = "$MIN_GSTREAMER_VERSION" ]; then
            echo "GStreamer version $GSTREAMER_VERSION_CLEAN meets minimum requirement ($MIN_GSTREAMER_VERSION). Installing via apt..."
            sudo apt-get install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
                    gstreamer1.0-tools \
                    gstreamer1.0-plugins-{base,good,bad} \
                    gstreamer1.0-libav 
        else
            echo "GStreamer version $GSTREAMER_VERSION_CLEAN is below minimum requirement ($MIN_GSTREAMER_VERSION). Installing from source..."
            install_gstreamer_from_source "$MIN_GSTREAMER_VERSION"
        fi
    else
        echo "Could not determine GStreamer version from apt-cache. Installing from source..."
        install_gstreamer_from_source "$MIN_GSTREAMER_VERSION"
    fi

    # Additional development libraries
    sudo apt-get install -y libeigen3-dev libjson-glib-dev librdkafka-dev libmosquitto-dev
    
    # Check if mosquitto pkg-config file exists
    if ! pkg-config --exists libmosquitto 2>/dev/null; then
        echo "libmosquitto pkg-config file not found. Creating pkg-config file for libmosquitto..."
        
        # Get mosquitto version from installed package
        MOSQUITTO_VERSION=$(dpkg-query -W -f='${Version}\n' libmosquitto-dev 2>/dev/null | head -n1)
        if [ -z "$MOSQUITTO_VERSION" ]; then
            MOSQUITTO_VERSION="1.4.15"  # fallback version
        fi
        echo "Using mosquitto version: $MOSQUITTO_VERSION"
        
        sudo tee /usr/lib/x86_64-linux-gnu/pkgconfig/libmosquitto.pc > /dev/null << EOF
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib/x86_64-linux-gnu
includedir=\${prefix}/include

Name: libmosquitto
Description: mosquitto MQTT library
Version: $MOSQUITTO_VERSION
Libs: -L\${libdir} -lmosquitto
Cflags: -I\${includedir}
EOF
        echo "Created pkg-config file for libmosquitto (version: $MOSQUITTO_VERSION)"
    else
        MOSQUITTO_VERSION=$(pkg-config --modversion libmosquitto 2>/dev/null || echo "unknown")
        echo "libmosquitto pkg-config file found (version: $MOSQUITTO_VERSION)"
    fi

    # OpenCV development
    UBUNTU_VERSION=$(lsb_release -rs)
    if [ "$UBUNTU_VERSION" = "18.04" ]; then
        echo "Ubuntu 18.04 detected. Installing OpenCV 4.x..."
        OPENCV_VERSION=$(apt-cache show libopencv-dev | grep Version | head -1 | cut -d' ' -f2)
        if [ -n "$OPENCV_VERSION" ]; then
            OPENCV_MAJOR_MINOR=$(echo "$OPENCV_VERSION" | cut -d'.' -f1,2)
            
            if [ "$(printf '%s\n' "4.2" "$OPENCV_MAJOR_MINOR" | sort -V | head -n1)" != "4.2" ]; then
                echo "OpenCV version $OPENCV_VERSION is below minimum required version 4.2.0"
                echo "Installing OpenCV from source..."
                install_opencv
            else
                echo "OpenCV version $OPENCV_VERSION meets minimum requirement (>= 4.2.0)"
                sudo apt-get install -y libopencv-dev
            fi
        else
            echo "Could not determine OpenCV version from apt-cache, installing from source..."
            install_opencv
        fi
    else
        echo "Installing OpenCV development package..."
        sudo apt-get install -y libopencv-dev
    fi

    # install libyuv
    if [ ! -f "/usr/local/lib/libyuv.so" ]; then
        echo "Installing libyuv..."
        libyuv_installed=false

        if apt-cache search libyuv0 | grep -q "libyuv0"; then
            echo "Installing libyuv0 and libyuv-dev using apt..."
            sudo apt-get -y install libyuv0 libyuv-dev
            libyuv_installed=true
        fi

        if [ "$libyuv_installed" == false ]; then
            if ! test -e $DX_SRC_DIR/util; then 
                mkdir -p "$DX_SRC_DIR/util"
            fi
            cd "$DX_SRC_DIR/util"

            if [ $(uname -m) == "x86_64" ]; then
                if [ ! -d "libyuv" ]; then
                    git clone -b main https://chromium.googlesource.com/libyuv/libyuv libyuv
                fi
                cd libyuv
                mkdir -p build && cd build
                cmake .. -DCMAKE_BUILD_TYPE=Release
                make -j"$(nproc)"
                sudo make install
                sudo ldconfig
                echo "Verifying libyuv installation..."
                if [ -f "/usr/local/lib/libyuv.a" ] || [ -f "/usr/local/lib/libyuv.so" ]; then
                    echo "libyuv successfully installed!"
                else
                    echo "Error: libyuv installation failed."
                    exit 1
                fi
                rm -rf ./libyuv
            elif [ $(uname -m) == "aarch64" ]; then
                wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv0_0.0.1888.20240710-3_arm64.deb
                wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv-dev_0.0.1888.20240710-3_arm64.deb
                sudo dpkg -i ./libyuv*
                sudo apt-get -f install
            else
                echo "Error: Unsupported architecture $(uname -m)"
                exit 1
            fi
        fi
    fi
}

function install_dx_stream_dep_runtime() {
    echo "Install DX-Stream Runtime Dependencies"
    sudo apt-get update

    # GStreamer runtime
    sudo apt-get install -y gstreamer1.0-tools \
        gstreamer1.0-plugins-{base,good,bad,ugly} gstreamer1.0-libav 

    # # Mosquitto runtime
    # UBUNTU_VERSION=$(lsb_release -rs)
    # if [ "$UBUNTU_VERSION" = "18.04" ]; then
    #     echo "Ubuntu 18.04 detected. Installing mosquitto runtime..."
    #     if ! apt-cache show mosquitto > /dev/null 2>&1; then
    #         sudo apt-get install -y libssl-dev libsasl2-dev libzstd-dev libz-dev default-jdk
    #         echo "mosquitto not available in apt. Installing from source..."
    #         cd /tmp
    #         wget https://mosquitto.org/files/source/mosquitto-2.0.15.tar.gz
    #         tar -xzf mosquitto-2.0.15.tar.gz
    #         cd mosquitto-2.0.15
    #         make
    #         sudo make install
    #         sudo ldconfig
    #         cd /tmp
    #         rm -rf mosquitto-2.0.15*
    #     else
    #         sudo apt-get install -y mosquitto mosquitto-clients
    #     fi
    # else
    #     echo "Installing mosquitto runtime..."
    #     sudo add-apt-repository -y ppa:mosquitto-dev/mosquitto-ppa
    #     sudo apt install -y mosquitto mosquitto-clients
    # fi

    # # for kafka demo
    # if ! test -e $DX_SRC_DIR/util; then 
    #     mkdir -p $DX_SRC_DIR/util
    # fi

    # cd $DX_SRC_DIR/util
    # wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
    # tar -xzf kafka_2.13-3.9.0.tgz
    # cd kafka_2.13-3.9.0.tgz
}

function show_information_message(){
    echo -e "${TAG_INFO} DX-Stream dependencies installed successfully."
}

[ $# -gt 0 ] && \
while (( $# )); do
    case "$1" in
        --help) help; exit 0;;
        *)       echo "Invalid argument : " $1 ; help; exit 1;;
    esac
done

if [ $target_arch == "arm64" ]; then
    target_arch=aarch64
    echo " Use arch64 instead of arm64"
fi

function main() {
    install_dx_stream_dep
    install_dx_stream_dep_runtime
    show_information_message
}

main

popd
