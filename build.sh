#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
VENV_PATH=${PROJECT_ROOT}/venv-dx_stream

# Set default PREFIX (can be overridden with --prefix option)
PREFIX="/usr/local"

# GStreamer plugin directory will be determined by meson's libdir option
# No need to calculate it here - meson handles it automatically

# Define variables
WRC=$PWD
BUILD_DIR=builddir
BUILD_TYPE="release"
TYPE_ARG=""
SONAR_MODE_ARG=""
V3_MODE=""
CLEAN_MODE=""
UNINSTALL_MODE=""

show_help() {
  echo "Usage: $(basename "$0") [--prefix=PATH] [--clean] [--v3] [--type=TYPE] [--sonar] [--uninstall] [--help]"
  echo "Example 1: $0"
  echo "Example 2: $0 --prefix=/opt/dx-stream"
  echo "Example 3: $0 --clean"
  echo "Example 4: $0 --v3"
  echo "Example 5: $0 --type=debug"
  echo "Example 6: $0 --type=Release"
  echo "Example 7: $0 --sonar"
  echo "Example 8: $0 --uninstall"
  echo "Example 9: $0 --uninstall --prefix=./install"
  echo "Options:"
  echo "  [--prefix=PATH] Set installation prefix (default: /usr/local)"
  echo "                  Plugin will be installed to PREFIX/lib/ARCH/gstreamer-1.0/"
  echo "                  Custom libs to PREFIX/share/gstdxstream/lib/"
  echo "                  Apps to PREFIX/share/gstdxstream/bin/"
  echo "  [--clean]       Remove previous build files before building & installing"
  echo "  [--v3]          Build for DEEPX V3 Standalone Device (skip Host installation)."
  echo "  [--uninstall]   Remove installed files (use --prefix if installed to custom location)."
  echo "  [--type=TYPE]   Set build type: Debug/debug or Release/release (default: release)"
  echo "  [--sonar]       Build for SonarQube analysis (includes coverage)"
  echo "  [--help]        Show this help message"
  echo ""
  echo "Python Environment:"
  echo "  To install pydxs in a virtual environment, activate it before running build.sh:"
  echo "    source venv/bin/activate"
  echo "    ./build.sh"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}

clean() {
    local mode="$1"     # uninstall | clean

    echo "Starting ${mode} process..."

    if [ "$mode" == "clean" ]; then
        echo "Cleaning build directories..."
        if [ -d "${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}" ]; then
            echo "Removing build directory: ${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}"
            rm -rf "${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}"
        else
            echo "Warn: Build directory ${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR} not found. So, skip to remove directory."
        fi
        
        # Clean pydxs build cache
        echo "Cleaning pydxs build cache..."
        rm -rf "${PROJECT_ROOT}/bindings/python/pydxs/build"
        rm -rf "${PROJECT_ROOT}/bindings/python/pydxs/_skbuild"
        rm -rf "${PROJECT_ROOT}/bindings/python/pydxs/dist"
        rm -rf "${PROJECT_ROOT}/bindings/python/pydxs"/*.egg-info
        
        return 0
    fi
    
    echo "Uninstalling DX-Stream plugin and related files..."
    
    # Remove GStreamer plugin (search in libdir/*/gstreamer-1.0)
    echo "Searching for libgstdxstream.so..."
    PLUGIN_FILES=$(find "${PREFIX}/lib" -name "libgstdxstream.so" -path "*/gstreamer-1.0/*" 2>/dev/null)
    if [ -n "$PLUGIN_FILES" ]; then
        for TARGET_FILE in $PLUGIN_FILES; do
            echo "Removing $TARGET_FILE..."
            sudo rm "$TARGET_FILE"
        done
    else
        echo "Warn: libgstdxstream.so not found. So, skip to remove file."
    fi

    # Remove pkg-config file
    PKGCONFIG_FILE="${PREFIX}/lib/pkgconfig/gstdxstream.pc"
    if [ -f "$PKGCONFIG_FILE" ]; then
        echo "Removing $PKGCONFIG_FILE..."
        sudo rm "$PKGCONFIG_FILE"
    else
        echo "Warn: $PKGCONFIG_FILE not found. So, skip to remove file."
    fi

    # Remove headers
    if [ -d "${PREFIX}/include/gstdxstream" ]; then
        echo "Removing ${PREFIX}/include/gstdxstream"
        sudo rm -rf "${PREFIX}/include/gstdxstream"
    else
        echo "Warn: ${PREFIX}/include/gstdxstream not found. So, skip to remove directory."
    fi

    # Remove shared data directory
    if [ -d "${PREFIX}/share/gstdxstream" ]; then
        echo "Removing ${PREFIX}/share/gstdxstream"
        sudo rm -rf "${PREFIX}/share/gstdxstream"
    else
        echo "Warn: ${PREFIX}/share/gstdxstream not found. So, skip to remove directory."
    fi

    if [ -d "${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}" ]; then
        echo "Removing build directory: ${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}"
        rm -rf "${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR}"
    else
        echo "Warn: Build directory ${PROJECT_ROOT}/gst-dxstream-plugin/${BUILD_DIR} not found. So, skip to remove directory."
    fi

    # Update library cache
    sudo ldconfig

    # uninstall custom libraries
    cd $WRC/dx_stream/custom_library
    ./build.sh --prefix=${PREFIX} --uninstall
}

# Check and remove old installation files from previous major versions
# This function can be removed in future releases when migration is no longer needed
check_and_remove_old_files() {
    echo "Checking for old installation files..."
    local OLD_FILES_FOUND=false
    local OLD_FILES_LIST=()

    # Check old header directory
    if [ -d "/usr/local/include/dx_stream" ]; then
        OLD_FILES_FOUND=true
        OLD_FILES_LIST+=("/usr/local/include/dx_stream")
    fi

    # Check old plugin in /usr/lib (with arch detection)
    for arch_dir in /usr/lib/*/gstreamer-1.0; do
        if [ -f "${arch_dir}/libgstdxstream.so" ]; then
            OLD_FILES_FOUND=true
            OLD_FILES_LIST+=("${arch_dir}/libgstdxstream.so")
        fi
    done

    # Check old plugin in /usr/local/lib (including symlinks)
    if [ -e "/usr/local/lib/libgstdxstream.so" ] || [ -L "/usr/local/lib/libgstdxstream.so" ]; then
        OLD_FILES_FOUND=true
        OLD_FILES_LIST+=("/usr/local/lib/libgstdxstream.so")
    fi

    # Check old share directory
    if [ -d "/usr/share/dx-stream" ]; then
        OLD_FILES_FOUND=true
        OLD_FILES_LIST+=("/usr/share/dx-stream")
    fi

    # If old files found, prompt for removal
    if [ "$OLD_FILES_FOUND" = true ]; then
        echo ""
        echo "=========================================="
        echo "⚠️  WARNING: Old DX-Stream Installation Detected"
        echo "=========================================="
        echo "The following old installation files were found:"
        echo ""
        for file in "${OLD_FILES_LIST[@]}"; do
            echo "  - $file"
        done
        echo ""
        echo "These files are from a previous major version and should be removed"
        echo "to avoid conflicts with the new installation."
        echo ""
        read -p "Do you want to remove these files? (y/N): " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            echo "Removing old installation files..."
            for file in "${OLD_FILES_LIST[@]}"; do
                if [ -d "$file" ] && [ ! -L "$file" ]; then
                    echo "  Removing directory: $file"
                    sudo rm -rf "$file"
                elif [ -f "$file" ] || [ -L "$file" ]; then
                    echo "  Removing file: $file"
                    sudo rm -f "$file"
                fi
            done
            # Update library cache after removal
            sudo ldconfig
            echo "✓ Old files removed successfully"
        else
            echo ""
            echo "⚠️  Skipping removal. Note: This may cause conflicts!"
            echo "   You can manually remove these files later with:"
            echo ""
            for file in "${OLD_FILES_LIST[@]}"; do
                echo "   sudo rm -rf $file"
            done
            echo ""
        fi
        echo "=========================================="
        echo ""
    fi
}

build() {
    # Check for old installation files from previous major version
    # TODO: Remove this call after a few releases when most users have migrated
    check_and_remove_old_files

    # Clean previous installed files
    if [ "$CLEAN_MODE" == "--clean" ]; then
        clean "clean"
    fi

    # Ensure PREFIX is absolute path (critical for profile.d scripts!)
    if [[ ! "$PREFIX" = /* ]]; then
        echo "Error: PREFIX must be an absolute path at this point!"
        echo "Current PREFIX: $PREFIX"
        exit 1
    fi

    # Set V3 option
    V3_OPTION=""
    if [ "$V3_MODE" == "--v3" ]; then
        V3_OPTION="-Dv3_flag=true"
        echo "Building in V3 mode..."
    fi

    # Set coverage option
    COVERAGE_OPTION=""
    if [ "$SONAR_MODE_ARG" == "--sonar" ]; then
        COVERAGE_OPTION="-Db_coverage=true"
        echo "Building for SonarQube with coverage instrumentation..."
    fi

    # Build DX-Stream GStreamer plugin
    echo "Starting build process... build_type(${BUILD_TYPE})"
    echo "  PREFIX: ${PREFIX}"
    cd gst-dxstream-plugin
    meson setup ${BUILD_DIR} --prefix=${PREFIX} --buildtype=${BUILD_TYPE} ${V3_OPTION} ${COVERAGE_OPTION}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson setup failed"
        exit 1
    fi
    meson compile -C ${BUILD_DIR}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson compile failed"
        exit 1
    fi
    
    # Install with sudo (needed for PREFIX-based paths)
    echo "Installing DX-Stream plugin to system directories..."
    echo "  - Headers: ${PREFIX}/include/gstdxstream"
    sudo env PYTHONPATH="$(python3 -c 'import site; print(site.getusersitepackages() + ":" + ":".join(site.getsitepackages()))')" "$(which meson)" install -C "${BUILD_DIR}" --no-rebuild
    if [ $? -ne 0 ]; then
        echo -e "Error: meson install failed"
        echo -e "Hint: Run 'which -a meson' to check if multiple meson versions are installed."
        echo -e "      If so, keep only one and remove the rest. (See: docs/source/docs/06_Troubleshooting_and_FAQ.md)"
        exit 1
    fi
    
    # Update library cache
    echo "Updating library cache..."
    sudo ldconfig

    # Note: pkg-config file is now generated by Meson (see gst-dxstream-plugin/src/meson.build)
    echo "✓ pkg-config file generated by Meson"

    # Update GStreamer plugin cache
    echo "Updating GStreamer plugin cache..."
    rm -rf ~/.cache/gstreamer-1.0/ 2>/dev/null || true

    # Get actual libdir for runtime paths
    if pkg-config --exists gstdxstream 2>/dev/null; then
        ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
    else
        ACTUAL_LIBDIR=$(find "${PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs dirname)
        if [ -z "$ACTUAL_LIBDIR" ]; then
            ACTUAL_LIBDIR="${PREFIX}/lib"
        fi
    fi

    # Automatically configure environment variables in bashrc
    setup_bashrc_env "$PREFIX" "$ACTUAL_LIBDIR"

    # Build DX-Stream custom libraries
    cd $WRC/dx_stream/custom_library
    ./build.sh --prefix=${PREFIX} ${TYPE_ARG} ${SONAR_MODE_ARG} ${V3_MODE} ${CLEAN_MODE}
    if [ $? -ne 0 ]; then
        echo -e "Error: custom_library build failed"
        exit 1
    fi

    if [ "$V3_MODE" == "--v3" ]; then
        echo "Skipping Application build in V3 mode."
        echo "Skipping test_plugin build in V3 mode."
        echo "Skipping pydxs installation in V3 mode."
        return 0
    fi

    # Build DX-Stream applications
    cd $WRC/dx_stream/apps
    ./build.sh --prefix=${PREFIX} ${TYPE_ARG} ${SONAR_MODE_ARG} ${CLEAN_MODE}
    if [ $? -ne 0 ]; then
        echo -e "Error: apps build failed"
        exit 1
    fi

    # Install Python bindings (pydxs)
    install_pydxs

    # Merge compile_commands.json files for SonarLint (only if sonar mode is enabled)
    if [ -n "${SONAR_MODE_ARG}" ]; then
        echo "Merging compile_commands.json files for SonarLint..."
        cd $WRC
        python3 scripts/merge_compile_commands.py
        if [ $? -eq 0 ]; then
            echo "Successfully merged compile_commands.json files"
        else
            echo "Warning: Failed to merge compile_commands.json files"
        fi
    fi

    echo "Build completed."

    # Show post-installation instructions
    echo ""
    echo "=========================================="
    echo "✅ Installation Complete!"
    echo "=========================================="
    echo "📦 Installed to: ${PREFIX}"
    echo ""

    if [ $? -eq 0 ]; then
        echo "✓ Environment variables have been added to ~/.bashrc"
        echo ""
        echo "To apply the changes:"
        echo "  • Open a new terminal, or"
        echo "  • Run: source ~/.bashrc"
        echo ""
        echo "For immediate use in this terminal:"
        echo "  $ export PKG_CONFIG_PATH=\"${PREFIX}/lib/pkgconfig:\${PKG_CONFIG_PATH}\""
        echo "  $ export GST_PLUGIN_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:\${GST_PLUGIN_PATH}\""
        echo "  $ export LD_LIBRARY_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:${PREFIX}/share/gstdxstream/lib:\${LD_LIBRARY_PATH}\""
        echo "  $ export PATH=\"${PREFIX}/share/gstdxstream/bin:\${PATH}\""
        echo ""
        echo "Verify installation:"
        echo "  $ gst-inspect-1.0 dxstream"
    else
        echo "⚠️  Could not automatically configure ~/.bashrc"
        echo ""
        show_manual_setup_guide "$PREFIX" "$ACTUAL_LIBDIR"
    fi
    
    echo "=========================================="
    echo ""
}

# Function to show manual setup guide
show_manual_setup_guide() {
    local PREFIX="$1"
    local ACTUAL_LIBDIR="$2"
    
    echo ""
    echo "Manual Setup Instructions:"
    echo ""
    echo "To use DX-Stream, set these environment variables:"
    echo ""
    echo "  # For building projects that depend on gstdxstream:"
    echo "  export PKG_CONFIG_PATH=\"${PREFIX}/lib/pkgconfig:\${PKG_CONFIG_PATH}\""
    echo ""
    echo "  # For running GStreamer pipelines:"
    echo "  export GST_PLUGIN_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:\${GST_PLUGIN_PATH}\""
    echo "  export LD_LIBRARY_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:${PREFIX}/share/gstdxstream/lib:\${LD_LIBRARY_PATH}\""
    echo "  export PATH=\"${PREFIX}/share/gstdxstream/bin:\${PATH}\""
    echo ""
    echo "💡 Tip: Add these to your ~/.bashrc for persistence:"
    echo ""
    echo "  cat >> ~/.bashrc <<'EOF'"
    echo "# DeepX Stream GStreamer Plugin"
    echo "export PKG_CONFIG_PATH=\"${PREFIX}/lib/pkgconfig:\${PKG_CONFIG_PATH}\""
    echo "export GST_PLUGIN_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:\${GST_PLUGIN_PATH}\""
    echo "export LD_LIBRARY_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:${PREFIX}/share/gstdxstream/lib:\${LD_LIBRARY_PATH}\""
    echo "export PATH=\"${PREFIX}/share/gstdxstream/bin:\${PATH}\""
    echo "# DeepX Stream GStreamer Plugin - END"
    echo "EOF"
    echo ""
    echo "Verify installation:"
    echo "  $ export GST_PLUGIN_PATH=\"${ACTUAL_LIBDIR}/gstreamer-1.0:\${GST_PLUGIN_PATH}\""
    echo "  $ gst-inspect-1.0 dxstream"
}

# Function to setup environment variables in bashrc
setup_bashrc_env() {
    local PREFIX="$1"
    local ACTUAL_LIBDIR="$2"
    local BASHRC="$HOME/.bashrc"
    local TEMP_FILE=$(mktemp)
    local START_MARKER="# DeepX Stream GStreamer Plugin"
    local END_MARKER="# DeepX Stream GStreamer Plugin - END"
    
    # Check write permission
    if [ ! -w "$BASHRC" ]; then
        if [ ! -f "$BASHRC" ]; then
            # File doesn't exist, try to create it
            if ! touch "$BASHRC" 2>/dev/null; then
                echo "Error: Cannot create $BASHRC (permission denied)"
                rm -f "$TEMP_FILE"
                return 1
            fi
        else
            echo "Error: No write permission for $BASHRC"
            rm -f "$TEMP_FILE"
            return 1
        fi
    fi
    
    # Remove existing DX-Stream configuration block if present
    if grep -q "^$START_MARKER" "$BASHRC" 2>/dev/null; then
        echo "→ Removing existing DX-Stream configuration..."
        # Use sed to delete lines between markers (inclusive)
        sed -i "/^$START_MARKER/,/^$END_MARKER/d" "$BASHRC"
        if [ $? -ne 0 ]; then
            echo "Error: Failed to remove existing configuration from $BASHRC"
            rm -f "$TEMP_FILE"
            return 1
        fi
    fi
    
    # Prepare new configuration block in temp file
    cat > "$TEMP_FILE" <<EOF
$START_MARKER
export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig:\${PKG_CONFIG_PATH}"
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:\${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${PREFIX}/share/gstdxstream/lib:\${LD_LIBRARY_PATH}"
export PATH="${PREFIX}/share/gstdxstream/bin:\${PATH}"
$END_MARKER
EOF
    
    if [ $? -ne 0 ]; then
        echo "Error: Failed to create temporary configuration file"
        rm -f "$TEMP_FILE"
        return 1
    fi
    
    # Atomically append to bashrc
    if cat "$TEMP_FILE" >> "$BASHRC"; then
        rm -f "$TEMP_FILE"
        return 0
    else
        echo "Error: Failed to write to $BASHRC"
        rm -f "$TEMP_FILE"
        return 1
    fi
}

activate_venv() {
    # activate venv
    source ${VENV_PATH}/bin/activate
    if [ $? -ne 0 ]; then
        echo -e "[ERROR] Failed to activate venv. Please run 'install.sh' first to set up the venv environment, then try again."
        exit 1
    fi
}

install_pydxs() {
    echo ""
    echo "=================================================="
    echo "🐍 Python Bindings Installation (pydxs)"
    echo "=================================================="
    
    deactivate 2>/dev/null || true

    local python_version=$(/usr/bin/python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
    echo "→ Python version: ${python_version}"
    echo "→ Build type: ${BUILD_TYPE}"
    echo ""
    
    # Create/activate virtual environment
    if [ ! -d "${VENV_PATH}" ]; then
        echo "→ Creating virtual environment: ${VENV_PATH}"
        /usr/bin/python3 -m venv --system-site-packages "${VENV_PATH}" || exit 1
    fi

    echo "→ Activating virtual environment"
    source "${VENV_PATH}/bin/activate" || exit 1

    # Determine pip flags based on Python version
    local py_minor=$(echo $python_version | cut -d. -f2)
    local pip_flags="-q"
    [ "$py_minor" -ge 11 ] && pip_flags="--break-system-packages -q"

    # Install build dependencies
    echo "→ Installing dependencies..."
    if [ "$py_minor" -le 7 ]; then
        # Python 3.6-3.7: Use compatible versions
        python3 -m pip install $pip_flags --upgrade "pip<21.0" "setuptools<60.0" "wheel<0.38" "pybind11>=2.6.0,<2.11.0"
    else
        # Python 3.8+: Use latest versions
        python3 -m pip install $pip_flags --upgrade pip setuptools wheel pybind11
    fi
    
    [ $? -ne 0 ] && { echo "✗ Dependency installation failed"; deactivate; exit 1; }

    # Install pydxs
    cd "${PROJECT_ROOT}/bindings/python/pydxs"
    echo "→ Installing pydxs..."
    
    export PROJECT_ROOT="${PROJECT_ROOT}"
    
    if python3 -m pip install $pip_flags .; then
        echo "✓ pydxs installed successfully"
        python3 -c "import pydxs" 2>/dev/null && echo "✓ Import verified" || { echo "✗ Import failed"; cd "${PROJECT_ROOT}"; deactivate; exit 1; }
    else
        echo "✗ Installation failed"
        echo "→ Try: source ${VENV_PATH}/bin/activate && cd ${PROJECT_ROOT}/bindings/python/pydxs && python3 -m pip install -v ."
        cd "${PROJECT_ROOT}"
        deactivate
        exit 1
    fi
    
    cd "${PROJECT_ROOT}"
    deactivate
    echo "=================================================="
    echo ""
}

# Parse arguments
for i in "$@"; do
    case "$1" in
        --prefix=*)
            PREFIX="${1#*=}"
            # Convert relative path to absolute path IMMEDIATELY
            if [[ ! "$PREFIX" = /* ]]; then
                ORIG_PREFIX="$PREFIX"
                # Remove leading ./
                PREFIX="$(echo "$PREFIX" | sed 's|^\./||')"
                # Get absolute path
                PREFIX="$(cd "$(dirname "$PREFIX")" 2>/dev/null && pwd -P)/$(basename "$PREFIX")"
                if [ $? -ne 0 ]; then
                    # Directory doesn't exist yet, construct absolute path from current dir
                    PREFIX="$(pwd -P)/$PREFIX"
                fi
            fi
            # Resolve any remaining relative components
            PREFIX="$(cd "$(dirname "$PREFIX")" 2>/dev/null && pwd -P)/$(basename "$PREFIX")" 2>/dev/null || PREFIX="$(pwd -P)/$(basename "$PREFIX")"
            
            # Final check: ensure it's absolute
            if [[ ! "$PREFIX" = /* ]]; then
                echo "Error: Failed to convert PREFIX to absolute path: $PREFIX"
                exit 1
            fi
            echo "Using custom PREFIX: ${PREFIX}"
            ;;
        --install)
            # nothing to do (for legacy option backward compatibility)
            ;;
        --clean)
            CLEAN_MODE="--clean"
            ;;
        --uninstall)
            UNINSTALL_MODE="--uninstall"
            ;;
        --type=*)
            BUILD_TYPE_INPUT="${1#*=}"
            # Convert to lowercase for validation
            BUILD_TYPE_LOWER=$(echo "$BUILD_TYPE_INPUT" | tr '[:upper:]' '[:lower:]')
            
            # Validate: only 'debug' or 'release' allowed
            if [[ "$BUILD_TYPE_LOWER" != "debug" && "$BUILD_TYPE_LOWER" != "release" ]]; then
                echo "Error: Invalid build type '$BUILD_TYPE_INPUT'"
                echo "Allowed values: Debug, debug, Release, release"
                exit 1
            fi
            
            # Use lowercase for meson
            BUILD_TYPE="$BUILD_TYPE_LOWER"
            TYPE_ARG="--type=$BUILD_TYPE_INPUT"
            ;;
        --sonar)
            SONAR_MODE_ARG="--sonar"
            ;;
        --v3)
            V3_MODE="--v3"
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help "error"
            ;;
    esac
    shift
done

# Handle uninstall mode after all arguments are parsed
if [ "$UNINSTALL_MODE" == "--uninstall" ]; then
    clean "uninstall"
    exit 0
fi

build

exit 0
