#!/usr/bin/env bash
# metadata test suite
set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
PASS=0; FAIL=0; SKIP=0

echo "===== Test Group: metadata ====="
if ! sources=$(find "$SCRIPT_DIR/base/metadata" -name 'test_*.cpp' -type f | sort); then
    echo "  [FAIL] Cannot enumerate metadata test sources"
    exit 1
fi
while IFS= read -r src; do
    [ -n "$src" ] || continue
    rel="${src#$SCRIPT_DIR/}"
    name="$(basename "$src" .cpp)"
    if "$SCRIPT_DIR/run_test.sh" "$rel"; then
        echo "  [PASS] $name"
        PASS=$((PASS+1))
    else
        status=$?
        if [ "$status" -eq 77 ]; then
            echo "  [SKIP] $name (contains optional skipped checks)"
            SKIP=$((SKIP+1))
            continue
        fi
        echo "  [FAIL] $name"
        for log in "$SCRIPT_DIR/_logs/$name.log.build" "$SCRIPT_DIR/_logs/$name.log"; do
            if [ -s "$log" ]; then
                echo "  ---- $(basename "$log") ----"
                sed 's/^/    /' "$log" | tail -50
            fi
        done
        FAIL=$((FAIL+1))
    fi
done <<< "$sources"
if [ "$((PASS+FAIL+SKIP))" -eq 0 ]; then
    echo "  [FAIL] No metadata test sources found"
    FAIL=1
fi
echo "  ── summary: PASS=$PASS FAIL=$FAIL SKIP=$SKIP ──"
echo

[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
