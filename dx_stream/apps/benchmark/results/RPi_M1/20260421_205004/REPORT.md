# YOLO26 Benchmark Report

**Generated:** 2026-04-22 13:49:10 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-21 20:50:04 | 2026-04-22 13:49:10 | 16h 59m 5s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 30.52 | 91.4 | 66.6 | 2 |
| yolo26n.dxnn | OFF | 28.38 | 91.5 | 86.6 | 2 |
| yolo26s.dxnn | ON | 37.74 | 91.5 | 68.6 | 2 |
| yolo26s.dxnn | OFF | 36.54 | 91.4 | 86.3 | 2 |
| yolo26m.dxnn | ON | 45.27 | 90.2 | 68.5 | 2 |
| yolo26m.dxnn | OFF | 42.32 | 91.3 | 86.0 | 2 |
| yolo26l.dxnn | ON | 53.23 | 71.5 | 68.0 | 2 |
| yolo26l.dxnn | OFF | 49.68 | 70.3 | 71.0 | 2 |
| yolo26x.dxnn | ON | 79.56 | 40.2 | 36.0 | 1 |
| yolo26x.dxnn | OFF | 76.88 | 40.5 | 36.0 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 24.78 | 135.4 | 83.7 | 2 |
| yolo26n-pose.dxnn | OFF | 23.17 | 135.3 | 124.7 | 4 |
| yolo26s-pose.dxnn | ON | 32.10 | 126.0 | 83.7 | 2 |
| yolo26s-pose.dxnn | OFF | 30.10 | 130.4 | 121.1 | 4 |
| yolo26m-pose.dxnn | ON | 38.72 | 93.8 | 83.4 | 2 |
| yolo26m-pose.dxnn | OFF | 37.48 | 93.1 | 93.5 | 2 |
| yolo26l-pose.dxnn | ON | 46.83 | 68.6 | 69.2 | 2 |
| yolo26l-pose.dxnn | OFF | 45.38 | 68.7 | 68.5 | 2 |
| yolo26x-pose.dxnn | ON | 75.64 | 39.6 | 36.5 | 1 |
| yolo26x-pose.dxnn | OFF | 72.92 | 39.4 | 35.5 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 51.17 | 48.7 | 47.1 | 1 |
| yolo26n-seg.dxnn | OFF | 47.45 | 48.7 | 48.7 | 1 |
| yolo26s-seg.dxnn | ON | 58.92 | 48.7 | 47.4 | 1 |
| yolo26s-seg.dxnn | OFF | 56.01 | 48.7 | 48.8 | 1 |
| yolo26m-seg.dxnn | ON | 76.29 | 48.6 | 47.0 | 1 |
| yolo26m-seg.dxnn | OFF | 69.23 | 48.7 | 48.7 | 1 |
| yolo26l-seg.dxnn | ON | 80.73 | 48.6 | 47.2 | 1 |
| yolo26l-seg.dxnn | OFF | 78.12 | 48.7 | 48.6 | 1 |
| yolo26x-seg.dxnn | ON | 120.25 | 30.1 | 27.4 | — |
| yolo26x-seg.dxnn | OFF | 116.81 | 30.5 | 24.1 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 44.71 | 80.2 | 72.5 | 2 |
| yolo26n-obb.dxnn | OFF | 42.97 | 78.3 | 79.4 | 2 |
| yolo26s-obb.dxnn | ON | 62.42 | 46.4 | 46.4 | 1 |
| yolo26s-obb.dxnn | OFF | 60.21 | 46.4 | 46.4 | 1 |
| yolo26m-obb.dxnn | ON | 80.21 | 34.0 | 34.2 | 1 |
| yolo26m-obb.dxnn | OFF | 78.88 | 34.1 | 34.2 | 1 |
| yolo26l-obb.dxnn | ON | 100.87 | 24.8 | 25.0 | — |
| yolo26l-obb.dxnn | OFF | 98.30 | 24.9 | 25.0 | — |
| yolo26x-obb.dxnn | ON | 171.31 | 14.4 | 12.9 | — |
| yolo26x-obb.dxnn | OFF | 170.26 | 14.4 | 13.0 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.48 | 2678.3 | 189.8 | — |
| yolo26n-cls.dxnn | OFF | 1.47 | 2680.4 | 189.7 | — |
| yolo26s-cls.dxnn | ON | 2.16 | 1936.8 | 190.0 | — |
| yolo26s-cls.dxnn | OFF | 2.15 | 1936.6 | 190.4 | — |
| yolo26m-cls.dxnn | ON | 2.73 | 1360.4 | 190.2 | — |
| yolo26m-cls.dxnn | OFF | 2.75 | 1359.3 | 190.3 | — |
| yolo26l-cls.dxnn | ON | 4.03 | 859.6 | 189.6 | — |
| yolo26l-cls.dxnn | OFF | 4.03 | 859.5 | 189.3 | — |
| yolo26x-cls.dxnn | ON | 6.56 | 463.2 | 190.2 | — |
| yolo26x-cls.dxnn | OFF | 6.64 | 463.4 | 189.9 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | dx-rpi |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.12.75+rpt-rpi-2712 |
| CPU | Cortex-A76 |
| CPU Cores | 4 |
| RAM | 7.9 GB |
| NPU SKU | M1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen2 X1 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.3.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.0 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.0 |
| dxtop | Yes | DX-TOP 1.1.0 |
| ffprobe | Yes | ffprobe version 5.1.8-0+deb12u1+rpt1 Copyright (c) 2007-2025... |

## Benchmark Parameters

| Parameter | Value |
|-----------|-------|
| Throughput duration (-t) | 30 sec |
| Latency loops (-l) | 300 |
| Model warmup runs | 1 |
| Model latency runs | 1 |
| Model throughput runs | 3 |
| E2E pipeline runs | 3 |
| Multi-stream FPS threshold | 30.0 fps |
| ORT modes | ON, OFF |
| Benchmark families | all |

## Measurement Protocol

| Item | Value |
|------|-------|
| Version | v2 |
| Thermal Mode | steady |
| Throughput Time | 30 s |
| Latency Loops | 300 |
| Model Warmup | 1 |
| Model Latency Runs | 1 |
| Model Throughput Runs | 3 |
| E2E Runs | 3 |
| FPS Threshold | 30.0 |
| Multi-Stream Search | single-stream-estimate-linear-boundary |
| Stable Capacity Rule | status_ok_and_all_runs_success_and_avg_per_channel_fps_ge_threshold |
| Cooldown Target ΔT | 10.0 °C |
| Cooldown Max Time | 300.0 s |
| NPU Warmup | 1.0 s |
| NPU Drain | 0.5 s |

## Benchmarked Models

| Model | Task | Input Size | NPU Memory (MB) | ORT CPU Offload | Multi-Stream Sweep |
|-------|------|------------|:----------------:|:---------------:|:------------------:|
| yolo26n.dxnn | Object Detection | 640×640 | 116.5 | Yes | ✅ |
| yolo26s.dxnn | Object Detection | 640×640 | 151.6 | Yes | ✅ |
| yolo26m.dxnn | Object Detection | 640×640 | 241.9 | Yes | ✅ |
| yolo26l.dxnn | Object Detection | 640×640 | 293.5 | Yes | ✅ |
| yolo26x.dxnn | Object Detection | 640×640 | 522.7 | Yes | ✅ |
| yolo26n-pose.dxnn | Pose Estimation | 640×640 | 118.5 | Yes | ✅ |
| yolo26s-pose.dxnn | Pose Estimation | 640×640 | 158.7 | Yes | ✅ |
| yolo26m-pose.dxnn | Pose Estimation | 640×640 | 254.0 | Yes | ✅ |
| yolo26l-pose.dxnn | Pose Estimation | 640×640 | 305.6 | Yes | ✅ |
| yolo26x-pose.dxnn | Pose Estimation | 640×640 | 516.5 | Yes | ✅ |
| yolo26n-seg.dxnn | Segmentation | 640×640 | 138.8 | Yes | ✅ |
| yolo26s-seg.dxnn | Segmentation | 640×640 | 177.6 | Yes | ✅ |
| yolo26m-seg.dxnn | Segmentation | 640×640 | 270.2 | Yes | ✅ |
| yolo26l-seg.dxnn | Segmentation | 640×640 | 321.8 | Yes | ✅ |
| yolo26x-seg.dxnn | Segmentation | 640×640 | 555.0 | Yes | ✅ |
| yolo26n-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 243.9 | Yes | ✅ |
| yolo26s-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 380.4 | Yes | ✅ |
| yolo26m-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 645.3 | Yes | ✅ |
| yolo26l-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 790.1 | Yes | ✅ |
| yolo26x-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 1316.5 | Yes | ✅ |
| yolo26n-cls.dxnn | Classification | 224×224 | 4.5 | No | — |
| yolo26s-cls.dxnn | Classification | 224×224 | 9.0 | No | — |
| yolo26m-cls.dxnn | Classification | 224×224 | 15.8 | No | — |
| yolo26l-cls.dxnn | Classification | 224×224 | 19.5 | No | — |
| yolo26x-cls.dxnn | Classification | 224×224 | 48.6 | No | — |

## Input Videos

### Object Detection / Pose / Segmentation

| Item | Value |
|------|-------|
| File | od_benchmark_video.mp4 |
| Resolution | 1920 x 1080 |
| Codec | h264 |
| FPS | 30.0 |
| Frames | 3455 |
| Duration | 115.3 sec |
| Bitrate | 4.47 Mbps |
| Format | QuickTime / MOV |
| Pixel Format | yuv420p |

### Oriented BBox (OBB)

| Item | Value |
|------|-------|
| File | obb_benchmark_video.mp4 |
| Resolution | 1920 x 1080 |
| Codec | h264 |
| FPS | 30.0 |
| Frames | 2640 |
| Duration | 88.0 sec |
| Bitrate | 4.87 Mbps |
| Format | QuickTime / MOV |
| Pixel Format | yuv420p |

### Classification

| Item | Value |
|------|-------|
| File | od_benchmark_video.mp4 |
| Resolution | 1920 x 1080 |
| Codec | h264 |
| FPS | 30.0 |
| Frames | 3455 |
| Duration | 115.3 sec |
| Bitrate | 4.47 Mbps |
| Format | QuickTime / MOV |
| Pixel Format | yuv420p |

## Model-Level Benchmarks

### Throughput (Multi-Core, Async)

#### Object Detection

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 91.4 ±0.0 | 140 | 23.4 | 74.8 | 45~47 | 1000 | ok |
| yolo26s.dxnn | 91.5 ±0.1 | 129 | 46.9 | 77.8 | 53~55 | 1000 | ok |
| yolo26m.dxnn | 90.2 ±0.2 | 130 | 83.0 | 92.8 | 64~70 | 1000 | ok |
| yolo26l.dxnn | 71.5 ±0.0 | 98 | 92.1 | 100.0 | 66~72 | 1000 | ok |
| yolo26x.dxnn | 40.2 ±0.3 | 57 | 89.0 | 100.0 | 69~76 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 91.5 ±0.0 | 52 | 24.2 | 74.1 | 49 | 1000 | ok |
| yolo26s.dxnn | 91.4 ±0.0 | 54 | 46.4 | 77.8 | 54~56 | 1000 | ok |
| yolo26m.dxnn | 91.3 ±0.2 | 50 | 82.7 | 93.8 | 65~71 | 1000 | ok |
| yolo26l.dxnn | 70.3 ±0.2 | 49 | 88.9 | 100.0 | 66~72 | 1000 | ok |
| yolo26x.dxnn | 40.5 ±0.3 | 29 | 89.2 | 100.0 | 69~76 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 135.4 ±0.2 | 120 | 39.7 | 71.0 | 54~55 | 1000 | ok |
| yolo26s-pose.dxnn | 126.0 ±1.5 | 125 | 81.3 | 99.6 | 58~62 | 1000 | ok |
| yolo26m-pose.dxnn | 93.8 ±0.3 | 87 | 92.5 | 100.0 | 66~73 | 1000 | ok |
| yolo26l-pose.dxnn | 68.6 ±0.2 | 67 | 91.3 | 100.0 | 65~71 | 1000 | ok |
| yolo26x-pose.dxnn | 39.6 ±0.4 | 39 | 89.5 | 100.0 | 68~75 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 135.3 ±0.2 | 58 | 40.6 | 67.3 | 53~55 | 1000 | ok |
| yolo26s-pose.dxnn | 130.4 ±0.8 | 58 | 88.1 | 100.0 | 60~63 | 1000 | ok |
| yolo26m-pose.dxnn | 93.1 ±0.2 | 46 | 91.3 | 100.0 | 66~73 | 1000 | ok |
| yolo26l-pose.dxnn | 68.7 ±0.4 | 33 | 90.0 | 100.0 | 65~71 | 1000 | ok |
| yolo26x-pose.dxnn | 39.4 ±0.1 | 18 | 89.6 | 100.0 | 68~75 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 48.7 ±0.0 | 100 | 15.0 | 49.0 | 50 | 1000 | ok |
| yolo26s-seg.dxnn | 48.7 ±0.0 | 100 | 28.1 | 91.0 | 52~53 | 1000 | ok |
| yolo26m-seg.dxnn | 48.6 ±0.1 | 100 | 53.9 | 89.9 | 63~68 | 1000 | ok |
| yolo26l-seg.dxnn | 48.6 ±0.1 | 99 | 71.2 | 83.8 | 65~71 | 1000 | ok |
| yolo26x-seg.dxnn | 30.1 ±0.3 | 75 | 89.0 | 100.0 | 70~78 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 48.7 ±0.0 | 58 | 14.9 | 49.0 | 48 | 1000 | ok |
| yolo26s-seg.dxnn | 48.7 ±0.0 | 58 | 29.6 | 90.9 | 54~55 | 1000 | ok |
| yolo26m-seg.dxnn | 48.7 ±0.0 | 62 | 53.5 | 90.3 | 63~68 | 1000 | ok |
| yolo26l-seg.dxnn | 48.7 ±0.1 | 57 | 73.9 | 85.3 | 64~70 | 1000 | ok |
| yolo26x-seg.dxnn | 30.5 ±0.2 | 44 | 88.8 | 100.0 | 70~78 | 800~1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 80.2 ±0.0 | 76 | 90.7 | 100.0 | 56~58 | 1000 | ok |
| yolo26s-obb.dxnn | 46.4 ±0.2 | 50 | 90.3 | 100.0 | 58~61 | 1000 | ok |
| yolo26m-obb.dxnn | 34.0 ±0.1 | 36 | 89.3 | 100.0 | 64~69 | 1000 | ok |
| yolo26l-obb.dxnn | 24.8 ±0.1 | 26 | 88.6 | 100.0 | 64~69 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.0 | 15 | 87.9 | 100.0 | 67~73 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 78.3 ±0.3 | 41 | 87.3 | 100.0 | 56~58 | 1000 | ok |
| yolo26s-obb.dxnn | 46.4 ±0.1 | 24 | 89.6 | 100.0 | 57~61 | 1000 | ok |
| yolo26m-obb.dxnn | 34.1 ±0.1 | 18 | 88.4 | 100.0 | 64~69 | 1000 | ok |
| yolo26l-obb.dxnn | 24.9 ±0.2 | 12 | 89.4 | 100.0 | 65~70 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.0 | 7 | 87.5 | 100.0 | 68~74 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 2678.3 ±1.7 | 55 | 65.5 | 72.6 | 52~53 | 1000 | ok |
| yolo26s-cls.dxnn | 1936.8 ±1.1 | 40 | 89.7 | 97.4 | 52~55 | 1000 | ok |
| yolo26m-cls.dxnn | 1360.4 ±0.5 | 30 | 89.2 | 97.8 | 60~66 | 1000 | ok |
| yolo26l-cls.dxnn | 859.6 ±0.5 | 19 | 90.2 | 98.5 | 59~63 | 1000 | ok |
| yolo26x-cls.dxnn | 463.2 ±0.2 | 10 | 91.8 | 99.5 | 62~67 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 2680.4 ±0.6 | 56 | 64.9 | 72.6 | 49~50 | 1000 | ok |
| yolo26s-cls.dxnn | 1936.6 ±2.8 | 40 | 88.6 | 97.4 | 53~56 | 1000 | ok |
| yolo26m-cls.dxnn | 1359.3 ±1.5 | 30 | 88.7 | 97.7 | 62~67 | 1000 | ok |
| yolo26l-cls.dxnn | 859.5 ±0.3 | 19 | 89.0 | 98.0 | 59~63 | 1000 | ok |
| yolo26x-cls.dxnn | 463.4 ±0.1 | 10 | 90.0 | 99.3 | 58~65 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 32.8 | 30.52 | 27.93 | 2.59 | 40 | ok |
| yolo26s.dxnn | 26.5 | 37.74 | 35.15 | 2.59 | 48 | ok |
| yolo26m.dxnn | 22.1 | 45.27 | 42.72 | 2.55 | 51 | ok |
| yolo26l.dxnn | 18.8 | 53.23 | 50.57 | 2.66 | 51 | ok |
| yolo26x.dxnn | 12.6 | 79.56 | 76.99 | 2.58 | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 35.2 | 28.38 | 28.38 | 47 | ok |
| yolo26s.dxnn | 27.4 | 36.54 | 36.54 | 49 | ok |
| yolo26m.dxnn | 23.6 | 42.32 | 42.32 | 51 | ok |
| yolo26l.dxnn | 20.1 | 49.68 | 49.68 | 52 | ok |
| yolo26x.dxnn | 13.0 | 76.88 | 76.88 | 53 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 40.4 | 24.78 | 23.23 | 1.55 | 50 | ok |
| yolo26s-pose.dxnn | 31.1 | 32.10 | 30.53 | 1.57 | 50 | ok |
| yolo26m-pose.dxnn | 25.8 | 38.72 | 37.23 | 1.49 | 51 | ok |
| yolo26l-pose.dxnn | 21.4 | 46.83 | 45.32 | 1.51 | 52 | ok |
| yolo26x-pose.dxnn | 13.2 | 75.64 | 74.09 | 1.56 | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 43.2 | 23.17 | 23.17 | 49 | ok |
| yolo26s-pose.dxnn | 33.2 | 30.10 | 30.10 | 50 | ok |
| yolo26m-pose.dxnn | 26.7 | 37.48 | 37.48 | 52 | ok |
| yolo26l-pose.dxnn | 22.0 | 45.38 | 45.38 | 52 | ok |
| yolo26x-pose.dxnn | 13.7 | 72.92 | 72.92 | 53 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 19.5 | 51.17 | 48.02 | 3.15 | 49 | ok |
| yolo26s-seg.dxnn | 17.0 | 58.92 | 55.89 | 3.03 | 48 | ok |
| yolo26m-seg.dxnn | 13.1 | 76.29 | 73.29 | 3.00 | 51 | ok |
| yolo26l-seg.dxnn | 12.4 | 80.73 | 77.67 | 3.06 | 51 | ok |
| yolo26x-seg.dxnn | 8.3 | 120.25 | 117.12 | 3.13 | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 21.1 | 47.45 | 47.45 | 47 | ok |
| yolo26s-seg.dxnn | 17.9 | 56.01 | 56.01 | 50 | ok |
| yolo26m-seg.dxnn | 14.4 | 69.23 | 69.23 | 51 | ok |
| yolo26l-seg.dxnn | 12.8 | 78.12 | 78.12 | 50 | ok |
| yolo26x-seg.dxnn | 8.6 | 116.81 | 116.81 | 52 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 22.4 | 44.71 | 42.92 | 1.79 | 49 | ok |
| yolo26s-obb.dxnn | 16.0 | 62.42 | 60.64 | 1.78 | 50 | ok |
| yolo26m-obb.dxnn | 12.5 | 80.21 | 78.48 | 1.73 | 52 | ok |
| yolo26l-obb.dxnn | 9.9 | 100.87 | 99.14 | 1.73 | 53 | ok |
| yolo26x-obb.dxnn | 5.8 | 171.31 | 169.48 | 1.83 | 55 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 23.3 | 42.97 | 42.97 | 50 | ok |
| yolo26s-obb.dxnn | 16.6 | 60.21 | 60.21 | 50 | ok |
| yolo26m-obb.dxnn | 12.7 | 78.88 | 78.88 | 52 | ok |
| yolo26l-obb.dxnn | 10.2 | 98.30 | 98.30 | 53 | ok |
| yolo26x-obb.dxnn | 5.9 | 170.26 | 170.26 | 55 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 675.7 | 1.48 | 1.48 | N/A | 50 | ok |
| yolo26s-cls.dxnn | 462.5 | 2.16 | 2.16 | N/A | 44 | ok |
| yolo26m-cls.dxnn | 366.9 | 2.73 | 2.73 | N/A | 47 | ok |
| yolo26l-cls.dxnn | 248.1 | 4.03 | 4.03 | N/A | 49 | ok |
| yolo26x-cls.dxnn | 152.4 | 6.56 | 6.56 | N/A | 50 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 679.0 | 1.47 | 1.47 | 45 | ok |
| yolo26s-cls.dxnn | 465.3 | 2.15 | 2.15 | 46 | ok |
| yolo26m-cls.dxnn | 363.8 | 2.75 | 2.75 | 50 | ok |
| yolo26l-cls.dxnn | 247.9 | 4.03 | 4.03 | 49 | ok |
| yolo26x-cls.dxnn | 150.6 | 6.64 | 6.64 | 45 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | avdec_h264 | 3455 | 3 | 66.6 ±0.2 | 51.85 | 336 | 17.3 | 54.2 | 47 | 1000 | 281 | ok |
| yolo26s.dxnn | avdec_h264 | 3455 | 3 | 68.6 ±0.2 | 50.39 | 336 | 33.5 | 70.2 | 54 | 1000 | 302 | ok |
| yolo26m.dxnn | avdec_h264 | 3455 | 3 | 68.5 ±0.3 | 50.47 | 331 | 53.5 | 83.3 | 68~70 | 1000 | 319 | ok |
| yolo26l.dxnn | avdec_h264 | 3455 | 3 | 68.0 ±0.4 | 50.84 | 293 | 86.3 | 97.5 | 57~77 | 1000 | 326 | ok |
| yolo26x.dxnn | avdec_h264 | 3455 | 3 | 36.0 ±2.0 | 95.92 | 113 | 94.6 | 100.0 | 82~83 | 600~1000 | 389 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | avdec_h264 | 3455 | 3 | 86.6 ±0.6 | 39.89 | 310 | 22.8 | 67.6 | 48~49 | 1000 | 316 | ok |
| yolo26s.dxnn | avdec_h264 | 3455 | 3 | 86.3 ±0.4 | 40.02 | 308 | 44.5 | 75.3 | 56~57 | 1000 | 330 | ok |
| yolo26m.dxnn | avdec_h264 | 3455 | 3 | 86.0 ±0.2 | 40.17 | 306 | 78.2 | 89.4 | 73~75 | 1000 | 351 | ok |
| yolo26l.dxnn | avdec_h264 | 3455 | 3 | 71.0 ±0.3 | 48.64 | 232 | 93.1 | 100.0 | 76~80 | 1000 | 338 | ok |
| yolo26x.dxnn | avdec_h264 | 3455 | 3 | 36.0 ±1.3 | 96.09 | 113 | 94.4 | 100.0 | 82 | 600~1000 | 400 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 66.6 | 86.6 | -20.0 | -23.1% |
| yolo26s.dxnn | 68.6 | 86.3 | -17.8 | -20.6% |
| yolo26m.dxnn | 68.5 | 86.0 | -17.5 | -20.4% |
| yolo26l.dxnn | 68.0 | 71.0 | -3.1 | -4.3% |
| yolo26x.dxnn | 36.0 | 36.0 | +0.1 | +0.2% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | avdec_h264 | 3455 | 3 | 83.7 ±0.1 | 41.29 | 344 | 23.5 | 62.7 | 45~47 | 1000 | 269 | ok |
| yolo26s-pose.dxnn | avdec_h264 | 3455 | 3 | 83.7 ±0.3 | 41.27 | 342 | 47.1 | 78.0 | 59~60 | 1000 | 286 | ok |
| yolo26m-pose.dxnn | avdec_h264 | 3455 | 3 | 83.4 ±0.3 | 41.45 | 330 | 77.4 | 91.2 | 74~76 | 1000 | 310 | ok |
| yolo26l-pose.dxnn | avdec_h264 | 3455 | 3 | 69.2 ±0.2 | 49.93 | 190 | 93.9 | 100.0 | 76~79 | 1000 | 321 | ok |
| yolo26x-pose.dxnn | avdec_h264 | 3455 | 3 | 36.5 ±1.3 | 94.79 | 95 | 94.9 | 100.0 | 82 | 600~1000 | 382 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | avdec_h264 | 3455 | 3 | 124.7 ±0.9 | 27.70 | 311 | 37.9 | 67.9 | 54 | 1000 | 258 | ok |
| yolo26s-pose.dxnn | avdec_h264 | 3455 | 3 | 121.1 ±1.0 | 28.53 | 283 | 77.0 | 91.6 | 63~64 | 1000 | 278 | ok |
| yolo26m-pose.dxnn | avdec_h264 | 3455 | 3 | 93.5 ±0.2 | 36.97 | 186 | 91.0 | 100.0 | 76~79 | 1000 | 299 | ok |
| yolo26l-pose.dxnn | avdec_h264 | 3455 | 3 | 68.5 ±0.2 | 50.47 | 133 | 93.4 | 100.0 | 76~79 | 1000 | 313 | ok |
| yolo26x-pose.dxnn | avdec_h264 | 3455 | 3 | 35.5 ±0.7 | 97.32 | 72 | 95.2 | 100.0 | 82~83 | 600~1000 | 372 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 83.7 | 124.7 | -41.1 | -32.9% |
| yolo26s-pose.dxnn | 83.7 | 121.1 | -37.4 | -30.9% |
| yolo26m-pose.dxnn | 83.4 | 93.5 | -10.1 | -10.8% |
| yolo26l-pose.dxnn | 69.2 | 68.5 | +0.7 | +1.1% |
| yolo26x-pose.dxnn | 36.5 | 35.5 | +1.0 | +2.7% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | avdec_h264 | 3455 | 3 | 47.1 ±0.1 | 73.36 | 337 | 15.2 | 49.0 | 48~49 | 1000 | 379 | ok |
| yolo26s-seg.dxnn | avdec_h264 | 3455 | 3 | 47.4 ±0.2 | 72.87 | 332 | 28.8 | 78.2 | 55~56 | 1000 | 394 | ok |
| yolo26m-seg.dxnn | avdec_h264 | 3455 | 3 | 47.0 ±0.1 | 73.46 | 308 | 53.2 | 85.0 | 73~75 | 1000 | 416 | ok |
| yolo26l-seg.dxnn | avdec_h264 | 3455 | 3 | 47.2 ±0.1 | 73.12 | 280 | 73.3 | 86.5 | 77~80 | 800~1000 | 423 | ok |
| yolo26x-seg.dxnn | avdec_h264 | 3455 | 3 | 27.4 ±3.1 | 126.14 | 126 | 94.8 | 100.0 | 74~82 | 600~1000 | 494 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | avdec_h264 | 3455 | 3 | 48.7 ±0.0 | 70.90 | 216 | 16.0 | 49.0 | 48 | 1000 | 376 | ok |
| yolo26s-seg.dxnn | avdec_h264 | 3455 | 3 | 48.8 ±0.0 | 70.87 | 200 | 30.0 | 78.2 | 56~57 | 1000 | 392 | ok |
| yolo26m-seg.dxnn | avdec_h264 | 3455 | 3 | 48.7 ±0.0 | 70.89 | 208 | 55.6 | 89.2 | 65~71 | 1000 | 420 | ok |
| yolo26l-seg.dxnn | avdec_h264 | 3455 | 3 | 48.6 ±0.1 | 71.07 | 214 | 76.5 | 88.2 | 76~80 | 800~1000 | 437 | ok |
| yolo26x-seg.dxnn | avdec_h264 | 3455 | 3 | 24.1 ±0.2 | 143.06 | 102 | 94.0 | 100.0 | 82 | 600~1000 | 508 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 47.1 | 48.7 | -1.6 | -3.3% |
| yolo26s-seg.dxnn | 47.4 | 48.8 | -1.3 | -2.7% |
| yolo26m-seg.dxnn | 47.0 | 48.7 | -1.7 | -3.5% |
| yolo26l-seg.dxnn | 47.2 | 48.6 | -1.4 | -2.8% |
| yolo26x-seg.dxnn | 27.4 | 24.1 | +3.2 | +13.4% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | avdec_h264 | 2640 | 3 | 72.5 ±0.2 | 36.42 | 325 | 73.1 | 90.5 | 56~57 | 1000 | 301 | ok |
| yolo26s-obb.dxnn | avdec_h264 | 2640 | 3 | 46.4 ±0.1 | 56.88 | 155 | 93.7 | 100.0 | 62~64 | 1000 | 316 | ok |
| yolo26m-obb.dxnn | avdec_h264 | 2640 | 3 | 34.2 ±0.1 | 77.27 | 104 | 94.0 | 100.0 | 61~76 | 1000 | 338 | ok |
| yolo26l-obb.dxnn | avdec_h264 | 2640 | 3 | 25.0 ±0.1 | 105.66 | 76 | 94.7 | 100.0 | 78~80 | 1000 | 347 | ok |
| yolo26x-obb.dxnn | avdec_h264 | 2640 | 3 | 12.9 ±0.1 | 204.95 | 44 | 92.5 | 100.0 | 82 | 600~1000 | 418 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | avdec_h264 | 2640 | 3 | 79.4 ±0.2 | 33.25 | 256 | 90.5 | 100.0 | 58~59 | 1000 | 301 | ok |
| yolo26s-obb.dxnn | avdec_h264 | 2640 | 3 | 46.4 ±0.0 | 56.96 | 140 | 94.4 | 100.0 | 63~64 | 1000 | 312 | ok |
| yolo26m-obb.dxnn | avdec_h264 | 2640 | 3 | 34.2 ±0.0 | 77.26 | 101 | 94.3 | 100.0 | 76~80 | 1000 | 330 | ok |
| yolo26l-obb.dxnn | avdec_h264 | 2640 | 3 | 25.0 ±0.0 | 105.53 | 75 | 94.5 | 100.0 | 78~79 | 1000 | 345 | ok |
| yolo26x-obb.dxnn | avdec_h264 | 2640 | 3 | 13.0 ±0.1 | 203.49 | 43 | 92.2 | 100.0 | 82 | 600~1000 | 425 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 72.5 | 79.4 | -6.9 | -8.7% |
| yolo26s-obb.dxnn | 46.4 | 46.4 | +0.1 | +0.2% |
| yolo26m-obb.dxnn | 34.2 | 34.2 | +0.0 | +0.0% |
| yolo26l-obb.dxnn | 25.0 | 25.0 | -0.0 | -0.2% |
| yolo26x-obb.dxnn | 12.9 | 13.0 | -0.1 | -0.7% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | avdec_h264 | 3455 | 3 | 189.8 ±0.6 | 18.20 | 274 | 4.4 | 13.8 | 45~47 | 1000 | 186 | ok |
| yolo26s-cls.dxnn | avdec_h264 | 3455 | 3 | 190.0 ±0.8 | 18.18 | 272 | 8.0 | 24.1 | 46~48 | 1000 | 190 | ok |
| yolo26m-cls.dxnn | avdec_h264 | 3455 | 3 | 190.2 ±0.2 | 18.16 | 273 | 10.8 | 33.7 | 52~55 | 1000 | 191 | ok |
| yolo26l-cls.dxnn | avdec_h264 | 3455 | 3 | 189.6 ±0.6 | 18.23 | 273 | 18.1 | 44.4 | 52~54 | 1000 | 197 | ok |
| yolo26x-cls.dxnn | avdec_h264 | 3455 | 3 | 190.2 ±0.1 | 18.17 | 272 | 31.8 | 61.0 | 44~59 | 1000 | 227 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | avdec_h264 | 3455 | 3 | 189.7 ±0.3 | 18.21 | 274 | 4.3 | 13.8 | 44~46 | 1000 | 183 | ok |
| yolo26s-cls.dxnn | avdec_h264 | 3455 | 3 | 190.4 ±0.1 | 18.15 | 274 | 7.6 | 24.7 | 47~49 | 1000 | 181 | ok |
| yolo26m-cls.dxnn | avdec_h264 | 3455 | 3 | 190.3 ±0.8 | 18.16 | 273 | 11.4 | 33.6 | 53~56 | 1000 | 190 | ok |
| yolo26l-cls.dxnn | avdec_h264 | 3455 | 3 | 189.3 ±0.9 | 18.25 | 272 | 17.2 | 45.0 | 52~54 | 1000 | 196 | ok |
| yolo26x-cls.dxnn | avdec_h264 | 3455 | 3 | 189.9 ±0.4 | 18.19 | 271 | 32.2 | 62.1 | 55~57 | 1000 | 235 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 189.8 | 189.7 | +0.1 | +0.1% |
| yolo26s-cls.dxnn | 190.0 | 190.4 | -0.4 | -0.2% |
| yolo26m-cls.dxnn | 190.2 | 190.3 | -0.0 | -0.0% |
| yolo26l-cls.dxnn | 189.6 | 189.3 | +0.2 | +0.1% |
| yolo26x-cls.dxnn | 190.2 | 189.9 | +0.3 | +0.2% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 2 | 3 | 68.1 ±1.0 | 34.0 | 332 | 18.2 | 59.0 | 47~48 | 1000 | 423 | ok |
| yolo26n.dxnn | 3 | 3 | 68.6 ±0.2 | 22.9 | 332 | 18.5 | 56.7 | 48 | 1000 | 526 | ok |
| yolo26s.dxnn | 2 | 3 | 68.6 ±0.3 | 34.3 | 333 | 34.4 | 70.4 | 54~55 | 1000 | 434 | ok |
| yolo26s.dxnn | 3 | 3 | 68.3 ±0.0 | 22.8 | 334 | 34.7 | 70.7 | 54~55 | 1000 | 538 | ok |
| yolo26m.dxnn | 2 | 3 | 68.4 ±0.3 | 34.2 | 329 | 55.6 | 84.1 | 71~72 | 1000 | 456 | ok |
| yolo26m.dxnn | 3 | 3 | 68.1 ±0.1 | 22.7 | 329 | 55.3 | 83.9 | 72 | 1000 | 560 | ok |
| yolo26l.dxnn | 2 | 3 | 68.1 ±0.5 | 34.0 | 289 | 89.4 | 98.3 | 76~80 | 1000 | 466 | ok |
| yolo26l.dxnn | 3 | 3 | 68.1 ±0.4 | 22.7 | 288 | 89.9 | 99.2 | 81~82 | 1000 | 568 | ok |
| yolo26x.dxnn | 1 | 3 | 36.0 ±2.0 | 36.0 | 113 | 94.6 | 100.0 | 82~83 | 600~1000 | 389 | ok |
| yolo26x.dxnn | 2 | 3 | 34.7 ±0.3 | 17.3 | 109 | 96.6 | 100.0 | 82~83 | 600~1000 | 528 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 2 | 3 | 86.2 ±0.2 | 43.1 | 313 | 23.0 | 68.3 | 49 | 1000 | 457 | ok |
| yolo26n.dxnn | 3 | 3 | 86.2 ±0.5 | 28.8 | 305 | 23.4 | 69.3 | 48~49 | 1000 | 567 | ok |
| yolo26s.dxnn | 2 | 3 | 86.0 ±0.4 | 43.0 | 307 | 45.6 | 75.6 | 58~59 | 1000 | 470 | ok |
| yolo26s.dxnn | 3 | 3 | 86.3 ±0.1 | 28.8 | 301 | 46.1 | 76.9 | 59~60 | 1000 | 578 | ok |
| yolo26m.dxnn | 2 | 3 | 85.4 ±0.3 | 42.7 | 308 | 78.5 | 90.4 | 77~79 | 1000 | 492 | ok |
| yolo26m.dxnn | 3 | 3 | 85.6 ±0.2 | 28.5 | 309 | 79.6 | 89.6 | 79~81 | 1000 | 600 | ok |
| yolo26l.dxnn | 2 | 3 | 68.4 ±0.8 | 34.2 | 223 | 95.3 | 100.0 | 82~83 | 800~1000 | 487 | ok |
| yolo26l.dxnn | 3 | 3 | 67.2 ±0.2 | 22.4 | 220 | 96.5 | 100.0 | 83 | 800~1000 | 583 | ok |
| yolo26x.dxnn | 1 | 3 | 36.0 ±1.3 | 36.0 | 113 | 94.4 | 100.0 | 82 | 600~1000 | 400 | ok |
| yolo26x.dxnn | 2 | 3 | 34.7 ±0.2 | 17.4 | 112 | 96.6 | 100.0 | 81~83 | 600~1000 | 533 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 2 | 34.0 | 2 | 43.1 |
| yolo26s.dxnn | 2 | 34.3 | 2 | 43.0 |
| yolo26m.dxnn | 2 | 34.2 | 2 | 42.7 |
| yolo26l.dxnn | 2 | 34.0 | 2 | 34.2 |
| yolo26x.dxnn | 1 | 36.0 | 1 | 36.0 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 2 | 3 | 83.7 ±0.3 | 41.8 | 341 | 24.1 | 66.4 | 50~51 | 1000 | 415 | ok |
| yolo26n-pose.dxnn | 3 | 3 | 83.5 ±0.2 | 27.8 | 341 | 24.5 | 66.3 | 51 | 1000 | 528 | ok |
| yolo26s-pose.dxnn | 2 | 3 | 83.3 ±0.2 | 41.7 | 340 | 46.9 | 78.1 | 59~60 | 1000 | 431 | ok |
| yolo26s-pose.dxnn | 3 | 3 | 83.2 ±0.2 | 27.7 | 341 | 47.2 | 77.2 | 59~60 | 1000 | 537 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 83.3 ±0.2 | 41.7 | 327 | 79.7 | 91.0 | 78~80 | 1000 | 450 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 83.3 ±0.0 | 27.8 | 326 | 80.8 | 91.3 | 80~81 | 800~1000 | 557 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.5 ±0.8 | 34.3 | 191 | 95.4 | 100.0 | 75~81 | 800~1000 | 460 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 66.6 ±0.3 | 22.2 | 180 | 96.3 | 100.0 | 82 | 800~1000 | 568 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 36.5 ±1.3 | 36.5 | 95 | 94.9 | 100.0 | 82 | 600~1000 | 382 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 35.1 ±0.5 | 17.5 | 91 | 96.8 | 100.0 | 82~83 | 600~1000 | 522 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 4 | 3 | 125.9 ±0.2 | 31.5 | 344 | 41.1 | 67.7 | 55 | 1000 | 622 | ok |
| yolo26n-pose.dxnn | 5 | 3 | 126.3 ±0.8 | 25.2 | 343 | 41.3 | 69.1 | 55~56 | 1000 | 722 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 122.0 ±1.0 | 30.5 | 297 | 83.1 | 93.2 | 68~69 | 1000 | 632 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 121.8 ±1.2 | 24.4 | 298 | 83.4 | 95.2 | 68~69 | 1000 | 733 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 87.6 ±1.0 | 29.2 | 183 | 96.1 | 100.0 | 82~83 | 800~1000 | 548 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 88.2 ±0.8 | 44.1 | 182 | 95.0 | 100.0 | 82~83 | 800~1000 | 439 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 67.7 ±0.8 | 33.9 | 137 | 96.1 | 100.0 | 82 | 800~1000 | 452 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 65.9 ±0.2 | 22.0 | 132 | 96.5 | 100.0 | 82~83 | 800~1000 | 559 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 35.5 ±0.7 | 35.5 | 72 | 95.2 | 100.0 | 82~83 | 600~1000 | 372 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 34.0 ±0.5 | 17.0 | 72 | 96.2 | 100.0 | 82~83 | 600~1000 | 513 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 2 | 41.8 | 4 | 31.5 |
| yolo26s-pose.dxnn | 2 | 41.7 | 4 | 30.5 |
| yolo26m-pose.dxnn | 2 | 41.7 | 2 | 44.1 |
| yolo26l-pose.dxnn | 2 | 34.3 | 2 | 33.9 |
| yolo26x-pose.dxnn | 1 | 36.5 | 1 | 35.5 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 1 | 3 | 47.1 ±0.1 | 47.1 | 337 | 15.2 | 49.0 | 48~49 | 1000 | 379 | ok |
| yolo26n-seg.dxnn | 2 | 3 | 47.6 ±0.3 | 23.8 | 331 | 15.5 | 49.3 | 48 | 1000 | 523 | ok |
| yolo26s-seg.dxnn | 1 | 3 | 47.4 ±0.2 | 47.4 | 332 | 28.8 | 78.2 | 55~56 | 1000 | 394 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 47.3 ±0.1 | 23.7 | 331 | 29.1 | 74.8 | 56 | 1000 | 542 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 47.0 ±0.1 | 47.0 | 308 | 53.2 | 85.0 | 73~75 | 1000 | 416 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 47.5 ±0.3 | 23.8 | 304 | 55.0 | 89.1 | 75~77 | 1000 | 559 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 47.2 ±0.1 | 47.2 | 280 | 73.3 | 86.5 | 77~80 | 800~1000 | 423 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 46.2 ±0.2 | 23.1 | 274 | 74.6 | 88.6 | 80~81 | 800~1000 | 575 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 27.4 ±3.1 | 27.4 | 126 | 94.8 | 100.0 | 74~82 | 600~1000 | 494 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 1 | 3 | 48.7 ±0.0 | 48.7 | 216 | 16.0 | 49.0 | 48 | 1000 | 376 | ok |
| yolo26n-seg.dxnn | 2 | 3 | 48.8 ±0.0 | 24.4 | 208 | 16.2 | 49.0 | 49 | 1000 | 551 | ok |
| yolo26s-seg.dxnn | 1 | 3 | 48.8 ±0.0 | 48.8 | 200 | 30.0 | 78.2 | 56~57 | 1000 | 392 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 48.7 ±0.0 | 24.4 | 216 | 31.1 | 74.9 | 58 | 1000 | 550 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 48.7 ±0.0 | 48.7 | 208 | 55.6 | 89.2 | 65~71 | 1000 | 420 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 48.7 ±0.0 | 24.4 | 215 | 56.8 | 90.0 | 75~76 | 1000 | 575 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 48.6 ±0.1 | 48.6 | 214 | 76.5 | 88.2 | 76~80 | 800~1000 | 437 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 48.0 ±0.5 | 24.0 | 205 | 82.8 | 99.9 | 82 | 800~1000 | 588 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 24.1 ±0.2 | 24.1 | 102 | 94.0 | 100.0 | 82 | 600~1000 | 508 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 1 | 47.1 | 1 | 48.7 |
| yolo26s-seg.dxnn | 1 | 47.4 | 1 | 48.8 |
| yolo26m-seg.dxnn | 1 | 47.0 | 1 | 48.7 |
| yolo26l-seg.dxnn | 1 | 47.2 | 1 | 48.6 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 72.2 ±0.3 | 36.1 | 336 | 73.3 | 89.9 | 58~59 | 1000 | 438 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 69.7 ±1.5 | 23.2 | 323 | 70.0 | 91.0 | 58~59 | 1000 | 545 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.4 ±0.1 | 46.4 | 155 | 93.7 | 100.0 | 62~64 | 1000 | 316 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.5 ±0.1 | 23.2 | 153 | 96.0 | 100.0 | 65~66 | 1000 | 457 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.2 ±0.1 | 34.2 | 104 | 94.0 | 100.0 | 61~76 | 1000 | 338 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 34.1 ±0.1 | 17.1 | 108 | 95.8 | 100.0 | 70~80 | 1000 | 483 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.0 ±0.1 | 25.0 | 76 | 94.7 | 100.0 | 78~80 | 1000 | 347 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.9 ±0.1 | 12.9 | 44 | 92.5 | 100.0 | 82 | 600~1000 | 418 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 79.5 ±0.2 | 39.8 | 260 | 93.8 | 100.0 | 60~62 | 1000 | 435 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 79.5 ±0.2 | 26.5 | 265 | 94.3 | 100.0 | 61~62 | 1000 | 538 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.4 ±0.0 | 46.4 | 140 | 94.4 | 100.0 | 63~64 | 1000 | 312 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.4 ±0.0 | 23.2 | 140 | 96.1 | 100.0 | 65~66 | 1000 | 452 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.2 ±0.0 | 34.2 | 101 | 94.3 | 100.0 | 76~80 | 1000 | 330 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 33.8 ±0.2 | 16.9 | 104 | 96.2 | 100.0 | 80 | 800~1000 | 474 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.0 ±0.0 | 25.0 | 75 | 94.5 | 100.0 | 78~79 | 1000 | 345 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.0 ±0.1 | 13.0 | 43 | 92.2 | 100.0 | 82 | 600~1000 | 425 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 36.1 | 2 | 39.8 |
| yolo26s-obb.dxnn | 1 | 46.4 | 1 | 46.4 |
| yolo26m-obb.dxnn | 1 | 34.2 | 1 | 34.2 |

---
*Report generated by dx_stream benchmark tool*
