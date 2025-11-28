# Building DX-Stream on Orange Pi 5 Plus

This guide provides comprehensive instructions for building and running DX-Stream on the Orange Pi 5 Plus single-board computer with **DEEPX DX-M1 NPU** for AI accelerator.

The Orange Pi 5 Plus is a powerful ARM-based development board designed for high-performance edge computing. When paired with the DEEPX DX-M1 NPU, it provides significant acceleration for AI and computer vision applications. This guide covers the complete setup process from SD card preparation to running DX-Stream applications.

**DX-Stream Building Process**

The overall construction and validation process is broken down into four sequential steps.

- Step 1: SD Card Setup
- Step 2: DX-RT NPU Driver Installation
- Step 3: Build DX-Stream
- Step 4: Running Demo Applications


## Prerequisites

This section outlines the essential hardware and software components required for building and running DX-Stream on the Orange Pi 5 Plus with the DEEPX DX-M1 NPU. Ensuring these requirements is crucial for a successful installation.

### Hardware Requirements
- Orange Pi 5 Plus development board
- DEEPX DX-M1 NPU module (M.2 form factor)
- MicroSD card (32GB or larger, Class 10 recommended)
- Power supply (5V/4A minimum)
- Host Linux machine for SD card preparation

### Software Requirements
- 7-Zip or similar extraction utility
- SD card flashing tool or `dd` command access


## Building Process

### Step 1: SD Card Setup

Prepare the base Ubuntu OS environment.

#### Step 1.1 Download Ubuntu Image

Download the official Orange Pi 5 Plus Ubuntu 22.04 image:

- **Download Link**: [Orange Pi 5 Plus Ubuntu 22.04 Image](https://drive.google.com/file/d/1l72cF6dsTzwU5NloY3FEKTuKA9KbhX80/view?usp=drive_link)

- **Image Version**: Ubuntu 22.04 Jammy Desktop XFCE with Linux kernel 6.1.43

- **File Size**: Approximately 4GB (compressed)

#### Step 1.2 Extract Image

Extract the downloaded 7z archive:

```bash
7z x Orangepi5plus_1.2.0_ubuntu_jammy_desktop_xfce_linux6.1.43.7z
```

!!! note "NOTE" 

    If 7-Zip is not installed, install it using:
    ```bash
    # Ubuntu/Debian
    sudo apt install p7zip-full

    # CentOS/RHEL
    sudo yum install p7zip
    ```

#### Step 1.3 Flash Image to SD Card

!!! warning "WARNING" 

    Replace `/dev/sda` with your actual SD card device. Use `lsblk` or `fdisk -l` to identify the correct device.

```bash
sudo umount /dev/sda

sudo dd if=./Orangepi5plus_1.2.0_ubuntu_jammy_desktop_xfce_linux6.1.43.img of=/dev/sda bs=4M status=progress

sync
```

**Alternative Methods**:

- **Raspberry Pi Imager**: Cross-platform GUI tool
- **Balena Etcher**: User-friendly flashing utility
- **Win32DiskImager**: Windows-specific tool

#### Step 1.4 Initial Boot

    1. Insert the SD card into the Orange Pi 5 Plus
    2. Connect power supply and peripherals
    3. Power on the device

**Default Credentials** (if applicable):

- Username: `orangepi`
- Password: `orangepi`

### Step 2: DX-RT NPU Driver Installation

Install the necessary drivers and runtime for the DEEPX NPU accelerator.

All following steps should be performed on the Orange Pi 5 Plus itself after successful boot.

#### Step 2.1 Update Linux Kernel Headers

Install the required kernel headers for driver compilation:

```bash
cd /opt
sudo apt install ./linux-headers-current-rockchip-rk3588_1.2.0_arm64.deb
```

!!! note "NOTE" 

    The kernel headers package should be pre-installed in the Orange Pi image. If missing, contact DEEPX support.

#### Step 2.2 Configure Git (Optional)

If you encounter SSL certificate issues with GitHub:

```bash
git config --global http.sslVerify false
```

!!! note "NOTE" 

    This disables SSL verification globally. For production use, consider configuring proper certificates instead.

#### Step 2.3 Build and Install NPU Linux Driver

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

#### Step 2.4 Build and Install DX-RT Runtime

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

#### Step 2.5 Verify DX-RT Installation

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

### Step 3: Build DX-Stream

Compile and install the DX-Stream GStreamer framework and plugins.

#### Step 3.1 Install Build Dependencies

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

#### Step 3.2 Clone and Build DX-Stream

```bash
cd ~/dxnn
git clone https://github.com/DEEPX-AI/dx_stream.git
cd dx_stream
./build.sh
```

**Build Process**:

- Configures Meson build system
- Compiles all GStreamer plugins
- Builds sample applications and utilities
- Installs plugins to system directories

**Build Time**: Approximately 10-15 minutes on Orange Pi 5 Plus

#### Step 3.3 Verify Installation

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

### Step 4: Running Demo Applications

Verify the complate system functionality and NPU acceleration.

#### Step 4.1 Setup Sample Models and Videos

```bash
cd ~/dxnn/dx_stream
./setup.sh
```

#### Step 4.2 Run Demo Pipeline

```bash
./run_demo.sh
```

**Available Demo Options**:

- Object detection with YOLO models
- Face detection and recognition
- Pose estimation
- Real-time video processing

For detailed demo instructions, refer to the [**Installation Guide**](./02_DX-STREAM_Installation.md).

!!! note "NOTE" 

    Hardware Acceleration
    
    The Orange Pi 5 Plus image includes optimized drivers for:

    - Video Decode/Encode (VPU): Rockchip RK3588 VPU
    - GPU Acceleration: Mali-G610 MP4
    - NPU Acceleration (AI): DEEPX DX-M1 (25 TOPS)

    For maximum performance, ensure your pipelines utilize these specific hardware accelerators.


## Troubleshooting

This section provides guidance for resolving common issues encountered during the DX-Stream installation or runtime.

### Common Issues

**1. NPU Not Detected**

Verify the NPU module is correctly recongnized and the driver is loaded.

```bash
# Check PCIe connection
lspci | grep -i deepx
# Verify driver loading
dmesg | grep -i dx_rt
```

**2. GStreamer Plugin Not Found**

If DX-Stream plugins are not registered, manually refresh the plugin cache.

```bash
# Refresh plugin cache
gst-inspect-1.0 --plugin-path=/usr/local/lib/gstreamer-1.0
```

**3. Build Failures**

If the build process fails, clean the environment and rebuild.

```bash
# Clean and rebuild
./build.sh
```

**4. Performance Issues**

Monitor hardware status if performance is slower than expected.

- Verify NPU Status

    ```
    dxrt-cli -s
    ```

- Monitor CPU usage 

    ```
    htop
    ```

!!! note "NOTE" 
    
    Next Steps

    After successful installation:

    - Explore the [**Pipeline Examples**](./Pipeline_Example/05_01_Single-Stream.md)
    - Learn about [**Writing Custom Applications**](./04_Writing_Your_Own_Application.md)
    - Review [**Element Documentation**](./Elements/03_01_DxPreprocess.md) for advanced usage

