#!/bin/bash

# Essential build tools setup for DX-Stream
# This script checks and installs essential build tools if needed

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

setup_essential_tools() {
    print_message "info" "Checking essential build tools..."
    
    # Check and install essential build tools if needed
    local essential_tools=("build-essential" "make" "git" "curl" "wget" "tar" "zip" "unzip" "pkg-config" "python3-dev" "python3-pip")
    local missing_tools=()
    
    for tool in "${essential_tools[@]}"; do
        if ! is_package_installed "$tool"; then
            missing_tools+=("$tool")
        fi
    done
    
    if [ ${#missing_tools[@]} -gt 0 ]; then
        print_message "install" "Installing missing build tools: ${missing_tools[*]}"
        sudo apt-get update -y 2>/dev/null
        sudo apt-get install -y apt-utils software-properties-common "${missing_tools[@]}" 2>&1 | grep -v "Note, selecting"
        
        if [ $? -eq 0 ]; then
            print_message "success" "Essential build tools installed successfully"
        else
            print_message "error" "Failed to install some essential build tools"
            exit 1
        fi
    else
        print_message "success" "All essential build tools are already installed"
    fi
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_essential_tools
fi