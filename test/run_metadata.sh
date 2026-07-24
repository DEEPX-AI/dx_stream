#!/usr/bin/env bash
# metadata test suite
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
PASS=0; FAIL=0

echo "===== Test Group: metadata ====="
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
done < <(find "$SCRIPT_DIR/base/metadata" -name 'test_*.cpp' -type f | sort)
echo "  ── summary: PASS=$PASS FAIL=$FAIL ──"
echo

[ "$FAIL" -eq 0 ]
