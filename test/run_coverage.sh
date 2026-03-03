#!/bin/bash

set -e

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath "${SCRIPT_DIR}/..")
BUILD_DIR="${PROJECT_ROOT}/gst-dxstream-plugin/builddir"
COVERAGE_DIR="${PROJECT_ROOT}/coverage"

echo "=========================================="
echo "  DX-Stream Code Coverage Test"
echo "=========================================="
echo ""

# Step 1: Check if coverage build exists
if [ ! -d "${BUILD_DIR}" ]; then
    echo "❌ Error: Build directory not found!"
    echo "   Please run: ./build.sh --sonar"
    exit 1
fi

# Step 2: Check if .gcno files exist (coverage instrumentation)
GCNO_COUNT=$(find "${BUILD_DIR}" -name "*.gcno" 2>/dev/null | wc -l)
if [ ${GCNO_COUNT} -eq 0 ]; then
    echo "❌ Error: No .gcno files found!"
    echo "   Please run: ./build.sh --sonar"
    exit 1
fi

echo "✓ Found ${GCNO_COUNT} instrumented object files (.gcno)"
echo ""

# Step 3: Clean previous coverage data
echo "[1/5] Cleaning previous coverage data..."
find "${BUILD_DIR}" -name "*.gcda" -delete 2>/dev/null || true
rm -rf "${COVERAGE_DIR}"
mkdir -p "${COVERAGE_DIR}"
echo "✓ Cleanup complete"
echo ""

# Step 4: Setup environment
echo "[2/5] Setting up environment..."
INSTALL_PREFIX="${INSTALL_PREFIX:-/usr/local}"

# Find the actual libdir
if pkg-config --exists gstdxstream 2>/dev/null; then
    ACTUAL_LIBDIR=$(pkg-config --variable=libdir gstdxstream)
else
    ACTUAL_LIBDIR=$(find "${INSTALL_PREFIX}/lib" -type d -name "gstreamer-1.0" 2>/dev/null | head -n 1 | xargs dirname)
    [ -z "$ACTUAL_LIBDIR" ] && ACTUAL_LIBDIR="${INSTALL_PREFIX}/lib"
fi

export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig:${PKG_CONFIG_PATH}"
export GST_PLUGIN_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${GST_PLUGIN_PATH}"
export LD_LIBRARY_PATH="${ACTUAL_LIBDIR}/gstreamer-1.0:${INSTALL_PREFIX}/share/gstdxstream/lib:${LD_LIBRARY_PATH}"
export PATH="${INSTALL_PREFIX}/share/gstdxstream/bin:${PATH}"
echo "✓ Environment configured"
echo ""

# Step 5: Check plugin location
echo "[3/5] Checking plugin installation..."
# Clear GStreamer cache first
rm -rf ~/.cache/gstreamer-1.0/ 2>/dev/null || true

# Check if plugin loads
if gst-inspect-1.0 dxstream > /dev/null 2>&1; then
    PLUGIN_PATH=$(gst-inspect-1.0 dxstream 2>&1 | grep "Filename" | awk '{print $2}')
    echo "✓ Plugin found at: ${PLUGIN_PATH}"
else
    echo "❌ Error: dxstream plugin not found!"
    echo "   Please run: ./build.sh --sonar"
    exit 1
fi
echo ""

echo "TEST Inference"
run_model -m "${SCRIPT_DIR}/../dx_stream/samples/models/YOLOV5S_3.dxnn" -l 1000

# Step 6: Set GCOV environment variables
echo "[4/5] Setting up coverage environment..."
# IMPORTANT: Don't set GCOV_PREFIX/GCOV_PREFIX_STRIP!
# The .so file already has absolute paths to .gcno files
# Setting these variables will cause .gcda files to be written to wrong location
# Just ensure the build directory is writable
unset GCOV_PREFIX
unset GCOV_PREFIX_STRIP

# Make builddir writable
chmod -R 777 "${BUILD_DIR}" 2>/dev/null || sudo chmod -R 777 "${BUILD_DIR}"
echo "✓ Environment configured (using embedded paths in .so)"
echo ""

# Step 7: Run tests
echo "[5/5] Running tests to generate coverage data..."
echo ""

# Test 1: Plugin inspection (loads plugin code)
echo "  → Test 1: Inspecting dxstream plugin..."
gst-inspect-1.0 dxstream && echo "    ✓ Plugin loaded" || echo "    ✗ Plugin load failed"

# Test 2: Run element tests if available
if [ -d "${SCRIPT_DIR}/elements" ] && [ -f "${SCRIPT_DIR}/elements/test_elements.sh" ]; then
    echo "  → Test 2: Running element tests..."
    cd "${SCRIPT_DIR}/elements"
    bash test_elements.sh && echo "    ✓ Element tests passed" || echo "    ⚠ Some element tests failed (continuing...)"
    cd "${SCRIPT_DIR}"
fi

# Test 3: Run pipeline tests if available
if [ -d "${SCRIPT_DIR}/pipelines" ] && [ -f "${SCRIPT_DIR}/pipelines/test_pipeline.sh" ]; then
    echo "  → Test 3: Running pipeline tests..."
    cd "${SCRIPT_DIR}/pipelines"
    bash test_pipeline.sh && echo "    ✓ Pipeline tests passed" || echo "    ⚠ Some pipeline tests failed (continuing...)"
    cd "${SCRIPT_DIR}"
fi

# Test 4: Run demo pipelines
if [ -d "${SCRIPT_DIR}/demo_pipelines" ] && [ -f "${SCRIPT_DIR}/demo_pipelines/single_model_pipeline.sh" ]; then
    echo "  → Test 4: Running demo pipelines..."
    cd "${SCRIPT_DIR}/demo_pipelines"
    bash single_model_pipeline.sh && echo "    ✓ single model pipeline passed" || echo "    ⚠  single model pipeline failed (continuing...)"
    bash secondary_pipeline.sh && echo "    ✓ secondary pipeline passed" || echo "    ⚠  secondary pipeline failed (continuing...)"
    bash multi_stream_pipeline.sh && echo "    ✓ multi stream pipeline passed" || echo "    ⚠  multi stream pipeline failed (continuing...)"
    bash broker_pipeline.sh && echo "    ✓ broker pipeline passed" || echo "    ⚠  broker pipeline failed (continuing...)"
    cd "${SCRIPT_DIR}"
fi

echo ""
echo "✓ Test execution completed"
echo ""

# Step 8: Check if .gcda files were created
echo "[6/6] Checking coverage data..."
GCDA_COUNT=$(find "${BUILD_DIR}" -name "*.gcda" 2>/dev/null | wc -l)
echo "Coverage data files (.gcda) created: ${GCDA_COUNT}"
echo ""

if [ ${GCDA_COUNT} -eq 0 ]; then
    echo "❌ ERROR: No .gcda files generated!"
    echo ""
    echo "Debugging information:"
    echo "  Build dir:       ${BUILD_DIR}"
    echo "  Plugin location: ${PLUGIN_PATH}"
    echo "  GCOV_PREFIX:     ${GCOV_PREFIX}"
    echo ""
    echo "Possible causes:"
    echo "  1. Plugin was not actually executed"
    echo "  2. Permission issues writing .gcda files"
    echo "  3. Wrong plugin loaded (not the coverage build)"
    echo ""
    exit 1
fi

# Step 8: Generate coverage reports
echo "Generating coverage reports..."
echo ""

cd "${BUILD_DIR}"

# Generate lcov report
echo "  → Generating lcov report..."
lcov --capture \
     --directory . \
     --output-file "${COVERAGE_DIR}/coverage.info" \
     --rc lcov_branch_coverage=1 \
     2>&1 | grep -E "Found|Processing" | head -10

lcov --remove "${COVERAGE_DIR}/coverage.info" '/usr/*' '*/test/*' \
     --output-file "${COVERAGE_DIR}/coverage_filtered.info" \
     --rc lcov_branch_coverage=1 \
     > /dev/null 2>&1

# Generate HTML report
echo "  → Generating HTML report..."
genhtml "${COVERAGE_DIR}/coverage_filtered.info" --output-directory "${COVERAGE_DIR}/html" --branch-coverage > /dev/null 2>&1

# Generate XML report for SonarQube
echo "  → Generating XML report for SonarQube..."
gcovr --sonarqube \
      --root "${PROJECT_ROOT}" \
      --output "${COVERAGE_DIR}/coverage.xml" \
      --filter "${PROJECT_ROOT}/gst-dxstream-plugin/src/.*" \
      --filter "${PROJECT_ROOT}/gst-dxstream-plugin/metadata/.*" \
      --exclude ".*/test/.*" \
      --exclude "/usr/.*" \
      "${BUILD_DIR}"

if [ -f "${COVERAGE_DIR}/coverage.xml" ]; then
    sed -i 's/version="gcovr[^"]*"/version="1"/g' "${COVERAGE_DIR}/coverage.xml"
    echo "    ✓ XML report created"
else
    echo "    ✗ XML report creation failed"
    echo "    Please check if gcovr is installed: sudo apt-get install gcovr"
fi

# Generate text summary with lcov
echo "  → Generating summary..."
lcov --summary "${COVERAGE_DIR}/coverage_filtered.info" --rc lcov_branch_coverage=1 2>&1 | tee "${COVERAGE_DIR}/summary.txt"

echo ""
echo "=========================================="
echo "✅ Coverage Test Complete!"
echo "=========================================="
echo ""
echo "📊 Results:"
echo "  • .gcda files:  ${GCDA_COUNT}"
echo "  • HTML report:  ${COVERAGE_DIR}/html/index.html"
echo "  • XML report:   ${COVERAGE_DIR}/coverage.xml"
echo "  • Coverage info: ${COVERAGE_DIR}/coverage_filtered.info"
echo "  • Summary:      ${COVERAGE_DIR}/summary.txt"
echo ""
echo "To view HTML report:"
echo "  firefox ${COVERAGE_DIR}/html/index.html"
echo ""
