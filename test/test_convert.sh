#!/bin/bash
# =============================================================================
# test_convert.sh — Automated dxconvert visual pipeline tests (interactive)
#
# Tests all src_format → dst_format combinations via GStreamer pipeline.
# After each test, user judges pass(p) or fail(f) visually.
# Shows backend selection (libyuv/rga) from dxconvert GST_INFO logs.
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VIDEO_URI="file://${SCRIPT_DIR}/../dx_stream/samples/videos/dogs.mp4"
TIMEOUT_SEC=30

export GST_DEBUG="dxconvert:4,dxt_libyuv:4,dxt_rga:4"

FORMATS=("NV12" "I420" "RGB" "BGR")

PASS=0
FAIL=0
ERROR=0
FAILED_CASES=()
ERROR_CASES=()
TOTAL=0

echo "============================================================"
echo " dxconvert Visual Pipeline Test (interactive)"
echo " Video: ${VIDEO_URI}"
echo " After each test: p=pass, f=fail, s=skip"
echo "============================================================"

for SRC_FMT in "${FORMATS[@]}"; do
    for DST_FMT in "${FORMATS[@]}"; do
        TOTAL=$((TOTAL + 1))

        LABEL="${SRC_FMT} → ${DST_FMT}"
        echo ""
        echo "===  [${TOTAL}] ${LABEL}  ==="

        OUTPUT=$(timeout ${TIMEOUT_SEC} gst-launch-1.0 \
            urisourcebin uri="${VIDEO_URI}" ! decodebin ! \
            videoconvert ! "video/x-raw,format=${SRC_FMT}" ! \
            dxconvert ! \
            "video/x-raw,format=${DST_FMT}" ! \
            videoconvert ! fpsdisplaysink sync=false 2>&1)

        RC=$?

        BACKEND=$(echo "$OUTPUT" | grep -oP 'via \K\S+' | head -1)
        [ -z "$BACKEND" ] && BACKEND="(passthrough)"
        echo "  backend=${BACKEND}"

        if [[ $RC -ne 0 && $RC -ne 124 ]]; then
            echo "  ⚠️  Pipeline error (exit=${RC})"
            echo "$OUTPUT" | grep -iE "error|fail|negotiate" | head -3 | sed 's/^/     /'
            ERROR=$((ERROR + 1))
            ERROR_CASES+=("${LABEL}  backend=${BACKEND}  exit=${RC}")
            continue
        fi

        while true; do
            read -r -n1 -p "  Result? [p]ass / [f]ail / [s]kip: " JUDGE
            echo
            case "$JUDGE" in
                p|P)
                    PASS=$((PASS + 1))
                    echo "  ✅ PASS"
                    break ;;
                f|F)
                    FAIL=$((FAIL + 1))
                    FAILED_CASES+=("${LABEL}  backend=${BACKEND}")
                    echo "  ❌ FAIL"
                    break ;;
                s|S)
                    echo "  ⏭️  SKIP"
                    break ;;
                *)
                    echo "  (p/f/s)" ;;
            esac
        done
    done
done

echo ""
echo "============================================================"
echo " dxconvert Results: ${PASS} passed, ${FAIL} failed, ${ERROR} errors"
echo "============================================================"

if [[ ${#FAILED_CASES[@]} -gt 0 ]]; then
    echo ""
    echo " ❌ Failed cases:"
    for C in "${FAILED_CASES[@]}"; do
        echo "    - ${C}"
    done
fi

if [[ ${#ERROR_CASES[@]} -gt 0 ]]; then
    echo ""
    echo " ⚠️  Pipeline errors:"
    for C in "${ERROR_CASES[@]}"; do
        echo "    - ${C}"
    done
fi

echo ""
exit $((FAIL + ERROR))
