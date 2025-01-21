#!/bin/bash
# dependencies install script in host
pushd .
cmd=()
DX_SRC_DIR=$PWD
echo "DX_SRC_DIR the default one $DX_SRC_DIR"
target_arch=$(uname -m)
build_type='Release'

function help()
{
    echo "./install.sh"
    echo "    --help            show this help"
    echo "    --arch            target CPU architecture : [ x86_64, aarch64, riscv64 ]"
}

function compare_version()
{
    awk -v n1="$1" -v n2="$2" 'BEGIN { if (n1 >= n2) exit 0; else exit 1; }'
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

    sudo apt-get install -y libgtk2.0-dev libfreetype-dev v4l-utils libv4l-dev libeigen3-dev libjson-glib-1.0-0 libjson-glib-dev libmosquitto-dev

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

    # Check if librdkafka is installed
    if ldconfig -p | grep -q librdkafka; then
        echo "Kafka (librdkafka) is already installed. Skipping installation."
    else
        echo "Kafka (librdkafka) is not installed. Proceeding with installation."
        
        # Install Kafka (librdkafka v2.6.1)
        if ! test -e $DX_SRC_DIR/util; then 
            mkdir -p $DX_SRC_DIR/util
        fi

        cd $DX_SRC_DIR/util
        sudo apt update
        sudo apt install -y libssl-dev libsasl2-dev libzstd-dev libz-dev

        if [ ! -d librdkafka ]; then
            git clone --branch v2.6.1 https://github.com/confluentinc/librdkafka.git
        fi

        cd librdkafka
        ./configure --enable-ssl
        make -j"$(nproc)"
        sudo make install
        sudo ldconfig

        echo "Kafka (librdkafka) installation completed."
    fi
}

[ $# -gt 0 ] && \
while (( $# )); do
    case "$1" in
        --help) help; exit 0;;
        --arch)
            shift
            target_arch=$1
            shift;;
        *)       echo "Invalid argument : " $1 ; help; exit 1;;
    esac
done

if [ $target_arch == "arm64" ]; then
    target_arch=aarch64
    echo " Use arch64 instead of arm64"
fi

install_dx_stream_dep

popd
