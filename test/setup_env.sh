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

    for backend_dir in "${PROJECT_ROOT}/gst-dxstream-plugin/builddir"/src-{v3,vnpu,rga,gles}; do
        if [ -f "${backend_dir}/libgstdxstream-${backend_dir##*-}.so" ]; then
            export LD_LIBRARY_PATH="${backend_dir}:${LD_LIBRARY_PATH}"
        fi
    done
fi

# Memory-checked tests require valgrind; install it via apt (sudo may prompt for a password).
ensure_valgrind() {
    command -v valgrind >/dev/null 2>&1 && return 0
    if ! command -v apt-get >/dev/null 2>&1; then
        echo "  [SETUP] valgrind not found and apt-get unavailable: install valgrind manually"
        return 1
    fi
    local sudo=""
    [ "$(id -u)" -ne 0 ] && sudo="sudo"
    echo "  [SETUP] Installing valgrind ..."
    # CI images often ship without apt lists, so retry after an update.
    $sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq valgrind >/dev/null ||
        { $sudo apt-get update -qq >/dev/null &&
          $sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y -qq valgrind >/dev/null; }
    if ! command -v valgrind >/dev/null 2>&1; then
        echo "  [SETUP] valgrind installation failed"
        return 1
    fi
}

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
