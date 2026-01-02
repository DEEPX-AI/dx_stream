#!/bin/bash

# DX-Stream User Metadata Python Test Runner

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")
VENV_PATH="${PROJECT_ROOT}/venv-dx_stream"

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

if [ -f "/etc/profile.d/gstreamer.sh" ]; then
    echo "Sourcing GStreamer environment..."
    source /etc/profile.d/gstreamer.sh
fi

echo "=================================================="
echo "DX-Stream User Metadata Test (Python)"
echo "=================================================="

echo "Test configuration:"
echo "  Mode: videotestsrc (no external video needed)"
echo "  Frames: 50"
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
