#!/bin/bash

# DX-Stream User Metadata Python Test Runner

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")
VENV_PATH="${PROJECT_ROOT}/venv-dx_stream"

set -e

# Setup environment for DX-Stream
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

# Find the actual libdir
if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR=$(find "${INSTALL_PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs dirname)
    [ -z "$ACTUAL_LIBDIR" ] && ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi

export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:${LD_LIBRARY_PATH}"
export PATH="${INSTALL_PREFIX}/share/gstdxstream/bin:${PATH}"

echo "✓ Environment configured for DX-Stream"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=================================================="
echo "DX-Stream User Metadata Test (Python)"
echo "=================================================="

echo "Test configuration:"
echo "  Mode: videotestsrc (no external video needed)"
echo "  Frames: 50"
echo ""

echo "=================================================="
echo "Environment Diagnostics"
echo "=================================================="

# Check environment variables
echo "1. Checking environment variables:"
echo "  PKG_CONFIG_PATH: ${PKG_CONFIG_PATH:-<not set>}"
echo "  GST_PLUGIN_PATH: ${GST_PLUGIN_PATH:-<not set>}"
echo "  LD_LIBRARY_PATH: ${LD_LIBRARY_PATH:-<not set>}"
echo ""
echo ""

# Check environment variables
echo "2. Environment variables:"
echo "  GI_TYPELIB_PATH: ${GI_TYPELIB_PATH:-[NOT SET]}"
if [ "$GSTREAMER_PROFILE_SOURCED" = true ]; then
    echo "    (loaded from /etc/profile.d/gstreamer.sh)"
fi
echo "  LD_LIBRARY_PATH: ${LD_LIBRARY_PATH:-[NOT SET]}"
echo "  GST_PLUGIN_PATH: ${GST_PLUGIN_PATH:-[NOT SET]}"
echo "  PKG_CONFIG_PATH: ${PKG_CONFIG_PATH:-[NOT SET]}"
echo "  PYTHONPATH: ${PYTHONPATH:-[NOT SET]}"
echo ""

# Check venv configuration
echo "3. Virtual environment:"
if [ -d "${VENV_PATH}" ]; then
    echo "  ✓ venv exists: ${VENV_PATH}"
    if [ -f "${VENV_PATH}/pyvenv.cfg" ]; then
        INCLUDE_SYSTEM=$(grep "include-system-site-packages" "${VENV_PATH}/pyvenv.cfg" 2>/dev/null)
        echo "  Config: ${INCLUDE_SYSTEM}"
    fi
else
    echo "  ✗ venv NOT FOUND: ${VENV_PATH}"
fi
echo ""

# Check Python and gi module BEFORE activating venv
echo "4. System Python check:"
echo "  Python: $(/usr/bin/python3 --version)"
echo "  Testing gi module (system)..."
if /usr/bin/python3 -c "import gi; print('  [OK] gi module available (system)');" 2>/dev/null; then
    /usr/bin/python3 -c "import gi; print('    gi location:', gi.__file__)"
else
    echo "  ✗ gi module NOT available (system)"
fi
echo ""

echo "=================================================="
echo ""

# Setup GI_TYPELIB_PATH for source-built GStreamer (Ubuntu 18.04 compatibility)
echo "Configuring GStreamer typelib paths..."
GSTREAMER_PROFILE_SOURCED=false

# First, try to source the GStreamer profile script if it exists
if [ -f /etc/profile.d/gstreamer.sh ]; then
    echo "  Found /etc/profile.d/gstreamer.sh, sourcing..."
    source /etc/profile.d/gstreamer.sh
    GSTREAMER_PROFILE_SOURCED=true
    echo "  ✓ GStreamer environment variables loaded from profile"
fi

# If GI_TYPELIB_PATH is still not set, auto-detect typelib location
if [ -z "$GI_TYPELIB_PATH" ]; then
    echo "  GI_TYPELIB_PATH not set, attempting auto-detection..."
    for typelib_path in /usr/local/lib/*/girepository-1.0 /usr/local/lib/girepository-1.0 /usr/lib/*/girepository-1.0 /usr/lib/girepository-1.0; do
        if [ -d "$typelib_path" ] && [ -f "$typelib_path/Gst-1.0.typelib" ]; then
            export GI_TYPELIB_PATH="$typelib_path:${GI_TYPELIB_PATH}"
            echo "  ✓ Found GStreamer typelibs at: $typelib_path"
            echo "  ✓ GI_TYPELIB_PATH=$GI_TYPELIB_PATH"
            break
        fi
    done
    
    if [ -z "$GI_TYPELIB_PATH" ]; then
        echo "  ⚠ Warning: Could not auto-detect GStreamer typelib path"
    fi
else
    echo "  ✓ GI_TYPELIB_PATH already set: $GI_TYPELIB_PATH"
fi

echo ""

if [ -d "${VENV_PATH}" ]; then
    echo "Activating virtual environment: ${VENV_PATH}"
    source "${VENV_PATH}/bin/activate"
    if [ $? -ne 0 ]; then
        echo "[WARN] Failed to activate 'venv-dx_stream'. Using current Python environment."
    fi
else
    echo "[WARN] 'venv-dx_stream' not found under project root. Using current Python environment."
fi

# Check Python in venv
echo ""
echo "=================================================="
echo "Virtual Environment Check"
echo "=================================================="
echo "5. Python in venv:"
echo "  Python: $(python3 --version)"
echo "  Python path: $(which python3)"
echo ""

echo "6. Testing gi module in venv..."
if python3 -c "import gi; print('  [OK] gi module available'); print('    gi location:', gi.__file__)" 2>&1; then
    echo ""
    echo "7. Testing Gst import..."
    if python3 -c "import gi; gi.require_version('Gst', '1.0'); from gi.repository import Gst; Gst.init(None); print('  [OK] Gst module available'); print('    Gst version:', Gst.version_string())" 2>&1; then
        echo ""
    else
        echo "  [X] Gst import FAILED!"
        echo ""
        echo "Additional diagnostics:"
        python3 -c "import gi; import os; print('  GI_TYPELIB_PATH:', os.environ.get('GI_TYPELIB_PATH', 'NOT SET'))" 2>&1 || true
        echo ""
        echo -e "${RED}[ERROR] GStreamer Python bindings not available. Cannot proceed.${NC}"
        exit 1
    fi
else
    echo "  [X] gi module NOT available in venv!"
    echo ""
    echo "  Checking site-packages access..."
    python3 -c "import sys; print('  sys.path:'); [print('    -', p) for p in sys.path]"
    echo ""
    echo -e "${RED}[ERROR] gi module not available. Cannot proceed.${NC}"
    exit 1
fi

echo "=================================================="
echo ""

# Run test
echo "Running Python test..."
echo ""
python3 test_usermeta.py

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}✓ Test PASSED${NC}"
else
    echo -e "${RED}✗ Test FAILED (exit code: $EXIT_CODE)${NC}"
fi

exit $EXIT_CODE
