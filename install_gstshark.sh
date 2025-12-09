#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")

ARCH=$(uname -m)

sudo apt install graphviz libgraphviz-dev octave epstool babeltrace

mkdir -p "$PROJECT_ROOT/util"

cd "$PROJECT_ROOT/util" || exit 1

if [ ! -d "gstshark" ]; then
    git clone https://github.com/RidgeRun/gst-shark/
fi

cd gstshark || exit 1
./autogen.sh --prefix /usr --libdir /usr/lib/${ARCH}-linux-gnu
make
sudo make install
sudo ldconfig

# Insert gstshark-plot to PATH in .bashrc if not already present
if ! grep -q "${PROJECT_ROOT}/util/gst-shark/scripts/graphics" ~/.bashrc; then
    echo "export PATH=\"\$PATH:${PROJECT_ROOT}/util/gst-shark/scripts/graphics\"" >>~/.bashrc
    source ~/.bashrc
fi
