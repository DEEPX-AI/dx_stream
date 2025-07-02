#!/bin/bash

SCRIPT_DIR=$(realpath "$(dirname "$0")")

# Default values
PROJECT_NAME="dx_stream"
PROJECT_ROOT=$(realpath -s "${SCRIPT_DIR}")
RUNTIME_PATH=$(realpath -s "${PROJECT_ROOT}/..")
DX_AS_PATH=$(realpath -s "${RUNTIME_PATH}/..")
DOCKER_VOLUME_PATH=${DOCKER_VOLUME_PATH}
FORCE_ARGS=""

# color env settings
source ${SCRIPT_DIR}/color_env.sh

pushd $SCRIPT_DIR

# Function to display help message
show_help() {
    echo "Usage: $(basename "$0") [OPTIONS]"
    echo "Options:"
    echo "  --docker_volume_path=<path>    Set Docker volume path (required in container mode)"
    echo "  [--force]                      Force overwrite if the file already exists"
    echo "  [--help]                       Show this help message"

    if [ "$1" == "error" ] && [[ ! -n "$2" ]]; then
        echo -e "${TAG_ERROR} Invalid or missing arguments."
        exit 1
    elif [ "$1" == "error" ] && [[ -n "$2" ]]; then
        echo -e "${TAG_ERROR} $2"
        exit 1
    elif [[ "$1" == "warn" ]] && [[ -n "$2" ]]; then
        echo -e "${TAG_WARN} $2"
        return 0
    fi
    exit 0
}

# Parse arguments
for i in "$@"; do
    case $i in
        --docker_volume_path=*)
            DOCKER_VOLUME_PATH="${i#*=}"
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

setup_assets() {
    MODEL_PATH=./dx_stream/samples/models
    VIDEO_PATH=./dx_stream/samples/videos
    CONTAINER_MODE=false

    # Check if running in a container
    if grep -qE "/docker|/lxc|/containerd" /proc/1/cgroup || [ -f /.dockerenv ]; then
        CONTAINER_MODE=true
        echo "(container mode detected)"
        
        if [ -z "$DOCKER_VOLUME_PATH" ]; then
            show_help "error" "--docker_volume_path must be provided in container mode."
            exit 1
        fi

        SETUP_MODEL_ARGS="--output=${MODEL_PATH} --symlink_target_path=${DOCKER_VOLUME_PATH}/res/models"
        SETUP_VIDEO_ARGS="--output=${VIDEO_PATH} --symlink_target_path=${DOCKER_VOLUME_PATH}/res/videos"
    else
        echo "(host mode detected)"
        SETUP_MODEL_ARGS="--output=${MODEL_PATH} --symlink_target_path=${DX_AS_PATH}/workspace/res/models"
        SETUP_VIDEO_ARGS="--output=${VIDEO_PATH} --symlink_target_path=${DX_AS_PATH}/workspace/res/videos"
    fi

    echo -e "${TAG_INFO} MODEL_PATH: ${MODEL_PATH}"
    MODEL_REAL_PATH=$(readlink -f "$MODEL_PATH")
    # Check and set up models
    if [ ! -d "$MODEL_REAL_PATH" ] || [ "$FORCE_ARGS" != "" ]; then
        echo -e "${TAG_INFO} models directory not found. Running setup models script... ($MODEL_REAL_PATH)"
        ./setup_sample_models.sh $SETUP_MODEL_ARGS $FORCE_ARGS || { echo "Setup models script failed."; rm -rf $MODEL_PATH; exit 1; }
    else
        echo -e "${TAG_INFO} models directory found. ($MODEL_REAL_PATH)"
    fi

    echo "VIDEO_PATH: ${VIDEO_PATH}"
    VIDEO_REAL_PATH=$(readlink -f "$VIDEO_PATH")
    # Check and set up models
    if [ ! -d "$VIDEO_REAL_PATH" ] || [ "$FORCE_ARGS" != "" ]; then
        echo -e "${TAG_INFO} Video directory not found. Running setup models script... ($VIDEO_REAL_PATH)"
        ./setup_sample_videos.sh $SETUP_VIDEO_ARGS $FORCE_ARGS || { echo "Setup models script failed."; rm -rf $VIDEO_PATH; exit 1; }
    else
        echo -e "${TAG_INFO} Video directory found. ($VIDEO_REAL_PATH)"
    fi

    TARGET_VIDEO_NAME="crowded_in_subway.mp4"
    TARGET_VIDEO_PATH="${VIDEO_PATH}/${TARGET_VIDEO_NAME}"

    if [ -f "$TARGET_VIDEO_PATH" ]; then
        rm -f "$TARGET_VIDEO_PATH"
        if [ $? -eq 0 ]; then
            echo -e "${TAG_INFO} ${TARGET_VIDEO_NAME} deleted."
        else
            echo -e "${TAG_ERR} ${TARGET_VIDEO_NAME} deletion failed."
        fi
    else
        echo -e "${TAG_INFO} ${TARGET_VIDEO_NAME} not found."
    fi
}

main() {
    setup_assets
}

main

popd
