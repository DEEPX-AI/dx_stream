#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}/../..")
BUILD_TYPE="release"
SONAR_MODE_ARG=""
NATIVE_FILE_ARG=""


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

set_native_file_arg(){
    local gcc_version=$(gcc -dumpversion | cut -f1 -d.)
    local gpp_version=$(g++ -dumpversion | cut -f1 -d.)

    echo "Current GCC version: $gcc_version"
    echo "Current G++ version: $gpp_version"

    # If version is less than 11, install gcc-11 and g++-11
    if [ "$gcc_version" -lt 11 ] || [ "$gpp_version" -lt 11 ]; then
        # Check if gcc-11 is installed
        if dpkg -s gcc-11 >/dev/null 2>&1; then
            NATIVE_FILE_ARG="--native-file ${PROJECT_ROOT}/gcc11.ini"
            echo "GCC/G++ version 11 is already installed"
        else
            echo "Error: GCC/G++ version 11 or higher are required. Please run install.sh to install them and try again..."
            exit 1
        fi
    else
        echo "GCC/G++ 11 or higher is already installed."
    fi
}

build_and_install() {
    set_native_file_arg

    if [ -d "$SCRIPT_DIR/install" ]; then
        rm -rf $SCRIPT_DIR/install
    fi

    if [ -d "$SCRIPT_DIR/build" ]; then
        rm -rf $SCRIPT_DIR/build
    fi

    meson setup build --buildtype=debug --prefix="$SCRIPT_DIR" ${NATIVE_FILE_ARG}
    if [ $? -ne 0 ]; then
        echo -e "Error: meson setup failed"
        exit 1
    fi
    meson compile -C build
    if [ $? -ne 0 ]; then
        echo -e "Error: meson compile failed"
        exit 1
    fi
    yes | meson install -C build
    if [ $? -ne 0 ]; then
        echo -e "Error: meson install failed"
        exit 1
    fi
    if [ ! -n "${SONAR_MODE_ARG}" ]; then
        rm -rf build
    else
        echo -e "Warn: The '--sonar' option is set. So, Skip to remove 'build' directory"
    fi
}

build_and_install
