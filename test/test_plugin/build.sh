#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")
BUILD_TYPE="release"
SONAR_MODE_ARG=""
NATIVE_FILE_ARG=""
BUILD_DIR="builddir"
INSTALL_DIR="install"

show_help() {
  echo "Usage: $(basename "$0") [--debug] [--help]"
  echo "Example 1): $0"
  echo "Options:"
  echo "  [--help]        Show this help message"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}


# Parse arguments
for i in "$@"; do
    case "$1" in
        --sonar)
            SONAR_MODE_ARG="--sonar"
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help "error"
            ;;
    esac
    shift
done


echo "Using build type: $BUILD_TYPE"
echo "SONAR_MODE_ARG($SONAR_MODE_ARG) is set"

build_and_install() {
    if [ -d "$SCRIPT_DIR/$INSTALL_DIR" ]; then
        rm -rf "$SCRIPT_DIR/$INSTALL_DIR"
    fi

    if [ -d "$SCRIPT_DIR/$BUILD_DIR" ]; then
        rm -rf "$SCRIPT_DIR/$BUILD_DIR"
    fi

    meson setup "${BUILD_DIR}" --buildtype=debug --prefix="$SCRIPT_DIR"
    if [ $? -ne 0 ]; then
        echo -e "Error: meson setup failed"
        exit 1
    fi
    meson compile -C "${BUILD_DIR}"
    if [ $? -ne 0 ]; then
        echo -e "Error: meson compile failed"
        exit 1
    fi
    meson install -C "${BUILD_DIR}" --no-rebuild
    if [ $? -ne 0 ]; then
        echo -e "Error: meson install failed"
        echo -e "Hint: Run 'which -a meson' to check if multiple meson versions are installed."
        echo -e "      If so, keep only one and remove the rest. (See: docs/source/docs/06_Troubleshooting_and_FAQ.md)"
        exit 1
    fi
    if [ ! -n "${SONAR_MODE_ARG}" ]; then
        rm -rf "${BUILD_DIR}"
    else
        echo -e "Warn: The '--sonar' option is set. So, Skip to remove '${BUILD_DIR}' directory"
    fi
}

build_and_install
