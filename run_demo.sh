#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
DX_STREAM_PATH=$(realpath -s "${SCRIPT_DIR}")

# color env settings
source "${DX_STREAM_PATH}/scripts/color_env.sh"
source "${DX_STREAM_PATH}/scripts/common_util.sh"

INTERNAL_RTSP_ARG=""

for arg in "$@"; do
    case "$arg" in
        --internal-rtsp)
            INTERNAL_RTSP_ARG="--internal-rtsp"
            ;;
    esac
done

pushd $DX_STREAM_PATH

print_colored "DX_STREAM_PATH: $DX_STREAM_PATH" "INFO"

if ! gst-inspect-1.0 dxstream >/dev/null 2>&1; then
    print_colored "dxstream plugin not found. Trying to reload environment variables..." "WARNING"
    
    # Try sourcing bashrc to load environment variables
    if [ -f "$HOME/.bashrc" ]; then
        source "$HOME/.bashrc" 2>/dev/null
        
        # Check again after sourcing
        if gst-inspect-1.0 dxstream >/dev/null 2>&1; then
            print_colored "dxstream plugin found after reloading environment." "SUCCESS"
        else
            print_colored "dxstream plugin still not found." "ERROR"
            print_colored "Please build DX-STREAM first:" "ERROR"
            print_colored "  $ ./build.sh" "INFO"
            print_colored "Or if already built, try:" "INFO"
            print_colored "  $ source ~/.bashrc" "INFO"
            print_colored "  $ ./run_demo.sh" "INFO"
            exit 1
        fi
    else
        print_colored "~/.bashrc not found. Please build DX-STREAM first:" "ERROR"
        print_colored "  $ ./build.sh" "INFO"
        exit 1
    fi
fi

check_valid_dir_or_symlink() {
    local path="$1"
    if [ -d "$path" ] || { [ -L "$path" ] && [ -d "$(readlink -f "$path")" ]; }; then
        return 0
    else
        return 1
    fi
}

if check_valid_dir_or_symlink "./dx_stream/samples/models" && check_valid_dir_or_symlink "./dx_stream/samples/videos"; then
    print_colored "Models and Videos directory already exists. Skipping download." "INFO"
else
    print_colored "Models and Videos not found. Downloading now via setup.sh..." "INFO"
    rm -rf ./dx_stream/samples
    ./setup.sh
fi

WRC=$DX_STREAM_PATH

echo "0: Object Detection (YOLOv26n)"
echo "1: Object Detection (YOLOv5s with PPU)"
echo "2: Face Detection (YOLOv5s_Face)"
echo "3: Face Detection (SCRFD500M with PPU)"
echo "4: Pose Estimation (YOLOv26n_Pose)"
echo "5: Pose Estimation (YOLOV5Pose with PPU)"
echo "6: Semantic Segmentation (YOLOv26n_Seg)"
echo "7: Multi-Object Tracking"
echo "8: Multi-Channel Object Detection"
echo "9: Multi-Channel Object Detection (RTSP)"
echo "-: secondary mode"

read -t 10 -p "which AI demo do you want to run:(timeout:10s, default:0)" select

case $select in
    0)$WRC/dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh;;
    1)$WRC/dx_stream/pipelines/single_network/object_detection/run_YoloV5S_PPU.sh;;
    2)$WRC/dx_stream/pipelines/single_network/face_detection/run_YOLOv5s_Face.sh;;
    3)$WRC/dx_stream/pipelines/single_network/face_detection/run_SCRFD500M_PPU.sh;;
    4)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_yolo26n-pose.sh;;
    5)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_YOLOV5Pose_PPU.sh;;
    6)$WRC/dx_stream/pipelines/single_network/semantic_segmentation/run_yolo26n-seg.sh;;
    7)$WRC/dx_stream/pipelines/tracking/run_multi_object_tracker.sh;;
    8)$WRC/dx_stream/pipelines/multi_stream/run_multi_stream.sh;;
    9)$WRC/dx_stream/pipelines/rtsp/run_RTSP.sh $INTERNAL_RTSP_ARG;;
    -)$WRC/dx_stream/pipelines/secondary_mode/run_secondary_mode.sh;;
    *)$WRC/dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh;;
esac

popd
