#!/bin/bash

WRC=$(pwd)

echo "0: Object Detection (YOLOv7)"
echo "1: Multi-Object Tracking"
echo "2: Pose Estimation"
echo "3: Semantic Segmentation"
echo "4: Multi-Channel Object Detection"
echo "5: Multi-Channel RTSP"
echo "6: Msgbroker (mqtt)"
echo "7: Msgbroker (kafka)"
echo "8: secondary mode"

read -t 10 -p "which AI demo do you want to run:(timeout:10s, default:0)" select

case $select in
	0)$WRC/dx_stream/pipelines/single_network/object_detection/run_YOLOV7.sh;;
    1)$WRC/dx_stream/pipelines/tracking/run_YOLOV5S_tracker.sh;;
    2)$WRC/dx_stream/pipelines/single_network/pose_estimation/run_YOLOPOSE.sh;;
	3)$WRC/dx_stream/pipelines/single_network/semantic_segmentation/run_PIDNET.sh;;
	4)$WRC/dx_stream/pipelines/multi_stream/run_multi_stream_YOLOV5S.sh;;
    5)$WRC/dx_stream/pipelines/rtsp/run_RTSP.sh;;
	6)$WRC/dx_stream/pipelines/broker/run_dxmsgbroker_mqtt.sh;;
	7)$WRC/dx_stream/pipelines/broker/run_dxmsgbroker_kafka.sh;;
	8)$WRC/dx_stream/pipelines/secondary_mode/run_secondary_mode.sh;;
	*)$WRC/dx_stream/pipelines/single_network/object_detection/run_YOLOV7.sh;;
esac
