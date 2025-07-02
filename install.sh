#!/bin/bash
# dependencies install script in host

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
VENV_PATH=${PROJECT_ROOT}/venv-dx_stream

cmd=()
DX_SRC_DIR=$PWD
echo "DX_SRC_DIR the default one $DX_SRC_DIR"
target_arch=$(uname -m)
build_type='Release'

# color env settings
source ${PROJECT_ROOT}/scripts/color_env.sh

pushd $SCRIPT_DIR

function help()
{
    echo "./install.sh"
    echo "    --help            show this help"
}

# Version comparison: returns 0 if $1 < $2
version_lt() {
    dpkg --compare-versions "$1" lt "$2"
}

# Install package via apt
install_apt_pkgs() {
    local pacakges=$1
    echo "Installing $pkgs from apt..."
    sudo apt-get update
    sudo apt-get install -y "$pkgs"
}

# Function to get installed CMake version
get_cmake_version() {
    cmake --version 2>/dev/null | head -n1 | awk '{print $3}'
}

# Build CMake from source
build_cmake_from_source() {
    local version=$1
    echo "Building CMake $version from source..."
    sudo apt-get update
    sudo apt-get install -y build-essential libssl-dev wget

    cd /tmp
    rm -rf "cmake-$version" "cmake-$version.tar.gz"
    wget "https://github.com/Kitware/CMake/releases/download/v$version/cmake-$version.tar.gz"
    tar -zxvf "cmake-$version.tar.gz"
    cd "cmake-$version"

    ./bootstrap -- -DCMAKE_BUILD_TYPE:STRING=Release
    make -j$(nproc)
    sudo make install
    sudo ldconfig

    echo "CMake $(cmake --version | head -n1) installed from source!"
}

setup_cmake() {
    local package_name=$1
    local required_version=$2
    local installed_version=$(get_cmake_version || echo "")
    
    local build_needed=0
    local install_needed=0
    if [ -z "$installed_version" ]; then
        echo "${package_name} is not installed. Source build is required."
        install_needed=1

        local apt_prebuilt_version=$(get_apt_prebuilt_version "${package_name}")

        if version_lt "$apt_prebuilt_version" "$required_version"; then
            echo "${package_name} apt prebuilt version: $apt_prebuilt_version (required: $required_version). Source build is required."
            build_needed=1
        else
            echo "${package_name} apt prebuilt version: $apt_prebuilt_version (meets requirement). No source build needed."
            install_needed=1
            
        fi
    else
        if version_lt "$installed_version" "$required_version"; then
            echo "Installed ${package_name} version: $installed_version (required: $required_version). Source build is required."
            build_needed=1
        else
            echo "Installed ${package_name} version: $installed_version (meets requirement). No source build needed."
        fi
    fi

    if [ $build_needed -eq 1 ]; then
        if [ -n "$installed_version" ]; then
            remove_apt_package "$package_name"
        fi
        build_cmake_from_source "$required_version"
    elif [ $install_needed -eq 1 ]; then
        install_apt_pkgs "$package_name"
    fi
}

# Setup Python 3.7 and venv
setup_python_env() {
    # 1. Install Python 3.7
    sudo add-apt-repository ppa:deadsnakes/ppa -y
    sudo apt-get update
    sudo apt-get install -y python3.7 python3.7-dev python3.7-venv

    # 2. Create venv with Python 3.7
    python3.7 -m venv ${VENV_PATH}

    # 3. Upgrade pip, wheel, setuptools
    activate_venv
    echo "Upgrading pip, wheel, setuptools..."

    UBUNTU_VERSION=$(lsb_release -rs)
    if [ "$UBUNTU_VERSION" = "24.04" ]; then
        pip install --upgrade "setuptools<65"
    elif [ "$UBUNTU_VERSION" = "22.04" ] || [ "$UBUNTU_VERSION" = "20.04" ]; then
        pip install --upgrade "pip wheel setuptools<65"
    fi
}

activate_venv() {
    echo -e "=== activate_venv() ${TAG_START} ==="

    # activate venv
    source ${VENV_PATH}/bin/activate
    if [ $? -ne 0 ]; then
        echo -e "${TAG_ERROR} Activate venv failed! Please try installing again with the '--force' option."
        rm -rf "$VENV_PATH"
        echo -e "${TAG_ERROR} === ACTIVATE VENV FAIL ==="
        exit 1
    fi

    echo -e "=== activate_venv() ${TAG_DONE} ==="
}

# Install Meson
install_meson() {
    activate_venv
    pip install meson==1.3
}

setup_gcc_gpp() {
    # Check current gcc/g++ version
    gcc_version=$(gcc -dumpversion | cut -f1 -d.)
    gpp_version=$(g++ -dumpversion | cut -f1 -d.)

    echo "Current GCC version: $gcc_version"
    echo "Current G++ version: $gpp_version"

    # If version is less than 11, install gcc-11 and g++-11
    if [ "$gcc_version" -lt 11 ] || [ "$gpp_version" -lt 11 ]; then
        # Check if gcc-11 is installed
        
        if dpkg -s gcc-11 >/dev/null 2>&1; then
            echo "GCC/G++ 11 is already installed."
        else
            echo "Installing GCC/G++ 11..."

            sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
            sudo apt update
            sudo apt install -y gcc-11 g++-11

            if [ $? -ne 0 ]; then
                echo "Failed to install GCC-11. Exiting."
                exit 1
            fi
        fi
        
    else
        echo "Found GCC/G++ $gcc_version is already installed."
    fi
}

setup_build_env() {
    # Required minimum CMake version
    local required_cmake_version="3.17.0"
    setup_cmake "cmake" "${required_cmake_version}"

    # 2. Setup Python 3.7 environment
    setup_python_env

    # 3. Install Meson
    install_meson

    # 4. Setup gcc/g++
    setup_gcc_gpp
}

# Get installed version
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

# Build librdkafka from source
build_librdkafka_from_source() {
    local required_version=$1

    install_apt_pkgs "build-essential libssl-dev libsasl2-dev libzstd-dev liblz4-dev libcurl4-openssl-dev pkg-config git"

    echo "Cloning and building librdkafka from source..."
    git clone --depth 1 --branch v${required_version} https://github.com/confluentinc/librdkafka.git
    cd librdkafka

    ./configure --prefix=/usr/local
    make -j$(nproc)
    sudo make install

    ldconfig -p | grep librdkafka
    echo "librdkafka has been built and installed from source!"
}

setup_librdkafka() {
    local package_name=$1
    local required_version=$2
    local apt_prebuilt_version=$(get_apt_prebuilt_version "${package_name}")

    local build_needed=false
    if [ -z "$apt_prebuilt_version" ]; then
        echo "${package_name} apt prebuilt version is not exist. Source build is required."
        build_needed=true
    elif version_lt "$apt_prebuilt_version" "$required_version"; then
        echo "Installed ${package_name} version: $apt_prebuilt_version (required: $required_version). Source build is required."
        build_needed=true
    else
        echo "Installed ${package_name} version: $apt_prebuilt_version (meets requirement). No source build needed."
    fi

    if [ "$build_needed" = true ]; then
        if [ -n "$apt_prebuilt_version" ]; then
            remove_apt_package "${package_name}"
        fi
        build_librdkafka_from_source "${required_version}"
    else
        install_apt_pkgs "${package_name}"
    fi
}

function install_dx_stream_dep() {
    echo "Install DX-Stream Dependency"
    sudo apt-get update

    sudo apt-get install -y apt-utils software-properties-common cmake build-essential make zlib1g-dev libcurl4-openssl-dev git curl wget tar zip unzip ninja-build

    sudo apt-get install -y python3 python3-dev python3-setuptools python3-pip python3-tk python3-lxml python3-six

    sudo apt-get install -y libjpeg-dev libtiff5-dev libpng-dev libavcodec-dev libavformat-dev libswscale-dev libxvidcore-dev libx264-dev libxine2-dev

    sudo apt-get install -y libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev gstreamer1.0-tools \
        gstreamer1.0-plugins-{base,good,bad,ugly} gstreamer1.0-libav gstreamer1.0-rtsp gstreamer1.0-python3-plugin-loader \
        gir1.2-gst-rtsp-server-1.0 gstreamer1.0-gl

    sudo apt-get install -y libgtk2.0-dev v4l-utils libv4l-dev libeigen3-dev libjson-glib-1.0-0 libjson-glib-dev libmosquitto-dev

    if apt-cache show libfreetype-dev > /dev/null 2>&1; then
        sudo apt-get install -y libfreetype-dev
    fi

    sudo apt-get install -y x11-apps libx11-6 xauth libxext6 libxrender1 libxtst6 libxi6

    sudo apt-get install -y libopencv-dev python3-opencv

    # Install Meson
    if ! command -v meson >/dev/null 2>&1; then
        echo "Installing Meson..."
        if ! test -e $DX_SRC_DIR/util; then 
            mkdir -p "$DX_SRC_DIR/util"
        fi
        cd "$DX_SRC_DIR/util"
        git clone https://github.com/mesonbuild/meson.git meson
        cd meson

        # Install Meson from source
        sudo python3 setup.py install

        # Verify installation
        if meson --version >/dev/null 2>&1; then
            echo "Meson successfully installed! Version: $(meson --version)"
        else
            echo "Error: Meson installation failed!"
            exit 1
        fi
        rm -rf ./meson
    fi

    # install libyuv
    if [ ! -f "/usr/local/lib/libyuv.so" ]; then
        echo "Installing libyuv..."
        libyuv_installed=false

        if apt-cache search libyuv0 | grep -q "libyuv0"; then
            echo "Installing libyuv0 and libyuv-dev using apt..."
            sudo apt-get -y install libyuv0 libyuv-dev
            libyuv_installed=true
        fi

        if [ "$libyuv_installed" == false ]; then
            if ! test -e $DX_SRC_DIR/util; then 
                mkdir -p "$DX_SRC_DIR/util"
            fi
            cd "$DX_SRC_DIR/util"

            if [ $(uname -m) == "x86_64" ]; then
                if [ ! -d "libyuv" ]; then
                    git clone -b main https://chromium.googlesource.com/libyuv/libyuv libyuv
                fi
                cd libyuv
                mkdir -p build && cd build
                cmake .. -DCMAKE_BUILD_TYPE=Release
                make -j"$(nproc)"
                sudo make install
                sudo ldconfig
                echo "Verifying libyuv installation..."
                if [ -f "/usr/local/lib/libyuv.a" ] || [ -f "/usr/local/lib/libyuv.so" ]; then
                    echo "libyuv successfully installed!"
                else
                    echo "Error: libyuv installation failed."
                    exit 1
                fi
                rm -rf ./libyuv
            elif [ $(uname -m) == "aarch64" ]; then
                wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv0_0.0.1888.20240710-3_arm64.deb
                wget https://launchpad.net/ubuntu/+archive/primary/+files/libyuv-dev_0.0.1888.20240710-3_arm64.deb
                sudo dpkg -i ./libyuv*
                sudo apt-get -f install
            else
                echo "Error: Unsupported architecture $(uname -m)"
                exit 1
            fi
        fi
    fi

    sudo add-apt-repository -y ppa:mosquitto-dev/mosquitto-ppa
    sudo apt install -y mosquitto mosquitto-clients
    sudo apt install -y libssl-dev libsasl2-dev libzstd-dev libz-dev default-jdk

    setup_librdkafka "librdkafka-dev" "2.2.0"

    # # for kafka demo
    # if ! test -e $DX_SRC_DIR/util; then 
    #     mkdir -p $DX_SRC_DIR/util
    # fi

    # cd $DX_SRC_DIR/util
    # wget https://downloads.apache.org/kafka/3.9.0/kafka_2.13-3.9.0.tgz
    # tar -xzf kafka_2.13-3.9.0.tgz
    # cd kafka_2.13-3.9.0.tgz
}

function show_information_message(){
    echo -e "${TAG_INFO} To activate the virtual environment, run:"
    echo -e "${COLOR_BRIGHT_YELLOW_ON_BLACK}  source ${VENV_PATH}/bin/activate ${COLOR_RESET}"
}

[ $# -gt 0 ] && \
while (( $# )); do
    case "$1" in
        --help) help; exit 0;;
        *)       echo "Invalid argument : " $1 ; help; exit 1;;
    esac
done

if [ $target_arch == "arm64" ]; then
    target_arch=aarch64
    echo " Use arch64 instead of arm64"
fi

function main() {
    setup_build_env
    install_dx_stream_dep
    show_information_message
}

main

popd
