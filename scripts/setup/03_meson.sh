#!/bin/bash

# Meson setup for DX-Stream
# This script handles Meson installation with version checking and multiple installation methods

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Install Meson function with multiple fallback strategies
install_meson() {
    local required_version=${1:-"1.3"}
    print_message "build" "Installing Meson..."

    # Get OS information using lsb_release (supports both Ubuntu and Debian)
    local OS_ID=""
    local OS_VERSION=""
    
    # Extract OS ID from /etc/os-release
    if [ -f /etc/os-release ]; then
        OS_ID=$(grep "^ID=" /etc/os-release | sed 's/^ID=//' | tr -d '"')
    fi
    
    # Get OS version using lsb_release
    OS_VERSION=$(lsb_release -rs)
    
    echo -e "${TAG_INFO} Detected OS: ${OS_ID} ${OS_VERSION}"
    
    # 1. Try apt package manager first (most reliable and no dependency issues)
    local apt_version=$(apt-cache policy meson 2>/dev/null | grep "Candidate:" | awk '{print $2}' | cut -d'-' -f1)
    
    if [ -n "$apt_version" ]; then
        # Check if apt version meets requirement
        local apt_major_minor=$(echo "$apt_version" | cut -d'.' -f1-2)
        local required_major_minor=$(echo "$required_version" | cut -d'.' -f1-2)
        
        if dpkg --compare-versions "$apt_major_minor" ge "$required_major_minor" 2>/dev/null; then
            print_message "info" "Attempting to install meson $apt_version via apt..."
            if sudo apt-get install -y meson 2>/dev/null; then
                if command -v meson >/dev/null 2>&1; then
                    local installed_version=$(meson --version 2>/dev/null)
                    print_message "success" "Meson $installed_version installation succeeded (apt)"
                    return 0
                fi
            fi
        else
            print_message "info" "apt meson version $apt_version < required $required_version, trying backports..."
        fi
        
        # Try backports for newer version
        local backports_version=$(apt-cache policy meson 2>/dev/null | grep -A 1 "bookworm-backports" | grep -oP '^\s+\K[0-9]+\.[0-9]+\.[0-9]+' | head -1)
        if [ -n "$backports_version" ]; then
            local backports_major_minor=$(echo "$backports_version" | cut -d'.' -f1-2)
            if dpkg --compare-versions "$backports_major_minor" ge "$required_major_minor" 2>/dev/null; then
                print_message "info" "Trying backports meson $backports_version..."
                if sudo apt-get install -y -t bookworm-backports meson 2>/dev/null; then
                    if command -v meson >/dev/null 2>&1; then
                        local installed_version=$(meson --version 2>/dev/null)
                        print_message "success" "Meson $installed_version installation succeeded (apt backports)"
                        return 0
                    fi
                fi
            else
                print_message "info" "backports meson version $backports_version < required $required_version, trying pip..."
            fi
        fi
    else
        print_message "info" "meson not available in apt, trying alternative methods..."
    fi
    
    # 2. If in venv, use venv's pip (no sudo needed)
    if [ -n "$VIRTUAL_ENV" ]; then
        print_message "info" "Virtual environment detected, using venv pip..."
        if command -v pip >/dev/null 2>&1; then
            if pip install "meson>=$required_version"; then
                if command -v meson >/dev/null 2>&1; then
                    print_message "success" "Meson installation succeeded (venv pip)"
                    return 0
                fi
            fi
        fi
    fi
    
    # 3. Try system pip3 if available
    if command -v pip3 >/dev/null 2>&1; then
        print_message "info" "Attempting to install meson via system pip3..."
        # Try without --break-system-packages first (older Python versions)
        if sudo pip3 install "meson>=$required_version" 2>/dev/null; then
            if command -v meson >/dev/null 2>&1; then
                print_message "success" "Meson installation succeeded (system pip3)"
                return 0
            fi
        fi
        # Fallback: try with --break-system-packages (Python 3.11+)
        if sudo pip3 install --break-system-packages "meson>=$required_version" 2>/dev/null; then
            if command -v meson >/dev/null 2>&1; then
                print_message "success" "Meson installation succeeded (system pip3 with --break-system-packages)"
                return 0
            fi
        fi
    fi
    
    # 4. Install python3-pip then try again
    if ! command -v pip3 >/dev/null 2>&1; then
        print_message "info" "Installing python3-pip..."
        if sudo apt-get install -y python3-pip; then
            # Try without --break-system-packages first (older Python versions)
            if sudo pip3 install "meson>=$required_version" 2>/dev/null; then
                if command -v meson >/dev/null 2>&1; then
                    print_message "success" "Meson installation succeeded (pip3 after installation)"
                    return 0
                fi
            fi
            # Fallback: try with --break-system-packages (Python 3.11+)
            if sudo pip3 install --break-system-packages "meson>=$required_version" 2>/dev/null; then
                if command -v meson >/dev/null 2>&1; then
                    print_message "success" "Meson installation succeeded (pip3 after installation with --break-system-packages)"
                    return 0
                fi
            fi
        fi
    fi

    # 5. If still not possible, set up python3.7 environment (for 18.04, etc.)
    print_message "info" "pip install failed, setting up python3.7 environment and trying again..."
    # Only add deadsnakes PPA for Ubuntu (not needed for Debian)
    if [ "$OS_ID" = "ubuntu" ] && ! sudo add-apt-repository -y ppa:deadsnakes/ppa; then
        print_colored "Failed to add deadsnakes PPA." "ERROR"
        return 1
    fi
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
    
    print_message "error" "Meson installation failed: All installation methods failed."
    print_message "info" "Please install meson manually: sudo apt-get install meson"
    return 1
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
        # Version comparison using dpkg
        # Extract major.minor version for comparison (e.g., 1.0.1 -> 1.0, 1.3.0 -> 1.3)
        local current_major_minor=$(echo "$current_version" | cut -d'.' -f1-2)
        local required_major_minor=$(echo "$required_version" | cut -d'.' -f1-2)
        
        if dpkg --compare-versions "$current_major_minor" lt "$required_major_minor" 2>/dev/null; then
            print_message "warning" "Meson version $current_version below requirement ($required_version). Attempting upgrade..."
            need_install=true
        else
            print_message "success" "Meson version $current_version meets requirement (>= $required_version)"
            return 0
        fi
    fi

    if [ "$need_install" = true ]; then
        if ! install_meson "$required_version"; then
            # Check if meson was installed anyway (apt might have installed older version)
            local final_version=$(meson --version 2>/dev/null || echo "")
            if [ -n "$final_version" ]; then
                print_message "warning" "Meson $final_version installed (requested >= $required_version)"
                print_message "info" "Proceeding with available version..."
                return 0
            else
                print_message "error" "Meson installation failed. Cannot proceed."
                return 1
            fi
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