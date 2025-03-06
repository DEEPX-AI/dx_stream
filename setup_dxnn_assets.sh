#!/bin/bash
function help()
{
    echo "./build.sh"
    echo "    --help     show this help"
    echo "    --number   enter the regression ID."
}

regrID=3148

[ $# -gt 0 ] && \
while (( $# )); do
    case "$1" in
        --help)  help; exit 0;;
        --number) 
            shift
            regrID=$1
            shift;;
        *)       echo "Invalid argument : " $1 ; help; exit 1;;
    esac
done

mkdir -p dx_stream/samples/models
mkdir -p dx_stream/samples/videos

sudo mkdir -p /mnt/regression_storage
sudo mount -o nolock 192.168.30.201:/do/regression /mnt/regression_storage

for x in \
DeepLabV3PlusMobileNetV2_2 \
EfficientNetB0_4 \
EfficientNetB0_8 \
MobileNetV2_2 \
SCRFD500M_1 \
YOLOV5Pose640_1 \
YOLOV5S_1 \
YOLOV5S_3 \
YOLOV5S_4 \
YOLOV5S_6 \
YOLOV5X_2 \
YOLOv7_512 \
YoloV7 \
YoloV8N \
;\
do cp -r /mnt/regression_storage/dxnn_regr_data/M1A/$regrID/$x-*/$x.dxnn ./dx_stream/samples/models/ \
&& echo "$x --> DONE";\
done

for x in \
osnet_x0_5_market_256x128 \
;\
do cp -r /mnt/regression_storage/atd/dx_stream/Models/$x.dxnn ./dx_stream/samples/models/ \
&& echo "$x --> DONE";\
done

for file in /mnt/regression_storage/atd/dx_stream/Videos/*; do
    if [ -f "$file" ]; then
        cp -r "$file" ./dx_stream/samples/videos/ && echo "$(basename "$file") --> DONE"
    fi
done