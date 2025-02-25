#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

if [ -d "$SCRIPT_DIR/install" ]; then
    rm -rf $SCRIPT_DIR/install
fi

if [ -d "$SCRIPT_DIR/build" ]; then
    rm -rf $SCRIPT_DIR/build
fi

meson setup build --buildtype=debug --prefix="$SCRIPT_DIR"
meson compile -C build
meson install -C build
