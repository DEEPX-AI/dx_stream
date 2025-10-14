# DX-Stream Modular Setup Scripts

This directory contains modularized setup scripts for DX-Stream project dependency installation.

## 📁 Directory Structure

```
scripts/setup/
├── 00_common.sh           # Common utility functions
├── 01_essential_tools.sh  # Essential build tools setup
├── 02_cmake.sh           # CMake setup (version 3.17+)
├── 03_meson.sh           # Meson setup (version 1.3+)
├── 04_gstreamer.sh       # GStreamer smart installation
├── 05_opencv.sh          # OpenCV setup (version 4.2.0+)
├── 06_communication.sh   # Communication libraries setup
├── 07_libyuv.sh          # libyuv library setup
└── README.md             # This file
```

## 🚀 Usage

### Individual Script Execution
Each script can be executed independently:

```bash
# Install essential build tools
./scripts/setup/01_essential_tools.sh

# Setup CMake (specific version can be specified)
./scripts/setup/02_cmake.sh 3.17

# Setup Meson
./scripts/setup/03_meson.sh 1.3

# Install GStreamer
./scripts/setup/04_gstreamer.sh

# Install OpenCV (specific version can be specified)
./scripts/setup/05_opencv.sh 4.2.0

# Install communication libraries
./scripts/setup/06_communication.sh

# Install libyuv
./scripts/setup/07_libyuv.sh
```

### Complete Setup Execution
Install all dependencies sequentially through main `install.sh`:

```bash
./install.sh  # Executes all 7 steps automatically
```

## 📦 Script Details

### 00_common.sh
- Shared utility functions for all scripts
- Color definitions, message printing, package checking functions
- Project path configuration

### 01_essential_tools.sh
Essential build tools installed:
- build-essential, make, git, curl, wget
- tar, zip, unzip, pkg-config
- apt-utils, software-properties-common

### 02_cmake.sh
- CMake 3.17+ installation and version checking
- Smart selection between APT vs source build
- Automatic build dependency management

### 03_meson.sh
- Meson 1.3+ installation
- Automatic Python dependency installation
- ninja-build automatic checking
- Multiple installation method support

### 04_gstreamer.sh
- GStreamer 1.16.3+ smart installation
- Individual module checking (core, plugins)
- FFmpeg dependency verification
- Triple verification: pkg-config + gst-inspect + dpkg

### 05_opencv.sh
- OpenCV 4.2.0+ installation
- Smart selection between APT vs source build
- FFmpeg/GStreamer dependency verification
- Automatic build dependency management

### 06_communication.sh
Communication libraries installed:
- libeigen3-dev (Mathematical computation)
- libjson-glib-dev (JSON processing)
- librdkafka-dev (Apache Kafka)
- libmosquitto-dev (MQTT)
- libcurl4-openssl-dev (HTTP client)
- nlohmann-json3-dev (Modern C++ JSON)
- libpaho-mqtt-dev (Eclipse Paho MQTT)
- libprotobuf-dev, protobuf-compiler (Protocol Buffers)
- libgrpc++-dev, protobuf-compiler-grpc (gRPC)

### 07_libyuv.sh
- libyuv library installation
- Smart selection between APT vs source build
- Architecture-specific installation support (x86_64, aarch64)

## 🔧 Features

### Smart Installation System
- **Conditional Installation**: Skip already installed packages
- **Version Verification**: Validate minimum required versions
- **Multiple Installation Methods**: Auto-fallback to source build when APT fails
- **Dependency Management**: Install only necessary dependencies per component

### User-Friendly Interface
- **Color Coding**: Distinguish info, success, warning, error messages
- **Emoji Icons**: Provide visual feedback
- **Progress Indication**: Show step-by-step installation progress
- **Detailed Logging**: Provide detailed information for each step

### Maintainability
- **Modularization**: Independent management per component
- **Reusability**: Can be utilized in other projects
- **Extensibility**: Easy to add new dependencies
- **Testability**: Individual component unit testing possible

## 🛠️ Developer Guide

### Adding New Setup Scripts
1. Create new script in `scripts/setup/` directory
2. Source `00_common.sh`
3. Implement main function
4. Add direct execution support
5. Integrate into main `install.sh`

### Script Template
```bash
#!/bin/bash

# [Component] setup for DX-Stream
# This script handles [component] installation

# Source common utilities
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/00_common.sh"

setup_[component]() {
    print_message "info" "Setting up [component]..."
    
    # Implementation here
    
    print_message "success" "[Component] setup completed!"
}

# Execute if script is run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    setup_[component] "$@"
fi
```

## 📊 Compatibility

- **Ubuntu**: 18.04, 20.04, 22.04, 24.04
- **Architecture**: x86_64, aarch64
- **Shell**: bash 4.0+

## 🤝 Contributing

For adding new dependencies or improvements:
1. Write modular script for the component
2. Conduct testing
3. Update documentation
4. Submit Pull Request
