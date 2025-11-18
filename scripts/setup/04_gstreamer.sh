#!/bin/bash

# GStreamer setup for DX-Stream
# This script handles smart GStreamer installation with module-level checking

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# GStreamer version requirements
MIN_GSTREAMER_VERSION="1.16.3"
MIN_LIBAV_VERSION="1.16.2"
MIN_FFMPEG_VERSION="4.2"

# Install FFmpeg from source
install_ffmpeg_from_source() {
    local ffmpeg_version=${1:-"4.4"}
    
    print_message "build" "FFmpeg build from source (version: ${ffmpeg_version})"
    
    print_message "install" "Install FFmpeg build dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        libx264-dev libx265-dev libnuma-dev libvpx-dev \
        libmp3lame-dev libopus-dev libvorbis-dev libtheora-dev \
        libass-dev libfreetype6-dev libfontconfig1-dev libfribidi-dev \
        libgnutls28-dev libz-dev libbz2-dev liblzma-dev

    local build_dir="$DX_SRC_DIR/util/ffmpeg_build"
    mkdir -p "$build_dir"
    cd "$build_dir"

    print_message "info" "Downloading FFmpeg ${ffmpeg_version} source..."
    rm -f "ffmpeg-${ffmpeg_version}.tar.bz2"*
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://ffmpeg.org/releases/ffmpeg-${ffmpeg_version}.tar.bz2"; then
        print_message "error" "Failed to download FFmpeg (connection timeout or network error). Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! bzip2 -t "ffmpeg-${ffmpeg_version}.tar.bz2"; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "ffmpeg-${ffmpeg_version}.tar.bz2"
        exit 1
    fi
    
    tar -xf "ffmpeg-${ffmpeg_version}.tar.bz2"
    cd "ffmpeg-${ffmpeg_version}"
    
    print_message "build" "Configuring FFmpeg..."
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
    
    print_message "build" "Building FFmpeg..."
    make -j$(($(nproc) / 2))
    sudo make install
    sudo ldconfig
    
    print_message "success" "FFmpeg build completed!"
    ffmpeg -version
    
    cd ..
    rm -rf "$build_dir"
}

# Check if a GStreamer module is already installed with sufficient version
check_module_version() {
    local module="$1"
    local required_version="$2"
    
    # Map module names to pkg-config module names
    local pkg_name=""
    case "$module" in
        "gstreamer") pkg_name="gstreamer-1.0" ;;
        "gst-plugins-base") pkg_name="gstreamer-plugins-base-1.0" ;;
        "gst-plugins-good") pkg_name="gstreamer-plugins-good-1.0" ;;
        "gst-plugins-bad") pkg_name="gstreamer-plugins-bad-1.0" ;;
        "gst-libav") pkg_name="gstreamer-libav-1.0" ;;
        *) return 1 ;;
    esac
    
    # Ensure pkg-config can find both system and locally installed packages
    local arch=$(dpkg --print-architecture 2>/dev/null || uname -m)
    export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/lib/${arch}-linux-gnu/pkgconfig:/usr/lib/pkgconfig:/usr/lib/${arch}-linux-gnu/pkgconfig:${PKG_CONFIG_PATH}"
    
    if pkg-config --exists "$pkg_name" 2>/dev/null; then
        local installed_version
        installed_version=$(pkg-config --modversion "$pkg_name")
        if [ "$(printf '%s\n%s' "$required_version" "$installed_version" | sort -V | head -n1)" = "$required_version" ]; then
            print_message "success" "$module v$installed_version already installed (>= $required_version)"
            return 0
        else
            print_message "warning" "$module v$installed_version below required $required_version"
            return 1
        fi
    fi
    return 1
}

# Determine required minimum version for an APT package
get_required_version_for_pkg() {
    local pkg="$1"
    case "$pkg" in
        gstreamer1.0-libav)
            echo "$MIN_LIBAV_VERSION"
            ;;
        *)
            echo "$MIN_GSTREAMER_VERSION"
            ;;
    esac
}

# Retrieve candidate version from APT cache
get_apt_candidate_version() {
    local pkg="$1"
    apt-cache policy "$pkg" 2>/dev/null | awk '/Candidate:/ {print $2; exit}'
}

# Ensure the candidate APT version satisfies required minimum
ensure_apt_version_satisfied() {
    local pkg="$1"
    local min_version
    min_version="$(get_required_version_for_pkg "$pkg")"
    local candidate
    candidate="$(get_apt_candidate_version "$pkg")"

    if [ -z "$candidate" ] || [ "$candidate" = "(none)" ]; then
        print_message "error" "APT package '$pkg' not found in repositories. Cannot proceed." >&2
        return 1
    fi

    if dpkg --compare-versions "$candidate" lt "$min_version"; then
        print_message "error" "APT package '$pkg' candidate version $candidate is below required $min_version. Aborting installation." >&2
        return 1
    fi

    print_message "info" "APT package '$pkg' candidate version $candidate meets requirement (>= $min_version)." >&2
    return 0
}

# Install GStreamer build dependencies
install_build_dependencies() {
    print_message "install" "Installing GStreamer build dependencies..."
    sudo apt-get update
    sudo apt-get install -y \
        flex bison nasm \
        libglib2.0-dev libjpeg-dev libpng-dev \
        liborc-0.4-dev libgirepository1.0-dev
}

# Build GStreamer core
build_gstreamer_core() {
    local version=${1:-"1.16.3"}
    local build_dir=$2
    
    print_message "build" "Building GStreamer core (version: ${version})..."
    
    if check_module_version "gstreamer" "$version"; then
        return 0
    fi
    
    cd "$build_dir"
    print_message "info" "Downloading gstreamer ${version} source archive..."
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://gstreamer.freedesktop.org/src/gstreamer/gstreamer-${version}.tar.xz"; then
        print_message "error" "Failed to download gstreamer. Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! xz -t "gstreamer-${version}.tar.xz" 2>/dev/null; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "gstreamer-${version}.tar.xz"
        exit 1
    fi
    
    print_message "info" "Extracting source archive..."
    tar -xf "gstreamer-${version}.tar.xz"
    cd "gstreamer-${version}"
    
    meson setup build --prefix=/usr/local \
        -Dtests=disabled \
        -Dexamples=disabled \
        -Dintrospection=disabled
    
    if ! ninja -C build; then
        print_message "error" "Failed to build gstreamer core"
        return 1
    fi
    if ! sudo ninja -C build install; then
        print_message "error" "Failed to install gstreamer core"
        return 1
    fi
    cd ..
    
    rm -f "gstreamer-${version}.tar.xz"
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
}

# Build GStreamer plugins-base
build_gst_plugins_base() {
    local version=${1:-"1.16.3"}
    local build_dir=$2
    
    print_message "build" "Building GStreamer plugins-base (version: ${version})..."
    
    if check_module_version "gst-plugins-base" "$version"; then
        return 0
    fi
    
    cd "$build_dir"
    print_message "info" "Downloading gst-plugins-base ${version} source archive..."
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://gstreamer.freedesktop.org/src/gst-plugins-base/gst-plugins-base-${version}.tar.xz"; then
        print_message "error" "Failed to download gst-plugins-base. Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! xz -t "gst-plugins-base-${version}.tar.xz" 2>/dev/null; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "gst-plugins-base-${version}.tar.xz"
        exit 1
    fi
    
    print_message "info" "Extracting source archive..."
    tar -xf "gst-plugins-base-${version}.tar.xz"
    cd "gst-plugins-base-${version}"
    
    meson setup build --prefix=/usr/local \
        -Dintrospection=disabled \
        -Dexamples=disabled \
        -Dtests=disabled \
        -Dgl=disabled
    
    if ! ninja -C build; then
        print_message "error" "Failed to build gst-plugins-base"
        return 1
    fi
    if ! sudo ninja -C build install; then
        print_message "error" "Failed to install gst-plugins-base"
        return 1
    fi
    cd ..
    
    rm -f "gst-plugins-base-${version}.tar.xz"
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
}

# Build GStreamer plugins-good
build_gst_plugins_good() {
    local version=${1:-"1.16.3"}
    local build_dir=$2
    
    print_message "build" "Building GStreamer plugins-good (version: ${version})..."
    
    if check_module_version "gst-plugins-good" "$version"; then
        return 0
    fi
    
    cd "$build_dir"
    print_message "info" "Downloading gst-plugins-good ${version} source archive..."
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://gstreamer.freedesktop.org/src/gst-plugins-good/gst-plugins-good-${version}.tar.xz"; then
        print_message "error" "Failed to download gst-plugins-good. Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! xz -t "gst-plugins-good-${version}.tar.xz" 2>/dev/null; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "gst-plugins-good-${version}.tar.xz"
        exit 1
    fi
    
    print_message "info" "Extracting source archive..."
    tar -xf "gst-plugins-good-${version}.tar.xz"
    cd "gst-plugins-good-${version}"
    
    meson setup build --prefix=/usr/local \
        -Dexamples=disabled \
        -Dtests=disabled \
        -Djpeg=enabled \
        -Dpng=enabled \
        -Dvpx=disabled \
        -Dqt5=disabled \
        -Dgtk3=disabled
    
    if ! ninja -C build; then
        print_message "error" "Failed to build gst-plugins-good"
        return 1
    fi
    if ! sudo ninja -C build install; then
        print_message "error" "Failed to install gst-plugins-good"
        return 1
    fi
    cd ..
    
    rm -f "gst-plugins-good-${version}.tar.xz"
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
}

# Build GStreamer plugins-bad
build_gst_plugins_bad() {
    local version=${1:-"1.16.3"}
    local build_dir=$2
    
    print_message "build" "Building GStreamer plugins-bad (version: ${version})..."
    
    if check_module_version "gst-plugins-bad" "$version"; then
        return 0
    fi
    
    cd "$build_dir"
    print_message "info" "Downloading gst-plugins-bad ${version} source archive..."
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://gstreamer.freedesktop.org/src/gst-plugins-bad/gst-plugins-bad-${version}.tar.xz"; then
        print_message "error" "Failed to download gst-plugins-bad. Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! xz -t "gst-plugins-bad-${version}.tar.xz" 2>/dev/null; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "gst-plugins-bad-${version}.tar.xz"
        exit 1
    fi
    
    print_message "info" "Extracting source archive..."
    tar -xf "gst-plugins-bad-${version}.tar.xz"
    cd "gst-plugins-bad-${version}"
    
    meson setup build --prefix=/usr/local \
        -Dintrospection=disabled \
        -Dexamples=disabled \
        -Dtests=disabled \
        -Dopencv=disabled \
        -Dvdpau=disabled
    
    if ! ninja -C build; then
        print_message "error" "Failed to build gst-plugins-bad"
        return 1
    fi
    if ! sudo ninja -C build install; then
        print_message "error" "Failed to install gst-plugins-bad"
        return 1
    fi
    cd ..
    
    rm -f "gst-plugins-bad-${version}.tar.xz"
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
}

# Build GStreamer libav
build_gst_libav() {
    local version=${1:-"1.16.3"}
    local build_dir=$2
    
    print_message "build" "Building GStreamer libav (version: ${version})..."
    
    if check_module_version "gst-libav" "$version"; then
        return 0
    fi
    
    cd "$build_dir"
    print_message "info" "Downloading gst-libav ${version} source archive..."
    if ! wget --tries=1 --timeout=30 --connect-timeout=10 "https://gstreamer.freedesktop.org/src/gst-libav/gst-libav-${version}.tar.xz"; then
        print_message "error" "Failed to download gst-libav. Exiting."
        exit 1
    fi
    
    print_message "search" "Verifying download integrity..."
    if ! xz -t "gst-libav-${version}.tar.xz" 2>/dev/null; then
        print_message "error" "Downloaded file is corrupted. Exiting."
        rm -f "gst-libav-${version}.tar.xz"
        exit 1
    fi
    
    print_message "info" "Extracting source archive..."
    tar -xf "gst-libav-${version}.tar.xz"
    cd "gst-libav-${version}"
    
    meson setup build --prefix=/usr/local
    
    if ! ninja -C build; then
        print_message "error" "Failed to build gst-libav"
        return 1
    fi
    if ! sudo ninja -C build install; then
        print_message "error" "Failed to install gst-libav"
        return 1
    fi
    cd ..
    
    rm -f "gst-libav-${version}.tar.xz"
    sudo ldconfig
    pkg-config --reload-cache 2>/dev/null || true
}

# Install GStreamer from source (coordinating function)
install_gstreamer_from_source() {
    local target_version="$MIN_GSTREAMER_VERSION"
    
    print_message "build" "GStreamer source build initiated"
    
    # Detect if core is already installed via APT and get its version
    if pkg-config --exists gstreamer-1.0 2>/dev/null; then
        local core_version
        core_version=$(pkg-config --modversion gstreamer-1.0)
        if dpkg -l | grep -q "^ii.*libgstreamer1.0-0"; then
            print_message "info" "Detected APT-installed GStreamer core v$core_version"
            target_version="$core_version"
        fi
    fi
    
    print_message "info" "Target GStreamer version for all components: $target_version"
    
    # Install build dependencies
    install_build_dependencies
    
    # Create build directory
    local build_dir="$DX_SRC_DIR/util/gstreamer_build"
    mkdir -p "$build_dir"
    
    # Build each component with the same version
    if ! build_gstreamer_core "$target_version" "$build_dir"; then
        rm -rf "$build_dir"
        return 1
    fi
    if ! build_gst_plugins_base "$target_version" "$build_dir"; then
        rm -rf "$build_dir"
        return 1
    fi
    if ! build_gst_plugins_good "$target_version" "$build_dir"; then
        rm -rf "$build_dir"
        return 1
    fi
    if ! build_gst_plugins_bad "$target_version" "$build_dir"; then
        rm -rf "$build_dir"
        return 1
    fi
    if ! build_gst_libav "$target_version" "$build_dir"; then
        rm -rf "$build_dir"
        return 1
    fi
    
    # Cleanup
    rm -rf "$build_dir"
    
    print_message "success" "GStreamer source build completed!"
    
    # Verify installation
    if command -v gst-inspect-1.0 >/dev/null 2>&1; then
        local final_version=$(gst-inspect-1.0 --version 2>/dev/null | head -1 | awk '{print $3}')
        print_message "success" "GStreamer $final_version ready"
    fi
}

# Check and install Rockchip-specific dependencies
detect_and_install_rockchip_deps() {
    print_message "search" "Detecting hardware platform..."
    
    # Rockchip board detection
    local is_rockchip=false
    
    if [ -f /proc/device-tree/compatible ]; then
        if grep -qi "rockchip" /proc/device-tree/compatible; then
            is_rockchip=true
        fi
    fi
    
    # Also check CPU info
    if grep -qi "rockchip" /proc/cpuinfo 2>/dev/null; then
        is_rockchip=true
    fi
    
    if [ "$is_rockchip" = true ]; then
        print_message "info" "Rockchip platform detected. Checking librga dependencies..."
        
        # Check if librga-dev is installed
        if ! dpkg -l | grep -q "^ii.*librga-dev"; then
            print_message "install" "Installing librga-dev..."
            sudo apt-get update
            
            # Try to install librga-dev first
            if sudo apt-get install -y librga-dev; then
                print_message "success" "librga-dev installed successfully"
            else
                print_message "warning" "librga-dev installation failed, trying without libdrm-dev..."
                # librga-dev might already be installed, check again
                if dpkg -l | grep -q "^ii.*librga-dev"; then
                    print_message "success" "librga-dev is already installed"
                else
                    print_message "error" "Failed to install librga-dev"
                    print_message "warning" "Please install manually: sudo apt-get install librga-dev"
                    # Don't fail, continue without it
                fi
            fi
            
            # Check libdrm-dev separately (might have version conflicts)
            if ! dpkg -l | grep -q "^ii.*libdrm-dev"; then
                print_message "info" "Checking libdrm-dev..."
                if sudo apt-get install -y libdrm-dev 2>&1 | grep -q "E:"; then
                    print_message "warning" "libdrm-dev installation failed (version conflict), but libdrm2 is already present"
                    print_message "info" "This is acceptable - continuing with existing libdrm installation"
                else
                    print_message "success" "libdrm-dev installed successfully"
                fi
            else
                print_message "success" "libdrm-dev already installed"
            fi
        else
            print_message "success" "librga-dev already installed"
            
            # Also verify libdrm-dev
            if dpkg -l | grep -q "^ii.*libdrm-dev"; then
                print_message "success" "libdrm-dev already installed"
            else
                print_message "info" "libdrm-dev not installed, attempting installation..."
                if ! sudo apt-get install -y libdrm-dev 2>&1 | grep -q "E:"; then
                    print_message "success" "libdrm-dev installed successfully"
                else
                    print_message "warning" "libdrm-dev installation failed (version conflict)"
                    print_message "info" "Continuing with existing libdrm runtime libraries"
                fi
            fi
        fi
    else
        print_message "info" "Non-Rockchip platform detected. Skipping librga installation."
    fi
    
    return 0
}

# Check FFmpeg dependency
check_ffmpeg_dependency() {
    print_message "search" "Checking FFmpeg dependency..."
    local ffmpeg_ok=true
    
    if ! command -v ffmpeg >/dev/null 2>&1; then
        print_message "warning" "FFmpeg not found"
        ffmpeg_ok=false
    else
        local ffmpeg_output=$(ffmpeg -version 2>&1 | head -n1)
        
        # Check if FFmpeg is functional (no symbol lookup errors)
        if echo "$ffmpeg_output" | grep -q "symbol lookup error"; then
            print_message "warning" "FFmpeg found but not functional (symbol lookup error)"
            ffmpeg_ok=false
        elif echo "$ffmpeg_output" | grep -q "ffmpeg version"; then
            # Extract version number
            local ffmpeg_version=$(echo "$ffmpeg_output" | grep -o 'ffmpeg version [0-9]\+\.[0-9]\+' | awk '{print $3}')
            
            if [ -z "$ffmpeg_version" ]; then
                ffmpeg_version=$(echo "$ffmpeg_output" | sed -n 's/.*ffmpeg version \([0-9]\+\.[0-9]\+\.[0-9]\+\).*/\1/p')
            fi
            
            if [ -n "$ffmpeg_version" ]; then
                if [ "$(printf '%s\n%s' "$MIN_FFMPEG_VERSION" "$ffmpeg_version" | sort -V | head -n1)" = "$MIN_FFMPEG_VERSION" ]; then
                    print_message "success" "FFmpeg v$ffmpeg_version (sufficient)"
                    return 0
                else
                    print_message "warning" "FFmpeg v$ffmpeg_version below minimum ($MIN_FFMPEG_VERSION)"
                    ffmpeg_ok=false
                fi
            else
                print_message "warning" "FFmpeg found but version could not be determined"
                ffmpeg_ok=false
            fi
        else
            print_message "warning" "FFmpeg found but unexpected output format"
            ffmpeg_ok=false
        fi
    fi
    
    if [ "$ffmpeg_ok" = false ]; then
        print_message "install" "Trying to install FFmpeg via APT..."
        sudo apt-get install -y ffmpeg
        
        # Recheck after APT installation
        if command -v ffmpeg >/dev/null 2>&1; then
            local ffmpeg_version=$(ffmpeg -version 2>&1 | head -n1 | awk '{print $3}' | cut -d'-' -f1)
            if [ -n "$ffmpeg_version" ]; then
                if [ "$(printf '%s\n%s' "$MIN_FFMPEG_VERSION" "$ffmpeg_version" | sort -V | head -n1)" = "$MIN_FFMPEG_VERSION" ]; then
                    print_message "success" "FFmpeg v$ffmpeg_version installed via APT (sufficient)"
                    return 0
                else
                    print_message "warning" "APT FFmpeg v$ffmpeg_version still below minimum ($MIN_FFMPEG_VERSION)"
                fi
            fi
        fi
        
        # APT failed or version still insufficient, build from source
        print_message "warning" "FFmpeg APT installation insufficient. Building from source..."
        install_ffmpeg_from_source "4.4"
    fi
}

# Check GStreamer core modules
check_gstreamer_core() {
    print_message "info" "Analyzing GStreamer core modules..." >&2
    
    declare -A GST_CORE_MODULES=(
        ["gstreamer-1.0"]="libgstreamer1.0-dev:GStreamer core development"
        ["gstreamer-plugins-base-1.0"]="libgstreamer-plugins-base1.0-dev:GStreamer base plugins development"
    )
    
    local missing_packages=()
    
    for module in "${!GST_CORE_MODULES[@]}"; do
        local apt_package=$(echo "${GST_CORE_MODULES[$module]}" | cut -d':' -f1)
        local description=$(echo "${GST_CORE_MODULES[$module]}" | cut -d':' -f2)
        
        print_message "search" "Checking $module ($description)..." >&2
        
        if pkg-config --exists "$module"; then
            local version=$(pkg-config --modversion "$module")
            if [ "$(printf '%s\n%s' "$MIN_GSTREAMER_VERSION" "$version" | sort -V | head -n1)" != "$MIN_GSTREAMER_VERSION" ]; then
                print_message "warning" "Version $version too low (required: >=$MIN_GSTREAMER_VERSION)" >&2
                missing_packages+=("$apt_package")
            else
                if pkg-config --cflags "$module" >/dev/null 2>&1; then
                    print_message "success" "Found v$version with development headers" >&2
                else
                    print_message "warning" "Runtime only - development headers missing" >&2
                    missing_packages+=("$apt_package")
                fi
            fi
        else
            print_message "warning" "Not found" >&2
            missing_packages+=("$apt_package")
        fi
    done
    
    # Always check for GStreamer tools separately
    print_message "search" "Checking GStreamer tools (gst-inspect-1.0)..." >&2
    if ! command -v gst-inspect-1.0 >/dev/null 2>&1; then
        print_message "warning" "GStreamer tools not found" >&2
        missing_packages+=("gstreamer1.0-tools")
    else
        local tools_version=$(gst-inspect-1.0 --version 2>/dev/null | head -1 | awk '{print $3}' || echo "unknown")
        print_message "success" "Found GStreamer tools v$tools_version" >&2
    fi
    
    echo "${missing_packages[@]}"
}

# Check GStreamer plugins
check_gstreamer_plugins() {
    print_message "info" "Checking GStreamer plugins..." >&2
    
    declare -A GST_PLUGIN_CHECKS=(
        ["good"]="gstreamer1.0-plugins-good:GStreamer good plugins"
        ["bad"]="gstreamer1.0-plugins-bad:GStreamer bad plugins"
        ["libav"]="gstreamer1.0-libav:GStreamer libav plugin"
    )
    
    local missing_packages=()
    
    for plugin_type in "${!GST_PLUGIN_CHECKS[@]}"; do
        local package_info="${GST_PLUGIN_CHECKS[$plugin_type]}"
        local apt_package=$(echo "$package_info" | cut -d':' -f1)
        local description=$(echo "$package_info" | cut -d':' -f2)
        
        print_message "search" "Checking $plugin_type plugins ($description)..." >&2
        
        local plugin_available=false
        
        # First check if APT package is installed
        if is_package_installed "$apt_package"; then
            print_message "success" "APT package installed" >&2
            plugin_available=true
        elif command -v gst-inspect-1.0 >/dev/null 2>&1; then
            # Fallback to runtime check if APT package check fails
            case "$plugin_type" in
                "good")
                    if gst-inspect-1.0 audioconvert >/dev/null 2>&1 || gst-inspect-1.0 videoscale >/dev/null 2>&1; then
                        plugin_available=true
                    fi
                    ;;
                "bad")
                    if gst-inspect-1.0 | grep -q "bad:" 2>/dev/null; then
                        plugin_available=true
                    fi
                    ;;
                "libav")
                    if gst-inspect-1.0 | grep -q "libav:" 2>/dev/null; then
                        plugin_available=true
                    fi
                    ;;
            esac
        fi
        
        if [ "$plugin_available" = true ]; then
            if command -v gst-inspect-1.0 >/dev/null 2>&1; then
                local plugin_version=$(gst-inspect-1.0 --version 2>/dev/null | head -1 | awk '{print $3}')
                print_message "success" "Runtime available: v$plugin_version" >&2
                
                # Version check for libav
                local min_version="$MIN_GSTREAMER_VERSION"
                if [[ "$plugin_type" == "libav" ]]; then
                    min_version="$MIN_LIBAV_VERSION"
                fi
                
                if [ "$(printf '%s\n%s' "$min_version" "$plugin_version" | sort -V | head -n1)" != "$min_version" ]; then
                    print_message "warning" "Version too low (required: >=$min_version)" >&2
                    missing_packages+=("$apt_package")
                fi
            else
                print_message "success" "Package installed via APT" >&2
            fi
        else
            print_message "warning" "Not available" >&2
            missing_packages+=("$apt_package")
        fi
    done
    
    echo "${missing_packages[@]}"
}

# Main GStreamer installation function
install_gstreamer_smart() {
    print_message "info" "🎬 Smart GStreamer installation - checking individual modules..." >&2
    
    # Check and install Rockchip dependencies if needed
    detect_and_install_rockchip_deps

    # Check FFmpeg dependency first
    check_ffmpeg_dependency
    
    # Check core modules and plugins
    local missing_core=($(check_gstreamer_core))
    local missing_plugins=($(check_gstreamer_plugins))
    
    # Combine all missing packages and remove duplicates
    local all_missing=("${missing_core[@]}" "${missing_plugins[@]}")
    local unique_packages=($(printf '%s\n' "${all_missing[@]}" | sort -u))
    
    if [ ${#unique_packages[@]} -eq 0 ]; then
        print_message "success" "All GStreamer components are properly installed. Nothing to do."
        return 0
    fi
    
    # Check if GStreamer core is already installed with sufficient version
    local core_installed=false
    local core_version=""
    
    if pkg-config --exists gstreamer-1.0 2>/dev/null; then
        core_version=$(pkg-config --modversion gstreamer-1.0)
        if [ "$(printf '%s\n%s' "$MIN_GSTREAMER_VERSION" "$core_version" | sort -V | head -n1)" = "$MIN_GSTREAMER_VERSION" ]; then
            core_installed=true
            print_message "info" "GStreamer core v$core_version already installed (>= $MIN_GSTREAMER_VERSION)"
        fi
    fi
    
    # If core GStreamer is already installed with sufficient version, only install missing plugins via APT
    if [ "$core_installed" = true ]; then
        print_message "info" "Using existing GStreamer $core_version installation. Installing missing plugins via APT..."
        
        # For libav plugin, try APT installation regardless of version check failure
        local libav_missing=false
        for pkg in "${unique_packages[@]}"; do
            if [[ "$pkg" == "gstreamer1.0-libav" ]]; then
                libav_missing=true
                break
            fi
        done
        
        if [ "$libav_missing" = true ]; then
            print_message "install" "Attempting to install gstreamer1.0-libav via APT..."
            if sudo apt-get install -y gstreamer1.0-libav 2>&1; then
                print_message "success" "gstreamer1.0-libav installed successfully"
                # Remove from unique_packages array
                unique_packages=($(printf '%s\n' "${unique_packages[@]}" | grep -v "gstreamer1.0-libav"))
            else
                print_message "warning" "gstreamer1.0-libav APT installation failed. Will try source build."
            fi
        fi
        
        # Install remaining packages via APT (skip version validation)
        if [ ${#unique_packages[@]} -gt 0 ]; then
            print_message "install" "Installing remaining packages: ${unique_packages[*]}"
            sudo apt-get install -y "${unique_packages[@]}" 2>&1 | grep -v "Note, selecting"
        fi
        
        # Verify libav plugin
        if command -v gst-inspect-1.0 >/dev/null 2>&1; then
            if gst-inspect-1.0 libav >/dev/null 2>&1; then
                print_message "success" "GStreamer libav plugin verified successfully"
                return 0
            else
                print_message "warning" "GStreamer libav plugin not available after APT installation"
                print_message "info" "Building gst-libav from source to match GStreamer $core_version..."
                
                # Build only gst-libav from source with matching version
                local build_dir="$DX_SRC_DIR/util/gstreamer_build"
                mkdir -p "$build_dir"
                install_build_dependencies
                build_gst_libav "$core_version" "$build_dir"
                rm -rf "$build_dir"
                return $?
            fi
        fi
        
        return 0
    fi
    
    # If core GStreamer is not installed, validate APT versions before full installation
    local apt_version_ok=true
    for pkg in "${unique_packages[@]}"; do
        if ! ensure_apt_version_satisfied "$pkg"; then
            apt_version_ok=false
            break
        fi
    done

    # If APT versions are insufficient, fall back to source build
    if [ "$apt_version_ok" = false ]; then
        print_message "warning" "APT versions insufficient. Building GStreamer from source..."
        install_gstreamer_from_source
        return $?
    fi

    print_message "install" "Installing missing GStreamer components via APT..."
    print_message "info" "Packages to install: ${unique_packages[*]}"
    
    # Install missing packages
    sudo apt-get install -y "${unique_packages[@]}" 2>&1 | grep -v "Note, selecting"
    
    # Install additional tools if core GStreamer was installed and tools not present
    if [[ " ${unique_packages[*]} " =~ " libgstreamer1.0-dev " ]]; then
        if ! command -v gst-inspect-1.0 >/dev/null 2>&1; then
            print_message "install" "Installing GStreamer tools..."
            sudo apt-get install -y gstreamer1.0-tools 2>&1 | grep -v "Note, selecting"
        else
            print_message "success" "GStreamer tools already available"
        fi
    fi
    
    # Verify installation
    print_message "info" "Verifying GStreamer installation..."
    if command -v gst-inspect-1.0 >/dev/null 2>&1; then
        local gst_version=$(gst-inspect-1.0 --version 2>/dev/null | head -1 | awk '{print $3}')
        print_message "success" "GStreamer $gst_version installed successfully!"
    else
        print_message "error" "GStreamer installation verification failed"
        exit 1
    fi
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    install_gstreamer_smart
fi