#!/bin/bash

# Determine the architecture
ARCH=$(uname -m)

# Set the plugin directory based on the architecture
if [ "$ARCH" == "x86_64" ]; then
    GSTREAMER_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
elif [ "$ARCH" == "aarch64" ]; then
    GSTREAMER_PLUGIN_DIR="/usr/lib/aarch64-linux-gnu/gstreamer-1.0"
else
    echo "Error: Unsupported architecture '$ARCH'."
    exit 1
fi

# Define variables
WRC=$PWD
BUILD_DIR=builddir
USAGE="Usage: $0 [--debug | --install | --uninstall | --help]

Options:
  --debug       Build for debug
  --install     Build and install the project.
  --uninstall   Remove installed files.
  --help (--h)  Display this help message."

# Function to display usage
function show_help {
    echo "$USAGE"
    exit 0
}

# Check for arguments
if [[ $# -eq 0 ]]; then
    show_help
fi

# Parse arguments
case $1 in
    --debug)
        # remove exist library
        echo "Starting uninstallation process..."
        TARGET_FILE="/usr/local/lib/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi
        
        TARGET_FILE="$GSTREAMER_PLUGIN_DIR/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi

        if [ -f "/usr/local/lib/libdx_msgconvl.so" ]; then
            echo "Removing /usr/local/lib/libdx_msgconvl.so"
            sudo rm -rf /usr/local/lib/libdx_msgconvl.so
        else
            echo "Error: /usr/local/lib/libdx_msgconvl.so not found. Was the project installed?"
        fi

        if [ -d "/usr/local/include/dx_stream" ]; then
            echo "Removing /usr/local/include/dx_stream"
            sudo rm -rf /usr/local/include/dx_stream
        else
            echo "Error: /usr/local/include/dx_stream not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/bin" ]; then
            echo "Removing /usr/share/dx-stream/bin"
            sudo rm -rf /usr/share/dx-stream/bin
        else
            echo "Error: /usr/share/dx-stream/bin not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/lib" ]; then
            echo "Removing /usr/share/dx-stream/lib"
            sudo rm -rf /usr/share/dx-stream/lib
        else
            echo "Error: /usr/share/dx-stream/lib not found. Was the project installed?"
        fi
        echo "Uninstallation completed."

        # install for debug
        echo "Starting installation process..."
        cd gst-dxstream-plugin
        meson setup ${BUILD_DIR} --buildtype=debug
        meson compile -C ${BUILD_DIR}
        echo "Install DX-Stream to $GSTREAMER_PLUGIN_DIR"
        sudo meson install -C ${BUILD_DIR}
        rm -rf ${BUILD_DIR}
        sudo ln -s $GSTREAMER_PLUGIN_DIR/libgstdxstream.so /usr/local/lib/libgstdxstream.so
        sudo ldconfig
        cd $WRC/dx_stream/custom_library
        ./build.sh debug
        cd $WRC/dx_stream/apps
        ./build.sh debug
        echo "Installation completed."
        exit 0
        ;;
    --install)
        # remove exist library
        echo "Starting uninstallation process..."
        TARGET_FILE="/usr/local/lib/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi
        
        TARGET_FILE="$GSTREAMER_PLUGIN_DIR/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi

        if [ -f "/usr/local/lib/libdx_msgconvl.so" ]; then
            echo "Removing /usr/local/lib/libdx_msgconvl.so"
            sudo rm -rf /usr/local/lib/libdx_msgconvl.so
        else
            echo "Error: /usr/local/lib/libdx_msgconvl.so not found. Was the project installed?"
        fi

        if [ -d "/usr/local/include/dx_stream" ]; then
            echo "Removing /usr/local/include/dx_stream"
            sudo rm -rf /usr/local/include/dx_stream
        else
            echo "Error: /usr/local/include/dx_stream not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/bin" ]; then
            echo "Removing /usr/share/dx-stream/bin"
            sudo rm -rf /usr/share/dx-stream/bin
        else
            echo "Error: /usr/share/dx-stream/bin not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/lib" ]; then
            echo "Removing /usr/share/dx-stream/lib"
            sudo rm -rf /usr/share/dx-stream/lib
        else
            echo "Error: /usr/share/dx-stream/lib not found. Was the project installed?"
        fi
        echo "Uninstallation completed."

        echo "Starting installation process..."
        cd gst-dxstream-plugin
        meson setup ${BUILD_DIR} --buildtype=release
        meson compile -C ${BUILD_DIR}
        echo "Install DX-Stream to $GSTREAMER_PLUGIN_DIR"
        sudo meson install -C ${BUILD_DIR}
        rm -rf ${BUILD_DIR}
        sudo ln -s $GSTREAMER_PLUGIN_DIR/libgstdxstream.so /usr/local/lib/libgstdxstream.so
        sudo ldconfig
        cd $WRC/dx_stream/custom_library
        ./build.sh
        cd $WRC/dx_stream/apps
        ./build.sh
        echo "Installation completed."
        exit 0
        ;;
    --uninstall)
        echo "Starting uninstallation process..."
        TARGET_FILE="/usr/local/lib/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi
        
        TARGET_FILE="$GSTREAMER_PLUGIN_DIR/libgstdxstream.so"
        if [ -f "$TARGET_FILE" ]; then
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        else
            echo "Error: $TARGET_FILE not found. Was the project installed?"
        fi

        if [ -f "/usr/local/lib/libdx_msgconvl.so" ]; then
            echo "Removing /usr/local/lib/libdx_msgconvl.so"
            sudo rm -rf /usr/local/lib/libdx_msgconvl.so
        else
            echo "Error: /usr/local/lib/libdx_msgconvl.so not found. Was the project installed?"
        fi

        if [ -d "/usr/local/include/dx_stream" ]; then
            echo "Removing /usr/local/include/dx_stream"
            sudo rm -rf /usr/local/include/dx_stream
        else
            echo "Error: /usr/local/include/dx_stream not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/bin" ]; then
            echo "Removing /usr/share/dx-stream/bin"
            sudo rm -rf /usr/share/dx-stream/bin
        else
            echo "Error: /usr/share/dx-stream/bin not found. Was the project installed?"
        fi

        if [ -d "/usr/share/dx-stream/lib" ]; then
            echo "Removing /usr/share/dx-stream/lib"
            sudo rm -rf /usr/share/dx-stream/lib
        else
            echo "Error: /usr/share/dx-stream/lib not found. Was the project installed?"
        fi
        echo "Uninstallation completed."
        exit 0
        ;;
    --help | --h)
        show_help
        ;;
    *)
        echo "Error: Invalid option '$1'"
        show_help
        exit 1
        ;;
esac
