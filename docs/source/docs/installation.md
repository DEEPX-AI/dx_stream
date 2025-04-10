# Installation

## Prerequisites

### Supported Environments

| **Architecture**      | **Supported Environments**        |
|------------------------|------------------------------------|
| **x86_64**            | Ubuntu 20.04                     |
| **Orange Pi 5+ (aarch64)** | Ubuntu 20.04 ~ 22.04            |
| **Raspberry Pi 5 (aarch64)** | Raspberry Pi OS                 |


### Pre Installation

| **Package**              | **Version**     | **Description**               |
|---------------------------|-----------------|--------------------------------|
| **M1 Accelerator**        | FW v1.6.3      | Firmware for the M1A accelerator. |
| **NPU Driver**            | v1.3.3         | Includes PCIE v1.2.0 support. |
| **DX-RT**                 | v2.7.0         | Runtime library for AI inference. |

- Ensure the NPU Driver and DX-RT must be installed
    NPU Driver Installation : TBD
    DX-RT Installation : TBD

---

## Installation

### Build from sources

- Unzip Package 
    ```
    $ unzip dxstream_vX.X.X.zip -d dx_stream
    $ cd dx_stream
    ```

- Install Dependencies
    ```
    $ ./install.sh
    ```
    Running the provided script will seamlessly install all the necessary dependencies.

- Build DX-Stream
    ```
    $ ./build.sh --install
    ```

    (Optional) For debug dx-stream:

    ```
    $ ./build.sh --debug
    ```

- Verify the installation using:
    ```
    $ gst-inspect-1.0 dxstream
    ```

- Remove DX-Stream
    ```
    $ ./build.sh --uninstall
    ```

### Using Docker

#### Install Docker

- Install Docker by running the following commands
    ```
    # Update package list
    sudo apt-get update -y

    # Install curl
    sudo apt-get install -y curl

    # Download Docker installation script
    curl -fsSL https://get.docker.com -o get-docker.sh

    # Run Docker installation script
    sh get-docker.sh

    # Add current user to the Docker group
    sudo usermod -aG docker $USER

    # Reboot system to apply changes
    sudo reboot
    ```
- Verify Docker installation
    ```
    $ docker --version
    $ docker run hello-world
    ```

#### Running DX-Stream with Docker

- Load Docker Images
    ```
    $ ./dk load ./dxstream_vX.X.X_amd64_ubuntu20.04.tar
    ```

- Run Docker Container
    ```
    $ ./dk run
    ```
    
    **When running the demo in a Docker environment, you must stop the DX-RT service on the host system before executing the run command to access the Docker environment.**

- Remove Docker Image
    ```
    $ ./dk rmi
    ```
