#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
DX_STREAM_PATH=$(realpath -s "${SCRIPT_DIR}")

# color env settings
source "${DX_STREAM_PATH}/scripts/color_env.sh"
source "${DX_STREAM_PATH}/scripts/common_util.sh"

pushd $DX_STREAM_PATH

print_colored "DX_STREAM_PATH: $DX_STREAM_PATH" "INFO"

if ! test -e "/usr/local/lib/libgstdxstream.so"; then
    print_colored "dx_stream is not built. Building dx_stream first before running the demo." "INFO"
    ./build.sh
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

echo "0: Object Detection (YOLOv5s)"
echo "1: Object Detection (YOLOv5s with PPU)"
echo "2: Face Detection (YOLOV5S_Face)"
echo "3: Face Detection (SCRFD500M with PPU)"
echo "4: Pose Estimation (YOLOV5Pose)"
echo "5: Pose Estimation (YOLOV5Pose with PPU)"
echo "6: Multi-Object Tracking"
echo "7: Semantic Segmentation"
echo "8: Multi-Channel Object Detection"
echo "9: Multi-Channel RTSP"
echo "-: secondary mode"

read -t 10 -p "which AI demo do you want to run:(timeout:10s, default:0)" select

case $select in
    0)$WRC/dx_stream/pipelines/single_network/object_detection/run_YOLOV5S.sh;;
    1)$WRC/dx_stream/pipelines/single_network/object_detection/run_YOLOV5S_PPU.sh;;
    2)$WRC/dx_stream/pipelines/single_network/face_detection/run_YOLOFACE.sh;;
    3)$WRC/dx_stream/pipelines/single_network/face_detection/run_SCRFD500M_PPU.sh;;
    4)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_YOLOV5Pose.sh;;
    5)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_YOLOV5Pose_PPU.sh;;
    6)$WRC/dx_stream/pipelines/tracking/run_YOLOV5S_tracker.sh;;
    7)$WRC/dx_stream/pipelines/single_network/semantic_segmentation/run_DeepLabV3PlusMobileNetV2.sh;;
    8)$WRC/dx_stream/pipelines/multi_stream/run_multi_stream_YOLOV5S.sh;;
    9)$WRC/dx_stream/pipelines/rtsp/run_RTSP.sh;;
    -)$WRC/dx_stream/pipelines/secondary_mode/run_secondary_mode.sh;;
    *)$WRC/dx_stream/pipelines/single_network/object_detection/run_YOLOV5S.sh;;
esac

popd
