#!/bin/bash

# Communication libraries setup for DX-Stream
# This script handles installation of Kafka, MQTT, JSON and other communication libraries

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

# librdkafka < 1.8.0 (e.g. Ubuntu 20.04 APT 1.2.1) cannot consume from Kafka 4.x brokers
LIBRDKAFKA_MIN_VERSION="1.8.0"
LIBRDKAFKA_SRC_VERSION="2.2.0"

build_librdkafka_from_source() {
    print_message "build" "Building librdkafka v$LIBRDKAFKA_SRC_VERSION from source..."
    sudo apt-get install -y build-essential git pkg-config libssl-dev zlib1g-dev libzstd-dev liblz4-dev libsasl2-dev \
        2>&1 | grep -v "Note, selecting"

    local src_dir="$DX_SRC_DIR/util/librdkafka"
    mkdir -p "$DX_SRC_DIR/util"
    rm -rf "$src_dir"
    if ! git clone --depth 1 -b "v$LIBRDKAFKA_SRC_VERSION" https://github.com/confluentinc/librdkafka.git "$src_dir"; then
        print_message "error" "Failed to clone librdkafka v$LIBRDKAFKA_SRC_VERSION"
        return 1
    fi

    if ! (cd "$src_dir" && ./configure --prefix=/usr/local && make -j"$(nproc)" libs && sudo make install); then
        print_message "error" "librdkafka build/install failed"
        return 1
    fi
    sudo ldconfig
    rm -rf "$src_dir"

    print_message "success" "librdkafka $(pkg-config --modversion rdkafka 2>/dev/null) installed from source!"
}

setup_librdkafka() {
    print_message "search" "Checking librdkafka (Apache Kafka C/C++ library)..."

    local installed_version=$(pkg-config --modversion rdkafka 2>/dev/null)
    if [ -n "$installed_version" ] && ! version_lt "$installed_version" "$LIBRDKAFKA_MIN_VERSION"; then
        print_message "success" "librdkafka: v$installed_version"
        return 0
    fi

    local apt_version=$(apt-cache policy librdkafka-dev 2>/dev/null | awk '/Candidate:/{print $2}' | sed -E 's/^[0-9]+://; s/-[^-]*$//; s/^\(none\)$//')
    if [ -n "$apt_version" ] && ! version_lt "$apt_version" "$LIBRDKAFKA_MIN_VERSION"; then
        print_message "install" "Installing librdkafka-dev v$apt_version via APT..."
        sudo apt-get install -y librdkafka-dev 2>&1 | grep -v "Note, selecting"
        return ${PIPESTATUS[0]}
    fi

    print_message "warning" "APT librdkafka (${apt_version:-none}) is below $LIBRDKAFKA_MIN_VERSION. Building from source..."
    # Drop old APT headers/pkg-config so builds cannot mix them with /usr/local
    if is_package_installed librdkafka-dev; then
        remove_apt_package librdkafka-dev
    fi
    build_librdkafka_from_source
}

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

    setup_librdkafka || return 1
    
    # Special handling for libmosquitto pkg-config
    setup_mosquitto_pkgconfig
    
    print_message "success" "Communication libraries setup completed!"
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_communication_libraries
fi