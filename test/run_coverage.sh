#!/usr/bin/env bash
# Coverage mode: assumes instrumented build was done beforehand (run ./build.sh --sonar separately).
# Output: test_new/_coverage/summary.txt, test_new/_coverage/lcov.info

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
# shellcheck source=setup_env.sh
source "$SCRIPT_DIR/setup_env.sh"
OUT="$SCRIPT_DIR/_coverage"
mkdir -p "$OUT"

if ! command -v lcov >/dev/null 2>&1; then
    echo "[FATAL] lcov is required. sudo apt install lcov"
    exit 2
fi

BUILD_DIR="$PROJECT_ROOT/gst-dxstream-plugin/builddir"
if [ ! -d "$BUILD_DIR" ]; then
    echo "[FATAL] $BUILD_DIR not found. Run './build.sh --sonar' first."
    exit 1
fi
# Simple check for coverage-instrumented build (*.gcno must exist)
if ! find "$BUILD_DIR" -name '*.gcno' -print -quit | grep -q .; then
    echo "[FATAL] No *.gcno files in $BUILD_DIR. Rebuild with './build.sh --sonar'."
    exit 1
fi

SRC_DIR="$PROJECT_ROOT/gst-dxstream-plugin"
# resolve the versioned .p dir at runtime instead of hardcoding the .so version (breaks on every version bump)
OBJ_DIR="$(find "$BUILD_DIR/src" -maxdepth 1 -type d -name 'libgstdxstream.so.*.p' -print -quit)"
if [ -z "$OBJ_DIR" ]; then
    echo "[FATAL] No libgstdxstream.so.*.p directory under $BUILD_DIR/src."
    exit 1
fi

# Create symlinks so geninfo can find metadata sources (../metadata/) when running gcov
for subdir in metadata general src; do
    LINK="$OBJ_DIR/../$subdir"
    [ -e "$LINK" ] || ln -sf "$SRC_DIR/$subdir" "$LINK"
done

echo "===== [1/3] Clearing gcda files (removing previous run data)"
find "$BUILD_DIR" -name '*.gcda' -delete

echo "===== [2/3] run_all.sh (FAIL_FAST=0)"
FAIL_FAST=0 "$SCRIPT_DIR/run_all.sh" || echo "[WARN] Some tests failed (continuing with coverage measurement)"

echo "===== [3/3] lcov capture"
# Metadata gcno/gcda files start with ".._metadata_" prefix (generated when meson builds ../metadata/ sources).
# Perl glob("*.gcov.json.gz") ignores files starting with a dot (.),
# so we create symlinks with "dd_metadata_" prefix so geninfo can recognize them.
for f in "$OBJ_DIR"/.._metadata_*.gcno "$OBJ_DIR"/.._metadata_*.gcda; do
    [ -f "$f" ] || continue
    base="$(basename "$f")"
    link="$OBJ_DIR/dd_metadata_${base#.._metadata_}"
    [ -e "$link" ] || ln -sf "$f" "$link"
done

(cd "$OBJ_DIR" && lcov --rc lcov_branch_coverage=1 --capture \
     --directory . \
     -o "$OUT/lcov.info") >/dev/null 2>&1

# Clean up symlinks
rm -f "$OBJ_DIR"/dd_metadata_*

# Exclude external headers (keep source directory)
# separate output path avoids lcov re-reading the file it's still writing (harmless but noisy "cannot read file" errors)
lcov --rc lcov_branch_coverage=1 \
     --remove "$OUT/lcov.info" \
     '/usr/*' '*/test/*' '*/test_new/*' \
     -o "$OUT/lcov.info.tmp" >/dev/null
mv "$OUT/lcov.info.tmp" "$OUT/lcov.info"

lcov --rc lcov_branch_coverage=1 --list "$OUT/lcov.info" | tee "$OUT/summary.txt"
echo
echo "Coverage info: $OUT/lcov.info"
echo "Summary     : $OUT/summary.txt"
