#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")
RUNTIME_PATH=$(realpath -s "${SCRIPT_DIR}/..")
DX_AS_PATH=$(realpath -s "${RUNTIME_PATH}/..")

# color env settings
source ${SCRIPT_DIR}/scripts/color_env.sh
source ${SCRIPT_DIR}/scripts/common_util.sh

# --- Initialize variables ---
ENABLE_DEBUG_LOGS=0   # New flag for debug logging
DOCKER_VOLUME_PATH=${DOCKER_VOLUME_PATH}
FORCE_ARGS=""

pushd $SCRIPT_DIR

# Function to display help message
show_help() {
    print_colored "Usage: $(basename "$0") [OPTIONS]" "YELLOW"
    print_colored "Options:" "GREEN"
    print_colored "  --docker_volume_path=<path>    Set Docker volume path (required in container mode)" "GREEN"
    print_colored "  [--model=<modelname>]          Download only the specified model (must exist in model_list.json)" "GREEN"
    print_colored "  [--force]                      Force overwrite if the file already exists" "GREEN"
    print_colored "  [--verbose]                    Enable verbose (debug) logging." "GREEN"
    print_colored "  [--help]                       Show this help message" "GREEN"
    print_colored "" "GREEN"
    print_colored "If --model is given, only that model will be downloaded and linked. Otherwise, all models and videos are set up." "GREEN"

    if [ "$1" == "error" ] && [[ ! -n "$2" ]]; then
        print_colored "Invalid or missing arguments." "ERROR"
        exit 1
    elif [ "$1" == "error" ] && [[ -n "$2" ]]; then
        print_colored "$2" "ERROR"
        exit 1
    elif [[ "$1" == "warn" ]] && [[ -n "$2" ]]; then
        print_colored "$2" "WARNING"
        return 0
    fi
    exit 0
}


# Parse arguments
MODEL_ARG=""
for i in "$@"; do
    case $1 in
        --docker_volume_path=*)
            DOCKER_VOLUME_PATH="${i#*=}"
            shift
            ;;
        --force)
            FORCE_ARGS="--force"
            shift
            ;;
        --model=*)
            MODEL_ARG="${i#*=}"
            shift
            ;;
        --verbose)
            ENABLE_DEBUG_LOGS=1
            shift # Consume argument
            ;;
        --help)
            show_help
            ;;
        *)
            show_help "error" "Invalid option '$1'"
            ;;
    esac
done

print_colored "======== PATH INFO =========" "DEBUG"
print_colored "RUNTIME_PATH($RUNTIME_PATH)" "DEBUG"
print_colored "DX_AS_PATH($DX_AS_PATH)" "DEBUG"

# Default values
print_colored "=== DOCKER_VOLUME_PATH($DOCKER_VOLUME_PATH) is set ===" "INFO"

setup_assets() {
    MODEL_PATH=./dx_stream/samples/models
    VIDEO_PATH=./dx_stream/samples/videos
    CONTAINER_MODE=false

    # Check if running in a container
    if grep -qE "/docker|/lxc|/containerd" /proc/1/cgroup || [ -f /.dockerenv ]; then
        CONTAINER_MODE=true
        print_colored "(container mode detected)" "INFO"
        
        if [ -z "$DOCKER_VOLUME_PATH" ]; then
            show_help "error" "--docker_volume_path must be provided in container mode."
            exit 1
        fi

        SETUP_MODEL_ARGS="--output=${MODEL_PATH} --symlink_target_path=${DOCKER_VOLUME_PATH}/res/models"
        SETUP_VIDEO_ARGS="--output=${VIDEO_PATH} --symlink_target_path=${DOCKER_VOLUME_PATH}/res/videos"
    else
        print_colored "(host mode detected)" "INFO"
        SETUP_MODEL_ARGS="--output=${MODEL_PATH} --symlink_target_path=${DX_AS_PATH}/workspace/res/models"
        SETUP_VIDEO_ARGS="--output=${VIDEO_PATH} --symlink_target_path=${DX_AS_PATH}/workspace/res/videos"
    fi

    print_colored " MODEL_PATH: ${MODEL_PATH}" "INFO"
    MODEL_REAL_PATH=$(readlink -f "$MODEL_PATH")
    # Check and set up models
    NEED_MODEL_SETUP=0
    if [ ! -d "$MODEL_REAL_PATH" ] || [ "$FORCE_ARGS" != "" ]; then
        NEED_MODEL_SETUP=1
    else
        # Check if all models in model_list.json exist
        MODEL_LIST_JSON="$SCRIPT_DIR/model_list.json"
        if [ ! -f "$MODEL_LIST_JSON" ]; then
            print_colored "model_list.json not found at $MODEL_LIST_JSON" "ERROR"
            NEED_MODEL_SETUP=1
        else
            MODEL_VERSION=$(jq -r '.version' "$MODEL_LIST_JSON")
            MODEL_FILES=($(jq -r '.models[]' "$MODEL_LIST_JSON"))
            for model in "${MODEL_FILES[@]}"; do
                if [ ! -f "$MODEL_REAL_PATH/$model" ]; then
                    print_colored "Model file missing: $MODEL_REAL_PATH/$model" "WARNING"
                    NEED_MODEL_SETUP=1
                    break
                fi
            done
        fi
    fi
    if [ $NEED_MODEL_SETUP -eq 1 ]; then
        print_colored " models directory or files missing. Running setup models script... ($MODEL_REAL_PATH)" "INFO"
        ./setup_sample_models.sh $SETUP_MODEL_ARGS $FORCE_ARGS || { print_colored "Setup models script failed." "ERROR"; rm -rf $MODEL_PATH; exit 1; }
    else
        print_colored " All model files found. ($MODEL_REAL_PATH)" "INFO"
    fi

    print_colored "VIDEO_PATH: ${VIDEO_PATH}" "INFO"
    VIDEO_REAL_PATH=$(readlink -f "$VIDEO_PATH")
    # Check and set up models
    if [ ! -d "$VIDEO_REAL_PATH" ] || [ "$FORCE_ARGS" != "" ]; then
        print_colored " Video directory not found. Running setup models script... ($VIDEO_REAL_PATH)" "INFO"
        ./setup_sample_videos.sh $SETUP_VIDEO_ARGS $FORCE_ARGS || { print_colored "Setup videos script failed." "ERROR"; rm -rf $VIDEO_PATH; exit 1; }
    else
        print_colored " Video directory found. ($VIDEO_REAL_PATH)" "INFO"
    fi

    print_colored "[OK] Sample models and videos setup complete" "INFO"
}


main() {
    if [ -n "$MODEL_ARG" ]; then
        # Only download a single model
        print_colored "[MODE] Single model mode: $MODEL_ARG" "INFO"
        if grep -qE "/docker|/lxc|/containerd" /proc/1/cgroup || [ -f /.dockerenv ]; then
            if [ -z "$DOCKER_VOLUME_PATH" ]; then
                show_help "error" "--docker_volume_path must be provided in container mode."
                exit 1
            fi
            SYMLINK_PATH="--symlink_target_path=${DOCKER_VOLUME_PATH}/res/models"
            OUTPUT_PATH="--output=./dx_stream/samples/models"
        else
            SYMLINK_PATH="--symlink_target_path=${DX_AS_PATH}/workspace/res/models"
            OUTPUT_PATH="--output=./dx_stream/samples/models"
        fi
        ./setup_sample_models.sh $OUTPUT_PATH $SYMLINK_PATH --model="$MODEL_ARG" $FORCE_ARGS || { print_colored "Setup single model script failed." "ERROR"; exit 1; }
    else
        setup_assets
    fi
}

main

popd
