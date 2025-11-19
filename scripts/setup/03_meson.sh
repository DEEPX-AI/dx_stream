#!/bin/bash

# Meson setup for DX-Stream
# This script handles Meson installation with version checking and multiple installation methods

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

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

    # 5. If still not possible, set up python3.9+ environment (for 18.04, etc.)
    print_message "info" "pip install failed, setting up python3.9+ environment and trying again..."
    
    local python_ok=false
    local python_cmd=""
    local min_python_minor=9  # Meson 1.3+ requires Python 3.9+
    
    # Check current Python version
    if command -v python3 >/dev/null 2>&1; then
        local current_py_version=$(python3 --version 2>&1 | awk '{print $2}')
        local current_py_major=$(echo "$current_py_version" | cut -d'.' -f1)
        local current_py_minor=$(echo "$current_py_version" | cut -d'.' -f2)
        
        if [ "$current_py_major" -eq 3 ] && [ "$current_py_minor" -ge "$min_python_minor" ]; then
            print_message "success" "Current Python $current_py_version is sufficient for Meson $required_version"
            python_ok=true
            python_cmd="python3"
        else
            print_message "warning" "Current Python $current_py_version insufficient for Meson $required_version (need Python 3.9+)"
        fi
    fi
    
    # If Python is insufficient, install Python 3.9
    if [ "$python_ok" = false ]; then
        print_message "info" "Python 3.9+ required for Meson $required_version. Current Python insufficient."
        
        # Ubuntu 18.04 specific handling
        if [ "$OS_ID" = "ubuntu" ] && [ "$OS_VERSION" = "18.04" ]; then
            print_message "info" "Ubuntu 18.04 detected - attempting Python 3.9 source build..."
            
            # Install build dependencies
            print_message "install" "Installing Python build dependencies..."
            sudo apt-get update
            sudo apt-get install -y --no-install-recommends \
                build-essential wget curl ca-certificates \
                libssl-dev zlib1g-dev libncurses5-dev libncursesw5-dev \
                libreadline-dev libsqlite3-dev libgdbm-dev libdb5.3-dev \
                libbz2-dev libexpat1-dev liblzma-dev tk-dev libffi-dev uuid-dev
            
            if [ $? -ne 0 ]; then
                print_message "error" "Failed to install Python build dependencies"
                return 1
            fi
            
            # Build Python 3.9 from source
            local python_version="3.9.18"
            local build_dir="$DX_SRC_DIR/util/python_build"
            mkdir -p "$build_dir"
            cd "$build_dir"
            
            print_message "build" "Downloading Python ${python_version} source..."
            if ! wget --no-check-certificate "https://www.python.org/ftp/python/${python_version}/Python-${python_version}.tgz"; then
                print_message "error" "Failed to download Python source"
                cd "$DX_SRC_DIR"
                rm -rf "$build_dir"
                return 1
            fi
            
            print_message "build" "Extracting and building Python ${python_version}..."
            tar xzf "Python-${python_version}.tgz"
            cd "Python-${python_version}"
            
            ./configure --enable-optimizations --prefix=/usr/local
            if [ $? -ne 0 ]; then
                print_message "error" "Python configure failed"
                cd "$DX_SRC_DIR"
                rm -rf "$build_dir"
                return 1
            fi
            
            make -j$(($(nproc) / 2))
            if [ $? -ne 0 ]; then
                print_message "error" "Python build failed"
                cd "$DX_SRC_DIR"
                rm -rf "$build_dir"
                return 1
            fi
            
            sudo make altinstall
            if [ $? -ne 0 ]; then
                print_message "error" "Python installation failed"
                cd "$DX_SRC_DIR"
                rm -rf "$build_dir"
                return 1
            fi
            
            # Cleanup
            cd "$DX_SRC_DIR"
            rm -rf "$build_dir"
            
            python_cmd="python3.9"
            print_message "success" "Python 3.9 installed successfully"
            
        else
            # For other Ubuntu versions, try PPA
            print_message "info" "Adding deadsnakes PPA for Ubuntu..."
            if add-apt-repository -y ppa:deadsnakes/ppa 2>&1 | grep -qv "Cannot add PPA"; then
                sudo apt-get update
                sudo apt-get install -y python3.9 python3.9-dev python3.9-distutils
                python_cmd="python3.9"
            else
                print_message "error" "Failed to add deadsnakes PPA and Python source build not attempted"
                return 1
            fi
        fi
    fi
    
    # Verify Python availability
    if [ -z "$python_cmd" ] || ! command -v "$python_cmd" >/dev/null 2>&1; then
        print_message "error" "Python 3.9+ not available after installation attempts"
        return 1
    fi
    
    # Install pip for the Python version if needed
    local pip_cmd="${python_cmd} -m pip"
    if ! $python_cmd -m pip --version >/dev/null 2>&1; then
        print_message "info" "Installing pip for $python_cmd..."
        curl -sSL https://bootstrap.pypa.io/get-pip.py -o get-pip.py
        sudo $python_cmd get-pip.py
        rm -f get-pip.py
    fi
    
    # Try to install meson using the available Python
    print_message "install" "Installing Meson using $python_cmd..."
    
    # Install meson via pip
    local pip_output
    pip_output=$(sudo $pip_cmd install "meson>=$required_version" 2>&1)
    local pip_status=$?
    
    if [ $pip_status -eq 0 ] || echo "$pip_output" | grep -q "already satisfied"; then
        print_message "info" "Meson pip installation completed"
    else
        print_message "error" "Meson $required_version installation failed with $python_cmd"
        print_message "error" "pip output: $pip_output"
        return 1
    fi
    
    # Refresh shell's command hash table
    hash -r 2>/dev/null || true
    export PATH="/usr/local/bin:$PATH"
    
    # Check if meson command is available
    if command -v meson >/dev/null 2>&1 && meson --version >/dev/null 2>&1; then
        local installed_version=$(meson --version 2>/dev/null)
        print_message "success" "Meson $installed_version installation succeeded ($python_cmd environment)"
        return 0
    fi
    
    # Meson not in PATH, create wrapper
    print_message "warning" "Meson installed but not in PATH, creating wrapper script..."
    
    # Get Python version for finding meson module
    local python_version=$($python_cmd --version 2>&1 | awk '{print $2}' | cut -d'.' -f1-2)
    
    # Try to verify meson module is actually installed
    if ! $python_cmd -c "import mesonbuild.mesonmain" 2>/dev/null; then
        print_message "error" "Meson module not found in $python_cmd"
        return 1
    fi
    
    print_message "info" "Creating meson wrapper script at /usr/local/bin/meson"
    
    # Create wrapper script
    cat | sudo tee /usr/local/bin/meson > /dev/null << EOF
#!/bin/bash
exec $python_cmd -m mesonbuild.mesonmain "\$@"
EOF
    
    sudo chmod +x /usr/local/bin/meson
    
    # Verify wrapper works
    hash -r 2>/dev/null || true
    export PATH="/usr/local/bin:$PATH"
    
    if command -v meson >/dev/null 2>&1; then
        if meson --version >/dev/null 2>&1; then
            local installed_version=$(meson --version 2>/dev/null)
            print_message "success" "Meson $installed_version wrapper created successfully"
            return 0
        else
            print_message "error" "Meson wrapper created but execution failed"
            meson --version 2>&1
            return 1
        fi
    else
        print_message "error" "Meson wrapper created but not found in PATH"
        print_message "info" "PATH: $PATH"
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