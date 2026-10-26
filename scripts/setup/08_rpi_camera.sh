#!/bin/bash

# Raspberry Pi camera support for DX-Stream demos (optional)
# Installs the GStreamer libcamera source element (gstreamer1.0-libcamera) on Raspberry Pi only.

# Force English locale for consistent command output parsing
export LC_ALL=C
export LANG=C

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"
source "$DX_STREAM_ROOT/scripts/rpi_camera_util.sh"

# Never fails the installation: the camera demos are simply hidden when the element is missing.
setup_rpi_camera() {
    local pkg="gstreamer1.0-libcamera"

    if ! is_raspberry_pi; then
        print_message "info" "Not a Raspberry Pi, skipping $pkg"
        return 0
    fi
    if is_package_installed "$pkg"; then
        print_message "success" "$pkg already installed ($(get_apt_prebuilt_version "$pkg"))"
        return 0
    fi
    if ! apt-cache policy "$pkg" 2>/dev/null | grep -qE "Candidate: [^(]"; then
        print_message "warning" "$pkg is not available from the configured apt repositories; Raspberry Pi camera demos will be unavailable."
        print_message "info" "Raspberry Pi OS provides it from the Raspberry Pi repository (http://archive.raspberrypi.com/debian)."
        return 0
    fi
    if [ ! -f /etc/rpi-issue ]; then
        print_message "warning" "Not Raspberry Pi OS: $pkg from this distribution may not support the Pi camera."
    fi

    print_message "install" "Installing $pkg..."
    if sudo apt-get install -y "$pkg"; then
        print_message "success" "$pkg installed"
    else
        print_message "warning" "Failed to install $pkg; Raspberry Pi camera demos will be unavailable."
    fi
    return 0
}
