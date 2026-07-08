#!/usr/bin/env bash
# Common test environment setup. Source this from runner scripts.
# Sets PKG_CONFIG_PATH / GST_PLUGIN_PATH / LD_LIBRARY_PATH for the installed
# dxstream plugin so test binaries can find it.

_SETUP_ENV_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"
PROJECT_ROOT="$(cd "${_SETUP_ENV_DIR}/.." && pwd)"
LOCAL_PLUGIN_DIR="${PROJECT_ROOT}/gst-dxstream-plugin/builddir/src"

if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR=$(find "${INSTALL_PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs -r dirname)
    [ -z "$ACTUAL_LIBDIR" ] && ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi

export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${GST_PLUGIN_PATH:-}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:${LD_LIBRARY_PATH:-}"

if [ -f "${LOCAL_PLUGIN_DIR}/libgstdxstream.so" ]; then
    export GST_PLUGIN_PATH="${LOCAL_PLUGIN_DIR}:${GST_PLUGIN_PATH}"
    export LD_LIBRARY_PATH="${LOCAL_PLUGIN_DIR}:${LD_LIBRARY_PATH}"
fi

# Check dx-rt runtime availability (call once, result cached in DXRT_AVAILABLE)
check_dxrt_available() {
    if [ -n "${DXRT_AVAILABLE:-}" ]; then return; fi
    export DXRT_AVAILABLE=0
    local model="${PROJECT_ROOT}/dx_stream/samples/models/yolov5-s_640x640_ppu.dxnn"
    if command -v run_model >/dev/null 2>&1 && [ -f "$model" ]; then
        echo "  [CHECK] Verifying dx-rt runtime ..."
        local exit_code run_output
        run_output=$(timeout 10 run_model -m "$model" -l 100 2>&1); exit_code=$?
        if [ $exit_code -eq 0 ]; then
            echo "  [CHECK] dx-rt runtime: OK"
            export DXRT_AVAILABLE=1
        else
            echo "  [CHECK] dx-rt runtime: NOT AVAILABLE (exit code: $exit_code)"
            if [ -n "$run_output" ]; then
                echo "$run_output" | sed 's/^/    /'
            fi
        fi
    else
        echo "  [CHECK] run_model or model not found"
    fi
}
