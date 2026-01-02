#!/bin/bash

# DX-STREAM User Meta Test Script
# This script builds and runs the user metadata safety tests

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")
BUILD_TYPE="debug"

show_help() {
    echo "Usage: $(basename "$0") [--help]"
    echo "Example: $0"
    echo "Options:"
    echo "  [--help]        Show this help message"
    exit 0
}

# Parse arguments
for i in "$@"; do
    case "$1" in
        --help)
            show_help
            ;;
        *)
            if [ -n "$1" ]; then
                echo "Unknown option: $1"
                show_help
            fi
            ;;
    esac
    shift
done

echo "========================================"
echo "DX-STREAM User Meta Safety Test"
echo "========================================"
echo "Using build type: $BUILD_TYPE"
echo "Script directory: $SCRIPT_DIR"
echo "Project root: $PROJECT_ROOT"

# Function to build and install the test
build_and_install() {
    echo ""
    echo "🔨 Building user meta test..."
    
    # Clean previous build
    if [ -d "$SCRIPT_DIR/install" ]; then
        rm -rf "$SCRIPT_DIR/install"
    fi

    if [ -d "$SCRIPT_DIR/build" ]; then
        rm -rf "$SCRIPT_DIR/build"
    fi

    # Setup meson build
    meson setup build --buildtype="$BUILD_TYPE" --prefix="$SCRIPT_DIR/install"
    if [ $? -ne 0 ]; then
        echo "❌ Error: meson setup failed"
        return 1
    fi

    # Compile
    meson compile -C build
    if [ $? -ne 0 ]; then
        echo "❌ Error: meson compile failed"
        return 1
    fi

    # Install
    yes | meson install -C build > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "❌ Error: meson install failed"
        return 1
    fi

    echo "✅ Build completed successfully"
    return 0
}

# Function to run the test
run_test() {
    echo ""
    echo "🧪 Running user meta safety tests..."
    echo "========================================"
    
    # Set up environment for the test
    export LD_LIBRARY_PATH="$PROJECT_ROOT/install/lib/gstreamer-1.0:$PROJECT_ROOT/install/lib:$LD_LIBRARY_PATH"
    export GST_PLUGIN_PATH="$PROJECT_ROOT/install/lib/gstreamer-1.0:$GST_PLUGIN_PATH"
    
    # Run the test executable
    "$SCRIPT_DIR/install/bin/test_usermeta"
    test_result=$?
    
    echo "========================================"
    
    if [ $test_result -eq 0 ]; then
        echo "✅ All tests passed! User meta system is working correctly."
        echo ""
        echo "🔒 Validated safety features:"
        echo "   • Required copy/release function validation"
        echo "   • Add operation safety checks"
        echo "   • Deep copy verification"
        echo "   • Functional workflow validation"
        return 0
    else
        echo "❌ Some tests failed! Please check the output above."
        echo ""
        echo "🔍 Common issues to check:"
        echo "   • DX-Stream plugin installation"
        echo "   • GStreamer environment setup"
        echo "   • Missing dependencies"
        return 1
    fi
}

# Function to cleanup
cleanup() {
    echo ""
    echo "🧹 Cleaning up..."
    if [ -d "$SCRIPT_DIR/build" ]; then
        rm -rf "$SCRIPT_DIR/build"
    fi
    echo "✅ Cleanup completed"
}

# Main execution
main() {
    # Check if DX-Stream is installed
    if [ ! -f "$PROJECT_ROOT/install/lib/gstreamer-1.0/libgstdxstream.so" ]; then
        echo "❌ Error: DX-Stream plugin not found!"
        echo "Please build and install DX-Stream first:"
        echo "  cd $PROJECT_ROOT && ./build.sh"
        exit 1
    fi

    # Build the test
    build_and_install
    if [ $? -ne 0 ]; then
        exit 1
    fi

    # Run the test
    run_test
    test_result=$?

    # Cleanup
    cleanup

    # Exit with test result
    exit $test_result
}

# Trap to ensure cleanup on exit
trap cleanup EXIT

# Run main function
main
