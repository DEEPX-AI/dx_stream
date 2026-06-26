#!/usr/bin/env bash
# element test suite
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
PASS=0; FAIL=0

check_dxrt_available

echo "===== Test Group: element ====="
while IFS= read -r src; do
    rel="${src#$SCRIPT_DIR/}"
    name="$(basename "$src" .cpp)"

    # Fail inference tests if dx-rt is not available
    if [ "$DXRT_AVAILABLE" -eq 0 ] && [[ "$src" == */inference/* ]]; then
        echo "  [FAIL] $name (dx-rt unavailable)"
        FAIL=$((FAIL+1))
        continue
    fi

    if "$SCRIPT_DIR/run_test.sh" "$rel"; then
        echo "  [PASS] $name"
        if [ -s "$SCRIPT_DIR/_logs/$name.log" ]; then
            grep '\[DIAG\]\|\[INFO\]' "$SCRIPT_DIR/_logs/$name.log" | sed 's/^/    /' || true
        fi
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
done < <(find "$SCRIPT_DIR/base/element" -name 'test_*.cpp' -type f | sort)
echo "  ── summary: PASS=$PASS FAIL=$FAIL ──"
echo

[ "$FAIL" -eq 0 ]
