#!/bin/bash

# Common utility functions for DX-Stream setup scripts
# This file should be sourced by all setup scripts

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Emoji definitions for better UX
TAG_INFO="ℹ️"
TAG_ERROR="❌"
TAG_SUCCESS="✅"
TAG_WARNING="⚠️"
TAG_SEARCH="🔍"
TAG_INSTALL="📦"
TAG_BUILD="🔨"

# Get installed version from APT
get_apt_prebuilt_version() {
    local package_name=$1
    dpkg-query -W -f='${Version}\n' ${package_name} 2>/dev/null | head -n1
}

# Compare versions: returns 0 if $1 < $2
version_lt() {
    dpkg --compare-versions "$1" lt "$2"
}

# Remove package via apt
remove_apt_package() {
    local pkg=$1
    echo "Removing apt package: $pkg"
    sudo apt-get remove --purge -y "$pkg" || true
    sudo apt-get autoremove -y
    sudo ldconfig
}

# Check if package is installed
is_package_installed() {
    local package_name=$1
    dpkg -s "$package_name" >/dev/null 2>&1
}

# Print colored message
print_message() {
    local level=$1
    local message=$2
    
    case $level in
        "info")    echo -e "${BLUE}${TAG_INFO}${NC} $message" ;;
        "success") echo -e "${GREEN}${TAG_SUCCESS}${NC} $message" ;;
        "warning") echo -e "${YELLOW}${TAG_WARNING}${NC} $message" ;;
        "error")   echo -e "${RED}${TAG_ERROR}${NC} $message" ;;
        "search")  echo -e "${BLUE}${TAG_SEARCH}${NC} $message" ;;
        "install") echo -e "${GREEN}${TAG_INSTALL}${NC} $message" ;;
        "build")   echo -e "${YELLOW}${TAG_BUILD}${NC} $message" ;;
        *)         echo "$message" ;;
    esac
}

# Ensure we're in the right directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
DX_SRC_DIR="$DX_STREAM_ROOT"
PROJECT_ROOT="$DX_STREAM_ROOT"  # Compatibility with original script

# Export common variables
export DX_STREAM_ROOT
export DX_SRC_DIR
export PROJECT_ROOT
export TAG_INFO TAG_ERROR TAG_SUCCESS TAG_WARNING TAG_SEARCH TAG_INSTALL TAG_BUILD