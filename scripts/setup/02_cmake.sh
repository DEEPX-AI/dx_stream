#!/bin/bash

# CMake setup for DX-Stream
# This script handles CMake installation with version checking and smart source building

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Function to get installed CMake version
get_cmake_version() {
    cmake --version 2>/dev/null | head -n1 | awk '{print $3}'
}

# Install CMake build dependencies helper function
install_cmake_build_deps() {
    print_message "install" "Installing CMake build dependencies..."
    local cmake_deps=("libssl-dev" "libcurl4-openssl-dev" "zlib1g-dev" "build-essential" "curl")
    local missing_deps=()
    
    for dep in "${cmake_deps[@]}"; do
        if ! is_package_installed "$dep"; then
            missing_deps+=("$dep")
        fi
    done
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        print_message "install" "Installing missing CMake dependencies: ${missing_deps[*]}"
        sudo apt-get update -y 2>/dev/null
        sudo apt-get install -y "${missing_deps[@]}" 2>&1 | grep -v "Note, selecting"
    fi
}

# Build CMake from source
build_cmake_from_source() {
    local version=$1
    print_message "build" "Building CMake $version from source..."
    install_cmake_build_deps
    mkdir -p "$DX_SRC_DIR/util"
    cd "$DX_SRC_DIR/util"

    if [ ! -f "cmake-$version.0.tar.gz" ]; then
        if ! wget https://cmake.org/files/v$version/cmake-$version.0.tar.gz --no-check-certificate; then
            print_message "error" "Failed to download CMake. Exiting."
            exit 1
        fi
        
        print_message "search" "Verifying download integrity..."
        if ! gzip -t "cmake-$version.0.tar.gz" 2>/dev/null; then
            print_message "error" "Downloaded file is corrupted. Exiting."
            rm -f "cmake-$version.0.tar.gz"
            exit 1
        fi
    fi
    tar xvf cmake-$version.0.tar.gz
    cd "cmake-$version.0"

    ./bootstrap --system-curl
    make -j$(nproc)
    sudo make install
    sudo ldconfig

    # Verify installation
    if ! command -v cmake >/dev/null 2>&1; then
        print_message "error" "CMake installation failed. Exiting."
        exit 1
    fi

    # remove build directory
    cd "$DX_SRC_DIR/util"
    rm -rf "cmake-$version.0"

    print_message "success" "CMake $(cmake --version | head -n1) installed from source!"
}

setup_cmake() {
    local package_name=${1:-"cmake"}
    local required_version=${2:-"3.17"}
    local installed_version=$(get_cmake_version || echo "")
    
    print_message "search" "Checking CMake installation..."
    if [ -z "$installed_version" ]; then
        print_message "warning" "${package_name} is not installed"
        
        # Check APT version
        local apt_version=$(apt-cache show cmake 2>/dev/null | grep "Version:" | head -1 | awk '{print $2}' | cut -d'-' -f1)
        
        if [ -n "$apt_version" ]; then
            if [ "$(printf '%s\n%s' "$required_version" "$apt_version" | sort -V | head -n1)" = "$required_version" ]; then
                print_message "success" "APT version $apt_version meets requirement (>= $required_version). Installing via APT..."
                sudo apt-get install -y cmake
                return 0
            else
                print_message "warning" "APT version $apt_version below requirement ($required_version). Building from source..."
                build_cmake_from_source "$required_version"
                return 0
            fi
        else
            print_message "error" "CMake not available in APT. Building from source..."
            build_cmake_from_source "$required_version"
            return 0
        fi
    else
        if [ "$(printf '%s\n%s' "$required_version" "$installed_version" | sort -V | head -n1)" != "$required_version" ]; then
            print_message "warning" "Installed version $installed_version below requirement ($required_version)"
            
            # Check if APT has better version
            local apt_version=$(apt-cache show cmake 2>/dev/null | grep "Version:" | head -1 | awk '{print $2}' | cut -d'-' -f1)
            
            if [ -n "$apt_version" ] && [ "$(printf '%s\n%s' "$required_version" "$apt_version" | sort -V | head -n1)" = "$required_version" ]; then
                print_message "success" "APT version $apt_version meets requirement. Upgrading via APT..."
                remove_apt_package cmake
                sudo apt-get install -y cmake
            else
                print_message "build" "Building from source..."
                remove_apt_package cmake
                build_cmake_from_source "$required_version"
            fi
        else
            print_message "success" "Installed version $installed_version meets requirement (>= $required_version)"
        fi
    fi
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_cmake "cmake" "${1:-3.17}"
fi