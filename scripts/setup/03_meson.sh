#!/bin/bash

# Meson setup for DX-Stream
# This script handles Meson installation with version checking and multiple installation methods

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Install Meson function (based on original working code)
install_meson() {
    local required_version=${1:-"1.3"}
    print_message "build" "Installing Meson $required_version..."
    
    # 1. Try sudo python3 -m pip install meson==<version>
    if sudo python3 -m pip install "meson==$required_version"; then
        if command -v meson >/dev/null 2>&1; then
            print_message "success" "Meson $required_version installation succeeded (sudo python3 -m pip)"
            return 0
        else
            print_message "warning" "Meson package installed but binary not accessible. Trying alternative method..."
        fi
    fi

    # 2. Retry with --break-system-packages option (for 24.04, etc.)
    print_message "info" "Default pip install failed, retrying with --break-system-packages option..."
    if sudo python3 -m pip install --break-system-packages "meson==$required_version"; then
        if command -v meson >/dev/null 2>&1; then
            print_message "success" "Meson $required_version installation succeeded (--break-system-packages)"
            return 0
        else
            print_message "warning" "Meson package installed but binary not accessible. Trying alternative method..."
        fi
    fi

    # 3. If still not possible, set up python3.7 environment (for 18.04, etc.)
    print_message "info" "pip install failed, setting up python3.7 environment and trying again..."
    sudo add-apt-repository ppa:deadsnakes/ppa -y
    sudo apt-get update
    sudo apt-get install -y python3.7 python3.7-dev python3.7-distutils
    curl -sSL https://bootstrap.pypa.io/pip/3.7/get-pip.py -o get-pip.py
    sudo python3.7 get-pip.py
    if sudo python3.7 -m pip install "meson==$required_version"; then
        if command -v meson >/dev/null 2>&1; then
            print_message "success" "Meson $required_version installation succeeded (python3.7 environment)"
            rm -rf get-pip.py
            return 0
        else
            print_message "error" "Meson installed but command not found in PATH"
            rm -rf get-pip.py
            return 1
        fi
    else
        print_message "error" "Meson $required_version installation failed: All installation methods failed."
        rm -rf get-pip.py
        return 1
    fi
}

# Setup Meson with smart version checking
setup_meson() {
    local required_version=${1:-"1.3"}
    local current_version=$(meson --version 2>/dev/null || echo "")
    
    print_message "search" "Checking Meson installation..."
    
    # Check and install ninja-build if not present (required for Meson)
    if ! command -v ninja >/dev/null 2>&1 && ! is_package_installed ninja-build; then
        print_message "install" "Installing ninja-build (required for Meson)..."
        sudo apt-get install -y ninja-build
    fi
    
    local need_install=false
    if [ -z "$current_version" ]; then
        print_message "warning" "Meson not found. Installing..."
        need_install=true
    else
        # Version comparison using dpkg (like original code)
        if dpkg --compare-versions "$current_version" lt "$required_version"; then
            print_message "warning" "Meson version $current_version below requirement ($required_version). Reinstalling..."
            need_install=true
        else
            print_message "success" "Meson version $current_version meets requirement (>= $required_version)"
            return 0
        fi
    fi

    if [ "$need_install" = true ]; then
        if ! install_meson "$required_version"; then
            print_message "error" "Meson installation failed. Cannot proceed."
            return 1
        fi
        
        # Final verification
        local final_version=$(meson --version 2>/dev/null || echo "")
        if [ -z "$final_version" ]; then
            print_message "error" "Meson installation failed: command not found after installation"
            return 1
        fi
        
        print_message "success" "Meson $final_version is now available"
    fi
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_meson "${1:-1.3}"
fi