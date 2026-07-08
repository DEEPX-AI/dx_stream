#!/usr/bin/env bash
# pydxs Python binding test suite
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"

RUNNER="$SCRIPT_DIR/base/pydxs/run_pydxs_tests.sh"
if [ ! -x "$RUNNER" ]; then
    echo "===== Test Group: pydxs ====="
    echo "  [SKIP] run_pydxs_tests.sh not found"
    echo "  ── summary: PASS=0 FAIL=0 (skipped) ──"
    echo
    exit 0
fi

LOG_DIR="$SCRIPT_DIR/_logs"
mkdir -p "$LOG_DIR"

RESULT=0
echo "===== Test Group: pydxs ====="
if "$RUNNER" > "$LOG_DIR/pydxs.log" 2>&1; then
    echo "  [PASS] pydxs"
    echo "  ── summary: PASS=1 FAIL=0 ──"
else
    echo "  [FAIL] pydxs"
    if [ -s "$LOG_DIR/pydxs.log" ]; then
        echo "  ---- pydxs.log ----"
        sed 's/^/    /' "$LOG_DIR/pydxs.log" | tail -50
    fi
    echo "  ── summary: PASS=0 FAIL=1 ──"
    RESULT=1
fi
echo

exit $RESULT
