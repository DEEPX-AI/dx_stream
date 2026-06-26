# YOLO26 Benchmark Report

**Generated:** 2026-04-22 14:38:51 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-21 20:19:39 | 2026-04-22 14:38:51 | 18h 19m 12s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 23.14 | 199.1 | 172.7 | 5 |
| yolo26n.dxnn | OFF | 22.32 | 222.0 | 188.4 | 6 |
| yolo26s.dxnn | ON | 32.02 | 138.8 | 137.1 | 4 |
| yolo26s.dxnn | OFF | 30.25 | 138.1 | 138.0 | 4 |
| yolo26m.dxnn | ON | 39.51 | 96.0 | 96.0 | 2 |
| yolo26m.dxnn | OFF | 37.23 | 96.0 | 96.0 | 2 |
| yolo26l.dxnn | ON | 46.90 | 70.3 | 70.2 | 2 |
| yolo26l.dxnn | OFF | 45.99 | 70.3 | 70.3 | 2 |
| yolo26x.dxnn | ON | 73.82 | 40.3 | 34.8 | 1 |
| yolo26x.dxnn | OFF | 72.54 | 40.2 | 34.9 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 21.63 | 227.7 | 182.9 | 7 |
| yolo26n-pose.dxnn | OFF | 20.47 | 228.8 | 209.6 | 7 |
| yolo26s-pose.dxnn | ON | 29.13 | 133.3 | 133.6 | 4 |
| yolo26s-pose.dxnn | OFF | 27.61 | 133.2 | 133.1 | 4 |
| yolo26m-pose.dxnn | ON | 36.42 | 92.6 | 93.0 | 2 |
| yolo26m-pose.dxnn | OFF | 35.48 | 92.7 | 92.7 | 2 |
| yolo26l-pose.dxnn | ON | 44.74 | 68.3 | 68.4 | 2 |
| yolo26l-pose.dxnn | OFF | 43.25 | 68.5 | 68.5 | 2 |
| yolo26x-pose.dxnn | ON | 72.00 | 39.3 | 34.7 | 1 |
| yolo26x-pose.dxnn | OFF | 71.10 | 39.1 | 35.1 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 37.69 | 124.7 | 106.3 | 3 |
| yolo26n-seg.dxnn | OFF | 35.36 | 136.3 | 115.2 | 3 |
| yolo26s-seg.dxnn | ON | 47.29 | 100.2 | 98.3 | 3 |
| yolo26s-seg.dxnn | OFF | 45.25 | 102.8 | 102.5 | 3 |
| yolo26m-seg.dxnn | ON | 60.96 | 69.6 | 59.8 | 1 |
| yolo26m-seg.dxnn | OFF | 59.31 | 68.9 | 64.8 | 1 |
| yolo26l-seg.dxnn | ON | 68.88 | 54.2 | 52.9 | 1 |
| yolo26l-seg.dxnn | OFF | 67.24 | 54.2 | 52.4 | 1 |
| yolo26x-seg.dxnn | ON | 107.59 | 30.2 | 22.9 | — |
| yolo26x-seg.dxnn | OFF | 105.80 | 30.6 | 22.8 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 35.74 | 79.9 | 79.8 | 2 |
| yolo26n-obb.dxnn | OFF | 34.48 | 79.8 | 80.0 | 2 |
| yolo26s-obb.dxnn | ON | 52.90 | 46.4 | 46.4 | 1 |
| yolo26s-obb.dxnn | OFF | 52.20 | 46.5 | 46.5 | 1 |
| yolo26m-obb.dxnn | ON | 71.45 | 33.9 | 34.0 | 1 |
| yolo26m-obb.dxnn | OFF | 69.70 | 33.9 | 33.9 | 1 |
| yolo26l-obb.dxnn | ON | 91.58 | 24.7 | 24.8 | — |
| yolo26l-obb.dxnn | OFF | 89.85 | 24.7 | 24.8 | — |
| yolo26x-obb.dxnn | ON | 163.28 | 14.2 | 12.1 | — |
| yolo26x-obb.dxnn | OFF | 161.76 | 14.3 | 12.0 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.33 | 3532.3 | 283.4 | — |
| yolo26n-cls.dxnn | OFF | 1.36 | 3535.5 | 281.1 | — |
| yolo26s-cls.dxnn | ON | 1.99 | 1917.1 | 290.6 | — |
| yolo26s-cls.dxnn | OFF | 2.02 | 1918.2 | 290.0 | — |
| yolo26m-cls.dxnn | ON | 2.66 | 1357.0 | 291.2 | — |
| yolo26m-cls.dxnn | OFF | 2.64 | 1357.0 | 291.8 | — |
| yolo26l-cls.dxnn | ON | 3.99 | 854.0 | 292.8 | — |
| yolo26l-cls.dxnn | OFF | 4.21 | 852.4 | 291.8 | — |
| yolo26x-cls.dxnn | ON | 6.82 | 460.1 | 292.0 | — |
| yolo26x-cls.dxnn | OFF | 6.74 | 459.9 | 293.0 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | deepx |
| OS | Ubuntu 24.04.4 LTS |
| Kernel | 6.17.0-20-generic |
| CPU | Intel(R) N97 |
| CPU Cores | 4 |
| RAM | 7.5 GB |
| NPU SKU | M1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X2 [03:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.3.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.24.2 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.24.2 |
| dxtop | Yes | DX-TOP 1.1.0 |
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
| yolo26n.dxnn | 199.1 ±1.1 | 245 | 67.1 | 83.7 | 52~55 | 1000 | ok |
| yolo26s.dxnn | 138.8 ±0.1 | 153 | 91.1 | 100.0 | 65~67 | 1000 | ok |
| yolo26m.dxnn | 96.0 ±0.2 | 134 | 91.6 | 100.0 | 70~74 | 1000 | ok |
| yolo26l.dxnn | 70.3 ±0.2 | 109 | 91.3 | 100.0 | 70~73 | 1000 | ok |
| yolo26x.dxnn | 40.3 ±0.3 | 63 | 88.1 | 100.0 | 71~75 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 222.0 ±0.8 | 188 | 80.6 | 93.2 | 63~65 | 1000 | ok |
| yolo26s.dxnn | 138.1 ±0.1 | 140 | 91.0 | 100.0 | 68~69 | 1000 | ok |
| yolo26m.dxnn | 96.0 ±0.0 | 107 | 90.9 | 100.0 | 70~73 | 1000 | ok |
| yolo26l.dxnn | 70.3 ±0.1 | 82 | 92.0 | 100.0 | 70~73 | 1000 | ok |
| yolo26x.dxnn | 40.2 ±0.1 | 46 | 89.2 | 100.0 | 71~74 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 227.7 ±0.5 | 188 | 88.4 | 99.7 | 66~68 | 1000 | ok |
| yolo26s-pose.dxnn | 133.3 ±0.3 | 139 | 89.9 | 100.0 | 67~69 | 1000 | ok |
| yolo26m-pose.dxnn | 92.6 ±0.3 | 106 | 91.4 | 100.0 | 70~73 | 1000 | ok |
| yolo26l-pose.dxnn | 68.3 ±0.2 | 82 | 92.6 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-pose.dxnn | 39.3 ±0.2 | 48 | 90.4 | 100.0 | 70~74 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 228.8 ±0.6 | 161 | 90.0 | 99.6 | 67~68 | 1000 | ok |
| yolo26s-pose.dxnn | 133.2 ±0.3 | 104 | 91.0 | 100.0 | 66~68 | 1000 | ok |
| yolo26m-pose.dxnn | 92.7 ±0.4 | 77 | 91.8 | 100.0 | 70~73 | 1000 | ok |
| yolo26l-pose.dxnn | 68.5 ±0.1 | 60 | 88.6 | 100.0 | 69~72 | 1000 | ok |
| yolo26x-pose.dxnn | 39.1 ±0.0 | 33 | 90.8 | 100.0 | 70~74 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 124.7 ±0.1 | 298 | 47.3 | 75.7 | 65 | 1000 | ok |
| yolo26s-seg.dxnn | 100.2 ±0.3 | 242 | 80.4 | 92.4 | 64~67 | 1000 | ok |
| yolo26m-seg.dxnn | 69.6 ±0.1 | 161 | 92.8 | 100.0 | 71~76 | 1000 | ok |
| yolo26l-seg.dxnn | 54.2 ±0.2 | 149 | 88.7 | 100.0 | 68~72 | 1000 | ok |
| yolo26x-seg.dxnn | 30.2 ±0.4 | 78 | 89.1 | 100.0 | 70~74 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 136.3 ±1.2 | 258 | 56.0 | 80.7 | 60~62 | 1000 | ok |
| yolo26s-seg.dxnn | 102.8 ±1.1 | 201 | 83.0 | 100.0 | 69~71 | 1000 | ok |
| yolo26m-seg.dxnn | 68.9 ±0.4 | 145 | 89.4 | 100.0 | 69~73 | 1000 | ok |
| yolo26l-seg.dxnn | 54.2 ±0.1 | 126 | 90.4 | 100.0 | 68~72 | 1000 | ok |
| yolo26x-seg.dxnn | 30.6 ±0.4 | 64 | 88.7 | 100.0 | 70~74 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 79.9 ±0.0 | 92 | 90.7 | 100.0 | 64~65 | 1000 | ok |
| yolo26s-obb.dxnn | 46.4 ±0.1 | 54 | 91.9 | 100.0 | 61~63 | 1000 | ok |
| yolo26m-obb.dxnn | 33.9 ±0.0 | 37 | 88.6 | 100.0 | 65~69 | 1000 | ok |
| yolo26l-obb.dxnn | 24.7 ±0.1 | 30 | 91.2 | 100.0 | 68~71 | 1000 | ok |
| yolo26x-obb.dxnn | 14.2 ±0.0 | 17 | 88.5 | 100.0 | 69~73 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 79.8 ±0.0 | 65 | 91.6 | 100.0 | 60~62 | 1000 | ok |
| yolo26s-obb.dxnn | 46.5 ±0.1 | 38 | 90.1 | 100.0 | 62~64 | 1000 | ok |
| yolo26m-obb.dxnn | 33.9 ±0.2 | 29 | 88.6 | 100.0 | 69~72 | 1000 | ok |
| yolo26l-obb.dxnn | 24.7 ±0.1 | 21 | 88.0 | 100.0 | 68~71 | 1000 | ok |
| yolo26x-obb.dxnn | 14.3 ±0.1 | 13 | 86.9 | 100.0 | 70~73 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3532.3 ±4.7 | 119 | 88.5 | 95.9 | 65 | 1000 | ok |
| yolo26s-cls.dxnn | 1917.1 ±3.2 | 60 | 89.5 | 97.4 | 61~62 | 1000 | ok |
| yolo26m-cls.dxnn | 1357.0 ±0.9 | 44 | 89.3 | 97.7 | 63~67 | 1000 | ok |
| yolo26l-cls.dxnn | 854.0 ±0.0 | 28 | 89.4 | 98.6 | 62~64 | 1000 | ok |
| yolo26x-cls.dxnn | 460.1 ±0.3 | 17 | 91.1 | 99.2 | 63~67 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3535.5 ±1.1 | 119 | 88.0 | 95.3 | 60~61 | 1000 | ok |
| yolo26s-cls.dxnn | 1918.2 ±0.4 | 60 | 88.2 | 96.4 | 60~62 | 1000 | ok |
| yolo26m-cls.dxnn | 1357.0 ±0.5 | 44 | 90.8 | 97.8 | 64~67 | 1000 | ok |
| yolo26l-cls.dxnn | 852.4 ±0.8 | 29 | 91.0 | 98.7 | 62~65 | 1000 | ok |
| yolo26x-cls.dxnn | 459.9 ±0.0 | 17 | 90.6 | 99.3 | 64~67 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 43.2 | 23.14 | 21.89 | 1.25 | 48 | ok |
| yolo26s.dxnn | 31.2 | 32.02 | 30.68 | 1.34 | 62 | ok |
| yolo26m.dxnn | 25.3 | 39.51 | 38.14 | 1.37 | 65 | ok |
| yolo26l.dxnn | 21.3 | 46.90 | 45.54 | 1.36 | 65 | ok |
| yolo26x.dxnn | 13.5 | 73.82 | 72.42 | 1.40 | 65 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 44.8 | 22.32 | 22.32 | 61 | ok |
| yolo26s.dxnn | 33.1 | 30.25 | 30.25 | 65 | ok |
| yolo26m.dxnn | 26.9 | 37.23 | 37.23 | 65 | ok |
| yolo26l.dxnn | 21.7 | 45.99 | 45.99 | 65 | ok |
| yolo26x.dxnn | 13.8 | 72.54 | 72.54 | 65 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 46.2 | 21.63 | 20.63 | 1.00 | 64 | ok |
| yolo26s-pose.dxnn | 34.3 | 29.13 | 28.11 | 1.02 | 64 | ok |
| yolo26m-pose.dxnn | 27.5 | 36.42 | 35.44 | 0.98 | 64 | ok |
| yolo26l-pose.dxnn | 22.4 | 44.74 | 43.73 | 1.01 | 64 | ok |
| yolo26x-pose.dxnn | 13.9 | 72.00 | 70.99 | 1.02 | 65 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 48.9 | 20.47 | 20.47 | 64 | ok |
| yolo26s-pose.dxnn | 36.2 | 27.61 | 27.61 | 62 | ok |
| yolo26m-pose.dxnn | 28.2 | 35.48 | 35.48 | 65 | ok |
| yolo26l-pose.dxnn | 23.1 | 43.25 | 43.25 | 64 | ok |
| yolo26x-pose.dxnn | 14.1 | 71.10 | 71.10 | 64 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 26.5 | 37.69 | 36.47 | 1.21 | 63 | ok |
| yolo26s-seg.dxnn | 21.1 | 47.29 | 46.08 | 1.21 | 60 | ok |
| yolo26m-seg.dxnn | 16.4 | 60.96 | 59.72 | 1.24 | 64 | ok |
| yolo26l-seg.dxnn | 14.5 | 68.88 | 67.65 | 1.23 | 62 | ok |
| yolo26x-seg.dxnn | 9.3 | 107.59 | 106.35 | 1.24 | 63 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 28.3 | 35.36 | 35.36 | 58 | ok |
| yolo26s-seg.dxnn | 22.1 | 45.25 | 45.25 | 65 | ok |
| yolo26m-seg.dxnn | 16.9 | 59.31 | 59.31 | 62 | ok |
| yolo26l-seg.dxnn | 14.9 | 67.24 | 67.24 | 62 | ok |
| yolo26x-seg.dxnn | 9.5 | 105.80 | 105.80 | 62 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 28.0 | 35.74 | 34.67 | 1.07 | 62 | ok |
| yolo26s-obb.dxnn | 18.9 | 52.90 | 51.85 | 1.06 | 58 | ok |
| yolo26m-obb.dxnn | 14.0 | 71.45 | 70.39 | 1.06 | 60 | ok |
| yolo26l-obb.dxnn | 10.9 | 91.58 | 90.52 | 1.06 | 64 | ok |
| yolo26x-obb.dxnn | 6.1 | 163.28 | 162.18 | 1.10 | 64 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 29.0 | 34.48 | 34.48 | 58 | ok |
| yolo26s-obb.dxnn | 19.2 | 52.20 | 52.20 | 59 | ok |
| yolo26m-obb.dxnn | 14.3 | 69.70 | 69.70 | 64 | ok |
| yolo26l-obb.dxnn | 11.1 | 89.85 | 89.85 | 63 | ok |
| yolo26x-obb.dxnn | 6.2 | 161.76 | 161.76 | 65 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 753.7 | 1.33 | 1.33 | N/A | 63 | ok |
| yolo26s-cls.dxnn | 503.4 | 1.99 | 1.99 | N/A | 58 | ok |
| yolo26m-cls.dxnn | 375.7 | 2.66 | 2.66 | N/A | 58 | ok |
| yolo26l-cls.dxnn | 250.5 | 3.99 | 3.99 | N/A | 58 | ok |
| yolo26x-cls.dxnn | 146.6 | 6.82 | 6.82 | N/A | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 736.1 | 1.36 | 1.36 | 58 | ok |
| yolo26s-cls.dxnn | 494.2 | 2.02 | 2.02 | 57 | ok |
| yolo26m-cls.dxnn | 378.8 | 2.64 | 2.64 | 58 | ok |
| yolo26l-cls.dxnn | 237.7 | 4.21 | 4.21 | 58 | ok |
| yolo26x-cls.dxnn | 148.3 | 6.74 | 6.74 | 58 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vah264dec | 3455 | 3 | 172.7 ±3.0 | 20.01 | 282 | 50.6 | 81.0 | 56~58 | 1000 | 203 | ok |
| yolo26s.dxnn | vah264dec | 3455 | 3 | 137.1 ±0.1 | 25.21 | 251 | 85.4 | 99.6 | 69~70 | 1000 | 228 | ok |
| yolo26m.dxnn | vah264dec | 3455 | 3 | 96.0 ±0.2 | 35.99 | 161 | 92.1 | 100.0 | 77~80 | 1000 | 244 | ok |
| yolo26l.dxnn | vah264dec | 3455 | 3 | 70.2 ±0.1 | 49.20 | 138 | 94.8 | 100.0 | 77~80 | 1000 | 251 | ok |
| yolo26x.dxnn | vah264dec | 3455 | 3 | 34.8 ±3.6 | 99.42 | 77 | 95.3 | 100.0 | 83~84 | 600~1000 | 315 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vah264dec | 3455 | 3 | 188.4 ±1.6 | 18.34 | 307 | 56.0 | 82.6 | 65~66 | 1000 | 210 | ok |
| yolo26s.dxnn | vah264dec | 3455 | 3 | 138.0 ±0.2 | 25.03 | 242 | 89.2 | 100.0 | 70~71 | 1000 | 228 | ok |
| yolo26m.dxnn | vah264dec | 3455 | 3 | 96.0 ±0.3 | 35.99 | 174 | 90.7 | 100.0 | 76~80 | 1000 | 249 | ok |
| yolo26l.dxnn | vah264dec | 3455 | 3 | 70.3 ±0.2 | 49.15 | 145 | 93.7 | 100.0 | 77~81 | 1000 | 257 | ok |
| yolo26x.dxnn | vah264dec | 3455 | 3 | 34.9 ±3.7 | 99.08 | 80 | 94.8 | 100.0 | 82~84 | 600~1000 | 318 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 172.7 | 188.4 | -15.7 | -8.4% |
| yolo26s.dxnn | 137.1 | 138.0 | -1.0 | -0.7% |
| yolo26m.dxnn | 96.0 | 96.0 | +0.0 | +0.0% |
| yolo26l.dxnn | 70.2 | 70.3 | -0.1 | -0.1% |
| yolo26x.dxnn | 34.8 | 34.9 | -0.1 | -0.3% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 182.9 ±1.1 | 18.89 | 253 | 58.6 | 83.1 | 68 | 1000 | 184 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 133.6 ±0.2 | 25.86 | 170 | 89.6 | 100.0 | 70~71 | 1000 | 211 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 93.0 ±0.1 | 37.15 | 137 | 93.1 | 100.0 | 76~80 | 1000 | 233 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.4 ±0.1 | 50.50 | 115 | 94.0 | 100.0 | 77~80 | 1000 | 242 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 34.7 ±4.0 | 99.65 | 62 | 95.2 | 100.0 | 82~84 | 600~1000 | 326 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 209.6 ±0.7 | 16.48 | 207 | 70.4 | 88.6 | 68 | 1000 | 164 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 133.1 ±0.5 | 25.96 | 141 | 88.4 | 100.0 | 69~70 | 1000 | 197 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 92.7 ±0.5 | 37.27 | 115 | 93.6 | 100.0 | 77~80 | 1000 | 219 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.5 ±0.3 | 50.42 | 93 | 93.7 | 100.0 | 77~80 | 1000 | 230 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 35.1 ±3.5 | 98.42 | 49 | 94.7 | 100.0 | 82~84 | 600~1000 | 326 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 182.9 | 209.6 | -26.7 | -12.7% |
| yolo26s-pose.dxnn | 133.6 | 133.1 | +0.5 | +0.4% |
| yolo26m-pose.dxnn | 93.0 | 92.7 | +0.3 | +0.3% |
| yolo26l-pose.dxnn | 68.4 | 68.5 | -0.1 | -0.1% |
| yolo26x-pose.dxnn | 34.7 | 35.1 | -0.4 | -1.2% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 106.3 ±0.3 | 32.51 | 360 | 36.6 | 71.7 | 66 | 1000 | 317 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 98.3 ±0.1 | 35.14 | 302 | 75.3 | 87.7 | 70~72 | 1000 | 324 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 59.8 ±5.8 | 57.75 | 190 | 93.2 | 100.0 | 81~84 | 600~1000 | 342 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 52.9 ±2.1 | 65.36 | 168 | 93.7 | 100.0 | 79~83 | 600~1000 | 352 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 22.9 ±1.6 | 150.90 | 80 | 94.9 | 100.0 | 83~84 | 400~1000 | 419 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 115.2 ±0.4 | 30.00 | 369 | 44.2 | 72.2 | 63~64 | 1000 | 356 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 102.5 ±0.4 | 33.69 | 300 | 83.6 | 95.5 | 73~74 | 1000 | 353 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 64.8 ±3.7 | 53.29 | 198 | 93.3 | 100.0 | 79~83 | 600~1000 | 351 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 52.4 ±2.2 | 65.96 | 167 | 93.9 | 100.0 | 79~83 | 600~1000 | 361 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 22.8 ±1.7 | 151.78 | 75 | 94.6 | 100.0 | 83~84 | 400~1000 | 429 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 106.3 | 115.2 | -8.9 | -7.7% |
| yolo26s-seg.dxnn | 98.3 | 102.5 | -4.2 | -4.1% |
| yolo26m-seg.dxnn | 59.8 | 64.8 | -5.0 | -7.7% |
| yolo26l-seg.dxnn | 52.9 | 52.4 | +0.5 | +0.9% |
| yolo26x-seg.dxnn | 22.9 | 22.8 | +0.1 | +0.6% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 79.8 ±0.1 | 33.08 | 127 | 92.3 | 100.0 | 65~66 | 1000 | 229 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 46.4 ±0.1 | 56.88 | 89 | 94.1 | 100.0 | 67~69 | 1000 | 248 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 34.0 ±0.1 | 77.60 | 63 | 94.6 | 100.0 | 76~81 | 1000 | 268 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 24.8 ±0.2 | 106.64 | 47 | 95.0 | 100.0 | 79~83 | 800~1000 | 279 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 12.1 ±0.6 | 219.02 | 22 | 94.9 | 100.0 | 83~84 | 400~1000 | 349 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 80.0 ±0.4 | 33.02 | 130 | 93.1 | 100.0 | 63~65 | 1000 | 226 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 46.5 ±0.1 | 56.80 | 88 | 93.9 | 100.0 | 67~70 | 1000 | 226 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 33.9 ±0.2 | 77.89 | 65 | 94.8 | 100.0 | 78~82 | 800~1000 | 264 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 24.8 ±0.2 | 106.64 | 48 | 94.9 | 100.0 | 79~83 | 800~1000 | 272 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 12.0 ±0.5 | 219.19 | 22 | 95.1 | 100.0 | 84~85 | 400~1000 | 335 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 79.8 | 80.0 | -0.1 | -0.2% |
| yolo26s-obb.dxnn | 46.4 | 46.5 | -0.1 | -0.2% |
| yolo26m-obb.dxnn | 34.0 | 33.9 | +0.1 | +0.4% |
| yolo26l-obb.dxnn | 24.8 | 24.8 | +0.0 | +0.0% |
| yolo26x-obb.dxnn | 12.1 | 12.0 | +0.0 | +0.1% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 283.4 ±1.4 | 12.19 | 85 | 6.1 | 22.5 | 62~63 | 1000 | 83 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 290.6 ±0.5 | 11.89 | 82 | 11.4 | 42.1 | 60 | 1000 | 92 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 291.2 ±1.4 | 11.86 | 82 | 15.9 | 55.3 | 64 | 1000 | 104 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 292.8 ±0.3 | 11.80 | 82 | 25.4 | 76.5 | 62~63 | 1000 | 110 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 292.0 ±0.3 | 11.83 | 82 | 46.7 | 80.9 | 66 | 1000 | 181 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 281.1 ±1.0 | 12.29 | 85 | 5.8 | 21.8 | 59 | 1000 | 83 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 290.0 ±0.9 | 11.91 | 82 | 11.2 | 42.1 | 60 | 1000 | 92 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 291.8 ±1.6 | 11.84 | 82 | 16.4 | 55.1 | 63~64 | 1000 | 123 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 291.8 ±1.7 | 11.84 | 82 | 26.1 | 75.5 | 63 | 1000 | 113 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 293.0 ±0.3 | 11.79 | 82 | 47.8 | 81.1 | 66 | 1000 | 181 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 283.4 | 281.1 | +2.3 | +0.8% |
| yolo26s-cls.dxnn | 290.6 | 290.0 | +0.6 | +0.2% |
| yolo26m-cls.dxnn | 291.2 | 291.8 | -0.6 | -0.2% |
| yolo26l-cls.dxnn | 292.8 | 291.8 | +1.0 | +0.3% |
| yolo26x-cls.dxnn | 292.0 | 293.0 | -1.0 | -0.3% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 5 | 3 | 177.5 ±1.8 | 35.5 | 320 | 56.3 | 81.2 | 64~68 | 1000 | 409 | ok |
| yolo26n.dxnn | 6 | 3 | 176.0 ±0.5 | 29.3 | 318 | 56.1 | 82.0 | 71~73 | 1000 | 468 | ok |
| yolo26s.dxnn | 4 | 3 | 137.8 ±0.1 | 34.5 | 242 | 94.6 | 100.0 | 75~79 | 1000 | 382 | ok |
| yolo26s.dxnn | 5 | 3 | 137.8 ±0.1 | 27.6 | 242 | 95.2 | 100.0 | 81~83 | 1000 | 422 | ok |
| yolo26m.dxnn | 3 | 3 | 77.5 ±4.9 | 25.8 | 153 | 96.9 | 100.0 | 84~85 | 400~1000 | 360 | ok |
| yolo26m.dxnn | 2 | 3 | 75.5 ±0.8 | 37.8 | 149 | 95.7 | 100.0 | 84~85 | 400~1000 | 314 | ok |
| yolo26l.dxnn | 2 | 3 | 60.8 ±3.0 | 30.4 | 132 | 95.6 | 100.0 | 84~85 | 600~1000 | 324 | ok |
| yolo26l.dxnn | 3 | 3 | 57.5 ±0.2 | 19.2 | 126 | 97.2 | 100.0 | 84~85 | 600~1000 | 366 | ok |
| yolo26x.dxnn | 1 | 3 | 34.8 ±3.6 | 34.8 | 77 | 95.3 | 100.0 | 83~84 | 600~1000 | 315 | ok |
| yolo26x.dxnn | 2 | 3 | 30.1 ±0.8 | 15.1 | 64 | 96.5 | 100.0 | 85 | 400~1000 | 386 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 6 | 3 | 189.7 ±1.9 | 31.6 | 346 | 64.5 | 85.6 | 70~72 | 1000 | 514 | ok |
| yolo26n.dxnn | 7 | 3 | 193.6 ±1.9 | 27.6 | 349 | 66.5 | 85.5 | 74~75 | 1000 | 540 | ok |
| yolo26s.dxnn | 4 | 3 | 138.4 ±0.2 | 34.6 | 249 | 95.9 | 100.0 | 76~80 | 1000 | 405 | ok |
| yolo26s.dxnn | 5 | 3 | 138.4 ±0.0 | 27.7 | 248 | 96.6 | 100.0 | 82~83 | 1000 | 445 | ok |
| yolo26m.dxnn | 3 | 3 | 76.8 ±4.4 | 25.6 | 162 | 97.0 | 100.0 | 84~85 | 400~1000 | 365 | ok |
| yolo26m.dxnn | 2 | 3 | 73.8 ±3.5 | 36.9 | 159 | 95.9 | 100.0 | 84~85 | 400~1000 | 320 | ok |
| yolo26l.dxnn | 2 | 3 | 60.6 ±3.5 | 30.3 | 140 | 95.7 | 100.0 | 84~85 | 600~1000 | 329 | ok |
| yolo26l.dxnn | 3 | 3 | 57.0 ±0.5 | 19.0 | 130 | 97.3 | 100.0 | 85 | 600~1000 | 369 | ok |
| yolo26x.dxnn | 1 | 3 | 34.9 ±3.7 | 34.9 | 80 | 94.8 | 100.0 | 82~84 | 600~1000 | 318 | ok |
| yolo26x.dxnn | 2 | 3 | 30.0 ±0.4 | 15.0 | 69 | 96.7 | 100.0 | 84~85 | 400~1000 | 392 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 5 | 35.5 | 6 | 31.6 |
| yolo26s.dxnn | 4 | 34.5 | 4 | 34.6 |
| yolo26m.dxnn | 2 | 37.8 | 2 | 36.9 |
| yolo26l.dxnn | 2 | 30.4 | 2 | 30.3 |
| yolo26x.dxnn | 1 | 34.8 | 1 | 34.9 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 3 | 213.4 ±0.5 | 35.6 | 299 | 82.5 | 91.8 | 72~76 | 1000 | 451 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 212.6 ±0.3 | 30.4 | 299 | 82.6 | 91.9 | 78~79 | 1000 | 491 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 212.1 ±0.1 | 26.5 | 300 | 82.3 | 92.0 | 80~81 | 1000 | 524 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 133.6 ±0.1 | 33.4 | 192 | 96.1 | 100.0 | 65~79 | 1000 | 380 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 133.5 ±0.1 | 26.7 | 193 | 97.4 | 100.0 | 74~79 | 1000 | 412 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 74.4 ±4.4 | 24.8 | 134 | 96.5 | 100.0 | 84 | 400~1000 | 353 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 74.0 ±0.1 | 37.0 | 132 | 95.7 | 100.0 | 84~85 | 600~1000 | 308 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 60.1 ±4.0 | 30.0 | 114 | 95.7 | 100.0 | 84 | 600~1000 | 318 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 56.5 ±0.3 | 18.8 | 106 | 96.7 | 100.0 | 84~85 | 600~800 | 362 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 34.7 ±4.0 | 34.7 | 62 | 95.2 | 100.0 | 82~84 | 600~1000 | 326 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 29.9 ±0.2 | 14.9 | 52 | 97.6 | 100.0 | 84 | 600~1000 | 381 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 3 | 227.2 ±0.0 | 37.9 | 267 | 92.0 | 98.9 | 72~75 | 1000 | 442 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 227.2 ±0.1 | 32.5 | 266 | 92.5 | 98.2 | 77~79 | 1000 | 484 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 227.4 ±0.1 | 28.4 | 266 | 93.2 | 98.8 | 80 | 1000 | 517 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 133.4 ±0.1 | 33.4 | 173 | 96.8 | 100.0 | 75~78 | 1000 | 367 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 133.3 ±0.1 | 26.7 | 172 | 97.1 | 100.0 | 80~82 | 1000 | 402 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 75.7 ±2.8 | 25.2 | 114 | 96.7 | 100.0 | 84 | 600~1000 | 341 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 74.1 ±0.1 | 37.1 | 108 | 96.3 | 100.0 | 84 | 600~1000 | 294 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 60.7 ±3.8 | 30.4 | 95 | 95.5 | 100.0 | 84 | 600~1000 | 306 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 56.8 ±0.2 | 18.9 | 89 | 96.6 | 100.0 | 84~85 | 600~1000 | 350 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 35.1 ±3.5 | 35.1 | 49 | 94.7 | 100.0 | 82~84 | 600~1000 | 326 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 29.9 ±0.1 | 15.0 | 44 | 97.7 | 100.0 | 84 | 600~1000 | 367 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 7 | 30.4 | 7 | 32.5 |
| yolo26s-pose.dxnn | 4 | 33.4 | 4 | 33.4 |
| yolo26m-pose.dxnn | 2 | 37.0 | 2 | 37.1 |
| yolo26l-pose.dxnn | 2 | 30.0 | 2 | 30.4 |
| yolo26x-pose.dxnn | 1 | 34.7 | 1 | 35.1 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 104.4 ±0.5 | 34.8 | 359 | 37.9 | 70.1 | 68~70 | 1000 | 450 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 104.7 ±0.9 | 26.2 | 358 | 38.3 | 71.4 | 61~71 | 1000 | 499 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 97.4 ±0.1 | 32.5 | 304 | 78.6 | 86.6 | 78~82 | 1000 | 457 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 95.4 ±1.4 | 23.9 | 296 | 80.7 | 92.7 | 84~85 | 800~1000 | 500 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 59.8 ±5.8 | 59.8 | 190 | 93.2 | 100.0 | 81~84 | 600~1000 | 342 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 46.4 ±1.6 | 23.2 | 173 | 95.5 | 100.0 | 84 | 400~800 | 428 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 52.9 ±2.1 | 52.9 | 168 | 93.7 | 100.0 | 79~83 | 600~1000 | 352 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 41.7 ±0.4 | 20.9 | 140 | 96.5 | 100.0 | 84 | 400~1000 | 438 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 22.9 ±1.6 | 22.9 | 80 | 94.9 | 100.0 | 83~84 | 400~1000 | 419 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 113.6 ±0.7 | 37.9 | 375 | 44.4 | 71.2 | 67~69 | 1000 | 490 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 113.4 ±0.6 | 28.3 | 374 | 44.4 | 70.6 | 71~72 | 1000 | 541 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 103.3 ±0.2 | 34.4 | 306 | 88.4 | 96.2 | 80~83 | 1000 | 488 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 100.5 ±1.3 | 25.1 | 294 | 90.4 | 100.0 | 85 | 600~1000 | 540 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 47.3 ±3.4 | 23.7 | 167 | 96.1 | 100.0 | 84 | 400~1000 | 434 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 64.8 ±3.7 | 64.8 | 198 | 93.3 | 100.0 | 79~83 | 600~1000 | 351 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 52.4 ±2.2 | 52.4 | 167 | 93.9 | 100.0 | 79~83 | 600~1000 | 361 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 41.7 ±1.7 | 20.9 | 149 | 96.7 | 100.0 | 84~85 | 400~800 | 446 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 22.8 ±1.7 | 22.8 | 75 | 94.6 | 100.0 | 83~84 | 400~1000 | 429 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 34.8 | 3 | 37.9 |
| yolo26s-seg.dxnn | 3 | 32.5 | 3 | 34.4 |
| yolo26m-seg.dxnn | 1 | 59.8 | 1 | 64.8 |
| yolo26l-seg.dxnn | 1 | 52.9 | 1 | 52.4 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 79.8 ±0.1 | 39.9 | 166 | 95.5 | 100.0 | 68~70 | 1000 | 302 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 79.8 ±0.1 | 26.6 | 164 | 97.1 | 100.0 | 71~72 | 1000 | 350 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.4 ±0.1 | 46.4 | 89 | 94.1 | 100.0 | 67~69 | 1000 | 248 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.5 ±0.1 | 23.2 | 97 | 96.7 | 100.0 | 72~74 | 1000 | 317 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.0 ±0.1 | 34.0 | 63 | 94.6 | 100.0 | 76~81 | 1000 | 268 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.6 ±0.5 | 14.8 | 63 | 95.7 | 100.0 | 84 | 600~1000 | 340 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 24.8 ±0.2 | 24.8 | 47 | 95.0 | 100.0 | 79~83 | 800~1000 | 279 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.1 ±0.6 | 12.1 | 22 | 94.9 | 100.0 | 83~84 | 400~1000 | 349 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 79.8 ±0.2 | 39.9 | 159 | 96.1 | 100.0 | 67~69 | 1000 | 301 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 79.8 ±0.1 | 26.6 | 162 | 96.4 | 100.0 | 70~71 | 1000 | 339 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.5 ±0.1 | 46.5 | 88 | 93.9 | 100.0 | 67~70 | 1000 | 226 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.4 ±0.1 | 23.2 | 99 | 96.2 | 100.0 | 73~75 | 1000 | 313 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 33.9 ±0.2 | 33.9 | 65 | 94.8 | 100.0 | 78~82 | 800~1000 | 264 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.4 ±0.2 | 14.7 | 62 | 95.7 | 100.0 | 84 | 600~1000 | 333 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 24.8 ±0.2 | 24.8 | 48 | 94.9 | 100.0 | 79~83 | 800~1000 | 272 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.0 ±0.5 | 12.0 | 22 | 95.1 | 100.0 | 84~85 | 400~1000 | 335 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 39.9 | 2 | 39.9 |
| yolo26s-obb.dxnn | 1 | 46.4 | 1 | 46.5 |
| yolo26m-obb.dxnn | 1 | 34.0 | 1 | 33.9 |

---
*Report generated by dx_stream benchmark tool*
