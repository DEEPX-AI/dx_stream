# YOLO26 Benchmark Report

**Generated:** 2026-04-15 17:57:43 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-09 16:47:40 | 2026-04-10 12:12:13 | 19h 24m 32s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 29.70 | 155.8 | 163.5 | 5 |
| yolo26n-1.dxnn | OFF | 24.85 | 172.6 | 172.9 | 5 |
| yolo26s-1.dxnn | ON | 37.57 | 120.3 | 119.6 | 4 |
| yolo26s-1.dxnn | OFF | 33.48 | 122.5 | 122.2 | 4 |
| yolo26m-1.dxnn | ON | 46.09 | 84.7 | 84.6 | 2 |
| yolo26m-1.dxnn | OFF | 40.82 | 84.7 | 84.6 | 2 |
| yolo26l-1.dxnn | ON | 54.44 | 62.5 | 62.5 | 2 |
| yolo26l-1.dxnn | OFF | 49.91 | 62.5 | 62.5 | 2 |
| yolo26x-1.dxnn | ON | 83.64 | 35.3 | 34.3 | 1 |
| yolo26x-1.dxnn | OFF | 78.99 | 35.3 | 34.3 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 25.50 | 213.0 | 178.2 | 6 |
| yolo26n-pose.dxnn | OFF | 22.26 | 219.7 | 200.6 | 7 |
| yolo26s-pose.dxnn | ON | 32.29 | 130.3 | 130.1 | 4 |
| yolo26s-pose.dxnn | OFF | 29.37 | 130.5 | 130.2 | 4 |
| yolo26m-pose.dxnn | ON | 40.48 | 91.1 | 90.9 | 2 |
| yolo26m-pose.dxnn | OFF | 37.15 | 91.0 | 90.9 | 2 |
| yolo26l-pose.dxnn | ON | 48.41 | 68.0 | 67.9 | 2 |
| yolo26l-pose.dxnn | OFF | 45.48 | 68.0 | 68.0 | 2 |
| yolo26x-pose.dxnn | ON | 76.56 | 38.8 | 35.9 | 1 |
| yolo26x-pose.dxnn | OFF | 73.61 | 38.9 | 36.2 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 41.73 | 109.1 | 96.4 | 3 |
| yolo26n-seg.dxnn | OFF | 36.73 | 117.8 | 103.5 | 3 |
| yolo26s-seg.dxnn | ON | 51.17 | 81.7 | 90.7 | 2 |
| yolo26s-seg.dxnn | OFF | 46.60 | 88.4 | 93.8 | 3 |
| yolo26m-seg.dxnn | ON | 65.22 | 64.8 | 57.2 | 1 |
| yolo26m-seg.dxnn | OFF | 60.92 | 64.9 | 59.8 | 1 |
| yolo26l-seg.dxnn | ON | 72.46 | 53.1 | 47.7 | 1 |
| yolo26l-seg.dxnn | OFF | 69.31 | 53.1 | 49.2 | 1 |
| yolo26x-seg.dxnn | ON | 112.29 | 29.2 | 21.0 | — |
| yolo26x-seg.dxnn | OFF | 108.62 | 29.2 | 22.0 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 42.13 | 76.4 | 76.3 | 2 |
| yolo26n-obb.dxnn | OFF | 38.83 | 76.4 | 76.3 | 2 |
| yolo26s-obb.dxnn | ON | 60.46 | 44.2 | 44.1 | 1 |
| yolo26s-obb.dxnn | OFF | 57.23 | 44.2 | 44.1 | 1 |
| yolo26m-obb.dxnn | ON | 79.05 | 31.9 | 31.8 | 1 |
| yolo26m-obb.dxnn | OFF | 75.70 | 31.9 | 31.8 | 1 |
| yolo26l-obb.dxnn | ON | 99.59 | 23.9 | 23.8 | — |
| yolo26l-obb.dxnn | OFF | 96.43 | 23.9 | 23.8 | — |
| yolo26x-obb.dxnn | ON | 173.74 | 13.8 | 11.6 | — |
| yolo26x-obb.dxnn | OFF | 170.07 | 13.8 | 11.5 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.35 | 3343.3 | 285.7 | — |
| yolo26n-cls.dxnn | OFF | 1.35 | 3340.8 | 286.2 | — |
| yolo26s-cls.dxnn | ON | 2.03 | 1882.8 | 292.8 | — |
| yolo26s-cls.dxnn | OFF | 2.00 | 1882.6 | 293.5 | — |
| yolo26m-cls.dxnn | ON | 2.64 | 1351.6 | 293.3 | — |
| yolo26m-cls.dxnn | OFF | 2.62 | 1351.6 | 293.6 | — |
| yolo26l-cls.dxnn | ON | 4.58 | 846.8 | 293.8 | — |
| yolo26l-cls.dxnn | OFF | 4.91 | 847.1 | 294.3 | — |
| yolo26x-cls.dxnn | ON | 7.69 | 455.4 | 294.7 | — |
| yolo26x-cls.dxnn | OFF | 7.75 | 455.3 | 294.3 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | DX-AIPlayer-N97 |
| OS | Ubuntu 24.04.4 LTS |
| Kernel | 6.17.0-19-generic |
| CPU | Intel(R) N97 |
| CPU Cores | 4 |
| RAM | 7.5 GB |
| NPU SKU | M1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.1.0 |
| NPU Driver (PCIe) | v2.0.1 |
| NPU Firmware | v2.5.0 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X2 [03:00:00] |

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
| yolo26n-1.dxnn | 155.8 ±0.7 | 236 | 58.3 | 82.3 | 51~54 | 1000 | ok |
| yolo26s-1.dxnn | 120.3 ±0.2 | 151 | 88.7 | 100.0 | 65~67 | 1000 | ok |
| yolo26m-1.dxnn | 84.7 ±0.0 | 117 | 89.3 | 100.0 | 69~72 | 1000 | ok |
| yolo26l-1.dxnn | 62.5 ±0.0 | 90 | 91.7 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-1.dxnn | 35.3 ±0.0 | 55 | 90.3 | 100.0 | 70~73 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 172.6 ±0.6 | 195 | 68.0 | 84.9 | 63~64 | 1000 | ok |
| yolo26s-1.dxnn | 122.5 ±0.0 | 120 | 92.1 | 100.0 | 68~70 | 1000 | ok |
| yolo26m-1.dxnn | 84.7 ±0.1 | 91 | 91.5 | 100.0 | 69~72 | 1000 | ok |
| yolo26l-1.dxnn | 62.5 ±0.1 | 67 | 90.8 | 100.0 | 69~71 | 1000 | ok |
| yolo26x-1.dxnn | 35.3 ±0.1 | 42 | 88.7 | 100.0 | 70~73 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 213.0 ±0.1 | 191 | 85.4 | 97.1 | 66~68 | 1000 | ok |
| yolo26s-pose.dxnn | 130.3 ±0.3 | 129 | 90.7 | 100.0 | 67~68 | 1000 | ok |
| yolo26m-pose.dxnn | 91.1 ±0.0 | 94 | 90.2 | 100.0 | 64~68 | 1000 | ok |
| yolo26l-pose.dxnn | 68.0 ±0.0 | 72 | 91.8 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-pose.dxnn | 38.8 ±0.0 | 48 | 89.6 | 100.0 | 70~73 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 219.7 ±0.3 | 143 | 91.1 | 100.0 | 66~67 | 1000 | ok |
| yolo26s-pose.dxnn | 130.5 ±0.0 | 101 | 91.6 | 100.0 | 67~69 | 1000 | ok |
| yolo26m-pose.dxnn | 91.0 ±0.0 | 70 | 89.9 | 100.0 | 70~73 | 1000 | ok |
| yolo26l-pose.dxnn | 68.0 ±0.1 | 56 | 90.2 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-pose.dxnn | 38.9 ±0.0 | 38 | 89.4 | 100.0 | 70~73 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 109.1 ±0.5 | 290 | 43.3 | 73.3 | 64~65 | 1000 | ok |
| yolo26s-seg.dxnn | 81.7 ±0.4 | 233 | 64.4 | 85.5 | 61~64 | 1000 | ok |
| yolo26m-seg.dxnn | 64.8 ±0.1 | 136 | 88.7 | 100.0 | 72~76 | 1000 | ok |
| yolo26l-seg.dxnn | 53.1 ±0.0 | 115 | 89.9 | 100.0 | 71~75 | 1000 | ok |
| yolo26x-seg.dxnn | 29.2 ±0.1 | 67 | 88.9 | 100.0 | 71~75 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 117.8 ±0.5 | 255 | 48.5 | 75.6 | 62~63 | 1000 | ok |
| yolo26s-seg.dxnn | 88.4 ±0.7 | 210 | 70.8 | 89.5 | 67~69 | 1000 | ok |
| yolo26m-seg.dxnn | 64.9 ±0.0 | 112 | 89.5 | 100.0 | 71~75 | 1000 | ok |
| yolo26l-seg.dxnn | 53.1 ±0.0 | 95 | 90.1 | 100.0 | 70~74 | 1000 | ok |
| yolo26x-seg.dxnn | 29.2 ±0.0 | 55 | 88.0 | 100.0 | 70~74 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.4 ±0.0 | 87 | 92.1 | 100.0 | 65~66 | 1000 | ok |
| yolo26s-obb.dxnn | 44.2 ±0.1 | 57 | 88.6 | 100.0 | 62~64 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.0 | 44 | 89.4 | 100.0 | 66~69 | 1000 | ok |
| yolo26l-obb.dxnn | 23.9 ±0.0 | 33 | 90.0 | 100.0 | 70~72 | 1000 | ok |
| yolo26x-obb.dxnn | 13.8 ±0.0 | 19 | 85.7 | 100.0 | 70~73 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.4 ±0.0 | 66 | 90.4 | 100.0 | 62~63 | 1000 | ok |
| yolo26s-obb.dxnn | 44.2 ±0.1 | 46 | 91.3 | 100.0 | 63~65 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.0 | 34 | 92.2 | 100.0 | 69~72 | 1000 | ok |
| yolo26l-obb.dxnn | 23.9 ±0.0 | 25 | 89.5 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-obb.dxnn | 13.8 ±0.0 | 14 | 87.4 | 100.0 | 71~74 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3343.3 ±2.5 | 110 | 90.3 | 97.6 | 65 | 1000 | ok |
| yolo26s-cls.dxnn | 1882.8 ±0.6 | 58 | 90.7 | 98.4 | 61~63 | 1000 | ok |
| yolo26m-cls.dxnn | 1351.6 ±0.3 | 40 | 91.3 | 98.9 | 63~66 | 1000 | ok |
| yolo26l-cls.dxnn | 846.8 ±0.1 | 25 | 91.3 | 99.1 | 62~65 | 1000 | ok |
| yolo26x-cls.dxnn | 455.4 ±0.1 | 15 | 92.2 | 100.0 | 63~66 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3340.8 ±3.5 | 110 | 89.3 | 97.8 | 60~61 | 1000 | ok |
| yolo26s-cls.dxnn | 1882.6 ±0.3 | 58 | 91.4 | 98.5 | 61~62 | 1000 | ok |
| yolo26m-cls.dxnn | 1351.6 ±0.3 | 40 | 91.1 | 98.9 | 63~66 | 1000 | ok |
| yolo26l-cls.dxnn | 847.1 ±0.4 | 26 | 90.3 | 99.2 | 62~64 | 1000 | ok |
| yolo26x-cls.dxnn | 455.3 ±0.1 | 15 | 91.5 | 100.0 | 63~66 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 33.7 | 29.70 | 25.77 | 3.92 | 48 | ok |
| yolo26s-1.dxnn | 26.6 | 37.57 | 33.63 | 3.94 | 62 | ok |
| yolo26m-1.dxnn | 21.7 | 46.09 | 42.10 | 3.99 | 65 | ok |
| yolo26l-1.dxnn | 18.4 | 54.44 | 50.46 | 3.98 | 65 | ok |
| yolo26x-1.dxnn | 12.0 | 83.64 | 79.64 | 3.99 | 65 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 40.2 | 24.85 | 24.85 | 61 | ok |
| yolo26s-1.dxnn | 29.9 | 33.48 | 33.48 | 65 | ok |
| yolo26m-1.dxnn | 24.5 | 40.82 | 40.82 | 65 | ok |
| yolo26l-1.dxnn | 20.0 | 49.91 | 49.91 | 65 | ok |
| yolo26x-1.dxnn | 12.7 | 78.99 | 78.99 | 65 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 39.2 | 25.50 | 22.59 | 2.91 | 64 | ok |
| yolo26s-pose.dxnn | 31.0 | 32.29 | 29.39 | 2.89 | 64 | ok |
| yolo26m-pose.dxnn | 24.7 | 40.48 | 37.61 | 2.87 | 59 | ok |
| yolo26l-pose.dxnn | 20.7 | 48.41 | 45.49 | 2.92 | 64 | ok |
| yolo26x-pose.dxnn | 13.1 | 76.56 | 73.52 | 3.04 | 65 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 44.9 | 22.26 | 22.26 | 63 | ok |
| yolo26s-pose.dxnn | 34.0 | 29.37 | 29.37 | 64 | ok |
| yolo26m-pose.dxnn | 26.9 | 37.15 | 37.15 | 65 | ok |
| yolo26l-pose.dxnn | 22.0 | 45.48 | 45.48 | 64 | ok |
| yolo26x-pose.dxnn | 13.6 | 73.61 | 73.61 | 65 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 24.0 | 41.73 | 38.22 | 3.51 | 63 | ok |
| yolo26s-seg.dxnn | 19.5 | 51.17 | 47.75 | 3.42 | 58 | ok |
| yolo26m-seg.dxnn | 15.3 | 65.22 | 61.71 | 3.52 | 66 | ok |
| yolo26l-seg.dxnn | 13.8 | 72.46 | 68.90 | 3.55 | 65 | ok |
| yolo26x-seg.dxnn | 8.9 | 112.29 | 108.77 | 3.52 | 65 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 27.2 | 36.73 | 36.73 | 60 | ok |
| yolo26s-seg.dxnn | 21.5 | 46.60 | 46.60 | 64 | ok |
| yolo26m-seg.dxnn | 16.4 | 60.92 | 60.92 | 65 | ok |
| yolo26l-seg.dxnn | 14.4 | 69.31 | 69.31 | 65 | ok |
| yolo26x-seg.dxnn | 9.2 | 108.62 | 108.62 | 64 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 23.7 | 42.13 | 38.93 | 3.19 | 63 | ok |
| yolo26s-obb.dxnn | 16.5 | 60.46 | 57.20 | 3.25 | 58 | ok |
| yolo26m-obb.dxnn | 12.7 | 79.05 | 75.77 | 3.28 | 61 | ok |
| yolo26l-obb.dxnn | 10.0 | 99.59 | 96.33 | 3.26 | 65 | ok |
| yolo26x-obb.dxnn | 5.8 | 173.74 | 170.45 | 3.29 | 66 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 25.8 | 38.83 | 38.83 | 59 | ok |
| yolo26s-obb.dxnn | 17.5 | 57.23 | 57.23 | 60 | ok |
| yolo26m-obb.dxnn | 13.2 | 75.70 | 75.70 | 65 | ok |
| yolo26l-obb.dxnn | 10.4 | 96.43 | 96.43 | 65 | ok |
| yolo26x-obb.dxnn | 5.9 | 170.07 | 170.07 | 66 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 742.0 | 1.35 | 1.35 | N/A | 64 | ok |
| yolo26s-cls.dxnn | 492.2 | 2.03 | 2.03 | N/A | 58 | ok |
| yolo26m-cls.dxnn | 379.2 | 2.64 | 2.64 | N/A | 58 | ok |
| yolo26l-cls.dxnn | 218.4 | 4.58 | 4.58 | N/A | 58 | ok |
| yolo26x-cls.dxnn | 130.1 | 7.69 | 7.69 | N/A | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 740.8 | 1.35 | 1.35 | 58 | ok |
| yolo26s-cls.dxnn | 500.0 | 2.00 | 2.00 | 58 | ok |
| yolo26m-cls.dxnn | 381.1 | 2.62 | 2.62 | 58 | ok |
| yolo26l-cls.dxnn | 203.6 | 4.91 | 4.91 | 58 | ok |
| yolo26x-cls.dxnn | 129.1 | 7.75 | 7.75 | 58 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vah264dec | 3455 | 3 | 163.5 ±0.6 | 21.13 | 298 | 53.9 | 80.8 | 56~58 | 1000 | 216 | ok |
| yolo26s-1.dxnn | vah264dec | 3455 | 3 | 119.6 ±0.1 | 28.90 | 236 | 88.2 | 100.0 | 68~70 | 1000 | 230 | ok |
| yolo26m-1.dxnn | vah264dec | 3455 | 3 | 84.6 ±0.0 | 40.84 | 144 | 93.3 | 100.0 | 75~78 | 1000 | 248 | ok |
| yolo26l-1.dxnn | vah264dec | 3455 | 3 | 62.5 ±0.0 | 55.33 | 117 | 95.1 | 100.0 | 76~79 | 1000 | 257 | ok |
| yolo26x-1.dxnn | vah264dec | 3455 | 3 | 34.3 ±1.3 | 100.77 | 69 | 95.2 | 100.0 | 80~84 | 600~1000 | 321 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vah264dec | 3455 | 3 | 172.9 ±3.2 | 19.98 | 314 | 60.2 | 84.2 | 64~66 | 1000 | 236 | ok |
| yolo26s-1.dxnn | vah264dec | 3455 | 3 | 122.2 ±0.0 | 28.27 | 240 | 90.5 | 100.0 | 70~72 | 1000 | 244 | ok |
| yolo26m-1.dxnn | vah264dec | 3455 | 3 | 84.6 ±0.0 | 40.85 | 167 | 92.8 | 100.0 | 75~78 | 1000 | 252 | ok |
| yolo26l-1.dxnn | vah264dec | 3455 | 3 | 62.5 ±0.0 | 55.32 | 125 | 94.5 | 100.0 | 75~78 | 1000 | 259 | ok |
| yolo26x-1.dxnn | vah264dec | 3455 | 3 | 34.3 ±1.1 | 100.72 | 77 | 94.9 | 100.0 | 80~84 | 800~1000 | 321 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 163.5 | 172.9 | -9.4 | -5.4% |
| yolo26s-1.dxnn | 119.6 | 122.2 | -2.6 | -2.2% |
| yolo26m-1.dxnn | 84.6 | 84.6 | +0.0 | +0.0% |
| yolo26l-1.dxnn | 62.5 | 62.5 | -0.0 | -0.0% |
| yolo26x-1.dxnn | 34.3 | 34.3 | -0.0 | -0.0% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 178.2 ±0.8 | 19.39 | 258 | 61.3 | 82.1 | 67~68 | 1000 | 199 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 130.1 ±0.2 | 26.56 | 168 | 90.6 | 100.0 | 69~71 | 1000 | 218 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 90.9 ±0.0 | 37.99 | 131 | 93.6 | 100.0 | 72~76 | 1000 | 239 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 67.9 ±0.1 | 50.89 | 103 | 93.8 | 100.0 | 76~79 | 1000 | 247 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 35.9 ±3.2 | 96.15 | 61 | 94.7 | 100.0 | 81~84 | 600~1000 | 328 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 200.6 ±0.6 | 17.22 | 218 | 75.6 | 92.3 | 67~68 | 1000 | 174 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 130.2 ±0.1 | 26.53 | 141 | 89.8 | 100.0 | 70~71 | 1000 | 204 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 90.9 ±0.0 | 37.99 | 113 | 91.0 | 100.0 | 76~78 | 1000 | 224 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.0 ±0.0 | 50.84 | 87 | 93.6 | 100.0 | 76~79 | 1000 | 234 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 36.2 ±2.9 | 95.51 | 53 | 94.7 | 100.0 | 81~84 | 600~1000 | 328 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 178.2 | 200.6 | -22.4 | -11.1% |
| yolo26s-pose.dxnn | 130.1 | 130.2 | -0.2 | -0.1% |
| yolo26m-pose.dxnn | 90.9 | 90.9 | +0.0 | +0.0% |
| yolo26l-pose.dxnn | 67.9 | 68.0 | -0.1 | -0.1% |
| yolo26x-pose.dxnn | 35.9 | 36.2 | -0.2 | -0.7% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 96.4 ±0.6 | 35.85 | 364 | 35.1 | 70.2 | 65~66 | 1000 | 334 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 90.7 ±0.2 | 38.10 | 306 | 70.4 | 83.3 | 67~69 | 1000 | 339 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 57.2 ±7.8 | 60.44 | 172 | 93.4 | 100.0 | 81~84 | 400~1000 | 354 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 47.7 ±5.4 | 72.39 | 136 | 94.0 | 100.0 | 81~84 | 600~1000 | 361 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 21.0 ±2.2 | 164.82 | 65 | 93.9 | 100.0 | 84 | 400~1000 | 434 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 103.5 ±0.4 | 33.37 | 375 | 40.5 | 69.2 | 64~65 | 1000 | 388 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 93.8 ±0.6 | 36.82 | 306 | 74.9 | 88.9 | 71~73 | 1000 | 364 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 59.8 ±5.7 | 57.77 | 176 | 94.0 | 100.0 | 81~84 | 600~1000 | 364 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 49.2 ±4.7 | 70.14 | 145 | 93.4 | 100.0 | 81~84 | 600~1000 | 374 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 22.0 ±2.8 | 157.09 | 68 | 94.9 | 100.0 | 84 | 400~1000 | 445 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 96.4 | 103.5 | -7.2 | -6.9% |
| yolo26s-seg.dxnn | 90.7 | 93.8 | -3.2 | -3.4% |
| yolo26m-seg.dxnn | 57.2 | 59.8 | -2.6 | -4.4% |
| yolo26l-seg.dxnn | 47.7 | 49.2 | -1.5 | -3.1% |
| yolo26x-seg.dxnn | 21.0 | 22.0 | -1.0 | -4.7% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 76.3 ±0.1 | 34.62 | 123 | 91.3 | 100.0 | 67~68 | 1000 | 243 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 44.1 ±0.0 | 59.79 | 86 | 94.6 | 100.0 | 67~69 | 1000 | 256 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 31.8 ±0.1 | 83.08 | 69 | 94.6 | 100.0 | 76~80 | 1000 | 285 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 23.8 ±0.0 | 110.79 | 54 | 96.8 | 100.0 | 79~83 | 800~1000 | 292 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 11.6 ±0.9 | 227.96 | 26 | 89.6 | 100.0 | 84 | 400~1000 | 363 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 76.3 ±0.0 | 34.60 | 127 | 92.0 | 100.0 | 64~66 | 1000 | 237 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 44.1 ±0.0 | 59.81 | 90 | 94.4 | 100.0 | 68~70 | 1000 | 253 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 31.8 ±0.0 | 83.07 | 70 | 94.9 | 100.0 | 78~82 | 1000 | 273 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 23.8 ±0.0 | 110.74 | 55 | 96.8 | 100.0 | 79~83 | 800~1000 | 279 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 11.5 ±0.8 | 228.94 | 25 | 89.2 | 100.0 | 84 | 400~1000 | 354 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 76.3 | 76.3 | -0.0 | -0.1% |
| yolo26s-obb.dxnn | 44.1 | 44.1 | +0.0 | +0.0% |
| yolo26m-obb.dxnn | 31.8 | 31.8 | +0.0 | +0.0% |
| yolo26l-obb.dxnn | 23.8 | 23.8 | -0.0 | -0.0% |
| yolo26x-obb.dxnn | 11.6 | 11.5 | +0.1 | +0.4% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 285.7 ±0.2 | 12.09 | 85 | 6.0 | 23.6 | 63 | 1000 | 82 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 292.8 ±1.5 | 11.80 | 82 | 11.3 | 43.4 | 61 | 1000 | 97 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 293.3 ±0.9 | 11.78 | 82 | 16.3 | 53.6 | 63~64 | 1000 | 100 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 293.8 ±0.5 | 11.76 | 82 | 27.3 | 76.5 | 63 | 1000 | 109 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 294.7 ±1.2 | 11.72 | 81 | 50.9 | 82.2 | 65~66 | 1000 | 181 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 286.2 ±1.1 | 12.07 | 85 | 6.1 | 24.1 | 59 | 1000 | 80 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 293.5 ±0.6 | 11.77 | 82 | 11.9 | 43.6 | 60~61 | 1000 | 111 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 293.6 ±0.8 | 11.77 | 82 | 17.0 | 53.2 | 64 | 1000 | 106 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 294.3 ±0.7 | 11.74 | 82 | 27.2 | 74.8 | 63 | 1000 | 105 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 294.3 ±0.6 | 11.74 | 81 | 48.0 | 81.7 | 65~66 | 1000 | 180 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 285.7 | 286.2 | -0.5 | -0.2% |
| yolo26s-cls.dxnn | 292.8 | 293.5 | -0.7 | -0.2% |
| yolo26m-cls.dxnn | 293.3 | 293.6 | -0.3 | -0.1% |
| yolo26l-cls.dxnn | 293.8 | 294.3 | -0.5 | -0.2% |
| yolo26x-cls.dxnn | 294.7 | 294.3 | +0.4 | +0.1% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 5 | 3 | 164.6 ±0.2 | 32.9 | 321 | 60.6 | 81.0 | 64~69 | 1000 | 421 | ok |
| yolo26n-1.dxnn | 6 | 3 | 162.4 ±1.8 | 27.1 | 321 | 59.7 | 80.4 | 72~74 | 1000 | 465 | ok |
| yolo26s-1.dxnn | 3 | 3 | 120.6 ±0.1 | 40.2 | 227 | 94.5 | 100.0 | 74~77 | 1000 | 348 | ok |
| yolo26s-1.dxnn | 4 | 3 | 120.7 ±0.0 | 30.2 | 228 | 94.9 | 100.0 | 80~82 | 1000 | 396 | ok |
| yolo26s-1.dxnn | 5 | 3 | 120.7 ±0.1 | 24.1 | 230 | 95.7 | 100.0 | 83~84 | 1000 | 422 | ok |
| yolo26m-1.dxnn | 2 | 3 | 81.1 ±4.0 | 40.5 | 146 | 95.9 | 100.0 | 82~85 | 600~1000 | 324 | ok |
| yolo26m-1.dxnn | 3 | 3 | 69.8 ±1.0 | 23.3 | 130 | 97.1 | 100.0 | 84~85 | 600~1000 | 368 | ok |
| yolo26l-1.dxnn | 2 | 3 | 60.5 ±1.9 | 30.3 | 118 | 95.7 | 100.0 | 83~85 | 800~1000 | 334 | ok |
| yolo26l-1.dxnn | 3 | 3 | 54.4 ±0.5 | 18.1 | 111 | 95.7 | 100.0 | 84~85 | 600~1000 | 378 | ok |
| yolo26x-1.dxnn | 1 | 3 | 34.3 ±1.3 | 34.3 | 69 | 95.2 | 100.0 | 84~92 | 600~1000 | 321 | ok |
| yolo26x-1.dxnn | 2 | 3 | 28.2 ±0.0 | 14.1 | 60 | 93.8 | 100.0 | 84 | 600~1000 | 396 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 5 | 3 | 174.9 ±0.3 | 35.0 | 341 | 70.1 | 83.8 | 70~73 | 1000 | 463 | ok |
| yolo26n-1.dxnn | 6 | 3 | 174.8 ±0.3 | 29.1 | 342 | 69.9 | 100.0 | 75~76 | 1000 | 519 | ok |
| yolo26s-1.dxnn | 4 | 3 | 122.3 ±0.0 | 30.6 | 242 | 97.5 | 100.0 | 76~80 | 1000 | 407 | ok |
| yolo26s-1.dxnn | 5 | 3 | 122.3 ±0.0 | 24.5 | 242 | 97.5 | 100.0 | 81~83 | 1000 | 453 | ok |
| yolo26m-1.dxnn | 2 | 3 | 82.5 ±2.7 | 41.2 | 165 | 95.3 | 100.0 | 82~84 | 600~1000 | 334 | ok |
| yolo26m-1.dxnn | 3 | 3 | 69.7 ±0.1 | 23.2 | 136 | 97.5 | 100.0 | 85 | 600~1000 | 373 | ok |
| yolo26l-1.dxnn | 2 | 3 | 60.4 ±1.9 | 30.2 | 125 | 95.6 | 100.0 | 83~85 | 800~1000 | 340 | ok |
| yolo26l-1.dxnn | 3 | 3 | 54.0 ±0.6 | 18.0 | 118 | 95.4 | 100.0 | 84~85 | 600~1000 | 386 | ok |
| yolo26x-1.dxnn | 1 | 3 | 34.3 ±1.1 | 34.3 | 77 | 94.9 | 100.0 | 83~91 | 600~1000 | 321 | ok |
| yolo26x-1.dxnn | 2 | 3 | 28.3 ±0.1 | 14.2 | 65 | 92.2 | 100.0 | 84 | 600~1000 | 400 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 5 | 32.9 | 5 | 35.0 |
| yolo26s-1.dxnn | 4 | 30.2 | 4 | 30.6 |
| yolo26m-1.dxnn | 2 | 40.5 | 2 | 41.2 |
| yolo26l-1.dxnn | 2 | 30.3 | 2 | 30.2 |
| yolo26x-1.dxnn | 1 | 34.3 | 1 | 34.3 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 5 | 3 | 196.9 ±0.4 | 39.4 | 305 | 78.9 | 88.6 | 71~74 | 1000 | 414 | ok |
| yolo26n-pose.dxnn | 6 | 3 | 196.7 ±0.5 | 32.8 | 306 | 78.7 | 87.3 | 76~78 | 1000 | 463 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 196.0 ±0.5 | 28.0 | 308 | 78.9 | 89.1 | 79 | 1000 | 501 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 130.2 ±0.0 | 32.5 | 178 | 96.7 | 100.0 | 76~79 | 1000 | 379 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 130.3 ±0.0 | 26.1 | 179 | 97.0 | 100.0 | 81~82 | 1000 | 412 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 80.9 ±7.9 | 27.0 | 129 | 96.4 | 100.0 | 83~84 | 600~1000 | 359 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 72.6 ±0.1 | 36.3 | 116 | 95.9 | 100.0 | 84 | 600~1000 | 314 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 62.6 ±4.1 | 31.3 | 104 | 95.8 | 100.0 | 83~84 | 600~1000 | 325 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 56.5 ±0.4 | 18.8 | 101 | 95.9 | 100.0 | 84 | 600~1000 | 368 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 35.9 ±3.2 | 35.9 | 61 | 94.7 | 100.0 | 84~92 | 400~800 | 328 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 29.5 ±0.1 | 14.7 | 54 | 95.6 | 100.0 | 84 | 400~800 | 388 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 3 | 214.6 ±0.1 | 35.8 | 268 | 92.0 | 97.9 | 72~75 | 1000 | 464 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 214.6 ±0.1 | 30.6 | 268 | 92.3 | 98.2 | 77~79 | 1000 | 499 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 214.8 ±0.1 | 26.9 | 268 | 91.9 | 98.0 | 80 | 1000 | 531 | ok |
| yolo26s-pose.dxnn | 4 | 2/3 | 130.3 ±0.0 | 32.6 | 158 | 97.3 | 100.0 | 59~75 | 1000 | 375 | partial |
| yolo26s-pose.dxnn | 3 | 3 | 130.2 ±0.1 | 43.4 | 158 | 96.2 | 100.0 | 66~71 | 1000 | 329 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 77.9 ±7.1 | 26.0 | 108 | 96.8 | 100.0 | 84 | 600~1000 | 349 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 73.0 ±0.0 | 36.5 | 99 | 96.0 | 100.0 | 84 | 600~1000 | 304 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 63.0 ±3.8 | 31.5 | 88 | 95.4 | 100.0 | 83~84 | 600~1000 | 313 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 56.8 ±0.3 | 18.9 | 85 | 95.6 | 100.0 | 84 | 600~1000 | 360 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 36.2 ±2.9 | 36.2 | 53 | 94.7 | 100.0 | 84~92 | 600~1000 | 328 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 29.7 ±0.1 | 14.9 | 46 | 95.2 | 100.0 | 84 | 600~1000 | 376 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 6 | 32.8 | 7 | 30.6 |
| yolo26s-pose.dxnn | 4 | 32.5 | 4+ | 32.6 |
| yolo26m-pose.dxnn | 2 | 36.3 | 2 | 36.5 |
| yolo26l-pose.dxnn | 2 | 31.3 | 2 | 31.5 |
| yolo26x-pose.dxnn | 1 | 35.9 | 1 | 36.2 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 95.0 ±0.1 | 31.7 | 364 | 35.6 | 69.8 | 68~69 | 1000 | 465 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 94.6 ±0.4 | 23.7 | 364 | 35.7 | 68.8 | 71 | 1000 | 541 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 89.7 ±0.2 | 29.9 | 311 | 72.1 | 83.8 | 76~80 | 1000 | 486 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 89.7 ±0.2 | 44.9 | 311 | 71.6 | 83.9 | 81~82 | 1000 | 426 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 57.2 ±7.8 | 57.2 | 172 | 93.4 | 100.0 | 84~92 | 400~600 | 354 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 42.3 ±0.3 | 21.2 | 157 | 94.9 | 100.0 | 85 | 400~600 | 436 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 47.7 ±5.4 | 47.7 | 136 | 94.0 | 100.0 | 84~92 | 400~1000 | 361 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 35.7 ±0.4 | 17.8 | 138 | 95.4 | 100.0 | 84 | 400~1000 | 444 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 21.0 ±2.2 | 21.0 | 65 | 93.9 | 100.0 | 84 | 400~1000 | 434 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 103.4 ±0.8 | 34.5 | 380 | 41.3 | 72.1 | 62~65 | 1000 | 507 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 104.4 ±1.5 | 26.1 | 380 | 42.3 | 70.6 | 61~69 | 1000 | 594 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 94.1 ±0.1 | 31.4 | 309 | 79.0 | 89.3 | 78~81 | 1000 | 508 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 93.9 ±0.1 | 23.5 | 311 | 79.3 | 89.3 | 84~85 | 800~1000 | 570 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 59.8 ±5.7 | 59.8 | 176 | 94.0 | 100.0 | 84~92 | 400~800 | 364 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 42.1 ±0.1 | 21.1 | 152 | 95.4 | 100.0 | 85 | 400~800 | 453 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 49.2 ±4.7 | 49.2 | 145 | 93.4 | 100.0 | 83~93 | 400~1000 | 374 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 36.5 ±2.4 | 18.2 | 131 | 95.7 | 100.0 | 84~85 | 400~1000 | 464 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 22.0 ±2.8 | 22.0 | 68 | 94.9 | 100.0 | 84 | 400~1000 | 445 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 31.7 | 3 | 34.5 |
| yolo26s-seg.dxnn | 2 | 44.9 | 3 | 31.4 |
| yolo26m-seg.dxnn | 1 | 57.2 | 1 | 59.8 |
| yolo26l-seg.dxnn | 1 | 47.7 | 1 | 49.2 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.2 ±0.0 | 38.1 | 148 | 95.9 | 100.0 | 70~72 | 1000 | 318 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.2 ±0.1 | 25.4 | 148 | 95.9 | 100.0 | 72~73 | 1000 | 362 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.1 ±0.0 | 44.1 | 86 | 94.6 | 100.0 | 72~80 | 1000 | 256 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.1 ±0.0 | 22.1 | 95 | 96.0 | 100.0 | 73~75 | 1000 | 335 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.8 ±0.1 | 31.8 | 69 | 94.6 | 100.0 | 84~91 | 600~1000 | 285 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 28.4 ±1.3 | 14.2 | 68 | 95.1 | 100.0 | 84 | 600~1000 | 354 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 23.8 ±0.0 | 23.8 | 54 | 96.8 | 100.0 | 79~83 | 800~1000 | 292 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 11.6 ±0.9 | 11.6 | 26 | 89.6 | 100.0 | 84 | 400~1000 | 363 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.3 ±0.0 | 38.1 | 154 | 95.8 | 100.0 | 68~70 | 1000 | 304 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.2 ±0.0 | 25.4 | 154 | 97.0 | 100.0 | 61~73 | 1000 | 352 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.1 ±0.0 | 44.1 | 90 | 94.4 | 100.0 | 73~80 | 1000 | 253 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.1 ±0.0 | 22.1 | 101 | 96.7 | 100.0 | 73~75 | 1000 | 322 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.8 ±0.0 | 31.8 | 70 | 94.9 | 100.0 | 84~92 | 400~1000 | 273 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 27.4 ±0.8 | 13.7 | 67 | 94.4 | 100.0 | 84 | 400~1000 | 346 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 23.8 ±0.0 | 23.8 | 55 | 96.8 | 100.0 | 79~83 | 800~1000 | 279 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 11.5 ±0.8 | 11.5 | 25 | 89.2 | 100.0 | 84 | 400~1000 | 354 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 38.1 | 2 | 38.1 |
| yolo26s-obb.dxnn | 1 | 44.1 | 1 | 44.1 |
| yolo26m-obb.dxnn | 1 | 31.8 | 1 | 31.8 |

---
*Report generated by dx_stream benchmark tool*
