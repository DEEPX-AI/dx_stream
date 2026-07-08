# YOLO26 Benchmark Report

**Generated:** 2026-04-15 17:57:09 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-09 18:52:40 | 2026-04-10 09:59:07 | 15h 6m 27s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 28.30 | 189.7 | 190.9 | 6 |
| yolo26n-1.dxnn | OFF | 23.59 | 192.0 | 201.1 | 6 |
| yolo26s-1.dxnn | ON | 34.11 | 122.3 | 122.6 | 4 |
| yolo26s-1.dxnn | OFF | 31.81 | 123.2 | 123.2 | 4 |
| yolo26m-1.dxnn | ON | 42.61 | 85.3 | 85.2 | 2 |
| yolo26m-1.dxnn | OFF | 39.38 | 85.2 | 85.2 | 2 |
| yolo26l-1.dxnn | ON | 51.68 | 62.9 | 62.8 | 2 |
| yolo26l-1.dxnn | OFF | 48.13 | 62.9 | 62.9 | 2 |
| yolo26x-1.dxnn | ON | 79.23 | 35.5 | 35.5 | 1 |
| yolo26x-1.dxnn | OFF | 77.01 | 35.5 | 35.6 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 20.37 | 221.6 | 221.8 | 7 |
| yolo26n-pose.dxnn | OFF | 16.89 | 222.4 | 222.1 | 7 |
| yolo26s-pose.dxnn | ON | 28.04 | 131.1 | 131.0 | 4 |
| yolo26s-pose.dxnn | OFF | 24.53 | 131.1 | 130.9 | 4 |
| yolo26m-pose.dxnn | ON | 28.68 | 91.5 | 91.5 | 3 |
| yolo26m-pose.dxnn | OFF | 27.91 | 91.6 | 91.5 | 3 |
| yolo26l-pose.dxnn | ON | 36.90 | 68.5 | 68.5 | 2 |
| yolo26l-pose.dxnn | OFF | 35.99 | 68.4 | 68.4 | 2 |
| yolo26x-pose.dxnn | ON | 65.03 | 39.1 | 39.1 | 1 |
| yolo26x-pose.dxnn | OFF | 64.17 | 39.1 | 39.1 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 22.79 | 158.6 | 158.2 | 5 |
| yolo26n-seg.dxnn | OFF | 21.15 | 158.8 | 158.5 | 5 |
| yolo26s-seg.dxnn | ON | 34.37 | 105.8 | 105.4 | 3 |
| yolo26s-seg.dxnn | OFF | 29.22 | 106.0 | 105.8 | 3 |
| yolo26m-seg.dxnn | ON | 45.54 | 65.3 | 65.3 | 2 |
| yolo26m-seg.dxnn | OFF | 43.84 | 65.3 | 65.3 | 2 |
| yolo26l-seg.dxnn | ON | 53.34 | 53.4 | 53.4 | 1 |
| yolo26l-seg.dxnn | OFF | 52.24 | 53.4 | 53.4 | 1 |
| yolo26x-seg.dxnn | ON | 92.57 | 29.3 | 29.3 | — |
| yolo26x-seg.dxnn | OFF | 91.30 | 29.4 | 29.3 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 28.98 | 76.8 | 76.7 | 2 |
| yolo26n-obb.dxnn | OFF | 28.12 | 76.8 | 76.7 | 2 |
| yolo26s-obb.dxnn | ON | 46.94 | 44.4 | 44.4 | 1 |
| yolo26s-obb.dxnn | OFF | 46.34 | 44.4 | 44.4 | 1 |
| yolo26m-obb.dxnn | ON | 70.12 | 32.1 | 32.0 | 1 |
| yolo26m-obb.dxnn | OFF | 64.98 | 32.0 | 32.0 | 1 |
| yolo26l-obb.dxnn | ON | 86.59 | 24.1 | 24.1 | — |
| yolo26l-obb.dxnn | OFF | 85.81 | 24.1 | 24.1 | — |
| yolo26x-obb.dxnn | ON | 160.56 | 13.9 | 13.9 | — |
| yolo26x-obb.dxnn | OFF | 159.57 | 13.8 | 13.9 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.08 | 3353.6 | 624.6 | — |
| yolo26n-cls.dxnn | OFF | 1.07 | 3353.8 | 593.5 | — |
| yolo26s-cls.dxnn | ON | 1.77 | 1890.1 | 544.1 | — |
| yolo26s-cls.dxnn | OFF | 1.70 | 1890.6 | 621.6 | — |
| yolo26m-cls.dxnn | ON | 2.33 | 1351.9 | 620.2 | — |
| yolo26m-cls.dxnn | OFF | 2.36 | 1351.8 | 608.8 | — |
| yolo26l-cls.dxnn | ON | 3.69 | 849.5 | 621.9 | — |
| yolo26l-cls.dxnn | OFF | 3.70 | 849.6 | 621.9 | — |
| yolo26x-cls.dxnn | ON | 6.57 | 457.2 | 456.1 | — |
| yolo26x-cls.dxnn | OFF | 6.51 | 457.1 | 455.9 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | i7-14700K |
| OS | Ubuntu 24.04.2 LTS |
| Kernel | 6.17.0-19-generic |
| CPU | Intel(R) Core(TM) i7-14700K |
| CPU Cores | 28 |
| RAM | 62.5 GB |
| NPU SKU | M1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.2 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X4 [08:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.2.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.24.2 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.24.2 |
| dxtop | Yes | DX-TOP 1.0.1 |
| ffprobe | Yes | ffprobe version 6.1.1-3ubuntu5 Copyright (c) 2007-2023 the F... |

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
| yolo26n-1.dxnn | 189.7 ±13.3 | 143 | 77.0 | 94.1 | 45~46 | 1000 | ok |
| yolo26s-1.dxnn | 122.3 ±0.0 | 133 | 90.6 | 100.0 | 53 | 1000 | ok |
| yolo26m-1.dxnn | 85.3 ±0.0 | 108 | 92.8 | 100.0 | 55~57 | 1000 | ok |
| yolo26l-1.dxnn | 62.9 ±0.0 | 86 | 89.7 | 100.0 | 55~56 | 1000 | ok |
| yolo26x-1.dxnn | 35.5 ±0.0 | 53 | 88.6 | 100.0 | 56~58 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 192.0 ±1.4 | 147 | 77.1 | 88.7 | 51 | 1000 | ok |
| yolo26s-1.dxnn | 123.2 ±0.0 | 114 | 92.6 | 100.0 | 53~54 | 1000 | ok |
| yolo26m-1.dxnn | 85.2 ±0.1 | 91 | 91.1 | 100.0 | 55~57 | 1000 | ok |
| yolo26l-1.dxnn | 62.9 ±0.0 | 68 | 91.1 | 100.0 | 55~57 | 1000 | ok |
| yolo26x-1.dxnn | 35.5 ±0.0 | 42 | 89.0 | 100.0 | 56~58 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 221.6 ±0.1 | 135 | 91.1 | 100.0 | 53 | 1000 | ok |
| yolo26s-pose.dxnn | 131.1 ±0.0 | 116 | 91.2 | 100.0 | 54 | 1000 | ok |
| yolo26m-pose.dxnn | 91.5 ±0.0 | 20 | 91.3 | 100.0 | 57~59 | 1000 | ok |
| yolo26l-pose.dxnn | 68.5 ±0.0 | 16 | 91.7 | 100.0 | 56~58 | 1000 | ok |
| yolo26x-pose.dxnn | 39.1 ±0.0 | 10 | 93.1 | 100.0 | 57~60 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 222.4 ±0.1 | 124 | 90.9 | 100.0 | 53 | 1000 | ok |
| yolo26s-pose.dxnn | 131.1 ±0.0 | 78 | 91.0 | 100.0 | 54~55 | 1000 | ok |
| yolo26m-pose.dxnn | 91.6 ±0.0 | 13 | 90.5 | 100.0 | 57~59 | 1000 | ok |
| yolo26l-pose.dxnn | 68.4 ±0.0 | 36 | 92.9 | 100.0 | 57~59 | 1000 | ok |
| yolo26x-pose.dxnn | 39.1 ±0.0 | 7 | 92.5 | 100.0 | 57~60 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 158.6 ±0.4 | 140 | 71.7 | 88.2 | 53~54 | 1000 | ok |
| yolo26s-seg.dxnn | 105.8 ±0.0 | 93 | 90.7 | 100.0 | 55~56 | 1000 | ok |
| yolo26m-seg.dxnn | 65.3 ±0.0 | 57 | 90.1 | 100.0 | 58~61 | 1000 | ok |
| yolo26l-seg.dxnn | 53.4 ±0.1 | 46 | 92.8 | 100.0 | 58~61 | 1000 | ok |
| yolo26x-seg.dxnn | 29.3 ±0.0 | 25 | 88.7 | 100.0 | 58~62 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 158.8 ±0.1 | 118 | 72.0 | 87.1 | 53~54 | 1000 | ok |
| yolo26s-seg.dxnn | 106.0 ±0.0 | 78 | 89.7 | 100.0 | 55~57 | 1000 | ok |
| yolo26m-seg.dxnn | 65.3 ±0.0 | 48 | 91.5 | 100.0 | 57~60 | 1000 | ok |
| yolo26l-seg.dxnn | 53.4 ±0.0 | 38 | 91.2 | 100.0 | 58~61 | 1000 | ok |
| yolo26x-seg.dxnn | 29.4 ±0.0 | 21 | 88.3 | 100.0 | 58~60 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.8 ±0.0 | 24 | 91.4 | 100.0 | 52 | 1000 | ok |
| yolo26s-obb.dxnn | 44.4 ±0.0 | 14 | 90.7 | 100.0 | 50 | 1000 | ok |
| yolo26m-obb.dxnn | 32.1 ±0.0 | 12 | 89.9 | 100.0 | 53~54 | 1000 | ok |
| yolo26l-obb.dxnn | 24.1 ±0.0 | 8 | 91.1 | 100.0 | 54~55 | 1000 | ok |
| yolo26x-obb.dxnn | 13.9 ±0.0 | 5 | 86.6 | 100.0 | 54~56 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.8 ±0.0 | 18 | 91.6 | 100.0 | 51 | 1000 | ok |
| yolo26s-obb.dxnn | 44.4 ±0.0 | 11 | 91.5 | 100.0 | 51~52 | 1000 | ok |
| yolo26m-obb.dxnn | 32.0 ±0.0 | 9 | 91.4 | 100.0 | 54~55 | 1000 | ok |
| yolo26l-obb.dxnn | 24.1 ±0.0 | 6 | 90.8 | 100.0 | 53~54 | 1000 | ok |
| yolo26x-obb.dxnn | 13.8 ±0.0 | 7 | 87.6 | 100.0 | 55~57 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3353.6 ±0.5 | 26 | 89.5 | 97.4 | 51 | 1000 | ok |
| yolo26s-cls.dxnn | 1890.1 ±0.5 | 16 | 90.4 | 98.5 | 47~48 | 1000 | ok |
| yolo26m-cls.dxnn | 1351.9 ±0.8 | 21 | 91.2 | 98.9 | 51~54 | 1000 | ok |
| yolo26l-cls.dxnn | 849.5 ±0.1 | 7 | 90.8 | 99.4 | 54~55 | 1000 | ok |
| yolo26x-cls.dxnn | 457.2 ±0.1 | 5 | 90.7 | 99.6 | 55~57 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3353.8 ±1.1 | 26 | 88.8 | 97.4 | 47~48 | 1000 | ok |
| yolo26s-cls.dxnn | 1890.6 ±0.3 | 38 | 90.5 | 98.6 | 48~49 | 1000 | ok |
| yolo26m-cls.dxnn | 1351.8 ±0.6 | 21 | 89.4 | 98.9 | 54~55 | 1000 | ok |
| yolo26l-cls.dxnn | 849.6 ±0.3 | 7 | 92.6 | 99.5 | 54 | 1000 | ok |
| yolo26x-cls.dxnn | 457.1 ±0.0 | 4 | 92.2 | 100.0 | 55~57 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 35.3 | 28.30 | 24.60 | 3.69 | 42 | ok |
| yolo26s-1.dxnn | 29.3 | 34.11 | 31.44 | 2.67 | 51 | ok |
| yolo26m-1.dxnn | 23.5 | 42.61 | 39.72 | 2.89 | 51 | ok |
| yolo26l-1.dxnn | 19.4 | 51.68 | 48.61 | 3.06 | 51 | ok |
| yolo26x-1.dxnn | 12.6 | 79.23 | 76.83 | 2.40 | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 42.4 | 23.59 | 23.59 | 50 | ok |
| yolo26s-1.dxnn | 31.4 | 31.81 | 31.81 | 51 | ok |
| yolo26m-1.dxnn | 25.4 | 39.38 | 39.38 | 51 | ok |
| yolo26l-1.dxnn | 20.8 | 48.13 | 48.13 | 52 | ok |
| yolo26x-1.dxnn | 13.0 | 77.01 | 77.01 | 52 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 49.1 | 20.37 | 17.19 | 3.18 | 52 | ok |
| yolo26s-pose.dxnn | 35.7 | 28.04 | 24.89 | 3.16 | 51 | ok |
| yolo26m-pose.dxnn | 34.9 | 28.68 | 27.95 | 0.73 | 52 | ok |
| yolo26l-pose.dxnn | 27.1 | 36.90 | 36.13 | 0.77 | 52 | ok |
| yolo26x-pose.dxnn | 15.4 | 65.03 | 64.23 | 0.79 | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 59.2 | 16.89 | 16.89 | 51 | ok |
| yolo26s-pose.dxnn | 40.8 | 24.53 | 24.53 | 51 | ok |
| yolo26m-pose.dxnn | 35.8 | 27.91 | 27.91 | 52 | ok |
| yolo26l-pose.dxnn | 27.8 | 35.99 | 35.99 | 52 | ok |
| yolo26x-pose.dxnn | 15.6 | 64.17 | 64.17 | 53 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 43.9 | 22.79 | 21.48 | 1.31 | 52 | ok |
| yolo26s-seg.dxnn | 29.1 | 34.37 | 32.99 | 1.38 | 51 | ok |
| yolo26m-seg.dxnn | 22.0 | 45.54 | 44.18 | 1.36 | 52 | ok |
| yolo26l-seg.dxnn | 18.7 | 53.34 | 51.95 | 1.39 | 52 | ok |
| yolo26x-seg.dxnn | 10.8 | 92.57 | 91.16 | 1.41 | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 47.3 | 21.15 | 21.15 | 51 | ok |
| yolo26s-seg.dxnn | 34.2 | 29.22 | 29.22 | 52 | ok |
| yolo26m-seg.dxnn | 22.8 | 43.84 | 43.84 | 52 | ok |
| yolo26l-seg.dxnn | 19.1 | 52.24 | 52.24 | 52 | ok |
| yolo26x-seg.dxnn | 11.0 | 91.30 | 91.30 | 52 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 34.5 | 28.98 | 28.18 | 0.80 | 51 | ok |
| yolo26s-obb.dxnn | 21.3 | 46.94 | 46.17 | 0.77 | 49 | ok |
| yolo26m-obb.dxnn | 14.3 | 70.12 | 68.66 | 1.46 | 50 | ok |
| yolo26l-obb.dxnn | 11.5 | 86.59 | 85.74 | 0.84 | 51 | ok |
| yolo26x-obb.dxnn | 6.2 | 160.56 | 159.67 | 0.88 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 35.6 | 28.12 | 28.12 | 51 | ok |
| yolo26s-obb.dxnn | 21.6 | 46.34 | 46.34 | 51 | ok |
| yolo26m-obb.dxnn | 15.4 | 64.98 | 64.98 | 51 | ok |
| yolo26l-obb.dxnn | 11.7 | 85.81 | 85.81 | 51 | ok |
| yolo26x-obb.dxnn | 6.3 | 159.57 | 159.57 | 51 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 925.7 | 1.08 | 1.08 | N/A | 51 | ok |
| yolo26s-cls.dxnn | 566.0 | 1.77 | 1.77 | N/A | 45 | ok |
| yolo26m-cls.dxnn | 428.9 | 2.33 | 2.33 | N/A | 47 | ok |
| yolo26l-cls.dxnn | 270.9 | 3.69 | 3.69 | N/A | 51 | ok |
| yolo26x-cls.dxnn | 152.2 | 6.57 | 6.57 | N/A | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 936.2 | 1.07 | 1.07 | 48 | ok |
| yolo26s-cls.dxnn | 588.6 | 1.70 | 1.70 | 46 | ok |
| yolo26m-cls.dxnn | 424.0 | 2.36 | 2.36 | 51 | ok |
| yolo26l-cls.dxnn | 270.2 | 3.70 | 3.70 | 51 | ok |
| yolo26x-cls.dxnn | 153.6 | 6.51 | 6.51 | 52 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vah264dec | 3455 | 3 | 190.9 ±0.6 | 18.10 | 192 | 69.6 | 91.8 | 47 | 1000 | 224 | ok |
| yolo26s-1.dxnn | vah264dec | 3455 | 3 | 122.6 ±0.1 | 28.17 | 154 | 92.3 | 100.0 | 53~54 | 1000 | 238 | ok |
| yolo26m-1.dxnn | vah264dec | 3455 | 3 | 85.2 ±0.0 | 40.56 | 130 | 93.0 | 100.0 | 59~60 | 1000 | 259 | ok |
| yolo26l-1.dxnn | vah264dec | 3455 | 3 | 62.8 ±0.1 | 54.98 | 105 | 94.4 | 100.0 | 59~61 | 1000 | 266 | ok |
| yolo26x-1.dxnn | vah264dec | 3455 | 3 | 35.5 ±0.0 | 97.17 | 68 | 96.0 | 100.0 | 63~66 | 1000 | 329 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vah264dec | 3455 | 3 | 201.1 ±0.6 | 17.18 | 180 | 73.9 | 93.1 | 51 | 1000 | 236 | ok |
| yolo26s-1.dxnn | vah264dec | 3455 | 3 | 123.2 ±0.0 | 28.05 | 147 | 91.8 | 100.0 | 54 | 1000 | 241 | ok |
| yolo26m-1.dxnn | vah264dec | 3455 | 3 | 85.2 ±0.0 | 40.56 | 123 | 92.8 | 100.0 | 59~60 | 1000 | 258 | ok |
| yolo26l-1.dxnn | vah264dec | 3455 | 3 | 62.9 ±0.0 | 54.96 | 103 | 94.3 | 100.0 | 59~61 | 1000 | 266 | ok |
| yolo26x-1.dxnn | vah264dec | 3455 | 3 | 35.6 ±0.0 | 97.17 | 72 | 96.3 | 100.0 | 63~65 | 1000 | 324 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 190.9 | 201.1 | -10.2 | -5.1% |
| yolo26s-1.dxnn | 122.6 | 123.2 | -0.6 | -0.5% |
| yolo26m-1.dxnn | 85.2 | 85.2 | +0.0 | +0.0% |
| yolo26l-1.dxnn | 62.8 | 62.9 | -0.0 | -0.0% |
| yolo26x-1.dxnn | 35.5 | 35.6 | -0.0 | -0.0% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 221.8 ±0.3 | 15.58 | 153 | 84.7 | 100.0 | 53 | 1000 | 209 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 131.0 ±0.1 | 26.38 | 143 | 91.2 | 100.0 | 55 | 1000 | 225 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 91.5 ±0.0 | 37.74 | 32 | 94.0 | 100.0 | 61~63 | 1000 | 246 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.5 ±0.0 | 50.45 | 25 | 94.6 | 100.0 | 61~63 | 1000 | 256 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 39.1 ±0.0 | 88.33 | 15 | 96.8 | 100.0 | 65~68 | 1000 | 333 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 222.1 ±0.1 | 15.55 | 135 | 83.6 | 100.0 | 53 | 1000 | 196 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 130.9 ±0.1 | 26.39 | 87 | 91.8 | 100.0 | 55 | 1000 | 210 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 91.5 ±0.1 | 37.77 | 26 | 92.9 | 100.0 | 61~63 | 1000 | 229 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.4 ±0.1 | 50.50 | 21 | 93.8 | 100.0 | 62~63 | 1000 | 238 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 39.1 ±0.0 | 88.33 | 12 | 96.6 | 100.0 | 65~68 | 1000 | 333 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 221.8 | 222.1 | -0.4 | -0.2% |
| yolo26s-pose.dxnn | 131.0 | 130.9 | +0.0 | +0.0% |
| yolo26m-pose.dxnn | 91.5 | 91.5 | +0.1 | +0.1% |
| yolo26l-pose.dxnn | 68.5 | 68.4 | +0.1 | +0.1% |
| yolo26x-pose.dxnn | 39.1 | 39.1 | +0.0 | +0.0% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 158.2 ±0.1 | 21.84 | 182 | 69.0 | 89.1 | 53~54 | 1000 | 320 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 105.4 ±0.1 | 32.77 | 123 | 88.7 | 100.0 | 57~58 | 1000 | 333 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 65.3 ±0.0 | 52.93 | 74 | 95.1 | 100.0 | 65~68 | 1000 | 354 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 53.4 ±0.0 | 64.67 | 61 | 94.2 | 100.0 | 65~68 | 1000 | 364 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 29.3 ±0.0 | 117.85 | 39 | 95.7 | 100.0 | 69~73 | 1000 | 434 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 158.5 ±0.1 | 21.79 | 189 | 68.0 | 89.5 | 54 | 1000 | 330 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 105.8 ±0.1 | 32.65 | 126 | 90.9 | 100.0 | 57~58 | 1000 | 342 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 65.3 ±0.0 | 52.92 | 75 | 94.1 | 100.0 | 64~67 | 1000 | 353 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 53.4 ±0.0 | 64.69 | 61 | 95.7 | 100.0 | 65~68 | 1000 | 365 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 29.3 ±0.0 | 117.81 | 33 | 96.1 | 100.0 | 67~69 | 1000 | 431 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 158.2 | 158.5 | -0.4 | -0.2% |
| yolo26s-seg.dxnn | 105.4 | 105.8 | -0.4 | -0.4% |
| yolo26m-seg.dxnn | 65.3 | 65.3 | -0.0 | -0.0% |
| yolo26l-seg.dxnn | 53.4 | 53.4 | +0.0 | +0.0% |
| yolo26x-seg.dxnn | 29.3 | 29.3 | -0.0 | -0.0% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 76.7 ±0.0 | 34.40 | 42 | 90.4 | 100.0 | 52 | 1000 | 250 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 44.4 ±0.0 | 59.47 | 25 | 94.6 | 100.0 | 51 | 1000 | 265 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 32.0 ±0.0 | 82.37 | 19 | 95.4 | 100.0 | 56~58 | 1000 | 291 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 24.1 ±0.0 | 109.69 | 17 | 97.0 | 100.0 | 58~59 | 1000 | 299 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 13.9 ±0.0 | 190.35 | 8 | 93.6 | 100.0 | 62~65 | 1000 | 366 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 76.7 ±0.0 | 34.40 | 43 | 90.7 | 100.0 | 51 | 1000 | 242 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 44.4 ±0.0 | 59.48 | 26 | 95.3 | 100.0 | 51~52 | 1000 | 258 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 32.0 ±0.0 | 82.38 | 19 | 94.1 | 100.0 | 57~59 | 1000 | 268 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 24.1 ±0.0 | 109.67 | 14 | 96.8 | 100.0 | 57~59 | 1000 | 291 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 13.9 ±0.0 | 190.25 | 8 | 93.3 | 100.0 | 63~64 | 1000 | 358 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 76.7 | 76.7 | +0.0 | +0.0% |
| yolo26s-obb.dxnn | 44.4 | 44.4 | +0.0 | +0.0% |
| yolo26m-obb.dxnn | 32.0 | 32.0 | +0.0 | +0.0% |
| yolo26l-obb.dxnn | 24.1 | 24.1 | +0.0 | +0.0% |
| yolo26x-obb.dxnn | 13.9 | 13.9 | -0.0 | -0.1% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 624.6 ±4.1 | 5.53 | 53 | 12.2 | 39.8 | 48~49 | 1000 | 92 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 544.1 ±19.8 | 6.35 | 60 | 18.9 | 62.8 | 46 | 1000 | 104 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 620.2 ±3.5 | 5.57 | 54 | 30.1 | 69.2 | 51~52 | 1000 | 130 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 621.9 ±0.6 | 5.56 | 53 | 49.8 | 80.2 | 53 | 1000 | 123 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 456.1 ±0.3 | 7.58 | 41 | 72.5 | 100.0 | 57 | 1000 | 186 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 593.5 ±44.7 | 5.82 | 56 | 11.4 | 39.3 | 46 | 1000 | 92 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 621.6 ±4.8 | 5.56 | 53 | 21.4 | 65.7 | 47~48 | 1000 | 105 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 608.8 ±24.2 | 5.67 | 55 | 29.3 | 69.6 | 53 | 1000 | 114 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 621.9 ±2.0 | 5.55 | 53 | 50.6 | 79.5 | 53 | 1000 | 126 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 455.9 ±0.3 | 7.58 | 42 | 71.4 | 99.6 | 56 | 1000 | 186 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 624.6 | 593.5 | +31.1 | +5.2% |
| yolo26s-cls.dxnn | 544.1 | 621.6 | -77.5 | -12.5% |
| yolo26m-cls.dxnn | 620.2 | 608.8 | +11.4 | +1.9% |
| yolo26l-cls.dxnn | 621.9 | 621.9 | -0.1 | -0.0% |
| yolo26x-cls.dxnn | 456.1 | 455.9 | +0.2 | +0.0% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 6 | 3 | 183.2 ±0.8 | 30.5 | 220 | 77.3 | 90.0 | 50~51 | 1000 | 460 | ok |
| yolo26n-1.dxnn | 7 | 3 | 183.1 ±0.3 | 26.2 | 221 | 77.3 | 88.9 | 51~52 | 1000 | 486 | ok |
| yolo26s-1.dxnn | 4 | 3 | 122.4 ±0.1 | 30.6 | 173 | 96.3 | 100.0 | 56~57 | 1000 | 395 | ok |
| yolo26s-1.dxnn | 5 | 3 | 122.4 ±0.0 | 24.5 | 172 | 97.2 | 100.0 | 57 | 1000 | 428 | ok |
| yolo26m-1.dxnn | 2 | 3 | 85.3 ±0.0 | 42.6 | 140 | 96.4 | 100.0 | 62~63 | 1000 | 331 | ok |
| yolo26m-1.dxnn | 3 | 3 | 85.3 ±0.0 | 28.4 | 140 | 96.8 | 100.0 | 64~65 | 1000 | 372 | ok |
| yolo26l-1.dxnn | 2 | 3 | 62.9 ±0.0 | 31.5 | 112 | 97.4 | 100.0 | 62~63 | 1000 | 342 | ok |
| yolo26l-1.dxnn | 3 | 3 | 62.9 ±0.0 | 21.0 | 112 | 98.1 | 100.0 | 64 | 1000 | 383 | ok |
| yolo26x-1.dxnn | 1 | 3 | 35.5 ±0.0 | 35.5 | 68 | 96.0 | 100.0 | 67~76 | 1000 | 329 | ok |
| yolo26x-1.dxnn | 2 | 3 | 35.6 ±0.0 | 17.8 | 70 | 97.8 | 100.0 | 67~68 | 1000 | 402 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 6 | 3 | 200.3 ±0.2 | 33.4 | 202 | 86.0 | 95.2 | 52~53 | 1000 | 477 | ok |
| yolo26n-1.dxnn | 7 | 3 | 200.6 ±0.1 | 28.7 | 201 | 86.6 | 94.0 | 53 | 1000 | 527 | ok |
| yolo26s-1.dxnn | 4 | 3 | 123.3 ±0.0 | 30.8 | 159 | 97.8 | 100.0 | 56 | 1000 | 402 | ok |
| yolo26s-1.dxnn | 5 | 3 | 123.3 ±0.1 | 24.7 | 159 | 97.5 | 100.0 | 56 | 1000 | 443 | ok |
| yolo26m-1.dxnn | 2 | 3 | 85.3 ±0.0 | 42.6 | 131 | 95.9 | 100.0 | 62~64 | 1000 | 334 | ok |
| yolo26m-1.dxnn | 3 | 3 | 85.3 ±0.0 | 28.4 | 131 | 96.8 | 100.0 | 64~65 | 1000 | 373 | ok |
| yolo26l-1.dxnn | 2 | 3 | 62.9 ±0.0 | 31.5 | 106 | 97.7 | 100.0 | 62~63 | 1000 | 343 | ok |
| yolo26l-1.dxnn | 3 | 3 | 62.9 ±0.0 | 21.0 | 106 | 97.9 | 100.0 | 64 | 1000 | 384 | ok |
| yolo26x-1.dxnn | 1 | 3 | 35.6 ±0.0 | 35.6 | 72 | 96.3 | 100.0 | 67~76 | 1000 | 324 | ok |
| yolo26x-1.dxnn | 2 | 3 | 35.6 ±0.0 | 17.8 | 74 | 97.4 | 100.0 | 67 | 1000 | 395 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 6 | 30.5 | 6 | 33.4 |
| yolo26s-1.dxnn | 4 | 30.6 | 4 | 30.8 |
| yolo26m-1.dxnn | 2 | 42.6 | 2 | 42.6 |
| yolo26l-1.dxnn | 2 | 31.5 | 2 | 31.5 |
| yolo26x-1.dxnn | 1 | 35.5 | 1 | 35.6 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 221.7 ±0.1 | 31.7 | 198 | 96.5 | 100.0 | 55 | 1000 | 477 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 221.6 ±0.0 | 27.7 | 198 | 96.4 | 100.0 | 55~56 | 1000 | 521 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 131.1 ±0.0 | 32.8 | 166 | 97.6 | 100.0 | 57~58 | 1000 | 385 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 131.0 ±0.1 | 26.2 | 166 | 97.3 | 100.0 | 58~59 | 1000 | 419 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 91.6 ±0.0 | 30.5 | 35 | 96.7 | 100.0 | 66~67 | 1000 | 367 | ok |
| yolo26m-pose.dxnn | 4 | 3 | 91.6 ±0.0 | 22.9 | 45 | 97.0 | 100.0 | 67~68 | 1000 | 406 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.5 ±0.0 | 34.2 | 26 | 97.0 | 100.0 | 66~67 | 1000 | 333 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.5 ±0.0 | 22.8 | 27 | 97.7 | 100.0 | 67 | 1000 | 375 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 39.1 ±0.0 | 39.1 | 15 | 96.8 | 100.0 | 70~81 | 1000 | 333 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 39.1 ±0.0 | 19.6 | 16 | 98.2 | 100.0 | 71~72 | 1000 | 395 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 222.2 ±0.2 | 31.8 | 220 | 96.7 | 100.0 | 55~56 | 1000 | 474 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 222.3 ±0.1 | 27.8 | 219 | 97.2 | 100.0 | 56 | 1000 | 518 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 131.0 ±0.0 | 32.8 | 78 | 97.2 | 100.0 | 57~58 | 1000 | 380 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 131.0 ±0.0 | 26.2 | 76 | 97.4 | 100.0 | 59 | 1000 | 417 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 91.6 ±0.0 | 30.5 | 29 | 97.5 | 100.0 | 66~67 | 1000 | 353 | ok |
| yolo26m-pose.dxnn | 4 | 3 | 91.6 ±0.0 | 22.9 | 29 | 97.3 | 100.0 | 68 | 1000 | 399 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.5 ±0.0 | 34.2 | 28 | 97.1 | 100.0 | 66~67 | 1000 | 317 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.5 ±0.0 | 22.8 | 22 | 97.8 | 100.0 | 67~68 | 1000 | 362 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 39.1 ±0.0 | 39.1 | 12 | 96.6 | 100.0 | 70~80 | 1000 | 333 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 39.1 ±0.0 | 19.6 | 19 | 97.9 | 100.0 | 70~71 | 1000 | 378 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 7 | 31.7 | 7 | 31.8 |
| yolo26s-pose.dxnn | 4 | 32.8 | 4 | 32.8 |
| yolo26m-pose.dxnn | 3 | 30.5 | 3 | 30.5 |
| yolo26l-pose.dxnn | 2 | 34.2 | 2 | 34.2 |
| yolo26x-pose.dxnn | 1 | 39.1 | 1 | 39.1 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 5 | 3 | 158.3 ±0.1 | 31.7 | 192 | 75.4 | 89.2 | 55~56 | 1000 | 530 | ok |
| yolo26n-seg.dxnn | 6 | 3 | 158.3 ±0.0 | 26.4 | 191 | 76.0 | 90.1 | 56 | 1000 | 578 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 102.9 ±1.6 | 34.3 | 155 | 90.8 | 100.0 | 60~62 | 1000 | 470 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 105.7 ±0.0 | 26.4 | 128 | 95.0 | 100.0 | 62~63 | 1000 | 504 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 65.3 ±0.0 | 32.6 | 75 | 96.7 | 100.0 | 70~71 | 1000 | 438 | ok |
| yolo26m-seg.dxnn | 3 | 3 | 65.2 ±0.0 | 21.8 | 81 | 97.5 | 100.0 | 73 | 1000 | 487 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 53.4 ±0.0 | 53.4 | 61 | 94.2 | 100.0 | 70~82 | 1000 | 364 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 53.4 ±0.0 | 26.7 | 71 | 96.8 | 100.0 | 71~73 | 1000 | 450 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 29.3 ±0.0 | 29.3 | 39 | 95.7 | 100.0 | 69~73 | 1000 | 434 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 5 | 3 | 158.8 ±0.0 | 31.8 | 199 | 75.9 | 88.2 | 55~56 | 1000 | 554 | ok |
| yolo26n-seg.dxnn | 6 | 3 | 158.6 ±0.5 | 26.4 | 199 | 75.8 | 88.4 | 56 | 1000 | 610 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 105.9 ±0.0 | 35.3 | 130 | 95.3 | 100.0 | 60~62 | 1000 | 478 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 105.9 ±0.0 | 26.5 | 132 | 95.8 | 100.0 | 62 | 1000 | 527 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 65.3 ±0.0 | 32.6 | 76 | 96.7 | 100.0 | 70~71 | 1000 | 446 | ok |
| yolo26m-seg.dxnn | 3 | 3 | 65.3 ±0.0 | 21.8 | 82 | 97.3 | 100.0 | 73 | 1000 | 497 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 53.4 ±0.0 | 53.4 | 61 | 95.7 | 100.0 | 70~81 | 1000 | 365 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 53.4 ±0.0 | 26.7 | 62 | 97.2 | 100.0 | 71~72 | 1000 | 458 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 29.3 ±0.0 | 29.3 | 33 | 96.1 | 100.0 | 67~69 | 1000 | 431 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 5 | 31.7 | 5 | 31.8 |
| yolo26s-seg.dxnn | 3 | 34.3 | 3 | 35.3 |
| yolo26m-seg.dxnn | 2 | 32.6 | 2 | 32.6 |
| yolo26l-seg.dxnn | 1 | 53.4 | 1 | 53.4 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.8 ±0.0 | 38.4 | 45 | 95.8 | 100.0 | 52 | 1000 | 324 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.8 ±0.0 | 25.6 | 44 | 97.2 | 100.0 | 53 | 1000 | 372 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.4 ±0.0 | 44.4 | 25 | 94.6 | 100.0 | 52~57 | 1000 | 265 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.4 ±0.0 | 22.2 | 26 | 96.7 | 100.0 | 53 | 1000 | 340 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 32.0 ±0.0 | 32.0 | 19 | 95.4 | 100.0 | 59~67 | 1000 | 291 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 32.0 ±0.0 | 16.0 | 27 | 96.5 | 100.0 | 59~60 | 1000 | 362 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 24.1 ±0.0 | 24.1 | 17 | 97.0 | 100.0 | 58~59 | 1000 | 299 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.9 ±0.0 | 13.9 | 8 | 93.6 | 100.0 | 62~65 | 1000 | 366 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.8 ±0.0 | 38.4 | 45 | 96.0 | 100.0 | 50~51 | 1000 | 311 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.8 ±0.0 | 25.6 | 46 | 96.8 | 100.0 | 50~51 | 1000 | 354 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.4 ±0.0 | 44.4 | 26 | 95.3 | 100.0 | 52~57 | 1000 | 258 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.4 ±0.0 | 22.2 | 30 | 96.6 | 100.0 | 52 | 1000 | 324 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 32.0 ±0.0 | 32.0 | 19 | 94.1 | 100.0 | 59~68 | 1000 | 268 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 32.0 ±0.0 | 16.0 | 20 | 96.3 | 100.0 | 60 | 1000 | 346 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 24.1 ±0.0 | 24.1 | 14 | 96.8 | 100.0 | 57~59 | 1000 | 291 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.9 ±0.0 | 13.9 | 8 | 93.3 | 100.0 | 63~64 | 1000 | 358 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 38.4 | 2 | 38.4 |
| yolo26s-obb.dxnn | 1 | 44.4 | 1 | 44.4 |
| yolo26m-obb.dxnn | 1 | 32.0 | 1 | 32.0 |

---
*Report generated by dx_stream benchmark tool*
