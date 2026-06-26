#!/bin/bash
# P7 pydxs Python binding tests runner
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
VENV_PATH="${PROJECT_ROOT}/venv-dx_stream"

# Environment setup
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi

export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:${LD_LIBRARY_PATH}"

# GI_TYPELIB_PATH
if [ -z "$GI_TYPELIB_PATH" ]; then
    for p in /usr/local/lib/*/girepository-1.0 /usr/local/lib/girepository-1.0; do
        if [ -d "$p" ] && [ -f "$p/Gst-1.0.typelib" ]; then
            export GI_TYPELIB_PATH="$p:${GI_TYPELIB_PATH}"
            break
        fi
    done
fi

# Activate venv
if [ -d "${VENV_PATH}" ]; then
    source "${VENV_PATH}/bin/activate"
else
    echo "[WARN] venv not found: ${VENV_PATH}"
fi

# Run tests
exec python3 "${SCRIPT_DIR}/test_pydxs.py"
