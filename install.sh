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

    mkdir -p "$PROJECT_ROOT/util"
    cd "$PROJECT_ROOT/util"

    if [ ! -f "cmake-$version.tar.gz" ]; then
        wget https://cmake.org/files/v$version/cmake-$version.0.tar.gz --no-check-certificate
    fi
    tar xvf cmake-$version.0.tar.gz
    cd "cmake-$version.0"

    ./bootstrap --system-curl
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

    set -e

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
    echo "Trying to install Meson 1.3..."
    # 1. Try python3 -m pip install meson==1.3
    if python3 -m pip install meson==1.3; then
        echo "Meson 1.3 installation succeeded (python3 -m pip)"
        return 0
    fi

    # 2. Retry with --break-system-packages option (for 24.04, etc.)
    echo "Default pip install failed, retrying with --break-system-packages option..."
    if python3 -m pip install --break-system-packages meson==1.3; then
        echo "Meson 1.3 installation succeeded (--break-system-packages)"
        return 0
    fi

    # 3. If still not possible, set up python3.7 environment (for 18.04, etc.)
    echo "pip install failed, setting up python3.7 environment and trying again..."
    sudo add-apt-repository ppa:deadsnakes/ppa -y
    sudo apt-get update
    sudo apt-get install -y python3.7 python3.7-dev python3.7-distutils
    curl -sSL https://bootstrap.pypa.io/pip/3.7/get-pip.py -o get-pip.py
    python3.7 get-pip.py
    if python3.7 -m pip install meson==1.3; then
        echo "Meson 1.3 installation succeeded (python3.7 environment)"
        rm -rf get-pip.py
        return 0
    else
        echo "Meson 1.3 installation failed: All installation methods failed." >&2
        rm -rf get-pip.py
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
    # Check and install ffmpeg if not installed
    if ! command -v ffmpeg >/dev/null 2>&1; then
        echo "ffmpeg is not installed. Installing ffmpeg..."
        sudo apt-get install -y ffmpeg
    else
        echo "ffmpeg is already installed."
    fi

    # Check and install GStreamer development libraries if not installed
    GSTREAMER_DEV_PKGS=(
        libgstreamer1.0-dev
        libgstreamer-plugins-base1.0-dev
    )
    for pkg in "${GSTREAMER_DEV_PKGS[@]}"; do
        dpkg -s "$pkg" >/dev/null 2>&1
        if [ $? -ne 0 ]; then
            echo "$pkg is not installed. Installing $pkg..."
            sudo apt-get install -y "$pkg"
        else
            echo "$pkg is already installed."
        fi
    done
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
    --disable-static
  
  echo "🔨 Building FFmpeg..."
  # Reduce parallel jobs to avoid memory issues
  make -j$(($(nproc) / 2))
  sudo make install
  sudo ldconfig
  
  echo "✅ FFmpeg build completed!"
  ffmpeg -version
  
  # Clean up
  cd ..
  rm -rf "$build_dir"
}

function install_build_dependencies() {
  echo "📦 Install build tools and dependencies..."
  sudo apt-get update
  sudo apt-get install -y \
    flex bison nasm \
    libx264-dev libx265-dev libnuma-dev libvpx-dev libfdk-aac-dev \
    libmp3lame-dev libopus-dev \
    libglib2.0-dev libgirepository1.0-dev libcairo2-dev libpango1.0-dev \
    libgdk-pixbuf2.0-dev libatk1.0-dev libgtk-3-dev \
    libjpeg-dev libpng-dev libgif-dev libtiff-dev
}

function check_and_install_ffmpeg() {
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
}

function check_module_version() {
  local module=$1
  local min_version=$2
  
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

  # For gst-plugins-good, check if plugins are actually installed
  if [ "$module" = "gst-plugins-good" ]; then
    # Check multiple possible locations for gst-plugins-good plugins
    local plugin_found=false
    for plugin_dir in "/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0" "/usr/lib/x86_64-linux-gnu/gstreamer-1.0" "/usr/local/lib/gstreamer-1.0"; do
      if [ -d "$plugin_dir" ]; then
        # Check for common gst-plugins-good plugins
        if [ -f "$plugin_dir/libgstjpeg.so" ] || [ -f "$plugin_dir/libgstpng.so" ] || [ -f "$plugin_dir/libgstgif.so" ]; then
          plugin_found=true
          break
        fi
      fi
    done
    
    if [ "$plugin_found" = "true" ]; then
      echo "✅ Skip: gst-plugins-good plugins found (plugins already installed)"
      return 0
    fi
  fi

  # For gst-libav, check if plugins are actually installed
  if [ "$module" = "gst-libav" ]; then
    # Check multiple possible locations for gst-libav plugins
    local plugin_found=false
    for plugin_dir in "/usr/local/lib/x86_64-linux-gnu/gstreamer-1.0" "/usr/lib/x86_64-linux-gnu/gstreamer-1.0" "/usr/local/lib/gstreamer-1.0"; do
      if [ -d "$plugin_dir" ]; then
        # Check for gst-libav plugins (libav plugins)
        if [ -f "$plugin_dir/libgstlibav.so" ] || [ -f "$plugin_dir/libgstav.so" ]; then
          plugin_found=true
          break
        fi
      fi
    done
    
    if [ "$plugin_found" = "true" ]; then
      echo "✅ Skip: gst-libav plugins found (plugins already installed)"
      return 0
    fi
  fi

  if [ -n "$installed_version" ]; then
    # Simple version comparison using dpkg --compare-versions
    if dpkg --compare-versions "$installed_version" ge "$min_version" 2>/dev/null; then
      echo "✅ Skip: installed version(${installed_version}) is greater than or equal to minimum version(${min_version})"
      return 0
    else
      echo "⚠️ Build needed: installed version(${installed_version}) is less than minimum version(${min_version})"
      return 1
    fi
  else
    echo "ℹ️ Build needed: [ ${module} ] is not installed (pkg-config: ${installed_version:-"not found"})"
    return 1
  fi
}

function build_gstreamer_core() {
  local version=${1:-"1.16.3"}
  local build_dir=$2
  
  echo "▶️ Building GStreamer core (version: ${version})..."
  
  if check_module_version "gstreamer" "$version"; then
    return 0
  fi
  
  echo "📥 Downloading gstreamer ${version} source archive..."
  wget "https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-${version}.tar.xz"
  
  echo "📦 Extracting source archive..."
  tar -xf "gstreamer-${version}.tar.xz"
  cd "gstreamer-${version}"
  
  # GStreamer core specific configure options
  meson setup build --prefix=/usr/local \
    -Dtests=disabled \
    -Dexamples=disabled \
    -Dintrospection=disabled
  
  ninja -C build
  sudo ninja -C build install
  cd ..
  
  rm -f "gstreamer-${version}.tar.xz"
  sudo ldconfig
  pkg-config --reload-cache 2>/dev/null || true
}

function build_gst_plugins_base() {
  local version=${1:-"1.16.3"}
  local build_dir=$2
  
  echo "▶️ Building GStreamer plugins-base (version: ${version})..."
  
  if check_module_version "gst-plugins-base" "$version"; then
    return 0
  fi
  
  echo "📥 Downloading gst-plugins-base ${version} source archive..."
  wget "https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-${version}.tar.xz"
  
  echo "📦 Extracting source archive..."
  tar -xf "gst-plugins-base-${version}.tar.xz"
  cd "gst-plugins-base-${version}"
  
  # GStreamer plugins-base specific configure options
  meson setup build --prefix=/usr/local \
    -Dintrospection=disabled \
    -Dexamples=disabled \
    -Dtests=disabled \
    -Dgl=disabled
  
  ninja -C build
  sudo ninja -C build install
  cd ..
  
  rm -f "gst-plugins-base-${version}.tar.xz"
  sudo ldconfig
  pkg-config --reload-cache 2>/dev/null || true
}

function build_gst_plugins_good() {
  local version=${1:-"1.16.3"}
  local build_dir=$2
  
  echo "▶️ Building GStreamer plugins-good (version: ${version})..."
  
  if check_module_version "gst-plugins-good" "$version"; then
    return 0
  fi
  
  echo "📥 Downloading gst-plugins-good ${version} source archive..."
  wget "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${version}.tar.xz"
  
  echo "📦 Extracting source archive..."
  tar -xf "gst-plugins-good-${version}.tar.xz"
  cd "gst-plugins-good-${version}"
  
  # GStreamer plugins-good specific configure options with JPEG support
  meson setup build --prefix=/usr/local \
    -Dexamples=disabled \
    -Dtests=disabled \
    -Djpeg=enabled \
    -Dpng=enabled \
    -Dvpx=disabled \
    -Dqt5=disabled \
    -Dgtk3=disabled
  
  ninja -C build
  sudo ninja -C build install
  cd ..
  
  rm -f "gst-plugins-good-${version}.tar.xz"
  sudo ldconfig
  pkg-config --reload-cache 2>/dev/null || true
}

function build_gst_plugins_bad() {
  local version=${1:-"1.16.3"}
  local build_dir=$2
  
  echo "▶️ Building GStreamer plugins-bad (version: ${version})..."
  
  if check_module_version "gst-plugins-bad" "$version"; then
    return 0
  fi
  
  echo "📥 Downloading gst-plugins-bad ${version} source archive..."
  wget "https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-${version}.tar.xz"
  
  echo "📦 Extracting source archive..."
  tar -xf "gst-plugins-bad-${version}.tar.xz"
  cd "gst-plugins-bad-${version}"
  
  # GStreamer plugins-bad specific configure options
  meson setup build --prefix=/usr/local \
    -Dintrospection=disabled \
    -Dexamples=disabled \
    -Dtests=disabled \
    -Dopencv=disabled \
    -Dvdpau=disabled
  
  ninja -C build
  sudo ninja -C build install
  cd ..
  
  rm -f "gst-plugins-bad-${version}.tar.xz"
  sudo ldconfig
  pkg-config --reload-cache 2>/dev/null || true
}

function build_gst_libav() {
  local version=${1:-"1.16.3"}
  local build_dir=$2
  
  echo "▶️ Building GStreamer libav (version: ${version})..."
  
  if check_module_version "gst-libav" "$version"; then
    return 0
  fi
  
  echo "📥 Downloading gst-libav ${version} source archive..."
  wget "https://gstreamer.freedesktop.org/src/gst-libav/gst-libav-${version}.tar.xz"
  
  echo "📦 Extracting source archive..."
  tar -xf "gst-libav-${version}.tar.xz"
  cd "gst-libav-${version}"
  
  # GStreamer libav specific configure options
  meson setup build --prefix=/usr/local
  
  ninja -C build
  sudo ninja -C build install
  cd ..
  
  rm -f "gst-libav-${version}.tar.xz"
  sudo ldconfig
  pkg-config --reload-cache 2>/dev/null || true
}

function install_gstreamer_from_source() {
  local gstreamer_version=${1:-"1.16.3"}  # 기본값으로 1.16.3 사용
  
  set -e

  echo "🚀 GStreamer build from source (version: ${gstreamer_version})"
  
  # Install build dependencies
  install_build_dependencies
  
  # Check and install FFmpeg if needed
  check_and_install_ffmpeg

  export PATH="$HOME/.local/bin:$PATH"

  local build_dir="${PROJECT_ROOT}/util/gstreamer_conditional_build"
  
  if [ -d "$build_dir" ]; then
    echo "🧹 Cleaning up existing build directory: $build_dir"
    rm -rf "$build_dir"
  fi

  mkdir -p "$build_dir"
  cd "$build_dir"
  echo "📂 Manage source code in [ $build_dir ] directory"
  
  # Build each module separately with specific configure options
  build_gstreamer_core "$gstreamer_version" "$build_dir"
  build_gst_plugins_base "$gstreamer_version" "$build_dir"
  build_gst_plugins_good "$gstreamer_version" "$build_dir"
  build_gst_plugins_bad "$gstreamer_version" "$build_dir"
  build_gst_libav "$gstreamer_version" "$build_dir"

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
    sudo apt-get install -y apt-utils software-properties-common cmake build-essential make zlib1g-dev libcurl4-openssl-dev git curl wget tar zip unzip ninja-build pkg-config \
        python3-dev python3-setuptools python3-pip 

    local required_cmake_version="3.17"
    setup_cmake "cmake" "${required_cmake_version}"

    # Install Meson (already handled by install_meson function)
    MESON_MIN_VERSION="1.3"
    MESON_CUR_VERSION=$(meson --version 2>/dev/null || echo "")

    need_install_meson=false
    if [ -z "$MESON_CUR_VERSION" ]; then
        echo "Meson not found. Installing via pip..."
        need_install_meson=true
    else
        # 버전 비교 (MESON_CUR_VERSION < MESON_MIN_VERSION)
        if dpkg --compare-versions "$MESON_CUR_VERSION" lt "$MESON_MIN_VERSION"; then
            echo "Meson version $MESON_CUR_VERSION is less than required $MESON_MIN_VERSION. Reinstalling..."
            need_install_meson=true
        fi
    fi

    if [ "$need_install_meson" = true ]; then
        install_meson
    fi

    # Development libraries and headers
    # sudo apt-get install -y libjpeg-dev libtiff5-dev libpng-dev libavcodec-dev libavformat-dev libswscale-dev libxvidcore-dev libx264-dev libxine2-dev

    # GStreamer development
    MIN_GSTREAMER_VERSION="1.16.3"
    GST_MODULES=(
        gstreamer-1.0
        gstreamer-plugins-base-1.0
        gstreamer-plugins-good-1.0
        gstreamer-plugins-bad-1.0
        gstreamer-libav-1.0
    )
    all_gst_ok=true
    for module in "${GST_MODULES[@]}"; do
        ver=$(pkg-config --modversion "$module" 2>/dev/null || echo "")
        if [ -z "$ver" ] || [ "$(printf '%s\n%s' "$MIN_GSTREAMER_VERSION" "$ver" | sort -V | head -n1)" != "$MIN_GSTREAMER_VERSION" ]; then
            all_gst_ok=false
            echo "$module not found or version too low ($ver)"
        fi
    done
    if $all_gst_ok; then
        echo "All required GStreamer modules meet minimum version ($MIN_GSTREAMER_VERSION). Skip installation."
    else
        GSTREAMER_VERSION=$(apt-cache show libgstreamer1.0-dev | grep Version | head -1 | cut -d' ' -f2)
        GSTREAMER_VERSION_CLEAN=$(echo "$GSTREAMER_VERSION" | cut -d'-' -f1)
        if [ -n "$GSTREAMER_VERSION_CLEAN" ]; then
            if [ "$(printf '%s\n%s' "$MIN_GSTREAMER_VERSION" "$GSTREAMER_VERSION_CLEAN" | sort -V | head -n1)" = "$MIN_GSTREAMER_VERSION" ]; then
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
    fi

    # OpenCV development
    MIN_OPENCV_VERSION="4.2.0"
    PKG_OPENCV_MODULES=(opencv4)
    all_opencv_ok=true
    for module in "${PKG_OPENCV_MODULES[@]}"; do
        ver=$(pkg-config --modversion "$module" 2>/dev/null || echo "")
        if [ -z "$ver" ] || [ "$(printf '%s\n%s' "$MIN_OPENCV_VERSION" "$ver" | sort -V | head -n1)" != "$MIN_OPENCV_VERSION" ]; then
            all_opencv_ok=false
            echo "$module not found or version too low ($ver)"
        fi
    done
    if $all_opencv_ok; then
        echo "All required OpenCV modules meet minimum version ($MIN_OPENCV_VERSION). Skip installation."
    else
        OPENCV_VERSION=$(apt-cache show libopencv-dev | grep Version | head -1 | cut -d' ' -f2)
        if [ -n "$OPENCV_VERSION" ]; then
            MIN_OPENCV_MAJOR_MINOR=$(echo "$MIN_OPENCV_VERSION" | cut -d'.' -f1,2)
            OPENCV_MAJOR_MINOR=$(echo "$OPENCV_VERSION" | cut -d'.' -f1,2)
            if [ "$(printf '%s\n%s' "$MIN_OPENCV_MAJOR_MINOR" "$OPENCV_MAJOR_MINOR" | sort -V | head -n1)" != "$MIN_OPENCV_MAJOR_MINOR" ]; then
                echo "OpenCV version $OPENCV_VERSION is below minimum required version $MIN_OPENCV_VERSION"
                echo "Installing OpenCV from source..."
                install_opencv
            else
                echo "OpenCV version $OPENCV_VERSION meets minimum requirement (>= $MIN_OPENCV_VERSION)"
                sudo apt-get install -y libopencv-dev
            fi
        else
            echo "Could not determine OpenCV version from apt-cache, installing from source..."
            install_opencv
        fi
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
    show_information_message
}

main

popd
