#!/usr/bin/env bash
# VNPU hardware test suite
set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
export DXVNPU_TEST_DEVICE=1

if ! gst-inspect-1.0 dxvnpudec >/dev/null 2>&1; then
    echo "===== Test Group: vnpu ====="
    echo "  [SKIP] gst-inspect-1.0 dxvnpudec not found"
    echo "  ── summary: PASS=0 FAIL=0 SKIP=1 ──"
    echo
    exit 77
fi

PASS=0; FAIL=0; SKIP=0

echo "===== Test Group: vnpu ====="
for subdir in "$SCRIPT_DIR/dxvnpu/element" "$SCRIPT_DIR/dxvnpu/pipeline"; do
    [ -d "$subdir" ] || continue
    while IFS= read -r src; do
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
    done < <(find "$subdir" -name 'test_*.cpp' -type f | sort)
done
if [ "$((PASS+FAIL+SKIP))" -eq 0 ]; then
    echo "  [FAIL] No VNPU test sources found although the plugin is available"
    FAIL=1
fi
echo "  ── summary: PASS=$PASS FAIL=$FAIL SKIP=$SKIP ──"
echo

[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
