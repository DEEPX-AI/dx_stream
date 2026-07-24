#!/usr/bin/env bash
# pipeline test suite
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
PASS=0; FAIL=0

check_dxrt_available

if [ "$DXRT_AVAILABLE" -eq 0 ]; then
    echo "===== Test Group: pipeline ====="
    echo "  [FAIL] dx-rt unavailable — all pipeline tests failed"
    echo "  ── summary: PASS=0 FAIL=all ──"
    echo
    exit 1
fi

echo "===== Test Group: pipeline ====="
while IFS= read -r src; do
    rel="${src#$SCRIPT_DIR/}"
    name="$(basename "$src" .cpp)"
    if "$SCRIPT_DIR/run_test.sh" "$rel"; then
        echo "  [PASS] $name"
        PASS=$((PASS+1))
    else
        echo "  [FAIL] $name"
        for log in "$SCRIPT_DIR/_logs/$name.log.build" "$SCRIPT_DIR/_logs/$name.log"; do
            if [ -s "$log" ]; then
                echo "  ---- $(basename "$log") ----"
                sed 's/^/    /' "$log" | tail -50
            fi
        done
        FAIL=$((FAIL+1))
    fi
done < <(find "$SCRIPT_DIR/base/pipeline" -name 'test_*.cpp' -type f | sort)
echo "  ── summary: PASS=$PASS FAIL=$FAIL ──"
echo

[ "$FAIL" -eq 0 ]
