#!/bin/bash

# Communication libraries setup for DX-Stream
# This script handles installation of Kafka, MQTT, JSON and other communication libraries

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# Setup mosquitto pkg-config file if missing
setup_mosquitto_pkgconfig() {
    print_message "search" "Checking mosquitto pkg-config..."
    
    if ! pkg-config --exists libmosquitto 2>/dev/null; then
        print_message "warning" "libmosquitto pkg-config file not found. Creating..."
        
        # Get mosquitto version from installed package
        local mosquitto_version=$(dpkg-query -W -f='${Version}\n' libmosquitto-dev 2>/dev/null | head -n1)
        if [ -z "$mosquitto_version" ]; then
            mosquitto_version="1.4.15"  # fallback version
        fi
        print_message "info" "Using mosquitto version: $mosquitto_version"
        
        # Create pkg-config file with proper architecture detection
        local lib_arch=""
        case "$(uname -m)" in
            x86_64)  lib_arch="x86_64-linux-gnu" ;;
            aarch64) lib_arch="aarch64-linux-gnu" ;;
            arm*)    lib_arch="arm-linux-gnueabihf" ;;
            *)       lib_arch="$(uname -m)-linux-gnu" ;;
        esac
        
        local pkgconfig_path="/usr/lib/${lib_arch}/pkgconfig/libmosquitto.pc"
        sudo tee "$pkgconfig_path" > /dev/null << EOF
prefix=/usr
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib/${lib_arch}
includedir=\${prefix}/include

Name: libmosquitto
Description: mosquitto MQTT library
Version: $mosquitto_version
Libs: -L\${libdir} -lmosquitto
Cflags: -I\${includedir}
EOF
        print_message "success" "Created pkg-config file: $pkgconfig_path"
    else
        local mosquitto_version=$(pkg-config --modversion libmosquitto 2>/dev/null || echo "unknown")
        print_message "success" "libmosquitto pkg-config found (version: $mosquitto_version)"
    fi
}

# Setup communication libraries
setup_communication_libraries() {
    print_message "info" "🔗 Setting up communication libraries..."
    
    # Define communication libraries and their packages
    local comm_libs=(
        "libeigen3-dev:Mathematical computation library"
        "libjson-glib-dev:JSON handling library for GLib"
        "librdkafka-dev:Apache Kafka C/C++ library"
        "libmosquitto-dev:MQTT client library"
    )
    
    local missing_libs=()
    
    # Check which libraries are missing
    for lib_info in "${comm_libs[@]}"; do
        local lib=$(echo "$lib_info" | cut -d':' -f1)
        local desc=$(echo "$lib_info" | cut -d':' -f2)
        
        print_message "search" "Checking $lib ($desc)..."
        if ! is_package_installed "$lib"; then
            print_message "warning" "$lib not installed"
            missing_libs+=("$lib")
        else
            local version=$(dpkg-query -W -f='${Version}' "$lib" 2>/dev/null)
            print_message "success" "$lib: v$version"
        fi
    done
    
    # Install missing libraries
    if [ ${#missing_libs[@]} -gt 0 ]; then
        print_message "install" "Installing missing communication libraries: ${missing_libs[*]}"
        sudo apt-get install -y "${missing_libs[@]}" 2>&1 | grep -v "Note, selecting" | grep -v "E: Unable to locate package"
        
        if [ $? -ne 0 ]; then
            print_message "warning" "Some communication libraries failed to install, continuing..."
        fi
    else
        print_message "success" "All communication libraries are already installed."
    fi
    
    # Special handling for libmosquitto pkg-config
    setup_mosquitto_pkgconfig
    
    print_message "success" "Communication libraries setup completed!"
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_communication_libraries
fi