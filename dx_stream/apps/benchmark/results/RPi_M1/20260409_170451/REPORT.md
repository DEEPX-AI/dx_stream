# YOLO26 Benchmark Report

**Generated:** 2026-04-15 17:57:44 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-09 17:04:51 | 2026-04-10 11:46:10 | 18h 41m 19s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 39.09 | 90.8 | 63.4 | 2 |
| yolo26n-1.dxnn | OFF | 32.15 | 90.8 | 81.3 | 2 |
| yolo26s-1.dxnn | ON | 46.20 | 88.8 | 63.1 | 2 |
| yolo26s-1.dxnn | OFF | 38.58 | 88.8 | 81.0 | 2 |
| yolo26m-1.dxnn | ON | 55.56 | 83.4 | 63.4 | 2 |
| yolo26m-1.dxnn | OFF | 46.44 | 84.2 | 81.4 | 2 |
| yolo26l-1.dxnn | ON | 62.42 | 62.7 | 62.5 | 2 |
| yolo26l-1.dxnn | OFF | 54.83 | 62.7 | 62.7 | 2 |
| yolo26x-1.dxnn | ON | 92.04 | 35.4 | 35.4 | 1 |
| yolo26x-1.dxnn | OFF | 84.63 | 35.4 | 35.4 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 29.69 | 133.5 | 77.4 | 2 |
| yolo26n-pose.dxnn | OFF | 24.90 | 133.9 | 112.0 | 3 |
| yolo26s-pose.dxnn | ON | 37.52 | 122.6 | 77.3 | 2 |
| yolo26s-pose.dxnn | OFF | 31.99 | 127.8 | 111.8 | 3 |
| yolo26m-pose.dxnn | ON | 44.55 | 91.3 | 77.3 | 2 |
| yolo26m-pose.dxnn | OFF | 40.00 | 91.3 | 91.3 | 3 |
| yolo26l-pose.dxnn | ON | 51.89 | 68.2 | 48.8 | 2 |
| yolo26l-pose.dxnn | OFF | 47.45 | 68.3 | 68.2 | 2 |
| yolo26x-pose.dxnn | ON | 80.46 | 39.0 | 38.2 | 1 |
| yolo26x-pose.dxnn | OFF | 75.95 | 39.0 | 37.9 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 58.19 | 48.5 | 42.5 | 1 |
| yolo26n-seg.dxnn | OFF | 51.21 | 48.5 | 48.4 | 1 |
| yolo26s-seg.dxnn | ON | 69.87 | 48.4 | 42.5 | 1 |
| yolo26s-seg.dxnn | OFF | 59.96 | 48.3 | 48.2 | 1 |
| yolo26m-seg.dxnn | ON | 82.87 | 47.8 | 42.5 | 1 |
| yolo26m-seg.dxnn | OFF | 74.19 | 47.5 | 47.6 | 1 |
| yolo26l-seg.dxnn | ON | 93.83 | 47.5 | 42.8 | 1 |
| yolo26l-seg.dxnn | OFF | 81.71 | 47.1 | 47.1 | 1 |
| yolo26x-seg.dxnn | ON | 134.25 | 29.3 | 24.9 | — |
| yolo26x-seg.dxnn | OFF | 124.67 | 29.3 | 25.8 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 50.71 | 76.4 | 65.4 | 2 |
| yolo26n-obb.dxnn | OFF | 46.14 | 76.4 | 76.1 | 2 |
| yolo26s-obb.dxnn | ON | 70.27 | 44.1 | 44.1 | 1 |
| yolo26s-obb.dxnn | OFF | 64.46 | 44.2 | 44.2 | 1 |
| yolo26m-obb.dxnn | ON | 88.19 | 31.9 | 31.9 | 1 |
| yolo26m-obb.dxnn | OFF | 82.93 | 31.9 | 31.9 | 1 |
| yolo26l-obb.dxnn | ON | 108.89 | 24.0 | 24.0 | — |
| yolo26l-obb.dxnn | OFF | 104.77 | 23.9 | 23.9 | — |
| yolo26x-obb.dxnn | ON | 183.15 | 13.8 | 13.3 | — |
| yolo26x-obb.dxnn | OFF | 177.69 | 13.8 | 13.2 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.48 | 2698.3 | 190.5 | — |
| yolo26n-cls.dxnn | OFF | 1.51 | 2697.7 | 190.2 | — |
| yolo26s-cls.dxnn | ON | 2.12 | 1896.6 | 190.8 | — |
| yolo26s-cls.dxnn | OFF | 2.14 | 1896.2 | 191.5 | — |
| yolo26m-cls.dxnn | ON | 2.73 | 1357.4 | 191.0 | — |
| yolo26m-cls.dxnn | OFF | 2.76 | 1357.7 | 190.9 | — |
| yolo26l-cls.dxnn | ON | 4.02 | 851.0 | 190.6 | — |
| yolo26l-cls.dxnn | OFF | 4.06 | 850.1 | 190.4 | — |
| yolo26x-cls.dxnn | ON | 6.69 | 457.7 | 190.4 | — |
| yolo26x-cls.dxnn | OFF | 6.62 | 457.7 | 189.8 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | RPi |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.12.75+rpt-rpi-2712 |
| CPU | Cortex-A76 |
| CPU Cores | 4 |
| RAM | 7.9 GB |
| NPU SKU | M1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.1.0 |
| NPU Driver (PCIe) | v2.0.1 |
| NPU Firmware | v2.5.0 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen2 X1 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.2.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.0 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.0 |
| dxtop | Yes | DX-TOP 1.0.1 |
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
| yolo26n-1.dxnn | Object Detection | 640×640 | 102.5 | Yes | ✅ |
| yolo26s-1.dxnn | Object Detection | 640×640 | 119.0 | Yes | ✅ |
| yolo26m-1.dxnn | Object Detection | 640×640 | 148.4 | Yes | ✅ |
| yolo26l-1.dxnn | Object Detection | 640×640 | 162.5 | Yes | ✅ |
| yolo26x-1.dxnn | Object Detection | 640×640 | 266.4 | Yes | ✅ |
| yolo26n-pose.dxnn | Pose Estimation | 640×640 | 127.9 | Yes | ✅ |
| yolo26s-pose.dxnn | Pose Estimation | 640×640 | 158.8 | Yes | ✅ |
| yolo26m-pose.dxnn | Pose Estimation | 640×640 | 254.1 | Yes | ✅ |
| yolo26l-pose.dxnn | Pose Estimation | 640×640 | 305.7 | Yes | ✅ |
| yolo26x-pose.dxnn | Pose Estimation | 640×640 | 516.6 | Yes | ✅ |
| yolo26n-seg.dxnn | Segmentation | 640×640 | 148.2 | Yes | ✅ |
| yolo26s-seg.dxnn | Segmentation | 640×640 | 172.9 | Yes | ✅ |
| yolo26m-seg.dxnn | Segmentation | 640×640 | 265.5 | Yes | ✅ |
| yolo26l-seg.dxnn | Segmentation | 640×640 | 317.1 | Yes | ✅ |
| yolo26x-seg.dxnn | Segmentation | 640×640 | 550.4 | Yes | ✅ |
| yolo26n-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 336.7 | Yes | ✅ |
| yolo26s-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 496.0 | Yes | ✅ |
| yolo26m-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 645.4 | Yes | ✅ |
| yolo26l-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 790.2 | Yes | ✅ |
| yolo26x-obb.dxnn | Oriented BBox (OBB) | 1024×1024 | 1316.8 | Yes | ✅ |
| yolo26n-cls.dxnn | Classification | 224×224 | 4.5 | No | — |
| yolo26s-cls.dxnn | Classification | 224×224 | 9.0 | No | — |
| yolo26m-cls.dxnn | Classification | 224×224 | 13.7 | No | — |
| yolo26l-cls.dxnn | Classification | 224×224 | 19.6 | No | — |
| yolo26x-cls.dxnn | Classification | 224×224 | 48.7 | No | — |

## Input Videos

### Object Detection / Pose / Segmentation

| Item | Value |
|------|-------|
| File | 856056-hd_1920_1080_30fps_4loop.mp4 |
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
| File | obb_test_video_6loop.mp4 |
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
| File | 856056-hd_1920_1080_30fps_4loop.mp4 |
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
| yolo26n-1.dxnn | 90.8 ±0.0 | 173 | 26.6 | 71.5 | 44~46 | 1000 | ok |
| yolo26s-1.dxnn | 88.8 ±0.1 | 157 | 52.1 | 88.9 | 53~55 | 1000 | ok |
| yolo26m-1.dxnn | 83.4 ±0.2 | 135 | 87.0 | 100.0 | 61~67 | 1000 | ok |
| yolo26l-1.dxnn | 62.7 ±0.0 | 85 | 91.6 | 100.0 | 61~66 | 1000 | ok |
| yolo26x-1.dxnn | 35.4 ±0.0 | 53 | 89.3 | 100.0 | 63~69 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 90.8 ±0.1 | 62 | 27.6 | 85.3 | 49~50 | 1000 | ok |
| yolo26s-1.dxnn | 88.8 ±0.3 | 60 | 52.7 | 91.1 | 54~56 | 1000 | ok |
| yolo26m-1.dxnn | 84.2 ±0.3 | 57 | 91.3 | 100.0 | 62~67 | 1000 | ok |
| yolo26l-1.dxnn | 62.7 ±0.0 | 48 | 90.5 | 100.0 | 61~66 | 1000 | ok |
| yolo26x-1.dxnn | 35.4 ±0.0 | 29 | 89.8 | 100.0 | 63~68 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 133.5 ±0.1 | 172 | 42.9 | 72.5 | 51~52 | 1000 | ok |
| yolo26s-pose.dxnn | 122.6 ±0.9 | 144 | 83.1 | 95.5 | 56~60 | 1000 | ok |
| yolo26m-pose.dxnn | 91.3 ±0.0 | 88 | 88.9 | 100.0 | 63~68 | 1000 | ok |
| yolo26l-pose.dxnn | 68.2 ±0.1 | 67 | 92.9 | 100.0 | 62~67 | 1000 | ok |
| yolo26x-pose.dxnn | 39.0 ±0.0 | 38 | 90.2 | 100.0 | 64~71 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 133.9 ±0.1 | 64 | 42.6 | 71.4 | 51~53 | 1000 | ok |
| yolo26s-pose.dxnn | 127.8 ±0.3 | 60 | 88.5 | 98.4 | 57~60 | 1000 | ok |
| yolo26m-pose.dxnn | 91.3 ±0.0 | 48 | 91.8 | 100.0 | 63~69 | 1000 | ok |
| yolo26l-pose.dxnn | 68.3 ±0.0 | 36 | 90.3 | 100.0 | 62~68 | 1000 | ok |
| yolo26x-pose.dxnn | 39.0 ±0.0 | 21 | 90.1 | 100.0 | 65~70 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 48.5 ±0.0 | 138 | 15.9 | 51.8 | 48 | 1000 | ok |
| yolo26s-seg.dxnn | 48.4 ±0.0 | 129 | 28.8 | 86.1 | 50~52 | 1000 | ok |
| yolo26m-seg.dxnn | 47.8 ±0.1 | 132 | 54.6 | 92.9 | 60~65 | 1000 | ok |
| yolo26l-seg.dxnn | 47.5 ±0.0 | 122 | 74.4 | 84.2 | 63~68 | 1000 | ok |
| yolo26x-seg.dxnn | 29.3 ±0.1 | 70 | 88.9 | 100.0 | 67~75 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 48.5 ±0.0 | 70 | 15.9 | 51.8 | 47~48 | 1000 | ok |
| yolo26s-seg.dxnn | 48.3 ±0.1 | 70 | 29.3 | 93.7 | 51~53 | 1000 | ok |
| yolo26m-seg.dxnn | 47.5 ±0.1 | 67 | 53.0 | 91.3 | 60~65 | 1000 | ok |
| yolo26l-seg.dxnn | 47.1 ±0.1 | 68 | 73.9 | 82.2 | 63~69 | 1000 | ok |
| yolo26x-seg.dxnn | 29.3 ±0.1 | 47 | 88.4 | 100.0 | 67~74 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.4 ±0.1 | 82 | 92.0 | 100.0 | 55~57 | 1000 | ok |
| yolo26s-obb.dxnn | 44.1 ±0.0 | 53 | 89.9 | 100.0 | 55~58 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.0 | 37 | 87.9 | 100.0 | 61~65 | 1000 | ok |
| yolo26l-obb.dxnn | 24.0 ±0.0 | 28 | 89.8 | 100.0 | 62~66 | 1000 | ok |
| yolo26x-obb.dxnn | 13.8 ±0.0 | 16 | 85.2 | 100.0 | 64~69 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.4 ±0.0 | 50 | 91.5 | 100.0 | 54~56 | 1000 | ok |
| yolo26s-obb.dxnn | 44.2 ±0.0 | 31 | 91.2 | 100.0 | 55~58 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.0 | 21 | 91.9 | 100.0 | 61~66 | 1000 | ok |
| yolo26l-obb.dxnn | 23.9 ±0.0 | 16 | 90.6 | 100.0 | 61~65 | 1000 | ok |
| yolo26x-obb.dxnn | 13.8 ±0.0 | 9 | 86.2 | 100.0 | 65~70 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 2698.3 ±0.3 | 57 | 70.1 | 77.3 | 51~53 | 1000 | ok |
| yolo26s-cls.dxnn | 1896.6 ±0.1 | 39 | 89.9 | 97.9 | 51~55 | 1000 | ok |
| yolo26m-cls.dxnn | 1357.4 ±0.3 | 27 | 90.1 | 98.9 | 59~64 | 1000 | ok |
| yolo26l-cls.dxnn | 851.0 ±0.2 | 17 | 91.5 | 99.4 | 57~61 | 1000 | ok |
| yolo26x-cls.dxnn | 457.7 ±0.1 | 10 | 92.0 | 100.0 | 55~61 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 2697.7 ±0.4 | 58 | 70.9 | 77.5 | 46~49 | 1000 | ok |
| yolo26s-cls.dxnn | 1896.2 ±0.3 | 39 | 90.1 | 98.5 | 53~56 | 1000 | ok |
| yolo26m-cls.dxnn | 1357.7 ±0.1 | 27 | 89.5 | 98.5 | 60~65 | 1000 | ok |
| yolo26l-cls.dxnn | 850.1 ±0.9 | 17 | 91.2 | 99.5 | 57~61 | 1000 | ok |
| yolo26x-cls.dxnn | 457.7 ±0.1 | 10 | 89.9 | 99.8 | 60~64 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 25.6 | 39.09 | 31.02 | 8.07 | 39 | ok |
| yolo26s-1.dxnn | 21.6 | 46.20 | 38.55 | 7.65 | 47 | ok |
| yolo26m-1.dxnn | 18.0 | 55.56 | 47.80 | 7.76 | 48 | ok |
| yolo26l-1.dxnn | 16.0 | 62.42 | 54.74 | 7.67 | 49 | ok |
| yolo26x-1.dxnn | 10.9 | 92.04 | 84.55 | 7.49 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 31.1 | 32.15 | 32.15 | 47 | ok |
| yolo26s-1.dxnn | 25.9 | 38.58 | 38.58 | 48 | ok |
| yolo26m-1.dxnn | 21.5 | 46.44 | 46.44 | 49 | ok |
| yolo26l-1.dxnn | 18.2 | 54.83 | 54.83 | 49 | ok |
| yolo26x-1.dxnn | 11.8 | 84.63 | 84.63 | 51 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 33.7 | 29.69 | 25.00 | 4.69 | 48 | ok |
| yolo26s-pose.dxnn | 26.7 | 37.52 | 32.78 | 4.74 | 48 | ok |
| yolo26m-pose.dxnn | 22.4 | 44.55 | 39.83 | 4.72 | 49 | ok |
| yolo26l-pose.dxnn | 19.3 | 51.89 | 47.44 | 4.45 | 49 | ok |
| yolo26x-pose.dxnn | 12.4 | 80.46 | 75.87 | 4.60 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 40.2 | 24.90 | 24.90 | 47 | ok |
| yolo26s-pose.dxnn | 31.3 | 31.99 | 31.99 | 48 | ok |
| yolo26m-pose.dxnn | 25.0 | 40.00 | 40.00 | 49 | ok |
| yolo26l-pose.dxnn | 21.1 | 47.45 | 47.45 | 50 | ok |
| yolo26x-pose.dxnn | 13.2 | 75.95 | 75.95 | 51 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 17.2 | 58.19 | 49.49 | 8.70 | 47 | ok |
| yolo26s-seg.dxnn | 14.3 | 69.87 | 60.41 | 9.46 | 46 | ok |
| yolo26m-seg.dxnn | 12.1 | 82.87 | 73.77 | 9.10 | 49 | ok |
| yolo26l-seg.dxnn | 10.7 | 93.83 | 84.73 | 9.10 | 49 | ok |
| yolo26x-seg.dxnn | 7.4 | 134.25 | 124.92 | 9.32 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 19.5 | 51.21 | 51.21 | 46 | ok |
| yolo26s-seg.dxnn | 16.7 | 59.96 | 59.96 | 47 | ok |
| yolo26m-seg.dxnn | 13.5 | 74.19 | 74.19 | 49 | ok |
| yolo26l-seg.dxnn | 12.2 | 81.71 | 81.71 | 50 | ok |
| yolo26x-seg.dxnn | 8.0 | 124.67 | 124.67 | 51 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 19.7 | 50.71 | 45.52 | 5.19 | 48 | ok |
| yolo26s-obb.dxnn | 14.2 | 70.27 | 64.98 | 5.29 | 48 | ok |
| yolo26m-obb.dxnn | 11.3 | 88.19 | 82.79 | 5.40 | 50 | ok |
| yolo26l-obb.dxnn | 9.2 | 108.89 | 103.54 | 5.34 | 51 | ok |
| yolo26x-obb.dxnn | 5.5 | 183.15 | 177.72 | 5.43 | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 21.7 | 46.14 | 46.14 | 48 | ok |
| yolo26s-obb.dxnn | 15.5 | 64.46 | 64.46 | 48 | ok |
| yolo26m-obb.dxnn | 12.1 | 82.93 | 82.93 | 50 | ok |
| yolo26l-obb.dxnn | 9.5 | 104.77 | 104.77 | 51 | ok |
| yolo26x-obb.dxnn | 5.6 | 177.69 | 177.69 | 54 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 677.2 | 1.48 | 1.48 | N/A | 48 | ok |
| yolo26s-cls.dxnn | 471.7 | 2.12 | 2.12 | N/A | 43 | ok |
| yolo26m-cls.dxnn | 365.8 | 2.73 | 2.73 | N/A | 46 | ok |
| yolo26l-cls.dxnn | 248.6 | 4.02 | 4.02 | N/A | 47 | ok |
| yolo26x-cls.dxnn | 149.4 | 6.69 | 6.69 | N/A | 42 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 663.2 | 1.51 | 1.51 | 41 | ok |
| yolo26s-cls.dxnn | 467.5 | 2.14 | 2.14 | 46 | ok |
| yolo26m-cls.dxnn | 361.7 | 2.76 | 2.76 | 48 | ok |
| yolo26l-cls.dxnn | 246.5 | 4.06 | 4.06 | 48 | ok |
| yolo26x-cls.dxnn | 151.1 | 6.62 | 6.62 | 48 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | avdec_h264 | 3455 | 3 | 63.4 ±0.1 | 54.49 | 342 | 18.8 | 58.8 | 44~46 | 1000 | 292 | ok |
| yolo26s-1.dxnn | avdec_h264 | 3455 | 3 | 63.1 ±0.1 | 54.75 | 343 | 35.3 | 70.0 | 53 | 1000 | 308 | ok |
| yolo26m-1.dxnn | avdec_h264 | 3455 | 3 | 63.4 ±0.2 | 54.52 | 335 | 58.2 | 84.2 | 57~62 | 1000 | 330 | ok |
| yolo26l-1.dxnn | avdec_h264 | 3455 | 3 | 62.5 ±0.1 | 55.28 | 246 | 94.7 | 100.0 | 69~72 | 1000 | 341 | ok |
| yolo26x-1.dxnn | avdec_h264 | 3455 | 3 | 35.4 ±0.0 | 97.62 | 98 | 95.5 | 100.0 | 77~79 | 1000 | 399 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | avdec_h264 | 3455 | 3 | 81.3 ±0.6 | 42.51 | 320 | 24.2 | 69.3 | 49 | 1000 | 327 | ok |
| yolo26s-1.dxnn | avdec_h264 | 3455 | 3 | 81.0 ±0.5 | 42.63 | 320 | 48.0 | 79.9 | 55~57 | 1000 | 342 | ok |
| yolo26m-1.dxnn | avdec_h264 | 3455 | 3 | 81.4 ±0.6 | 42.43 | 315 | 88.3 | 98.5 | 69~71 | 1000 | 362 | ok |
| yolo26l-1.dxnn | avdec_h264 | 3455 | 3 | 62.7 ±0.0 | 55.12 | 202 | 95.3 | 100.0 | 70~73 | 1000 | 339 | ok |
| yolo26x-1.dxnn | avdec_h264 | 3455 | 3 | 35.4 ±0.0 | 97.63 | 109 | 95.3 | 100.0 | 76~79 | 1000 | 394 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 63.4 | 81.3 | -17.9 | -22.0% |
| yolo26s-1.dxnn | 63.1 | 81.0 | -17.9 | -22.1% |
| yolo26m-1.dxnn | 63.4 | 81.4 | -18.1 | -22.2% |
| yolo26l-1.dxnn | 62.5 | 62.7 | -0.2 | -0.3% |
| yolo26x-1.dxnn | 35.4 | 35.4 | +0.0 | +0.0% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | avdec_h264 | 3455 | 3 | 77.4 ±0.2 | 44.63 | 347 | 23.3 | 64.3 | 48~49 | 1000 | 279 | ok |
| yolo26s-pose.dxnn | avdec_h264 | 3455 | 3 | 77.3 ±0.2 | 44.67 | 346 | 43.8 | 74.3 | 56 | 1000 | 303 | ok |
| yolo26m-pose.dxnn | avdec_h264 | 3455 | 3 | 77.3 ±0.1 | 44.71 | 335 | 73.1 | 87.8 | 68~70 | 1000 | 320 | ok |
| yolo26l-pose.dxnn | avdec_h264 | 3455 | 3 | 48.8 ±21.5 | 70.82 | 136 | 73.5 | 100.0 | 61~71 | 1000 | 328 | ok |
| yolo26x-pose.dxnn | avdec_h264 | 3455 | 3 | 38.2 ±0.7 | 90.34 | 88 | 95.2 | 100.0 | 80~82 | 800~1000 | 389 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | avdec_h264 | 3455 | 3 | 112.0 ±0.1 | 30.84 | 319 | 35.9 | 66.8 | 51~52 | 1000 | 263 | ok |
| yolo26s-pose.dxnn | avdec_h264 | 3455 | 3 | 111.8 ±0.2 | 30.90 | 316 | 72.0 | 89.0 | 60~61 | 1000 | 287 | ok |
| yolo26m-pose.dxnn | avdec_h264 | 3455 | 3 | 91.3 ±0.0 | 37.83 | 185 | 90.2 | 100.0 | 52~64 | 1000 | 305 | ok |
| yolo26l-pose.dxnn | avdec_h264 | 3455 | 3 | 68.2 ±0.0 | 50.63 | 129 | 93.2 | 100.0 | 63~71 | 1000 | 314 | ok |
| yolo26x-pose.dxnn | avdec_h264 | 3455 | 3 | 37.9 ±1.2 | 91.23 | 74 | 94.7 | 100.0 | 80~81 | 800~1000 | 376 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 77.4 | 112.0 | -34.6 | -30.9% |
| yolo26s-pose.dxnn | 77.3 | 111.8 | -34.5 | -30.8% |
| yolo26m-pose.dxnn | 77.3 | 91.3 | -14.0 | -15.4% |
| yolo26l-pose.dxnn | 48.8 | 68.2 | -19.5 | -28.5% |
| yolo26x-pose.dxnn | 38.2 | 37.9 | +0.4 | +1.0% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | avdec_h264 | 3455 | 3 | 42.5 ±0.1 | 81.37 | 349 | 14.5 | 47.3 | 47 | 1000 | 390 | ok |
| yolo26s-seg.dxnn | avdec_h264 | 3455 | 3 | 42.5 ±0.0 | 81.20 | 347 | 26.8 | 74.5 | 52~53 | 1000 | 407 | ok |
| yolo26m-seg.dxnn | avdec_h264 | 3455 | 3 | 42.5 ±0.1 | 81.38 | 328 | 49.4 | 79.9 | 67~69 | 1000 | 430 | ok |
| yolo26l-seg.dxnn | avdec_h264 | 3455 | 3 | 42.8 ±0.3 | 80.66 | 314 | 66.8 | 85.3 | 66~73 | 1000 | 441 | ok |
| yolo26x-seg.dxnn | avdec_h264 | 3455 | 3 | 24.9 ±0.4 | 138.86 | 108 | 92.4 | 100.0 | 81~82 | 600~1000 | 504 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | avdec_h264 | 3455 | 3 | 48.4 ±0.0 | 71.42 | 243 | 16.9 | 50.7 | 48 | 1000 | 390 | ok |
| yolo26s-seg.dxnn | avdec_h264 | 3455 | 3 | 48.2 ±0.1 | 71.68 | 232 | 31.3 | 76.7 | 50~53 | 1000 | 406 | ok |
| yolo26m-seg.dxnn | avdec_h264 | 3455 | 3 | 47.6 ±0.0 | 72.58 | 231 | 56.5 | 92.4 | 70~73 | 1000 | 434 | ok |
| yolo26l-seg.dxnn | avdec_h264 | 3455 | 3 | 47.1 ±0.0 | 73.27 | 233 | 77.2 | 86.3 | 75~78 | 1000 | 440 | ok |
| yolo26x-seg.dxnn | avdec_h264 | 3455 | 3 | 25.8 ±0.6 | 133.72 | 113 | 93.4 | 100.0 | 82 | 600~1000 | 510 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 42.5 | 48.4 | -5.9 | -12.2% |
| yolo26s-seg.dxnn | 42.5 | 48.2 | -5.7 | -11.7% |
| yolo26m-seg.dxnn | 42.5 | 47.6 | -5.1 | -10.8% |
| yolo26l-seg.dxnn | 42.8 | 47.1 | -4.3 | -9.2% |
| yolo26x-seg.dxnn | 24.9 | 25.8 | -1.0 | -3.7% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | avdec_h264 | 2640 | 3 | 65.4 ±0.3 | 40.37 | 338 | 67.0 | 83.4 | 49~53 | 1000 | 314 | ok |
| yolo26s-obb.dxnn | avdec_h264 | 2640 | 3 | 44.1 ±0.0 | 59.87 | 145 | 94.1 | 100.0 | 59~61 | 1000 | 332 | ok |
| yolo26m-obb.dxnn | avdec_h264 | 2640 | 3 | 31.9 ±0.0 | 82.79 | 103 | 94.2 | 100.0 | 72~75 | 1000 | 359 | ok |
| yolo26l-obb.dxnn | avdec_h264 | 2640 | 3 | 24.0 ±0.0 | 110.18 | 78 | 97.4 | 100.0 | 73~76 | 1000 | 364 | ok |
| yolo26x-obb.dxnn | avdec_h264 | 2640 | 3 | 13.3 ±0.3 | 197.88 | 46 | 91.7 | 100.0 | 81 | 800~1000 | 431 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | avdec_h264 | 2640 | 3 | 76.1 ±0.0 | 34.69 | 284 | 91.2 | 100.0 | 48~52 | 1000 | 316 | ok |
| yolo26s-obb.dxnn | avdec_h264 | 2640 | 3 | 44.2 ±0.0 | 59.79 | 141 | 93.9 | 100.0 | 60~63 | 1000 | 319 | ok |
| yolo26m-obb.dxnn | avdec_h264 | 2640 | 3 | 31.9 ±0.0 | 82.68 | 107 | 94.1 | 100.0 | 59~71 | 1000 | 342 | ok |
| yolo26l-obb.dxnn | avdec_h264 | 2640 | 3 | 23.9 ±0.0 | 110.31 | 81 | 96.9 | 100.0 | 73~76 | 1000 | 352 | ok |
| yolo26x-obb.dxnn | avdec_h264 | 2640 | 3 | 13.2 ±0.3 | 199.23 | 47 | 91.3 | 100.0 | 81~82 | 800~1000 | 419 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 65.4 | 76.1 | -10.7 | -14.1% |
| yolo26s-obb.dxnn | 44.1 | 44.2 | -0.1 | -0.2% |
| yolo26m-obb.dxnn | 31.9 | 31.9 | -0.0 | -0.1% |
| yolo26l-obb.dxnn | 24.0 | 23.9 | +0.0 | +0.1% |
| yolo26x-obb.dxnn | 13.3 | 13.2 | +0.1 | +0.7% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | avdec_h264 | 3455 | 3 | 190.5 ±0.7 | 18.13 | 275 | 4.5 | 14.7 | 40 | 1000 | 168 | ok |
| yolo26s-cls.dxnn | avdec_h264 | 3455 | 3 | 190.8 ±0.7 | 18.10 | 274 | 8.2 | 25.4 | 46~48 | 1000 | 176 | ok |
| yolo26m-cls.dxnn | avdec_h264 | 3455 | 3 | 191.0 ±0.2 | 18.09 | 273 | 11.6 | 33.7 | 51~53 | 1000 | 197 | ok |
| yolo26l-cls.dxnn | avdec_h264 | 3455 | 3 | 190.6 ±0.9 | 18.13 | 272 | 18.4 | 45.3 | 51~53 | 1000 | 196 | ok |
| yolo26x-cls.dxnn | avdec_h264 | 3455 | 3 | 190.4 ±0.1 | 18.14 | 270 | 33.2 | 67.8 | 53~54 | 1000 | 227 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | avdec_h264 | 3455 | 3 | 190.2 ±0.4 | 18.16 | 273 | 4.4 | 14.5 | 43~44 | 1000 | 168 | ok |
| yolo26s-cls.dxnn | avdec_h264 | 3455 | 3 | 191.5 ±0.5 | 18.04 | 275 | 7.9 | 25.6 | 46~48 | 1000 | 182 | ok |
| yolo26m-cls.dxnn | avdec_h264 | 3455 | 3 | 190.9 ±0.7 | 18.10 | 273 | 11.2 | 33.3 | 51~54 | 1000 | 191 | ok |
| yolo26l-cls.dxnn | avdec_h264 | 3455 | 3 | 190.4 ±0.2 | 18.14 | 273 | 18.4 | 46.9 | 41~52 | 1000 | 196 | ok |
| yolo26x-cls.dxnn | avdec_h264 | 3455 | 3 | 189.8 ±0.6 | 18.21 | 271 | 34.0 | 61.2 | 55~56 | 1000 | 232 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 190.5 | 190.2 | +0.3 | +0.2% |
| yolo26s-cls.dxnn | 190.8 | 191.5 | -0.6 | -0.3% |
| yolo26m-cls.dxnn | 191.0 | 190.9 | +0.2 | +0.1% |
| yolo26l-cls.dxnn | 190.6 | 190.4 | +0.1 | +0.1% |
| yolo26x-cls.dxnn | 190.4 | 189.8 | +0.7 | +0.4% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 2 | 3 | 63.3 ±0.1 | 31.6 | 340 | 19.2 | 57.9 | 47~48 | 1000 | 437 | ok |
| yolo26n-1.dxnn | 3 | 3 | 63.3 ±0.0 | 21.1 | 338 | 19.5 | 65.6 | 47~48 | 1000 | 541 | ok |
| yolo26s-1.dxnn | 2 | 3 | 63.1 ±0.2 | 31.6 | 338 | 36.4 | 70.4 | 54 | 1000 | 450 | ok |
| yolo26s-1.dxnn | 3 | 3 | 63.1 ±0.2 | 21.0 | 338 | 36.7 | 70.4 | 54 | 1000 | 557 | ok |
| yolo26m-1.dxnn | 2 | 3 | 63.1 ±0.1 | 31.6 | 333 | 59.2 | 83.6 | 65~67 | 1000 | 470 | ok |
| yolo26m-1.dxnn | 3 | 3 | 63.2 ±0.2 | 21.1 | 332 | 60.0 | 86.1 | 67 | 1000 | 570 | ok |
| yolo26l-1.dxnn | 2 | 3 | 62.5 ±0.1 | 31.2 | 246 | 96.2 | 100.0 | 74~76 | 1000 | 477 | ok |
| yolo26l-1.dxnn | 3 | 3 | 62.2 ±0.0 | 20.7 | 260 | 96.5 | 100.0 | 76 | 1000 | 585 | ok |
| yolo26x-1.dxnn | 1 | 3 | 35.4 ±0.0 | 35.4 | 98 | 95.5 | 100.0 | 77~89 | 1000 | 399 | ok |
| yolo26x-1.dxnn | 2 | 3 | 35.4 ±0.0 | 17.7 | 101 | 97.1 | 100.0 | 79~80 | 1000 | 536 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 2 | 3 | 80.5 ±0.1 | 40.2 | 326 | 24.8 | 70.3 | 49 | 1000 | 472 | ok |
| yolo26n-1.dxnn | 3 | 3 | 80.6 ±0.6 | 26.9 | 323 | 24.9 | 69.3 | 49~50 | 1000 | 578 | ok |
| yolo26s-1.dxnn | 2 | 3 | 80.6 ±0.8 | 40.3 | 319 | 49.3 | 79.1 | 57~58 | 1000 | 485 | ok |
| yolo26s-1.dxnn | 3 | 3 | 80.1 ±0.0 | 26.7 | 323 | 49.3 | 79.2 | 57 | 1000 | 591 | ok |
| yolo26m-1.dxnn | 2 | 3 | 81.4 ±0.5 | 40.7 | 315 | 91.2 | 98.9 | 73~74 | 1000 | 506 | ok |
| yolo26m-1.dxnn | 3 | 3 | 80.7 ±0.2 | 26.9 | 321 | 90.5 | 98.5 | 74~75 | 1000 | 610 | ok |
| yolo26l-1.dxnn | 2 | 3 | 62.7 ±0.0 | 31.4 | 204 | 97.7 | 100.0 | 75 | 1000 | 502 | ok |
| yolo26l-1.dxnn | 3 | 3 | 62.7 ±0.0 | 20.9 | 206 | 97.7 | 100.0 | 76~77 | 1000 | 593 | ok |
| yolo26x-1.dxnn | 1 | 3 | 35.4 ±0.0 | 35.4 | 109 | 95.3 | 100.0 | 78~90 | 1000 | 394 | ok |
| yolo26x-1.dxnn | 2 | 3 | 35.4 ±0.0 | 17.7 | 111 | 97.2 | 100.0 | 80 | 1000 | 535 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 2 | 31.6 | 2 | 40.2 |
| yolo26s-1.dxnn | 2 | 31.6 | 2 | 40.3 |
| yolo26m-1.dxnn | 2 | 31.6 | 2 | 40.7 |
| yolo26l-1.dxnn | 2 | 31.2 | 2 | 31.4 |
| yolo26x-1.dxnn | 1 | 35.4 | 1 | 35.4 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 2 | 3 | 76.7 ±0.3 | 38.3 | 345 | 23.7 | 64.2 | 48 | 1000 | 423 | ok |
| yolo26n-pose.dxnn | 3 | 3 | 76.8 ±0.3 | 25.6 | 345 | 24.0 | 64.0 | 49 | 1000 | 532 | ok |
| yolo26s-pose.dxnn | 2 | 3 | 77.0 ±0.2 | 38.5 | 343 | 44.0 | 75.8 | 56 | 1000 | 438 | ok |
| yolo26s-pose.dxnn | 3 | 3 | 76.7 ±0.1 | 25.6 | 343 | 44.6 | 77.3 | 56~57 | 1000 | 543 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 77.5 ±0.2 | 38.7 | 332 | 76.1 | 88.5 | 71~73 | 1000 | 460 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 76.9 ±0.2 | 25.6 | 332 | 75.8 | 87.2 | 63~73 | 1000 | 565 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.2 ±0.0 | 34.1 | 181 | 96.3 | 100.0 | 76~79 | 1000 | 468 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.2 ±0.0 | 22.7 | 179 | 97.2 | 100.0 | 79~80 | 1000 | 570 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 38.2 ±0.7 | 38.2 | 88 | 95.2 | 100.0 | 80~92 | 800~1000 | 389 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 36.5 ±0.3 | 18.2 | 90 | 94.2 | 100.0 | 81~82 | 800~1000 | 532 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 3 | 3 | 113.8 ±0.6 | 37.9 | 362 | 38.8 | 69.1 | 52~53 | 1000 | 527 | ok |
| yolo26n-pose.dxnn | 4 | 3 | 113.9 ±0.5 | 28.5 | 361 | 39.0 | 69.0 | 53 | 1000 | 634 | ok |
| yolo26s-pose.dxnn | 3 | 3 | 111.7 ±0.2 | 37.2 | 329 | 75.1 | 87.1 | 63~64 | 1000 | 537 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 112.2 ±0.7 | 28.1 | 331 | 76.7 | 89.0 | 61~63 | 1000 | 643 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 91.3 ±0.0 | 30.4 | 197 | 96.4 | 100.0 | 77~80 | 1000 | 556 | ok |
| yolo26m-pose.dxnn | 4 | 3 | 89.7 ±0.5 | 22.4 | 192 | 96.7 | 100.0 | 81 | 800~1000 | 661 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.2 ±0.0 | 34.1 | 133 | 96.3 | 100.0 | 76~78 | 1000 | 456 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.2 ±0.0 | 22.7 | 137 | 97.2 | 100.0 | 78~79 | 1000 | 560 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 37.9 ±1.2 | 37.9 | 74 | 94.7 | 100.0 | 80~92 | 800~1000 | 376 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 36.1 ±0.1 | 18.0 | 75 | 94.1 | 100.0 | 82 | 800~1000 | 518 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 2 | 38.3 | 3 | 37.9 |
| yolo26s-pose.dxnn | 2 | 38.5 | 3 | 37.2 |
| yolo26m-pose.dxnn | 2 | 38.7 | 3 | 30.4 |
| yolo26l-pose.dxnn | 2 | 34.1 | 2 | 34.1 |
| yolo26x-pose.dxnn | 1 | 38.2 | 1 | 37.9 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 1 | 3 | 42.5 ±0.1 | 42.5 | 349 | 14.5 | 47.3 | 46~49 | 1000 | 390 | ok |
| yolo26n-seg.dxnn | 2 | 3 | 42.6 ±0.1 | 21.3 | 346 | 14.7 | 48.5 | 47 | 1000 | 539 | ok |
| yolo26s-seg.dxnn | 1 | 3 | 42.5 ±0.0 | 42.5 | 347 | 26.8 | 74.5 | 52~57 | 1000 | 407 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 42.7 ±0.2 | 21.4 | 342 | 27.1 | 74.7 | 53 | 1000 | 556 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 42.5 ±0.1 | 42.5 | 328 | 49.4 | 79.9 | 69~78 | 1000 | 430 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 42.7 ±0.3 | 21.4 | 329 | 50.7 | 82.9 | 70 | 1000 | 585 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 42.8 ±0.3 | 42.8 | 314 | 66.8 | 85.3 | 74~85 | 1000 | 441 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 42.8 ±0.1 | 21.4 | 315 | 67.8 | 86.1 | 75~76 | 1000 | 587 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 24.9 ±0.4 | 24.9 | 108 | 92.4 | 100.0 | 81~82 | 600~1000 | 504 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 1 | 3 | 48.4 ±0.0 | 48.4 | 243 | 16.9 | 50.7 | 47~50 | 1000 | 390 | ok |
| yolo26n-seg.dxnn | 2 | 3 | 48.4 ±0.1 | 24.2 | 238 | 17.1 | 51.8 | 48 | 1000 | 564 | ok |
| yolo26s-seg.dxnn | 1 | 3 | 48.2 ±0.1 | 48.2 | 232 | 31.3 | 76.7 | 54~59 | 1000 | 406 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 48.2 ±0.1 | 24.1 | 233 | 31.6 | 76.7 | 55~56 | 1000 | 582 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 47.6 ±0.0 | 47.6 | 231 | 56.5 | 92.4 | 72~84 | 1000 | 434 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 47.7 ±0.0 | 23.8 | 228 | 57.6 | 92.5 | 74~75 | 1000 | 586 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 47.1 ±0.0 | 47.1 | 233 | 77.2 | 86.3 | 78~90 | 800~1000 | 440 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 47.1 ±0.0 | 23.6 | 229 | 78.7 | 90.4 | 80 | 800~1000 | 588 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 25.8 ±0.6 | 25.8 | 113 | 93.4 | 100.0 | 82 | 600~1000 | 510 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 1 | 42.5 | 1 | 48.4 |
| yolo26s-seg.dxnn | 1 | 42.5 | 1 | 48.2 |
| yolo26m-seg.dxnn | 1 | 42.5 | 1 | 47.6 |
| yolo26l-seg.dxnn | 1 | 42.8 | 1 | 47.1 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 64.8 ±0.1 | 32.4 | 344 | 66.2 | 87.1 | 56~57 | 1000 | 458 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 64.7 ±0.2 | 21.6 | 342 | 67.0 | 92.1 | 57 | 1000 | 593 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.1 ±0.0 | 44.1 | 145 | 94.1 | 100.0 | 61~68 | 1000 | 332 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.1 ±0.0 | 22.1 | 155 | 96.3 | 100.0 | 62~63 | 1000 | 469 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.9 ±0.0 | 31.9 | 103 | 94.2 | 100.0 | 74~85 | 1000 | 359 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 31.9 ±0.0 | 15.9 | 110 | 96.2 | 100.0 | 76~77 | 1000 | 494 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 24.0 ±0.0 | 24.0 | 78 | 97.4 | 100.0 | 73~76 | 1000 | 364 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.3 ±0.3 | 13.3 | 46 | 91.7 | 100.0 | 81 | 800~1000 | 431 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.1 ±0.1 | 38.0 | 291 | 94.4 | 100.0 | 57~60 | 1000 | 455 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.0 ±0.1 | 25.3 | 294 | 95.4 | 100.0 | 59~60 | 1000 | 561 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.2 ±0.0 | 44.2 | 141 | 93.9 | 100.0 | 62~70 | 1000 | 319 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.2 ±0.0 | 22.1 | 143 | 96.0 | 100.0 | 64 | 1000 | 458 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.9 ±0.0 | 31.9 | 107 | 94.1 | 100.0 | 74~85 | 1000 | 342 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 31.9 ±0.0 | 15.9 | 104 | 96.0 | 100.0 | 76~77 | 1000 | 481 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 23.9 ±0.0 | 23.9 | 81 | 96.9 | 100.0 | 73~76 | 1000 | 352 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.2 ±0.3 | 13.2 | 47 | 91.3 | 100.0 | 81~82 | 800~1000 | 419 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 32.4 | 2 | 38.0 |
| yolo26s-obb.dxnn | 1 | 44.1 | 1 | 44.2 |
| yolo26m-obb.dxnn | 1 | 31.9 | 1 | 31.9 |

---
*Report generated by dx_stream benchmark tool*
