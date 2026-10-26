#!/usr/bin/env bash
# element test suite
set -uo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
PASS=0; FAIL=0; SKIP=0

check_dxrt_available
ensure_valgrind || true

echo "===== Test Group: element ====="
if ! sources=$(find "$SCRIPT_DIR/base/element" -name 'test_*.cpp' -type f | sort); then
    echo "  [FAIL] Cannot enumerate element test sources"
    exit 1
fi
while IFS= read -r src; do
    [ -n "$src" ] || continue
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
        # Known heap-safety regression (stream-info stride/size mismatch OOB
        # read): the plain run above doesn't reliably SIGSEGV — it depends on
        # heap layout/ASLR — so also require a clean valgrind pass here.
        if [ "$name" = "test_dxpreprocess_wrapped_caps" ]; then
            if ! command -v valgrind >/dev/null 2>&1; then
                echo "  [FAIL] $name (valgrind not installed: memory check required)"
                FAIL=$((FAIL+1))
            elif VALGRIND=1 "$SCRIPT_DIR/run_test.sh" "$rel" > /dev/null 2>&1; then
                PASS=$((PASS+1))
            else
                echo "  [FAIL] $name (valgrind detected invalid memory access)"
                sed 's/^/    /' "$SCRIPT_DIR/_logs/$name.log" | tail -50
                FAIL=$((FAIL+1))
            fi
        else
            PASS=$((PASS+1))
        fi
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
    echo "  [FAIL] No element test sources found"
    FAIL=1
fi
echo "  ── summary: PASS=$PASS FAIL=$FAIL SKIP=$SKIP ──"
echo

[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
