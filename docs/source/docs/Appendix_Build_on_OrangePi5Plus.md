# Building DX-Stream on Orange Pi 5 Plus

This guide provides comprehensive instructions for building and running DX-Stream on the Orange Pi 5 Plus single-board computer with DEEPX DX-M1 NPU accelerator.

## Overview

The Orange Pi 5 Plus is a powerful ARM-based development board that can be equipped with the DEEPX DX-M1 NPU for AI acceleration. This guide covers the complete setup process from SD card preparation to running DX-Stream applications.

## Prerequisites

### Hardware Requirements
- Orange Pi 5 Plus development board
- DEEPX DX-M1 NPU module (M.2 form factor)
- MicroSD card (32GB or larger, Class 10 recommended)
- Power supply (5V/4A minimum)
- Host Linux machine for SD card preparation

### Software Requirements
- 7-Zip or similar extraction utility
- SD card flashing tool or `dd` command access

## Step 1: SD Card Setup

### 1.1 Download Ubuntu Image

Download the official Orange Pi 5 Plus Ubuntu 22.04 image:
- **Download Link**: [Orange Pi 5 Plus Ubuntu 22.04 Image](https://drive.google.com/file/d/1l72cF6dsTzwU5NloY3FEKTuKA9KbhX80/view?usp=drive_link)
- **Image Version**: Ubuntu 22.04 Jammy Desktop XFCE with Linux kernel 6.1.43
- **File Size**: Approximately 4GB (compressed)

### 1.2 Extract Image

Extract the downloaded 7z archive:

```bash
7z x Orangepi5plus_1.2.0_ubuntu_jammy_desktop_xfce_linux6.1.43.7z
```

**Note**: If 7-Zip is not installed, install it using:
```bash
# Ubuntu/Debian
sudo apt install p7zip-full

# CentOS/RHEL
sudo yum install p7zip
```

### 1.3 Flash Image to SD Card

**⚠️ Warning**: Replace `/dev/sda` with your actual SD card device. Use `lsblk` or `fdisk -l` to identify the correct device.

```bash
sudo umount /dev/sda

sudo dd if=./Orangepi5plus_1.2.0_ubuntu_jammy_desktop_xfce_linux6.1.43.img of=/dev/sda bs=4M status=progress

sync
```

**Alternative Methods**:
- **Raspberry Pi Imager**: Cross-platform GUI tool
- **Balena Etcher**: User-friendly flashing utility
- **Win32DiskImager**: Windows-specific tool

### 1.4 Initial Boot

1. Insert the SD card into the Orange Pi 5 Plus
2. Connect power supply and peripherals
3. Power on the device

**Default Credentials** (if applicable):
- Username: `orangepi`
- Password: `orangepi`

## Step 2: DX-RT NPU Driver Installation

All following steps should be performed on the Orange Pi 5 Plus itself after successful boot.

### 2.1 Update Linux Kernel Headers

Install the required kernel headers for driver compilation:

```bash
cd /opt
sudo apt install ./linux-headers-current-rockchip-rk3588_1.2.0_arm64.deb
```

**Note**: The kernel headers package should be pre-installed in the Orange Pi image. If missing, contact DEEPX support.

### 2.2 Configure Git (Optional)

If you encounter SSL certificate issues with GitHub:

```bash
git config --global http.sslVerify false
```

**Security Note**: This disables SSL verification globally. For production use, consider configuring proper certificates instead.

### 2.3 Build and Install NPU Linux Driver

```bash
cd ~
mkdir -p dxnn && cd dxnn
git clone https://github.com/DEEPX-AI/dx_rt_npu_linux_driver.git
cd dx_rt_npu_linux_driver/modules
./build.sh
sudo ./build.sh -c install
```

**What this does**:
- Downloads the NPU kernel driver source code
- Compiles the driver for the current kernel
- Installs the driver modules and creates device nodes

### 2.4 Build and Install DX-RT Runtime

```bash
cd ~/dxnn
git clone https://github.com/DEEPX-AI/dx_rt.git
cd dx_rt
./install.sh --all
./build.sh
```

**Build Options**:
- `--all`: Installs all dependencies and development tools
- Alternative: `./install.sh --minimal` for runtime-only installation

### 2.5 Verify DX-RT Installation

Test the NPU driver and runtime installation:

```bash
dxrt-cli -s
```

**Expected Output**:
```
DXRT v3.0.0
=======================================================
 * Device 0: M1, Accelerator type
---------------------   Version   ---------------------
 * RT Driver version   : v1.7.1
 * PCIe Driver version : v1.4.1
-------------------------------------------------------
 * FW version          : v2.1.4
--------------------- Device Info ---------------------
 * Memory : LPDDR5 6000 MHz, 3.92GiB
 * Board  : M.2, Rev 1.5
 * Chip Offset : 0
 * PCIe   : Gen3 X4 [01:00:00]

NPU 0: voltage 750 mV, clock 1000 MHz, temperature 40'C
NPU 1: voltage 750 mV, clock 1000 MHz, temperature 40'C
NPU 2: voltage 750 mV, clock 1000 MHz, temperature 40'C
dvfs Disabled
=======================================================
```

**Troubleshooting**:
- If no device is detected, check M.2 module connection
- Verify driver installation with `lsmod | grep dx`
- Check system logs with `dmesg | grep -i deepx`

## Step 3: Build DX-Stream

### 3.1 Install Build Dependencies

The Orange Pi 5 Plus Ubuntu image includes pre-configured GStreamer packages with Rockchip hardware acceleration support. Install the additional required packages:

```bash
sudo apt-get update
sudo apt install -y python3-pip ninja-build
sudo pip install meson
sudo apt-get install -y libeigen3-dev libjson-glib-dev librdkafka-dev libmosquitto-dev libyuv-dev
```

**Package Descriptions**:
- `python3-pip`, `ninja-build`, `meson`: Build system components
- `libeigen3-dev`: Linear algebra library for computer vision
- `libjson-glib-dev`: JSON parsing library for GLib
- `librdkafka-dev`: Apache Kafka client library
- `libmosquitto-dev`: MQTT broker client library
- `libyuv-dev`: YUV color space conversion library

### 3.2 Clone and Build DX-Stream

```bash
cd ~/dxnn
git clone https://github.com/DEEPX-AI/dx_stream.git
cd dx_stream
./build.sh
```

**Build Process**:
1. Configures Meson build system
2. Compiles all GStreamer plugins
3. Builds sample applications and utilities
4. Installs plugins to system directories

**Build Time**: Approximately 10-15 minutes on Orange Pi 5 Plus

### 3.3 Verify Installation

Check if DX-Stream plugins are properly installed:

```bash
gst-inspect-1.0 | grep dx
```

**Expected Output**:
```
dxstream:  dxgather: DX Gather
dxstream:  dxinfer: DX Inference
dxstream:  dxmsgbroker: DX Message Broker
dxstream:  dxmsgconv: DX Message Converter
dxstream:  dxosd: DX On-Screen Display
dxstream:  dxpostprocess: DX Post-process
dxstream:  dxpreprocess: DX Pre-process
...
```

## Step 4: Running Demo Applications

### 4.1 Setup Sample Models and Videos

```bash
cd ~/dxnn/dx_stream
./setup.sh
```

### 4.2 Run Demo Pipeline

```bash
./run_demo.sh
```

**Available Demo Options**:
- Object detection with YOLO models
- Face detection and recognition
- Pose estimation
- Real-time video processing

For detailed demo instructions, refer to the [**Installation Guide**](./02_DX-STREAM_Installation.md).

## Performance Optimization

### Hardware Acceleration Features

The Orange Pi 5 Plus image includes optimized drivers for:
- **Video Decode/Encode**: Rockchip RK3588 VPU
- **GPU Acceleration**: Mali-G610 MP4
- **NPU Acceleration**: DEEPX DX-M1 (25 TOPS)

## Troubleshooting

### Common Issues

**1. NPU Not Detected**
```bash
# Check PCIe connection
lspci | grep -i deepx
# Verify driver loading
dmesg | grep -i dx_rt
```

**2. GStreamer Plugin Not Found**
```bash
# Refresh plugin cache
gst-inspect-1.0 --plugin-path=/usr/local/lib/gstreamer-1.0
```

**3. Build Failures**
```bash
# Clean and rebuild
./build.sh
```

**4. Performance Issues**
- Verify NPU clock speeds with `dxrt-cli -s`
- Check thermal throttling with `cat /sys/class/thermal/thermal_zone*/temp`
- Monitor CPU usage with `htop`

### Getting Help

- **Documentation**: [DX-Stream User Manual](../DEEPX_DX-STREAM_UM_v2.0.0.pdf)
- **Support**: Contact DEEPX technical support
- **Community**: Check GitHub issues and discussions

## Next Steps

After successful installation:

1. Explore the [**Pipeline Examples**](./Pipeline_Example/05_01_Single-Stream.md)
2. Learn about [**Writing Custom Applications**](./04_Writing_Your_Own_Application.md)
3. Review [**Element Documentation**](./Elements/03_01_DxPreprocess.md) for advanced usage

