#!/usr/bin/env bash
# Single test compile + run
#
# Usage:
#   ./run_test.sh base/metadata/test_dxframemeta.cpp   # C++ test
#   ./run_test.sh pydxs                                 # Python binding test
#
# Exit code: 0=PASS, 1=FAIL

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
export DX_STREAM_ROOT="${DX_STREAM_ROOT:-$PROJECT_ROOT}"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"

BUILD_DIR="$SCRIPT_DIR/_bin"
LOG_DIR="$SCRIPT_DIR/_logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <test_source.cpp | pydxs>"
    exit 1
fi

TARGET="$1"

# ---- pydxs special handling ----
if [ "$TARGET" = "pydxs" ]; then
    RUNNER="$SCRIPT_DIR/base/pydxs/run_pydxs_tests.sh"
    if [ ! -x "$RUNNER" ]; then
        echo "[SKIP] pydxs (run_pydxs_tests.sh not found)"
        exit 0
    fi
    echo "===== [PYDXS] base/pydxs"
    if "$RUNNER" > "$LOG_DIR/pydxs.log" 2>&1; then
        echo "[PASS] pydxs"
        exit 0
    else
        echo "[FAIL] pydxs (see $LOG_DIR/pydxs.log)"
        exit 1
    fi
fi

# ---- C++ test ----
SRC="$SCRIPT_DIR/$TARGET"
if [ ! -f "$SRC" ]; then
    SRC="$TARGET"
fi
if [ ! -f "$SRC" ]; then
    echo "[ERROR] Source file not found: $TARGET"
    exit 1
fi

if ! pkg-config --exists gstdxstream; then
    echo "[FATAL] gstdxstream pkg-config not found. Run './build.sh' first."
    exit 2
fi

CFLAGS="-g -O0 -std=c++14 -Wall -Wno-unused-parameter -fno-strict-aliasing"
CFLAGS+=" -I$SCRIPT_DIR/common"
CFLAGS+=" -I$PROJECT_ROOT/gst-dxstream-plugin/src"
CFLAGS+=" -I$PROJECT_ROOT/gst-dxstream-plugin/metadata"
CFLAGS+=" -I$PROJECT_ROOT/gst-dxstream-plugin/general"
CFLAGS+=" $(pkg-config --cflags gstdxstream gstreamer-check-1.0 gstreamer-app-1.0 gstreamer-video-1.0)"

LIBS="$(pkg-config --libs gstdxstream gstreamer-check-1.0 gstreamer-app-1.0 gstreamer-video-1.0) -lpthread"

NAME="$(basename "$SRC" .cpp)"
BIN="$BUILD_DIR/$NAME"
LOG="$LOG_DIR/$NAME.log"

if ! g++ $CFLAGS "$SRC" $LIBS -o "$BIN" 2> "$LOG.build"; then
    exit 1
fi

if "$BIN" > "$LOG" 2>&1; then
    exit 0
else
    exit 1
fi
