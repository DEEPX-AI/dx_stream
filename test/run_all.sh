#!/usr/bin/env bash
# test_new full runner — calls group scripts sequentially, stops immediately on failure.
#
# Usage:
#   ./run_all.sh               # run all groups
#   FAIL_FAST=1 ./run_all.sh   # stop on first failure within a group
#
# Exit code: 0=all PASS, 1=any FAIL

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
export FAIL_FAST="${FAIL_FAST:-0}"

TEST_GROUPS=("metadata" "element" "pipeline" "pydxs" "vnpu")
FAILED_GROUPS=()

for group in "${TEST_GROUPS[@]}"; do
    if ! "$SCRIPT_DIR/run_${group}.sh"; then
        FAILED_GROUPS+=("$group")
        echo
        echo "[STOP] $group failed — skipping remaining groups"
        break
    fi
done

echo
echo "============================================================"
if [ ${#FAILED_GROUPS[@]} -eq 0 ]; then
    echo " test_new: ALL GROUPS PASSED"
else
    echo " test_new: FAILED groups: ${FAILED_GROUPS[*]}"
fi
echo "============================================================"

[ ${#FAILED_GROUPS[@]} -eq 0 ]
