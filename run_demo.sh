#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
DX_STREAM_PATH=$(realpath -s "${SCRIPT_DIR}")

# color env settings
source "${DX_STREAM_PATH}/scripts/color_env.sh"
source "${DX_STREAM_PATH}/scripts/common_util.sh"
source "${DX_STREAM_PATH}/scripts/rpi_camera_util.sh"

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

# Raspberry Pi camera demos are listed when the libcamera element and a CSI camera are present.
RPI_MENU=0
if is_raspberry_pi; then
    if ! has_libcamerasrc; then
        print_colored "Raspberry Pi camera demos need gstreamer1.0-libcamera: sudo apt install gstreamer1.0-libcamera" "INFO"
    elif has_rpi_camera; then
        RPI_MENU=1
    fi
fi

echo "0: Object Detection (YOLO26n)"
echo "1: Object Detection (YOLOv5s with PPU)"
echo "2: Face Detection (YOLOv5s_Face)"
echo "3: Pose Estimation (YOLO26n_Pose)"
echo "4: Instance Segmentation (YOLO26n_Seg)"
echo "5: Depth Estimation (YOLO26n_Depth)"
echo "6: Multi-Object Tracking"
echo "7: Multi-Channel Object Detection"
echo "8: Multi-Channel Object Detection (RTSP)"
echo "9: Secondary Mode (Multi-Model Cascade)"
if [ "$RPI_MENU" = 1 ]; then
    echo "[Raspberry Pi Camera]"
    echo "c: Object Detection (YOLO26n)"
    echo "p: Pose Estimation (YOLO26n_Pose)"
    echo "s: Instance Segmentation (YOLO26n_Seg)"
    echo "d: Depth Estimation (YOLO26n_Depth)"
fi

read -t 10 -p "which AI demo do you want to run:(timeout:10s, default:0)" select

RPI_DEMO=""
if [ "$RPI_MENU" = 1 ]; then
    case $select in
        c) RPI_DEMO=run_rpi_camera_yolo26n.sh;;
        p) RPI_DEMO=run_rpi_camera_yolo26n-pose.sh;;
        s) RPI_DEMO=run_rpi_camera_yolo26n-seg.sh;;
        d) RPI_DEMO=run_rpi_camera_yolo26n-depth.sh;;
    esac
fi

if [ -n "$RPI_DEMO" ]; then
    "$WRC/dx_stream/pipelines/raspberry_pi/$RPI_DEMO"
else
case $select in
    0)$WRC/dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh;;
    1)$WRC/dx_stream/pipelines/single_network/object_detection/run_YoloV5S_PPU.sh;;
    2)$WRC/dx_stream/pipelines/single_network/face_detection/run_YOLOv5s_Face.sh;;
    3)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_yolo26n-pose.sh;;
    4)$WRC/dx_stream/pipelines/single_network/instance_segmentation/run_yolo26n-seg.sh;;
    5)$WRC/dx_stream/pipelines/single_network/depth_estimation/run_yolo26n-depth.sh;;
    6)$WRC/dx_stream/pipelines/tracking/run_multi_object_tracker.sh;;
    7)$WRC/dx_stream/pipelines/multi_stream/run_multi_stream.sh;;
    8)$WRC/dx_stream/pipelines/rtsp/run_RTSP.sh $INTERNAL_RTSP_ARG;;
    9)$WRC/dx_stream/pipelines/secondary_mode/run_secondary_mode.sh;;
    *)$WRC/dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh;;
esac
fi

popd
