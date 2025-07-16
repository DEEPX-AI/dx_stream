#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")

BASE_URL="https://sdk.deepx.ai/"

# default value
SOURCE_PATH="res/models/models-1_60_1.tar.gz"
OUTPUT_DIR=$SCRIPT_DIR/dx_stream/samples/models
SYMLINK_TARGET_PATH=""
SYMLINK_ARGS=""
FORCE_ARGS=""

# Function to display help message
show_help() {
  
  echo "Usage: $(basename "$0") [OPTIONS]"
  echo "Options:"
  echo "  [--force]                  Force overwrite if the file already exists"
  echo "  [--help]                   Show this help message"

  if [ "$1" == "error" ]; then
    echo "Error: Invalid or missing arguments."
    exit 1
  fi
  exit 0
}

# parse args
for i in "$@"; do
    case "$1" in
        --src_path=*)
            SOURCE_PATH="${1#*=}"
            ;;
        --output=*)
            OUTPUT_DIR="${1#*=}"

            # Symbolic link cannot be created when output_dir is the current directory.
            OUTPUT_REAL_DIR=$(readlink -f "$OUTPUT_DIR")
            CURRENT_REAL_DIR=$(readlink -f "./")
            if [ "$OUTPUT_REAL_DIR" == "$CURRENT_REAL_DIR" ]; then
                echo "'--output' is the same as the current directory. Please specify a different directory."
                exit 1
            fi
            ;;
        --symlink_target_path=*)
            SYMLINK_TARGET_PATH="${1#*=}"
            SYMLINK_ARGS="--symlink_target_path=$SYMLINK_TARGET_PATH"
            ;;
        --force)
            FORCE_ARGS="--force"
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

SCRIPT_DIR=$(realpath "$(dirname "$0")")
GET_RES_CMD="$SCRIPT_DIR/scripts/get_resource.sh --src_path=$SOURCE_PATH --output=$OUTPUT_DIR $SYMLINK_ARGS $FORCE_ARGS --extract"
echo "Get Resources from remote server ..."
echo "$GET_RES_CMD"

$GET_RES_CMD
if [ $? -ne 0 ]; then
    echo "Get resource failed!"
    exit 1
fi

exit 0
