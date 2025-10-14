#!/bin/bash

# libyuv library setup for DX-Stream
# This script handles libyuv installation with smart detection and multiple installation methods

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Build libyuv from source for x86_64
build_libyuv_from_source() {
    local build_dir="$DX_SRC_DIR/util"
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    if [ ! -d "libyuv" ]; then
        print_message "info" "Cloning libyuv repository..."
        git clone -b main https://chromium.googlesource.com/libyuv/libyuv libyuv
    fi
    
    cd libyuv
    mkdir -p build && cd build
    
    print_message "build" "Configuring and building libyuv..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_EXE_LINKER_FLAGS="-pthread" \
        -DCMAKE_C_FLAGS="-pthread" \
        -DCMAKE_CXX_FLAGS="-pthread" | cat
    
    make -j"$(nproc)"
    sudo make install
    sudo ldconfig
    
    print_message "search" "Verifying libyuv installation..."
    if [ -f "/usr/local/lib/libyuv.a" ] || [ -f "/usr/local/lib/libyuv.so" ]; then
        print_message "success" "libyuv successfully installed!"
    else
        print_message "error" "libyuv installation verification failed."
        exit 1
    fi
    
    # Clean up
    cd "$build_dir"
    rm -rf libyuv
}

# Install libyuv packages for aarch64
install_libyuv_aarch64_packages() {
    print_message "install" "Installing libyuv packages for aarch64..."
    
    local temp_dir=$(mktemp -d)
    cd "$temp_dir"
    
    if ! wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv0_0.0.1888.20240710-3_arm64.deb; then
        print_message "error" "Failed to download libyuv0 package."
        cd /
        rm -rf "$temp_dir"
        return 1
    fi

    if ! wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv-dev_0.0.1888.20240710-3_arm64.deb; then
        print_message "error" "Failed to download libyuv-dev package."
        cd /
        rm -rf "$temp_dir"
        return 1
    fi

    if ! ls libyuv0_*.deb libyuv-dev_*.deb >/dev/null 2>&1; then
        print_message "error" "Downloaded libyuv packages not found."
        cd /
        rm -rf "$temp_dir"
        return 1
    fi

    local dpkg_output
    dpkg_output=$(sudo dpkg -i ./libyuv*.deb 2>&1 | grep -v "Note, selecting")
    local dpkg_status=${PIPESTATUS[0]}
    printf '%s\n' "$dpkg_output"

    if [ $dpkg_status -ne 0 ]; then
        print_message "warning" "dpkg installation reported errors. Attempting to fix dependencies..."
        local apt_fix_output
        apt_fix_output=$(sudo apt-get -f install -y 2>&1 | grep -v "Note, selecting")
        local apt_fix_status=${PIPESTATUS[0]}
        printf '%s\n' "$apt_fix_output"
        if [ $apt_fix_status -ne 0 ]; then
            print_message "error" "Failed to repair libyuv installation dependencies."
            cd /
            rm -rf "$temp_dir"
            return 1
        fi
    fi

    # Final verification via dpkg status
    if ! dpkg -s libyuv0 >/dev/null 2>&1 || ! dpkg -s libyuv-dev >/dev/null 2>&1; then
        print_message "error" "libyuv packages are not properly installed."
        cd /
        rm -rf "$temp_dir"
        return 1
    fi
    
    # Clean up
    cd /
    rm -rf "$temp_dir"
    
    print_message "success" "libyuv packages installed for aarch64"
}

# Check whether libyuv libraries are already installed
libyuv_is_installed() {
    if [ -f "/usr/local/lib/libyuv.so" ] || [ -f "/usr/local/lib/libyuv.a" ]; then
        return 0
    fi

    if [ -f "/usr/lib/x86_64-linux-gnu/libyuv.so" ] || [ -f "/usr/lib/aarch64-linux-gnu/libyuv.so" ]; then
        return 0
    fi

    return 1
}

# Setup libyuv library with smart installation
setup_libyuv() {
    print_message "search" "Checking libyuv installation..."
    
    if libyuv_is_installed; then
        print_message "success" "libyuv libraries already present."
        return 0
    fi
    
    print_message "build" "Installing libyuv..."
    
    # Try APT installation first
    if apt-cache search libyuv0 | grep -q "libyuv0"; then
        print_message "install" "Installing libyuv via APT..."
        sudo apt-get install -y libyuv0 libyuv-dev
        if [ $? -eq 0 ]; then
            print_message "success" "libyuv installed successfully via APT"
            return 0
        else
            print_message "warning" "APT installation failed, falling back to source build"
        fi
    fi
    
    # Build from source as fallback
    print_message "build" "Building libyuv from source..."
    local arch=$(uname -m)
    
    case "$arch" in
        x86_64)
            build_libyuv_from_source || return 1
            ;;
        aarch64)
            install_libyuv_aarch64_packages || return 1
            ;;
        *)
            print_message "error" "Unsupported architecture: $arch"
            return 1
            ;;
    esac

    if libyuv_is_installed; then
        print_message "success" "libyuv setup completed!"
        return 0
    else
        print_message "error" "libyuv installation verification failed."
        return 1
    fi
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_libyuv
fi