#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
# color env settings
source ${SCRIPT_DIR}/scripts/color_env.sh
source ${SCRIPT_DIR}/scripts/common_util.sh

# Check if jq is installed, if not, try to install it
if ! command -v jq >/dev/null 2>&1; then
    print_colored "jq not found, attempting to install..." "WARNING"
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get update && sudo apt-get install -y jq
    elif command -v yum >/dev/null 2>&1; then
        sudo yum install -y jq
    else
        print_colored "Could not install jq automatically. Please install jq manually." "ERROR"
        exit 1
    fi
    if ! command -v jq >/dev/null 2>&1; then
        print_colored "jq installation failed. Please install jq manually." "ERROR"
        exit 1
    fi
    print_colored "jq installed successfully." "SUCCESS"
fi

BASE_URL="https://sdk.deepx.ai/modelzoo/dxnn"
MODEL_LIST_JSON="$SCRIPT_DIR/model_list.json"
OUTPUT_DIR="$SCRIPT_DIR/dx_stream/samples/models"
SYMLINK_TARGET_PATH=""
FORCE=0

show_help() {
    echo "Usage: $(basename "$0") [OPTIONS]"
    echo "Options:"
    echo "  [--output=<dir>]                Output directory for symlink (default: ./dx_stream/samples/models)"
    echo "  [--symlink_target_path=<dir>]    Target path for real model files (default: workspace/res/models)"
    echo "  [--model=<modelname>]            Download only the specified model (must exist in model_list.json)"
    echo "  [--force]                        Force overwrite if the file or link already exists"
    echo "  [--help]                         Show this help message"
    echo
    echo "If --model is given, only that model will be downloaded and linked. Otherwise, all models are processed."
    exit 0
}

# Parse args
MODEL_NAME=""
for i in "$@"; do
    case "$1" in
        --output=*)
            OUTPUT_DIR="${1#*=}"
            ;;
        --symlink_target_path=*)
            SYMLINK_TARGET_PATH="${1#*=}"
            ;;
        --model=*)
            MODEL_NAME="${1#*=}"
            ;;
        --force)
            FORCE=1
            ;;
        --help)
            show_help
            ;;
        *)
            echo "Unknown option: $1"
            show_help
            ;;
    esac
    shift
done

if [ -z "$SYMLINK_TARGET_PATH" ]; then
    print_colored "--symlink_target_path must be provided." "ERROR"
    exit 1
fi

if [ ! -f "$MODEL_LIST_JSON" ]; then
    print_colored "model_list.json not found at $MODEL_LIST_JSON" "ERROR"
    exit 1
fi


# Parse version and models from model_list.json
MODEL_VERSION=$(jq -r '.version' "$MODEL_LIST_JSON")
ALL_MODEL_FILES=($(jq -r '.models[]' "$MODEL_LIST_JSON"))
REAL_MODEL_DIR="$SYMLINK_TARGET_PATH/models-$MODEL_VERSION"

if [ -n "$MODEL_NAME" ]; then
    FOUND=0
    for m in "${ALL_MODEL_FILES[@]}"; do
        if [ "$m" == "$MODEL_NAME" ]; then
            FOUND=1
            break
        fi
    done
    if [ $FOUND -eq 0 ]; then
        print_colored "Model $MODEL_NAME not found in model_list.json" "ERROR"
        exit 1
    fi
    MODEL_FILES=("$MODEL_NAME")
else
    MODEL_FILES=("${ALL_MODEL_FILES[@]}")
fi

print_colored "Model version: $MODEL_VERSION" "INFO"
print_colored "Model files: ${MODEL_FILES[*]}" "INFO"
print_colored "Download dir: $REAL_MODEL_DIR" "INFO"

# Create real model dir
if [ $FORCE -eq 1 ]; then
    print_colored "--force: removing $REAL_MODEL_DIR" "WARNING"
    rm -rf "$REAL_MODEL_DIR"
fi
mkdir -p "$REAL_MODEL_DIR"

# Download each model file
for model in "${MODEL_FILES[@]}"; do
    url="$BASE_URL/$MODEL_VERSION/$model"
    dest="$REAL_MODEL_DIR/$model"
    if [ -f "$dest" ] && [ $FORCE -ne 1 ]; then
        print_colored "$model already exists, skipping." "INFO"
        continue
    fi
    print_colored "Downloading $model ..." "INFO"
    curl -fSL -o "$dest" "$url"
    if [ $? -ne 0 ]; then
        print_colored "Failed to download $url" "ERROR"
        rm -f "$dest"
        exit 1
    fi
done

# Create symlink from OUTPUT_DIR to REAL_MODEL_DIR
NEED_SYMLINK=1
if [ -L "$OUTPUT_DIR" ]; then
    LINK_TARGET=$(readlink "$OUTPUT_DIR")
    if [ "$LINK_TARGET" = "$REAL_MODEL_DIR" ]; then
        NEED_SYMLINK=0
    elif [ $FORCE -eq 1 ]; then
        print_colored "--force: removing existing symlink $OUTPUT_DIR" "WARNING"
        rm -rf "$OUTPUT_DIR"
    else
        print_colored "$OUTPUT_DIR already exists but points to $LINK_TARGET. Use --force to overwrite." "ERROR"
        exit 1
    fi
elif [ -d "$OUTPUT_DIR" ]; then
    if [ $FORCE -eq 1 ]; then
        print_colored "--force: removing existing directory $OUTPUT_DIR" "WARNING"
        rm -rf "$OUTPUT_DIR"
    else
        print_colored "$OUTPUT_DIR already exists as a directory. Use --force to overwrite." "ERROR"
        exit 1
    fi
fi


# Ensure parent directory exists before creating symlink
PARENT_DIR=$(dirname "$OUTPUT_DIR")
mkdir -p "$PARENT_DIR"

if [ $NEED_SYMLINK -eq 1 ]; then
    ln -s "$(readlink -f "$REAL_MODEL_DIR")" "$OUTPUT_DIR"
    print_colored "Symlink created: $OUTPUT_DIR -> $REAL_MODEL_DIR" "SUCCESS"
else
    print_colored "Symlink already correct: $OUTPUT_DIR -> $REAL_MODEL_DIR" "INFO"
fi

print_colored "[OK] All models downloaded and symlinked." "SUCCESS"
exit 0
