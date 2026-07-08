# YOLO26 Benchmark Report

**Generated:** 2026-04-22 19:13:50 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | resume | 2026-04-22 11:51:34 | 2026-04-22 19:13:50 | 7h 22m 15s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 21.16 | 244.3 | 243.5 | 8 |
| yolo26n.dxnn | OFF | 21.73 | 242.1 | 239.9 | 8 |
| yolo26s.dxnn | ON | 29.15 | 139.9 | 139.7 | 4 |
| yolo26s.dxnn | OFF | 27.91 | 139.8 | 139.3 | 4 |
| yolo26m.dxnn | ON | 36.67 | 96.4 | 96.6 | 3 |
| yolo26m.dxnn | OFF | 35.06 | 96.5 | 96.5 | 3 |
| yolo26l.dxnn | ON | 44.63 | 70.6 | 70.8 | 2 |
| yolo26l.dxnn | OFF | 42.93 | 70.7 | 70.7 | 2 |
| yolo26x.dxnn | ON | 70.60 | 40.4 | 40.7 | 1 |
| yolo26x.dxnn | OFF | 69.01 | 40.3 | 40.7 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 16.90 | 232.2 | 230.0 | 7 |
| yolo26n-pose.dxnn | OFF | 15.32 | 230.8 | 229.7 | 7 |
| yolo26s-pose.dxnn | ON | 24.36 | 134.3 | 134.0 | 4 |
| yolo26s-pose.dxnn | OFF | 23.65 | 134.3 | 133.7 | 4 |
| yolo26m-pose.dxnn | ON | 31.67 | 93.3 | 93.2 | 3 |
| yolo26m-pose.dxnn | OFF | 26.54 | 93.5 | 93.1 | 3 |
| yolo26l-pose.dxnn | ON | 34.57 | 68.7 | 68.7 | 2 |
| yolo26l-pose.dxnn | OFF | 39.62 | 68.6 | 69.0 | 2 |
| yolo26x-pose.dxnn | ON | 62.27 | 39.6 | 40.1 | 1 |
| yolo26x-pose.dxnn | OFF | 61.69 | 39.7 | 39.8 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 20.72 | 187.7 | 188.2 | 6 |
| yolo26n-seg.dxnn | OFF | 19.67 | 185.5 | 186.3 | 6 |
| yolo26s-seg.dxnn | ON | 29.58 | 109.1 | 109.3 | 3 |
| yolo26s-seg.dxnn | OFF | 40.61 | 108.6 | 109.0 | 3 |
| yolo26m-seg.dxnn | ON | 55.32 | 68.8 | 69.1 | 2 |
| yolo26m-seg.dxnn | OFF | 54.03 | 68.8 | 68.9 | 2 |
| yolo26l-seg.dxnn | ON | 61.55 | 54.6 | 54.5 | 1 |
| yolo26l-seg.dxnn | OFF | 62.07 | 54.5 | 54.6 | 1 |
| yolo26x-seg.dxnn | ON | 100.21 | 31.1 | 31.1 | 1 |
| yolo26x-seg.dxnn | OFF | 99.62 | 30.4 | 30.8 | 1 |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 33.98 | 80.1 | 80.0 | 2 |
| yolo26n-obb.dxnn | OFF | 32.60 | 80.2 | 80.2 | 2 |
| yolo26s-obb.dxnn | ON | 50.29 | 46.7 | 46.6 | 1 |
| yolo26s-obb.dxnn | OFF | 49.70 | 46.7 | 46.7 | 1 |
| yolo26m-obb.dxnn | ON | 68.05 | 34.2 | 34.2 | 1 |
| yolo26m-obb.dxnn | OFF | 67.53 | 34.0 | 34.2 | 1 |
| yolo26l-obb.dxnn | ON | 88.66 | 24.9 | 25.1 | — |
| yolo26l-obb.dxnn | OFF | 87.73 | 25.0 | 25.1 | — |
| yolo26x-obb.dxnn | ON | 159.15 | 14.4 | 14.5 | — |
| yolo26x-obb.dxnn | OFF | 158.40 | 14.4 | 14.5 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.34 | 3558.7 | 574.8 | — |
| yolo26n-cls.dxnn | OFF | 1.29 | 3566.0 | 554.8 | — |
| yolo26s-cls.dxnn | ON | 2.00 | 1928.8 | 550.7 | — |
| yolo26s-cls.dxnn | OFF | 2.04 | 1929.6 | 545.9 | — |
| yolo26m-cls.dxnn | ON | 2.56 | 1362.3 | 542.9 | — |
| yolo26m-cls.dxnn | OFF | 2.64 | 1359.6 | 555.2 | — |
| yolo26l-cls.dxnn | ON | 3.98 | 855.6 | 546.6 | — |
| yolo26l-cls.dxnn | OFF | 3.98 | 855.4 | 549.8 | — |
| yolo26x-cls.dxnn | ON | 6.63 | 462.4 | 453.4 | — |
| yolo26x-cls.dxnn | OFF | 6.81 | 461.2 | 457.5 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | yjsong |
| OS | Ubuntu 24.04.2 LTS |
| Kernel | 6.17.0-20-generic |
| CPU | Intel(R) Core(TM) i7-14700K |
| CPU Cores | 28 |
| RAM | 62.5 GB |
| NPU SKU | M1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X4 [08:00:00] |

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
| yolo26n.dxnn | 244.3 ±0.2 | 146 | 90.9 | 100.0 | 44~46 | 1000 | ok |
| yolo26s.dxnn | 139.9 ±0.1 | 142 | 90.7 | 100.0 | 53~54 | 1000 | ok |
| yolo26m.dxnn | 96.4 ±0.3 | 119 | 93.1 | 100.0 | 55~58 | 1000 | ok |
| yolo26l.dxnn | 70.6 ±0.0 | 96 | 90.6 | 100.0 | 55~57 | 1000 | ok |
| yolo26x.dxnn | 40.4 ±0.3 | 52 | 91.5 | 100.0 | 56~59 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 242.1 ±0.7 | 143 | 91.2 | 100.0 | 51~52 | 1000 | ok |
| yolo26s.dxnn | 139.8 ±0.3 | 124 | 92.6 | 100.0 | 53~54 | 1000 | ok |
| yolo26m.dxnn | 96.5 ±0.2 | 95 | 91.6 | 100.0 | 55~58 | 1000 | ok |
| yolo26l.dxnn | 70.7 ±0.1 | 74 | 89.6 | 100.0 | 55~58 | 1000 | ok |
| yolo26x.dxnn | 40.3 ±0.0 | 40 | 92.2 | 100.0 | 56~59 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 232.2 ±0.5 | 126 | 91.1 | 100.0 | 52~53 | 1000 | ok |
| yolo26s-pose.dxnn | 134.3 ±0.4 | 117 | 91.8 | 100.0 | 53~55 | 1000 | ok |
| yolo26m-pose.dxnn | 93.3 ±0.2 | 83 | 92.6 | 100.0 | 56~58 | 1000 | ok |
| yolo26l-pose.dxnn | 68.7 ±0.1 | 17 | 92.6 | 100.0 | 55~58 | 1000 | ok |
| yolo26x-pose.dxnn | 39.6 ±0.4 | 9 | 88.3 | 100.0 | 56~59 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 230.8 ±0.0 | 122 | 91.2 | 100.0 | 52~53 | 1000 | ok |
| yolo26s-pose.dxnn | 134.3 ±0.2 | 76 | 90.6 | 100.0 | 53~55 | 1000 | ok |
| yolo26m-pose.dxnn | 93.5 ±0.7 | 14 | 91.9 | 100.0 | 56~58 | 1000 | ok |
| yolo26l-pose.dxnn | 68.6 ±0.1 | 27 | 92.0 | 100.0 | 55~58 | 1000 | ok |
| yolo26x-pose.dxnn | 39.7 ±0.4 | 6 | 90.5 | 100.0 | 56~60 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 187.7 ±0.3 | 155 | 92.1 | 100.0 | 52~53 | 1000 | ok |
| yolo26s-seg.dxnn | 109.1 ±0.1 | 90 | 92.5 | 100.0 | 53~56 | 1000 | ok |
| yolo26m-seg.dxnn | 68.8 ±0.6 | 112 | 92.3 | 100.0 | 57~60 | 1000 | ok |
| yolo26l-seg.dxnn | 54.6 ±0.1 | 91 | 90.3 | 100.0 | 57~60 | 1000 | ok |
| yolo26x-seg.dxnn | 31.1 ±0.1 | 52 | 89.3 | 100.0 | 59~63 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 185.5 ±0.1 | 127 | 90.9 | 100.0 | 52~53 | 1000 | ok |
| yolo26s-seg.dxnn | 108.6 ±0.1 | 122 | 92.0 | 100.0 | 54~56 | 1000 | ok |
| yolo26m-seg.dxnn | 68.8 ±0.2 | 100 | 90.6 | 100.0 | 58~61 | 1000 | ok |
| yolo26l-seg.dxnn | 54.5 ±0.4 | 81 | 91.4 | 100.0 | 57~60 | 1000 | ok |
| yolo26x-seg.dxnn | 30.4 ±0.1 | 47 | 88.9 | 100.0 | 60~64 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 80.1 ±0.2 | 85 | 92.5 | 100.0 | 55~56 | 1000 | ok |
| yolo26s-obb.dxnn | 46.7 ±0.0 | 51 | 89.7 | 100.0 | 53~55 | 1000 | ok |
| yolo26m-obb.dxnn | 34.2 ±0.1 | 37 | 87.7 | 100.0 | 56~59 | 1000 | ok |
| yolo26l-obb.dxnn | 24.9 ±0.1 | 27 | 91.6 | 100.0 | 56~59 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.1 | 15 | 88.4 | 100.0 | 57~60 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 80.2 ±0.1 | 65 | 91.5 | 100.0 | 53~54 | 1000 | ok |
| yolo26s-obb.dxnn | 46.7 ±0.1 | 38 | 92.4 | 100.0 | 53~55 | 1000 | ok |
| yolo26m-obb.dxnn | 34.0 ±0.1 | 28 | 88.0 | 100.0 | 56~59 | 1000 | ok |
| yolo26l-obb.dxnn | 25.0 ±0.0 | 20 | 87.1 | 100.0 | 56~58 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.1 | 11 | 87.0 | 100.0 | 56~59 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3558.7 ±10.5 | 112 | 87.5 | 95.8 | 51 | 1000 | ok |
| yolo26s-cls.dxnn | 1928.8 ±4.9 | 62 | 89.2 | 97.6 | 51~52 | 1000 | ok |
| yolo26m-cls.dxnn | 1362.3 ±0.3 | 43 | 90.6 | 97.5 | 54~57 | 1000 | ok |
| yolo26l-cls.dxnn | 855.6 ±1.3 | 28 | 89.8 | 98.3 | 53~54 | 1000 | ok |
| yolo26x-cls.dxnn | 462.4 ±0.6 | 17 | 91.1 | 99.4 | 54~56 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3566.0 ±1.6 | 112 | 87.9 | 95.4 | 51 | 1000 | ok |
| yolo26s-cls.dxnn | 1929.6 ±3.3 | 62 | 87.3 | 96.4 | 51~52 | 1000 | ok |
| yolo26m-cls.dxnn | 1359.6 ±1.5 | 44 | 88.4 | 98.1 | 54~57 | 1000 | ok |
| yolo26l-cls.dxnn | 855.4 ±1.6 | 28 | 89.9 | 98.4 | 53~55 | 1000 | ok |
| yolo26x-cls.dxnn | 461.2 ±1.7 | 17 | 91.1 | 99.3 | 54~56 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 47.3 | 21.16 | 20.16 | 1.00 | 40 | ok |
| yolo26s.dxnn | 34.3 | 29.15 | 28.14 | 1.01 | 50 | ok |
| yolo26m.dxnn | 27.3 | 36.67 | 35.57 | 1.10 | 50 | ok |
| yolo26l.dxnn | 22.4 | 44.63 | 43.51 | 1.13 | 50 | ok |
| yolo26x.dxnn | 14.2 | 70.60 | 69.74 | 0.85 | 50 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 46.0 | 21.73 | 21.73 | 50 | ok |
| yolo26s.dxnn | 35.8 | 27.91 | 27.91 | 50 | ok |
| yolo26m.dxnn | 28.5 | 35.06 | 35.06 | 50 | ok |
| yolo26l.dxnn | 23.3 | 42.93 | 42.93 | 50 | ok |
| yolo26x.dxnn | 14.5 | 69.01 | 69.01 | 51 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 59.2 | 16.90 | 15.81 | 1.09 | 50 | ok |
| yolo26s-pose.dxnn | 41.0 | 24.36 | 23.24 | 1.12 | 50 | ok |
| yolo26m-pose.dxnn | 31.6 | 31.67 | 30.61 | 1.05 | 50 | ok |
| yolo26l-pose.dxnn | 28.9 | 34.57 | 34.31 | 0.26 | 51 | ok |
| yolo26x-pose.dxnn | 16.1 | 62.27 | 61.99 | 0.27 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 65.3 | 15.32 | 15.32 | 50 | ok |
| yolo26s-pose.dxnn | 42.3 | 23.65 | 23.65 | 50 | ok |
| yolo26m-pose.dxnn | 37.7 | 26.54 | 26.54 | 51 | ok |
| yolo26l-pose.dxnn | 25.2 | 39.62 | 39.62 | 50 | ok |
| yolo26x-pose.dxnn | 16.2 | 61.69 | 61.69 | 51 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 48.3 | 20.72 | 20.27 | 0.45 | 49 | ok |
| yolo26s-seg.dxnn | 33.8 | 29.58 | 29.13 | 0.45 | 50 | ok |
| yolo26m-seg.dxnn | 18.1 | 55.32 | 54.64 | 0.67 | 50 | ok |
| yolo26l-seg.dxnn | 16.2 | 61.55 | 60.85 | 0.70 | 51 | ok |
| yolo26x-seg.dxnn | 10.0 | 100.21 | 99.58 | 0.63 | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 50.8 | 19.67 | 19.67 | 50 | ok |
| yolo26s-seg.dxnn | 24.6 | 40.61 | 40.61 | 50 | ok |
| yolo26m-seg.dxnn | 18.5 | 54.03 | 54.03 | 51 | ok |
| yolo26l-seg.dxnn | 16.1 | 62.07 | 62.07 | 51 | ok |
| yolo26x-seg.dxnn | 10.0 | 99.62 | 99.62 | 54 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 29.4 | 33.98 | 33.02 | 0.96 | 53 | ok |
| yolo26s-obb.dxnn | 19.9 | 50.29 | 49.37 | 0.91 | 50 | ok |
| yolo26m-obb.dxnn | 14.7 | 68.05 | 67.33 | 0.73 | 51 | ok |
| yolo26l-obb.dxnn | 11.3 | 88.66 | 87.80 | 0.86 | 51 | ok |
| yolo26x-obb.dxnn | 6.3 | 159.15 | 158.29 | 0.87 | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 30.7 | 32.60 | 32.60 | 50 | ok |
| yolo26s-obb.dxnn | 20.1 | 49.70 | 49.70 | 50 | ok |
| yolo26m-obb.dxnn | 14.8 | 67.53 | 67.53 | 51 | ok |
| yolo26l-obb.dxnn | 11.4 | 87.73 | 87.73 | 51 | ok |
| yolo26x-obb.dxnn | 6.3 | 158.40 | 158.40 | 51 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 748.6 | 1.34 | 1.34 | N/A | 50 | ok |
| yolo26s-cls.dxnn | 501.1 | 2.00 | 2.00 | N/A | 49 | ok |
| yolo26m-cls.dxnn | 390.6 | 2.56 | 2.56 | N/A | 50 | ok |
| yolo26l-cls.dxnn | 251.5 | 3.98 | 3.98 | N/A | 50 | ok |
| yolo26x-cls.dxnn | 150.8 | 6.63 | 6.63 | N/A | 50 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 776.2 | 1.29 | 1.29 | 49 | ok |
| yolo26s-cls.dxnn | 490.1 | 2.04 | 2.04 | 49 | ok |
| yolo26m-cls.dxnn | 378.1 | 2.64 | 2.64 | 50 | ok |
| yolo26l-cls.dxnn | 251.0 | 3.98 | 3.98 | 50 | ok |
| yolo26x-cls.dxnn | 146.9 | 6.81 | 6.81 | 50 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vah264dec | 3455 | 3 | 243.5 ±0.3 | 14.19 | 174 | 86.2 | 100.0 | 47~48 | 1000 | 208 | ok |
| yolo26s.dxnn | vah264dec | 3455 | 3 | 139.7 ±0.2 | 24.74 | 154 | 91.0 | 100.0 | 55~56 | 1000 | 223 | ok |
| yolo26m.dxnn | vah264dec | 3455 | 3 | 96.6 ±0.2 | 35.77 | 136 | 92.2 | 100.0 | 60~62 | 1000 | 244 | ok |
| yolo26l.dxnn | vah264dec | 3455 | 3 | 70.8 ±0.2 | 48.82 | 117 | 93.4 | 100.0 | 60~62 | 1000 | 254 | ok |
| yolo26x.dxnn | vah264dec | 3455 | 3 | 40.7 ±0.2 | 84.86 | 71 | 95.0 | 100.0 | 65~68 | 1000 | 319 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vah264dec | 3455 | 3 | 239.9 ±0.3 | 14.40 | 162 | 83.2 | 100.0 | 52 | 1000 | 216 | ok |
| yolo26s.dxnn | vah264dec | 3455 | 3 | 139.3 ±0.1 | 24.80 | 146 | 90.2 | 100.0 | 54~55 | 1000 | 227 | ok |
| yolo26m.dxnn | vah264dec | 3455 | 3 | 96.5 ±0.4 | 35.81 | 127 | 92.1 | 100.0 | 60~62 | 1000 | 249 | ok |
| yolo26l.dxnn | vah264dec | 3455 | 3 | 70.7 ±0.0 | 48.89 | 119 | 93.9 | 100.0 | 60~63 | 1000 | 257 | ok |
| yolo26x.dxnn | vah264dec | 3455 | 3 | 40.7 ±0.1 | 84.83 | 75 | 95.2 | 100.0 | 65~67 | 1000 | 319 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 243.5 | 239.9 | +3.7 | +1.5% |
| yolo26s.dxnn | 139.7 | 139.3 | +0.3 | +0.2% |
| yolo26m.dxnn | 96.6 | 96.5 | +0.1 | +0.1% |
| yolo26l.dxnn | 70.8 | 70.7 | +0.1 | +0.2% |
| yolo26x.dxnn | 40.7 | 40.7 | -0.0 | -0.0% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 230.0 ±0.4 | 15.02 | 146 | 86.6 | 100.0 | 53 | 1000 | 200 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 134.0 ±0.0 | 25.78 | 136 | 90.4 | 100.0 | 55~56 | 1000 | 216 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 93.2 ±0.2 | 37.07 | 116 | 93.6 | 100.0 | 61~63 | 1000 | 236 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 68.7 ±0.1 | 50.28 | 26 | 95.1 | 100.0 | 61~63 | 1000 | 246 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 40.1 ±0.3 | 86.21 | 24 | 95.4 | 100.0 | 65~68 | 1000 | 330 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vah264dec | 3455 | 3 | 229.7 ±0.2 | 15.04 | 130 | 85.5 | 100.0 | 53 | 1000 | 185 | ok |
| yolo26s-pose.dxnn | vah264dec | 3455 | 3 | 133.7 ±0.4 | 25.85 | 118 | 89.5 | 100.0 | 55~56 | 1000 | 200 | ok |
| yolo26m-pose.dxnn | vah264dec | 3455 | 3 | 93.1 ±0.3 | 37.12 | 27 | 94.3 | 100.0 | 61~63 | 1000 | 222 | ok |
| yolo26l-pose.dxnn | vah264dec | 3455 | 3 | 69.0 ±0.3 | 50.11 | 21 | 95.2 | 100.0 | 61~63 | 1000 | 236 | ok |
| yolo26x-pose.dxnn | vah264dec | 3455 | 3 | 39.8 ±0.3 | 86.81 | 20 | 95.3 | 100.0 | 65~68 | 1000 | 330 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 230.0 | 229.7 | +0.4 | +0.2% |
| yolo26s-pose.dxnn | 134.0 | 133.7 | +0.3 | +0.3% |
| yolo26m-pose.dxnn | 93.2 | 93.1 | +0.1 | +0.2% |
| yolo26l-pose.dxnn | 68.7 | 69.0 | -0.2 | -0.3% |
| yolo26x-pose.dxnn | 40.1 | 39.8 | +0.3 | +0.7% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 188.2 ±0.4 | 18.35 | 201 | 88.7 | 100.0 | 54 | 1000 | 305 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 109.3 ±0.6 | 31.61 | 119 | 92.0 | 100.0 | 57~58 | 1000 | 321 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 69.1 ±0.2 | 49.98 | 130 | 94.0 | 100.0 | 65~68 | 1000 | 346 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 54.5 ±0.2 | 63.43 | 111 | 95.3 | 100.0 | 65~69 | 1000 | 355 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 31.1 ±0.2 | 111.24 | 66 | 96.0 | 100.0 | 73~77 | 1000 | 425 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vah264dec | 3455 | 3 | 186.3 ±0.5 | 18.55 | 213 | 84.8 | 100.0 | 54 | 1000 | 312 | ok |
| yolo26s-seg.dxnn | vah264dec | 3455 | 3 | 109.0 ±0.2 | 31.71 | 147 | 91.0 | 100.0 | 57~58 | 1000 | 328 | ok |
| yolo26m-seg.dxnn | vah264dec | 3455 | 3 | 68.9 ±0.3 | 50.15 | 126 | 92.9 | 100.0 | 66~69 | 1000 | 353 | ok |
| yolo26l-seg.dxnn | vah264dec | 3455 | 3 | 54.6 ±0.2 | 63.25 | 95 | 95.4 | 100.0 | 65~69 | 1000 | 363 | ok |
| yolo26x-seg.dxnn | vah264dec | 3455 | 3 | 30.8 ±0.1 | 112.23 | 65 | 95.5 | 100.0 | 73~77 | 1000 | 432 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 188.2 | 186.3 | +2.0 | +1.1% |
| yolo26s-seg.dxnn | 109.3 | 109.0 | +0.3 | +0.3% |
| yolo26m-seg.dxnn | 69.1 | 68.9 | +0.2 | +0.3% |
| yolo26l-seg.dxnn | 54.5 | 54.6 | -0.1 | -0.3% |
| yolo26x-seg.dxnn | 31.1 | 30.8 | +0.3 | +0.9% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 80.0 ±0.2 | 33.00 | 112 | 91.3 | 100.0 | 56~57 | 1000 | 227 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 46.6 ±0.0 | 56.61 | 81 | 94.0 | 100.0 | 57~59 | 1000 | 247 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 34.2 ±0.1 | 77.12 | 61 | 95.6 | 100.0 | 64~66 | 1000 | 270 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 25.1 ±0.0 | 105.21 | 46 | 94.3 | 100.0 | 64~67 | 1000 | 275 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 14.5 ±0.0 | 182.59 | 27 | 95.6 | 100.0 | 69~71 | 1000 | 348 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vah264dec | 2640 | 3 | 80.2 ±0.1 | 32.91 | 121 | 92.6 | 100.0 | 55~56 | 1000 | 228 | ok |
| yolo26s-obb.dxnn | vah264dec | 2640 | 3 | 46.7 ±0.1 | 56.53 | 83 | 94.2 | 100.0 | 57~59 | 1000 | 241 | ok |
| yolo26m-obb.dxnn | vah264dec | 2640 | 3 | 34.2 ±0.2 | 77.17 | 64 | 94.7 | 100.0 | 64~67 | 1000 | 266 | ok |
| yolo26l-obb.dxnn | vah264dec | 2640 | 3 | 25.1 ±0.1 | 105.34 | 44 | 94.7 | 100.0 | 64~67 | 1000 | 264 | ok |
| yolo26x-obb.dxnn | vah264dec | 2640 | 3 | 14.5 ±0.0 | 182.38 | 27 | 95.5 | 100.0 | 68~71 | 1000 | 339 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 80.0 | 80.2 | -0.2 | -0.3% |
| yolo26s-obb.dxnn | 46.6 | 46.7 | -0.1 | -0.1% |
| yolo26m-obb.dxnn | 34.2 | 34.2 | +0.0 | +0.1% |
| yolo26l-obb.dxnn | 25.1 | 25.1 | +0.0 | +0.1% |
| yolo26x-obb.dxnn | 14.5 | 14.5 | -0.0 | -0.1% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 574.8 ±9.5 | 6.01 | 58 | 9.6 | 39.2 | 49~50 | 1000 | 98 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 550.7 ±1.8 | 6.27 | 60 | 17.0 | 61.2 | 50~51 | 1000 | 108 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 542.9 ±8.7 | 6.36 | 60 | 25.2 | 62.7 | 54~55 | 1000 | 126 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 546.6 ±4.4 | 6.32 | 60 | 43.9 | 79.8 | 53 | 1000 | 132 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 453.4 ±3.4 | 7.62 | 61 | 69.5 | 98.8 | 56~57 | 1000 | 184 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vah264dec | 3455 | 3 | 554.8 ±13.3 | 6.23 | 59 | 10.2 | 38.4 | 49~50 | 1000 | 101 | ok |
| yolo26s-cls.dxnn | vah264dec | 3455 | 3 | 545.9 ±12.0 | 6.33 | 61 | 19.0 | 61.1 | 50~51 | 1000 | 104 | ok |
| yolo26m-cls.dxnn | vah264dec | 3455 | 3 | 555.2 ±11.1 | 6.22 | 60 | 29.1 | 64.4 | 54~55 | 1000 | 120 | ok |
| yolo26l-cls.dxnn | vah264dec | 3455 | 3 | 549.8 ±8.2 | 6.28 | 60 | 36.1 | 79.1 | 53 | 1000 | 125 | ok |
| yolo26x-cls.dxnn | vah264dec | 3455 | 3 | 457.5 ±1.6 | 7.55 | 61 | 69.5 | 99.1 | 56 | 1000 | 184 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 574.8 | 554.8 | +19.9 | +3.6% |
| yolo26s-cls.dxnn | 550.7 | 545.9 | +4.7 | +0.9% |
| yolo26m-cls.dxnn | 542.9 | 555.2 | -12.2 | -2.2% |
| yolo26l-cls.dxnn | 546.6 | 549.8 | -3.2 | -0.6% |
| yolo26x-cls.dxnn | 453.4 | 457.5 | -4.2 | -0.9% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 8 | 3 | 243.3 ±0.1 | 30.4 | 195 | 97.0 | 100.0 | 53~55 | 1000 | 511 | ok |
| yolo26n.dxnn | 9 | 3 | 243.4 ±0.2 | 27.0 | 197 | 97.2 | 100.0 | 56 | 1000 | 554 | ok |
| yolo26s.dxnn | 4 | 3 | 139.8 ±0.0 | 35.0 | 178 | 97.0 | 100.0 | 58~59 | 1000 | 379 | ok |
| yolo26s.dxnn | 5 | 3 | 139.7 ±0.1 | 27.9 | 176 | 97.3 | 100.0 | 60 | 1000 | 414 | ok |
| yolo26m.dxnn | 3 | 3 | 96.4 ±0.1 | 32.1 | 150 | 97.1 | 100.0 | 66~68 | 1000 | 358 | ok |
| yolo26m.dxnn | 4 | 3 | 96.4 ±0.1 | 24.1 | 151 | 97.1 | 100.0 | 68~69 | 1000 | 401 | ok |
| yolo26l.dxnn | 2 | 3 | 70.7 ±0.1 | 35.4 | 125 | 96.6 | 100.0 | 65~66 | 1000 | 328 | ok |
| yolo26l.dxnn | 3 | 3 | 70.7 ±0.0 | 23.6 | 124 | 97.4 | 100.0 | 67 | 1000 | 369 | ok |
| yolo26x.dxnn | 1 | 3 | 40.7 ±0.2 | 40.7 | 71 | 95.0 | 100.0 | 65~68 | 1000 | 319 | ok |
| yolo26x.dxnn | 2 | 3 | 40.8 ±0.3 | 20.4 | 72 | 96.6 | 100.0 | 71 | 1000 | 389 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 7 | 3 | 240.4 ±0.1 | 34.4 | 181 | 97.0 | 100.0 | 55~56 | 1000 | 505 | ok |
| yolo26n.dxnn | 8 | 3 | 240.2 ±0.1 | 30.0 | 180 | 97.1 | 100.0 | 56 | 1000 | 531 | ok |
| yolo26n.dxnn | 9 | 3 | 240.3 ±0.2 | 26.7 | 182 | 97.1 | 100.0 | 57 | 1000 | 575 | ok |
| yolo26s.dxnn | 4 | 3 | 139.3 ±0.2 | 34.8 | 159 | 97.0 | 100.0 | 58~59 | 1000 | 388 | ok |
| yolo26s.dxnn | 5 | 3 | 139.2 ±0.1 | 27.8 | 160 | 97.0 | 100.0 | 60 | 1000 | 424 | ok |
| yolo26m.dxnn | 3 | 3 | 96.3 ±0.1 | 32.1 | 145 | 96.9 | 100.0 | 66~67 | 1000 | 360 | ok |
| yolo26m.dxnn | 4 | 3 | 96.4 ±0.0 | 24.1 | 148 | 97.6 | 100.0 | 68 | 1000 | 407 | ok |
| yolo26l.dxnn | 2 | 3 | 70.7 ±0.1 | 35.3 | 125 | 96.6 | 100.0 | 65~66 | 1000 | 332 | ok |
| yolo26l.dxnn | 3 | 3 | 70.7 ±0.0 | 23.6 | 127 | 96.9 | 100.0 | 67~68 | 1000 | 374 | ok |
| yolo26x.dxnn | 1 | 3 | 40.7 ±0.1 | 40.7 | 75 | 95.2 | 100.0 | 65~67 | 1000 | 319 | ok |
| yolo26x.dxnn | 2 | 3 | 40.5 ±0.1 | 20.2 | 75 | 97.0 | 100.0 | 70~71 | 1000 | 394 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 8 | 30.4 | 8 | 30.0 |
| yolo26s.dxnn | 4 | 35.0 | 4 | 34.8 |
| yolo26m.dxnn | 3 | 32.1 | 3 | 32.1 |
| yolo26l.dxnn | 2 | 35.4 | 2 | 35.3 |
| yolo26x.dxnn | 1 | 40.7 | 1 | 40.7 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 231.7 ±0.2 | 33.1 | 188 | 96.7 | 100.0 | 56~57 | 1000 | 468 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 231.7 ±0.2 | 29.0 | 192 | 97.2 | 100.0 | 57~58 | 1000 | 512 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 134.0 ±0.2 | 33.5 | 168 | 96.6 | 100.0 | 59~60 | 1000 | 376 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 134.1 ±0.1 | 26.8 | 170 | 96.9 | 100.0 | 61 | 1000 | 412 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 93.2 ±0.1 | 31.1 | 128 | 97.3 | 100.0 | 67~69 | 1000 | 356 | ok |
| yolo26m-pose.dxnn | 4 | 3 | 93.7 ±0.1 | 23.4 | 76 | 97.5 | 100.0 | 70 | 1000 | 397 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.7 ±0.1 | 34.4 | 28 | 96.5 | 100.0 | 65~67 | 1000 | 322 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.7 ±0.0 | 22.9 | 29 | 97.5 | 100.0 | 67 | 1000 | 366 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 40.1 ±0.3 | 40.1 | 24 | 95.4 | 100.0 | 65~68 | 1000 | 330 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 40.1 ±0.1 | 20.1 | 18 | 96.8 | 100.0 | 71 | 1000 | 384 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 230.0 ±0.1 | 32.9 | 221 | 97.3 | 100.0 | 56~57 | 1000 | 460 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 230.2 ±0.1 | 28.8 | 222 | 97.3 | 100.0 | 57~58 | 1000 | 504 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 134.3 ±0.1 | 33.6 | 151 | 96.9 | 100.0 | 59~60 | 1000 | 367 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 134.3 ±0.1 | 26.9 | 152 | 97.5 | 100.0 | 61 | 1000 | 406 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 93.7 ±0.2 | 31.2 | 40 | 96.9 | 100.0 | 66~68 | 1000 | 345 | ok |
| yolo26m-pose.dxnn | 4 | 3 | 93.7 ±0.2 | 23.4 | 31 | 97.9 | 100.0 | 69 | 1000 | 391 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 68.7 ±0.1 | 34.3 | 23 | 96.4 | 100.0 | 65~66 | 1000 | 309 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 68.7 ±0.0 | 22.9 | 23 | 97.4 | 100.0 | 67~68 | 1000 | 353 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 39.8 ±0.3 | 39.8 | 20 | 95.3 | 100.0 | 65~68 | 1000 | 330 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 39.8 ±0.2 | 19.9 | 13 | 97.0 | 100.0 | 71~72 | 1000 | 371 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 7 | 33.1 | 7 | 32.9 |
| yolo26s-pose.dxnn | 4 | 33.5 | 4 | 33.6 |
| yolo26m-pose.dxnn | 3 | 31.1 | 3 | 31.2 |
| yolo26l-pose.dxnn | 2 | 34.4 | 2 | 34.3 |
| yolo26x-pose.dxnn | 1 | 40.1 | 1 | 39.8 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 6 | 3 | 188.4 ±0.1 | 31.4 | 209 | 96.8 | 100.0 | 56~57 | 1000 | 563 | ok |
| yolo26n-seg.dxnn | 7 | 3 | 188.7 ±0.3 | 27.0 | 210 | 97.4 | 100.0 | 58 | 1000 | 610 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 108.9 ±0.3 | 36.3 | 160 | 96.2 | 100.0 | 53~58 | 1000 | 447 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 109.2 ±0.1 | 27.3 | 164 | 97.5 | 100.0 | 62~63 | 1000 | 493 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 69.1 ±0.0 | 34.5 | 133 | 96.6 | 100.0 | 72~74 | 1000 | 430 | ok |
| yolo26m-seg.dxnn | 3 | 3 | 69.1 ±0.1 | 23.0 | 134 | 97.1 | 100.0 | 75~76 | 1000 | 472 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 54.5 ±0.2 | 54.5 | 111 | 95.3 | 100.0 | 65~69 | 1000 | 355 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 54.6 ±0.1 | 27.3 | 112 | 96.5 | 100.0 | 72~73 | 1000 | 444 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 31.1 ±0.2 | 31.1 | 66 | 96.0 | 100.0 | 73~77 | 1000 | 425 | ok |
| yolo26x-seg.dxnn | 2 | 3 | 29.3 ±0.1 | 14.6 | 62 | 96.2 | 100.0 | 78 | 800~1000 | 510 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 6 | 3 | 186.3 ±0.3 | 31.0 | 217 | 97.5 | 100.0 | 57 | 1000 | 584 | ok |
| yolo26n-seg.dxnn | 7 | 3 | 186.5 ±0.1 | 26.6 | 216 | 97.3 | 100.0 | 58 | 1000 | 634 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 108.9 ±0.1 | 36.3 | 155 | 97.2 | 100.0 | 61~62 | 1000 | 457 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 109.0 ±0.1 | 27.2 | 156 | 97.2 | 100.0 | 63~64 | 1000 | 508 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 69.2 ±0.1 | 34.6 | 128 | 96.2 | 100.0 | 73~75 | 1000 | 436 | ok |
| yolo26m-seg.dxnn | 3 | 3 | 69.0 ±0.2 | 23.0 | 128 | 97.1 | 100.0 | 76 | 1000 | 483 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 54.6 ±0.2 | 54.6 | 95 | 95.4 | 100.0 | 65~69 | 1000 | 363 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 54.6 ±0.2 | 27.3 | 110 | 96.9 | 100.0 | 72~74 | 1000 | 448 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 30.8 ±0.1 | 30.8 | 65 | 95.5 | 100.0 | 73~77 | 1000 | 432 | ok |
| yolo26x-seg.dxnn | 2 | 3 | 29.0 ±0.2 | 14.5 | 59 | 96.0 | 100.0 | 78~79 | 800~1000 | 516 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 6 | 31.4 | 6 | 31.0 |
| yolo26s-seg.dxnn | 3 | 36.3 | 3 | 36.3 |
| yolo26m-seg.dxnn | 2 | 34.5 | 2 | 34.6 |
| yolo26l-seg.dxnn | 1 | 54.5 | 1 | 54.6 |
| yolo26x-seg.dxnn | 1 | 31.1 | 1 | 30.8 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 80.1 ±0.1 | 40.1 | 134 | 94.9 | 100.0 | 58 | 1000 | 304 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 80.2 ±0.1 | 26.7 | 142 | 96.4 | 100.0 | 59~60 | 1000 | 352 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.6 ±0.0 | 46.6 | 81 | 94.0 | 100.0 | 57~59 | 1000 | 247 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.7 ±0.1 | 23.4 | 89 | 96.8 | 100.0 | 60~61 | 1000 | 320 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.2 ±0.1 | 34.2 | 61 | 95.6 | 100.0 | 64~66 | 1000 | 270 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 34.2 ±0.1 | 17.1 | 65 | 96.4 | 100.0 | 69~70 | 1000 | 343 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.1 ±0.0 | 25.1 | 46 | 94.3 | 100.0 | 64~67 | 1000 | 275 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 14.5 ±0.0 | 14.5 | 27 | 95.6 | 100.0 | 69~71 | 1000 | 348 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 80.2 ±0.2 | 40.1 | 145 | 95.3 | 100.0 | 57~58 | 1000 | 299 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 80.2 ±0.1 | 26.7 | 147 | 96.1 | 100.0 | 58~59 | 1000 | 340 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.7 ±0.1 | 46.7 | 83 | 94.2 | 100.0 | 57~59 | 1000 | 241 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.6 ±0.1 | 23.3 | 88 | 96.8 | 100.0 | 60~61 | 1000 | 313 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.2 ±0.2 | 34.2 | 64 | 94.7 | 100.0 | 64~67 | 1000 | 266 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 34.2 ±0.0 | 17.1 | 66 | 96.6 | 100.0 | 70 | 1000 | 331 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.1 ±0.1 | 25.1 | 44 | 94.7 | 100.0 | 64~67 | 1000 | 264 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 14.5 ±0.0 | 14.5 | 27 | 95.5 | 100.0 | 68~71 | 1000 | 339 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 40.1 | 2 | 40.1 |
| yolo26s-obb.dxnn | 1 | 46.6 | 1 | 46.7 |
| yolo26m-obb.dxnn | 1 | 34.2 | 1 | 34.2 |

---
*Report generated by dx_stream benchmark tool*
