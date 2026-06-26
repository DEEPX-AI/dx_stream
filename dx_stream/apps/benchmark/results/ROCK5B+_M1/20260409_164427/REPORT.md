# YOLO26 Benchmark Report

**Generated:** 2026-04-15 17:57:44 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-09 16:44:27 | 2026-04-10 13:50:20 | 21h 5m 52s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 37.31 | 157.1 | 146.2 | 4 |
| yolo26n-1.dxnn | OFF | 25.00 | 181.1 | 95.8 | 3 |
| yolo26s-1.dxnn | ON | 52.52 | 113.6 | 112.9 | 3 |
| yolo26s-1.dxnn | OFF | 42.18 | 115.5 | 96.0 | 3 |
| yolo26m-1.dxnn | ON | 60.83 | 79.8 | 79.7 | 2 |
| yolo26m-1.dxnn | OFF | 56.63 | 79.8 | 79.8 | 2 |
| yolo26l-1.dxnn | ON | 71.74 | 59.1 | 59.2 | 1 |
| yolo26l-1.dxnn | OFF | 63.25 | 59.1 | 59.2 | 1 |
| yolo26x-1.dxnn | ON | 116.05 | 33.5 | 33.6 | 1 |
| yolo26x-1.dxnn | OFF | 99.25 | 33.6 | 33.6 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 32.63 | 201.3 | 197.2 | 6 |
| yolo26n-pose.dxnn | OFF | 30.61 | 209.1 | 208.4 | 6 |
| yolo26s-pose.dxnn | ON | 44.88 | 123.6 | 123.1 | 4 |
| yolo26s-pose.dxnn | OFF | 34.88 | 123.8 | 123.6 | 4 |
| yolo26m-pose.dxnn | ON | 54.29 | 86.2 | 86.2 | 2 |
| yolo26m-pose.dxnn | OFF | 43.71 | 86.2 | 86.2 | 1 |
| yolo26l-pose.dxnn | ON | — | 64.6 | 64.7 | 2 |
| yolo26l-pose.dxnn | OFF | 51.52 | 64.7 | 64.7 | 2 |
| yolo26x-pose.dxnn | ON | 100.11 | 37.1 | 37.2 | 1 |
| yolo26x-pose.dxnn | OFF | 86.71 | 37.1 | 37.2 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 44.95 | 106.9 | 95.0 | 3 |
| yolo26n-seg.dxnn | OFF | 41.99 | 124.3 | 79.4 | 2 |
| yolo26s-seg.dxnn | ON | 61.95 | 85.7 | 86.9 | 2 |
| yolo26s-seg.dxnn | OFF | 47.80 | 93.0 | 78.1 | 2 |
| yolo26m-seg.dxnn | ON | 77.70 | 61.4 | 61.6 | 1 |
| yolo26m-seg.dxnn | OFF | 68.18 | 61.7 | 61.7 | 1 |
| yolo26l-seg.dxnn | ON | 86.73 | 50.5 | 50.6 | 1 |
| yolo26l-seg.dxnn | OFF | 75.86 | 50.6 | 50.6 | 1 |
| yolo26x-seg.dxnn | ON | 145.00 | 28.1 | 26.3 | — |
| yolo26x-seg.dxnn | OFF | 123.44 | 28.1 | 26.0 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 64.41 | 71.4 | 71.4 | 2 |
| yolo26n-obb.dxnn | OFF | 51.63 | 71.5 | 71.4 | 2 |
| yolo26s-obb.dxnn | ON | 80.34 | 35.4 | 41.2 | 1 |
| yolo26s-obb.dxnn | OFF | 71.99 | 41.2 | 41.2 | 1 |
| yolo26m-obb.dxnn | ON | 102.84 | 30.1 | 30.1 | 1 |
| yolo26m-obb.dxnn | OFF | 91.93 | 30.1 | 30.1 | 1 |
| yolo26l-obb.dxnn | ON | 123.06 | 22.4 | 22.5 | — |
| yolo26l-obb.dxnn | OFF | 113.19 | 22.4 | 22.5 | — |
| yolo26x-obb.dxnn | ON | 199.33 | 13.0 | 12.6 | — |
| yolo26x-obb.dxnn | OFF | 193.81 | 13.0 | 12.6 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.39 | 3258.0 | 978.1 | — |
| yolo26n-cls.dxnn | OFF | 1.34 | 3259.0 | 964.5 | — |
| yolo26s-cls.dxnn | ON | 3.23 | 1839.4 | 979.5 | — |
| yolo26s-cls.dxnn | OFF | 1.99 | 1838.8 | 971.7 | — |
| yolo26m-cls.dxnn | ON | 2.62 | 1321.2 | 987.3 | — |
| yolo26m-cls.dxnn | OFF | 4.18 | 1320.9 | 975.2 | — |
| yolo26l-cls.dxnn | ON | 3.89 | 831.1 | 807.3 | — |
| yolo26l-cls.dxnn | OFF | 3.91 | 830.8 | 791.3 | — |
| yolo26x-cls.dxnn | ON | 6.56 | 444.9 | 443.3 | — |
| yolo26x-cls.dxnn | OFF | 6.87 | 445.0 | 444.0 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | ROCK5B+ |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.1.84-6-rk2410 |
| CPU | - |
| CPU Cores | 8 |
| RAM | 7.8 GB |
| NPU SKU | M1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.1.0 |
| NPU Driver (PCIe) | v2.0.1 |
| NPU Firmware | v2.5.2 |
| NPU Memory | LPDDR5 5600 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X2 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.2.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.9 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.9 |
| dxtop | Yes | DX-TOP 1.0.1 |
| ffprobe | Yes | ffprobe version 5.1.7-0+deb12u1 Copyright (c) 2007-2025 the ... |

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
| yolo26l-pose.dxnn | Pose Estimation | 640×640 | — | Yes | ✅ |
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
| yolo26n-1.dxnn | 157.1 ±1.0 | 224 | 55.0 | 91.5 | 47~50 | 1000 | ok |
| yolo26s-1.dxnn | 113.6 ±0.7 | 165 | 89.3 | 100.0 | 57~59 | 1000 | ok |
| yolo26m-1.dxnn | 79.8 ±0.1 | 111 | 89.0 | 100.0 | 58~62 | 1000 | ok |
| yolo26l-1.dxnn | 59.1 ±0.0 | 81 | 89.5 | 100.0 | 60~62 | 1000 | ok |
| yolo26x-1.dxnn | 33.5 ±0.0 | 68 | 89.1 | 100.0 | 61~64 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 181.1 ±0.4 | 136 | 72.8 | 88.6 | 56~57 | 1000 | ok |
| yolo26s-1.dxnn | 115.5 ±0.1 | 100 | 91.7 | 100.0 | 57~59 | 1000 | ok |
| yolo26m-1.dxnn | 79.8 ±0.0 | 77 | 90.4 | 100.0 | 61~63 | 1000 | ok |
| yolo26l-1.dxnn | 59.1 ±0.0 | 67 | 90.5 | 100.0 | 60~63 | 1000 | ok |
| yolo26x-1.dxnn | 33.6 ±0.0 | 46 | 89.1 | 100.0 | 63~66 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 201.3 ±1.8 | 197 | 86.8 | 96.9 | 59~61 | 1000 | ok |
| yolo26s-pose.dxnn | 123.6 ±0.1 | 115 | 90.7 | 100.0 | 57~58 | 1000 | ok |
| yolo26m-pose.dxnn | 86.2 ±0.0 | 88 | 90.8 | 100.0 | 59~63 | 1000 | ok |
| yolo26l-pose.dxnn | 64.6 ±0.0 | 87 | 89.3 | 100.0 | 51~56 | 1000 | ok |
| yolo26x-pose.dxnn | 37.1 ±0.0 | 64 | 88.4 | 100.0 | 63~67 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 209.1 ±0.2 | 114 | 88.7 | 100.0 | 57~59 | 1000 | ok |
| yolo26s-pose.dxnn | 123.8 ±0.0 | 87 | 90.8 | 100.0 | 57~59 | 1000 | ok |
| yolo26m-pose.dxnn | 86.2 ±0.1 | 75 | 91.3 | 100.0 | 61~64 | 1000 | ok |
| yolo26l-pose.dxnn | 64.7 ±0.0 | 63 | 89.8 | 100.0 | 61~64 | 1000 | ok |
| yolo26x-pose.dxnn | 37.1 ±0.0 | 49 | 88.3 | 100.0 | 63~65 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 106.9 ±0.6 | 315 | 40.5 | 70.7 | 58 | 1000 | ok |
| yolo26s-seg.dxnn | 85.7 ±0.5 | 226 | 69.4 | 87.9 | 57~59 | 1000 | ok |
| yolo26m-seg.dxnn | 61.4 ±0.0 | 155 | 89.7 | 100.0 | 61~65 | 1000 | ok |
| yolo26l-seg.dxnn | 50.5 ±0.0 | 124 | 89.6 | 100.0 | 62~65 | 1000 | ok |
| yolo26x-seg.dxnn | 28.1 ±0.0 | 76 | 88.0 | 100.0 | 64~68 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 124.3 ±0.4 | 209 | 50.4 | 83.3 | 55~57 | 1000 | ok |
| yolo26s-seg.dxnn | 93.0 ±0.8 | 165 | 77.9 | 94.1 | 60~62 | 1000 | ok |
| yolo26m-seg.dxnn | 61.7 ±0.1 | 107 | 89.7 | 100.0 | 63~67 | 1000 | ok |
| yolo26l-seg.dxnn | 50.6 ±0.0 | 103 | 89.8 | 100.0 | 63~67 | 1000 | ok |
| yolo26x-seg.dxnn | 28.1 ±0.0 | 65 | 87.8 | 100.0 | 64~68 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 71.4 ±0.1 | 97 | 91.5 | 100.0 | 60~61 | 1000 | ok |
| yolo26s-obb.dxnn | 35.4 ±10.2 | 62 | 70.8 | 100.0 | 57~58 | 1000 | ok |
| yolo26m-obb.dxnn | 30.1 ±0.0 | 64 | 89.9 | 100.0 | 60~63 | 1000 | ok |
| yolo26l-obb.dxnn | 22.4 ±0.0 | 62 | 87.7 | 100.0 | 62~64 | 1000 | ok |
| yolo26x-obb.dxnn | 13.0 ±0.0 | 42 | 85.4 | 100.0 | 62~66 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 71.5 ±0.0 | 76 | 90.8 | 100.0 | 56~58 | 1000 | ok |
| yolo26s-obb.dxnn | 41.2 ±0.0 | 64 | 90.0 | 100.0 | 58~60 | 1000 | ok |
| yolo26m-obb.dxnn | 30.1 ±0.0 | 53 | 90.4 | 100.0 | 63~65 | 1000 | ok |
| yolo26l-obb.dxnn | 22.4 ±0.0 | 49 | 88.3 | 100.0 | 61~64 | 1000 | ok |
| yolo26x-obb.dxnn | 13.0 ±0.0 | 30 | 85.5 | 100.0 | 64~66 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3258.0 ±8.1 | 105 | 88.3 | 96.1 | 59 | 1000 | ok |
| yolo26s-cls.dxnn | 1839.4 ±2.7 | 72 | 88.8 | 98.1 | 56~57 | 1000 | ok |
| yolo26m-cls.dxnn | 1321.2 ±0.6 | 52 | 88.5 | 98.5 | 58~61 | 1000 | ok |
| yolo26l-cls.dxnn | 831.1 ±0.1 | 61 | 89.2 | 99.3 | 57~60 | 1000 | ok |
| yolo26x-cls.dxnn | 444.9 ±0.4 | 49 | 89.0 | 99.7 | 59~61 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3259.0 ±7.8 | 107 | 88.2 | 96.8 | 55~56 | 1000 | ok |
| yolo26s-cls.dxnn | 1838.8 ±3.2 | 71 | 89.2 | 97.9 | 56~57 | 1000 | ok |
| yolo26m-cls.dxnn | 1320.9 ±0.1 | 53 | 89.3 | 98.4 | 59~61 | 1000 | ok |
| yolo26l-cls.dxnn | 830.8 ±0.2 | 59 | 89.3 | 99.3 | 58~60 | 1000 | ok |
| yolo26x-cls.dxnn | 445.0 ±0.1 | 51 | 89.2 | 99.6 | 59~61 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 26.8 | 37.31 | 27.61 | 9.70 | 44 | ok |
| yolo26s-1.dxnn | 19.0 | 52.52 | 43.20 | 9.32 | 53 | ok |
| yolo26m-1.dxnn | 16.4 | 60.83 | 50.49 | 10.34 | 54 | ok |
| yolo26l-1.dxnn | 13.9 | 71.74 | 60.48 | 11.26 | 56 | ok |
| yolo26x-1.dxnn | 8.6 | 116.05 | 101.08 | 14.97 | 57 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 40.0 | 25.00 | 25.00 | 54 | ok |
| yolo26s-1.dxnn | 23.7 | 42.18 | 42.18 | 54 | ok |
| yolo26m-1.dxnn | 17.7 | 56.63 | 56.63 | 56 | ok |
| yolo26l-1.dxnn | 15.8 | 63.25 | 63.25 | 56 | ok |
| yolo26x-1.dxnn | 10.1 | 99.25 | 99.25 | 58 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 30.6 | 32.63 | 26.76 | 5.87 | 57 | ok |
| yolo26s-pose.dxnn | 22.3 | 44.88 | 36.84 | 8.03 | 53 | ok |
| yolo26m-pose.dxnn | 18.4 | 54.29 | 45.99 | 8.30 | 54 | ok |
| yolo26l-pose.dxnn | N/A | N/A | N/A | N/A | 51~62 | timeout |
| yolo26x-pose.dxnn | 10.0 | 100.11 | 83.33 | 16.77 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 32.7 | 30.61 | 30.61 | 55 | ok |
| yolo26s-pose.dxnn | 28.7 | 34.88 | 34.88 | 54 | ok |
| yolo26m-pose.dxnn | 22.9 | 43.71 | 43.71 | 56 | ok |
| yolo26l-pose.dxnn | 19.4 | 51.52 | 51.52 | 57 | ok |
| yolo26x-pose.dxnn | 11.5 | 86.71 | 86.71 | 58 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 22.2 | 44.95 | 35.43 | 9.52 | 57 | ok |
| yolo26s-seg.dxnn | 16.1 | 61.95 | 49.91 | 12.04 | 53 | ok |
| yolo26m-seg.dxnn | 12.9 | 77.70 | 64.94 | 12.76 | 55 | ok |
| yolo26l-seg.dxnn | 11.5 | 86.73 | 73.52 | 13.20 | 57 | ok |
| yolo26x-seg.dxnn | 6.9 | 145.00 | 128.19 | 16.81 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 23.8 | 41.99 | 41.99 | 54 | ok |
| yolo26s-seg.dxnn | 20.9 | 47.80 | 47.80 | 56 | ok |
| yolo26m-seg.dxnn | 14.7 | 68.18 | 68.18 | 58 | ok |
| yolo26l-seg.dxnn | 13.2 | 75.86 | 75.86 | 58 | ok |
| yolo26x-seg.dxnn | 8.1 | 123.44 | 123.44 | 58 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 15.5 | 64.41 | 46.09 | 18.32 | 59 | ok |
| yolo26s-obb.dxnn | 12.4 | 80.34 | 69.42 | 10.92 | 54 | ok |
| yolo26m-obb.dxnn | 9.7 | 102.84 | 91.55 | 11.29 | 55 | ok |
| yolo26l-obb.dxnn | 8.1 | 123.06 | 111.56 | 11.49 | 58 | ok |
| yolo26x-obb.dxnn | 5.0 | 199.33 | 187.67 | 11.67 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 19.4 | 51.63 | 51.63 | 54 | ok |
| yolo26s-obb.dxnn | 13.9 | 71.99 | 71.99 | 55 | ok |
| yolo26m-obb.dxnn | 10.9 | 91.93 | 91.93 | 58 | ok |
| yolo26l-obb.dxnn | 8.8 | 113.19 | 113.19 | 57 | ok |
| yolo26x-obb.dxnn | 5.2 | 193.81 | 193.81 | 59 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 717.3 | 1.39 | 1.39 | N/A | 58 | ok |
| yolo26s-cls.dxnn | 309.2 | 3.23 | 3.23 | N/A | 54 | ok |
| yolo26m-cls.dxnn | 381.9 | 2.62 | 2.62 | N/A | 54 | ok |
| yolo26l-cls.dxnn | 257.1 | 3.89 | 3.89 | N/A | 54 | ok |
| yolo26x-cls.dxnn | 152.5 | 6.56 | 6.56 | N/A | 54 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 748.8 | 1.34 | 1.34 | 54 | ok |
| yolo26s-cls.dxnn | 502.8 | 1.99 | 1.99 | 54 | ok |
| yolo26m-cls.dxnn | 239.2 | 4.18 | 4.18 | 54 | ok |
| yolo26l-cls.dxnn | 255.8 | 3.91 | 3.91 | 54 | ok |
| yolo26x-cls.dxnn | 145.5 | 6.87 | 6.87 | 54 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | mppvideodec | 3455 | 3 | 146.2 ±1.2 | 23.63 | 268 | 48.3 | 83.6 | 52~53 | 1000 | 162 | ok |
| yolo26s-1.dxnn | mppvideodec | 3455 | 2/3 | 112.9 ±0.3 | 30.59 | 214 | 90.1 | 100.0 | 47~60 | 1000 | 176 | partial |
| yolo26m-1.dxnn | mppvideodec | 3455 | 3 | 79.7 ±0.1 | 43.37 | 152 | 92.1 | 100.0 | 65~68 | 1000 | 199 | ok |
| yolo26l-1.dxnn | mppvideodec | 3455 | 3 | 59.2 ±0.0 | 58.40 | 100 | 93.7 | 100.0 | 65~68 | 1000 | 207 | ok |
| yolo26x-1.dxnn | mppvideodec | 3455 | 3 | 33.6 ±0.0 | 102.69 | 74 | 95.3 | 100.0 | 72~76 | 1000 | 321 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | mppvideodec | 3455 | 3 | 95.8 ±0.4 | 36.07 | 216 | 29.1 | 92.6 | 56 | 1000 | 180 | ok |
| yolo26s-1.dxnn | mppvideodec | 3455 | 3 | 96.0 ±0.7 | 35.99 | 209 | 64.3 | 76.3 | 59~61 | 1000 | 194 | ok |
| yolo26m-1.dxnn | mppvideodec | 3455 | 3 | 79.8 ±0.0 | 43.27 | 168 | 92.9 | 100.0 | 65~68 | 1000 | 212 | ok |
| yolo26l-1.dxnn | mppvideodec | 3455 | 3 | 59.2 ±0.0 | 58.39 | 131 | 94.8 | 100.0 | 66~69 | 1000 | 222 | ok |
| yolo26x-1.dxnn | mppvideodec | 3455 | 3 | 33.6 ±0.0 | 102.70 | 101 | 95.7 | 100.0 | 72~76 | 1000 | 321 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 146.2 | 95.8 | +50.4 | +52.7% |
| yolo26s-1.dxnn | 112.9 | 96.0 | +16.9 | +17.6% |
| yolo26m-1.dxnn | 79.7 | 79.8 | -0.2 | -0.2% |
| yolo26l-1.dxnn | 59.2 | 59.2 | -0.0 | -0.0% |
| yolo26x-1.dxnn | 33.6 | 33.6 | +0.0 | +0.0% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 197.2 ±1.1 | 17.52 | 270 | 75.7 | 93.0 | 60 | 1000 | 152 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 123.1 ±0.7 | 28.06 | 157 | 88.7 | 100.0 | 59~61 | 1000 | 168 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 86.2 ±0.0 | 40.08 | 106 | 92.0 | 100.0 | 66~69 | 1000 | 189 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 64.7 ±0.0 | 53.43 | 82 | 93.0 | 100.0 | 62~67 | 1000 | 198 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 37.2 ±0.0 | 92.84 | 73 | 95.2 | 100.0 | 73~78 | 1000 | 331 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 208.4 ±0.7 | 16.58 | 186 | 84.6 | 99.8 | 58~59 | 1000 | 142 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 123.6 ±0.1 | 27.95 | 118 | 91.2 | 100.0 | 59~60 | 1000 | 157 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 2/3 | 86.2 ±0.0 | 40.09 | 94 | 92.7 | 100.0 | 65~67 | 1000 | 180 | partial |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 64.7 ±0.0 | 53.41 | 73 | 93.5 | 100.0 | 67~70 | 1000 | 186 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 37.2 ±0.0 | 92.89 | 78 | 94.7 | 100.0 | 71~76 | 1000 | 331 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 197.2 | 208.4 | -11.2 | -5.4% |
| yolo26s-pose.dxnn | 123.1 | 123.6 | -0.5 | -0.4% |
| yolo26m-pose.dxnn | 86.2 | 86.2 | +0.0 | +0.0% |
| yolo26l-pose.dxnn | 64.7 | 64.7 | -0.0 | -0.0% |
| yolo26x-pose.dxnn | 37.2 | 37.2 | +0.0 | +0.1% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 95.0 ±1.1 | 36.35 | 386 | 35.0 | 60.7 | 58~59 | 1000 | 258 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 86.9 ±0.2 | 39.77 | 337 | 70.6 | 83.6 | 61~64 | 1000 | 273 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 61.6 ±0.0 | 56.08 | 212 | 93.3 | 100.0 | 70~75 | 1000 | 299 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 50.6 ±0.0 | 68.26 | 177 | 95.0 | 100.0 | 71~77 | 1000 | 304 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 26.3 ±1.5 | 131.31 | 96 | 93.9 | 100.0 | 79~82 | 800~1000 | 376 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 79.4 ±1.8 | 43.52 | 299 | 27.6 | 87.5 | 56~57 | 1000 | 297 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 78.1 ±1.8 | 44.24 | 287 | 56.9 | 92.7 | 62~64 | 1000 | 312 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 61.7 ±0.0 | 55.99 | 216 | 93.8 | 100.0 | 72~77 | 1000 | 330 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 50.6 ±0.0 | 68.25 | 173 | 94.5 | 100.0 | 72~76 | 1000 | 340 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 26.0 ±1.8 | 132.75 | 102 | 94.0 | 100.0 | 79~83 | 600~1000 | 384 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 95.0 | 79.4 | +15.7 | +19.7% |
| yolo26s-seg.dxnn | 86.9 | 78.1 | +8.8 | +11.2% |
| yolo26m-seg.dxnn | 61.6 | 61.7 | -0.1 | -0.2% |
| yolo26l-seg.dxnn | 50.6 | 50.6 | -0.0 | -0.0% |
| yolo26x-seg.dxnn | 26.3 | 26.0 | +0.3 | +1.1% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 71.4 ±0.1 | 36.98 | 116 | 91.9 | 100.0 | 61~62 | 1000 | 188 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 41.2 ±0.0 | 64.04 | 90 | 94.3 | 100.0 | 61~64 | 1000 | 205 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 30.1 ±0.0 | 87.57 | 79 | 96.0 | 100.0 | 70~74 | 1000 | 225 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 22.5 ±0.0 | 117.53 | 70 | 94.2 | 100.0 | 71~75 | 1000 | 236 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 12.6 ±0.4 | 208.97 | 55 | 94.8 | 100.0 | 77~80 | 1000 | 344 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 71.4 ±0.1 | 36.97 | 116 | 92.2 | 100.0 | 59~60 | 1000 | 190 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 41.2 ±0.0 | 64.05 | 89 | 94.4 | 100.0 | 62~65 | 1000 | 194 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 30.1 ±0.0 | 87.55 | 88 | 96.3 | 100.0 | 70~74 | 1000 | 223 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 22.5 ±0.0 | 117.53 | 94 | 94.2 | 100.0 | 70~74 | 1000 | 234 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 12.6 ±0.4 | 209.16 | 57 | 95.0 | 100.0 | 78~80 | 1000 | 344 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 71.4 | 71.4 | -0.0 | -0.0% |
| yolo26s-obb.dxnn | 41.2 | 41.2 | +0.0 | +0.0% |
| yolo26m-obb.dxnn | 30.1 | 30.1 | +0.0 | +0.0% |
| yolo26l-obb.dxnn | 22.5 | 22.5 | +0.0 | +0.0% |
| yolo26x-obb.dxnn | 12.6 | 12.6 | +0.0 | +0.1% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 978.1 ±13.1 | 3.53 | 152 | 14.5 | 47.1 | 56~57 | 1000 | 58 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 979.5 ±16.8 | 3.53 | 155 | 30.1 | 67.9 | 56 | 1000 | 71 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 987.3 ±25.1 | 3.50 | 153 | 42.9 | 84.2 | 59 | 1000 | 94 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 807.3 ±4.3 | 4.28 | 132 | 56.9 | 98.9 | 58 | 1000 | 107 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 443.3 ±0.5 | 7.79 | 94 | 70.9 | 99.8 | 60~61 | 1000 | 183 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 964.5 ±18.9 | 3.58 | 153 | 13.5 | 49.1 | 54 | 1000 | 58 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 971.7 ±25.9 | 3.56 | 152 | 28.8 | 69.2 | 55~56 | 1000 | 71 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 975.2 ±38.8 | 3.54 | 151 | 36.3 | 85.0 | 59 | 1000 | 94 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 791.3 ±17.2 | 4.37 | 133 | 54.4 | 99.0 | 58 | 1000 | 107 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 444.0 ±0.4 | 7.78 | 105 | 66.1 | 99.6 | 60~61 | 1000 | 183 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 978.1 | 964.5 | +13.7 | +1.4% |
| yolo26s-cls.dxnn | 979.5 | 971.7 | +7.8 | +0.8% |
| yolo26m-cls.dxnn | 987.3 | 975.2 | +12.2 | +1.2% |
| yolo26l-cls.dxnn | 807.3 | 791.3 | +16.0 | +2.0% |
| yolo26x-cls.dxnn | 443.3 | 444.0 | -0.8 | -0.2% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 4 | 3 | 145.2 ±0.3 | 36.3 | 290 | 52.6 | 83.3 | 58~61 | 1000 | 184 | ok |
| yolo26n-1.dxnn | 5 | 3 | 145.1 ±0.1 | 29.0 | 286 | 52.8 | 83.3 | 63~65 | 1000 | 189 | ok |
| yolo26s-1.dxnn | 3 | 3 | 114.5 ±0.1 | 38.2 | 203 | 94.8 | 100.0 | 57~64 | 1000 | 196 | ok |
| yolo26s-1.dxnn | 4 | 3 | 114.6 ±0.1 | 28.6 | 202 | 95.0 | 100.0 | 67~70 | 1000 | 201 | ok |
| yolo26m-1.dxnn | 2 | 3 | 79.9 ±0.0 | 40.0 | 151 | 95.7 | 100.0 | 72~76 | 1000 | 210 | ok |
| yolo26m-1.dxnn | 3 | 3 | 77.9 ±1.6 | 26.0 | 141 | 96.8 | 100.0 | 77~78 | 1000 | 216 | ok |
| yolo26l-1.dxnn | 1 | 3 | 59.2 ±0.0 | 59.2 | 100 | 93.7 | 100.0 | 72~82 | 1000 | 207 | ok |
| yolo26l-1.dxnn | 2 | 3 | 59.3 ±0.0 | 29.6 | 103 | 96.3 | 100.0 | 73~76 | 1000 | 220 | ok |
| yolo26x-1.dxnn | 1 | 3 | 33.6 ±0.0 | 33.6 | 74 | 95.3 | 100.0 | 78~87 | 1000 | 321 | ok |
| yolo26x-1.dxnn | 2 | 3 | 31.9 ±0.4 | 16.0 | 74 | 96.4 | 100.0 | 79 | 1000 | 321 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 3 | 3 | 95.8 ±0.5 | 31.9 | 214 | 30.0 | 93.6 | 57~58 | 1000 | 204 | ok |
| yolo26n-1.dxnn | 4 | 3 | 96.1 ±0.2 | 24.0 | 210 | 30.3 | 93.4 | 58 | 1000 | 211 | ok |
| yolo26s-1.dxnn | 3 | 3 | 95.1 ±0.1 | 31.7 | 215 | 65.3 | 83.9 | 63~65 | 1000 | 218 | ok |
| yolo26s-1.dxnn | 4 | 3 | 95.4 ±0.5 | 23.9 | 214 | 66.3 | 86.5 | 65~66 | 1000 | 227 | ok |
| yolo26m-1.dxnn | 2 | 3 | 80.0 ±0.0 | 40.0 | 172 | 96.0 | 100.0 | 71~75 | 1000 | 225 | ok |
| yolo26m-1.dxnn | 3 | 3 | 78.0 ±1.3 | 26.0 | 165 | 97.0 | 100.0 | 78 | 1000 | 227 | ok |
| yolo26l-1.dxnn | 1 | 3 | 59.2 ±0.0 | 59.2 | 131 | 94.8 | 100.0 | 72~81 | 1000 | 222 | ok |
| yolo26l-1.dxnn | 2 | 3 | 59.2 ±0.0 | 29.6 | 126 | 96.8 | 100.0 | 72~75 | 1000 | 235 | ok |
| yolo26x-1.dxnn | 1 | 3 | 33.6 ±0.0 | 33.6 | 101 | 95.7 | 100.0 | 78~88 | 1000 | 321 | ok |
| yolo26x-1.dxnn | 2 | 3 | 31.9 ±0.4 | 16.0 | 98 | 96.5 | 100.0 | 79~80 | 1000 | 321 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 4 | 36.3 | 3 | 31.9 |
| yolo26s-1.dxnn | 3 | 38.2 | 3 | 31.7 |
| yolo26m-1.dxnn | 2 | 40.0 | 2 | 40.0 |
| yolo26l-1.dxnn | 1 | 59.2 | 1 | 59.2 |
| yolo26x-1.dxnn | 1 | 33.6 | 1 | 33.6 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 3 | 196.9 ±0.2 | 32.8 | 291 | 85.0 | 91.2 | 64~67 | 1000 | 194 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 196.6 ±0.1 | 28.1 | 294 | 85.6 | 90.9 | 67~70 | 1000 | 201 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 123.8 ±0.0 | 31.0 | 165 | 96.9 | 100.0 | 65~68 | 1000 | 193 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 123.9 ±0.1 | 24.8 | 172 | 97.0 | 100.0 | 71~72 | 1000 | 197 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 86.3 ±0.1 | 43.1 | 110 | 95.9 | 100.0 | 72~75 | 1000 | 204 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 83.3 ±1.9 | 27.8 | 105 | 96.8 | 100.0 | 78 | 1000 | 210 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 64.8 ±0.0 | 32.4 | 83 | 96.6 | 100.0 | 72~76 | 1000 | 213 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 62.7 ±1.1 | 20.9 | 82 | 97.2 | 100.0 | 78 | 1000 | 219 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 37.2 ±0.0 | 37.2 | 73 | 95.2 | 100.0 | 80~90 | 800~1000 | 331 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 34.6 ±0.7 | 17.3 | 71 | 95.9 | 100.0 | 80~82 | 800~1000 | 331 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 2/3 | 209.1 ±0.1 | 34.9 | 202 | 96.3 | 100.0 | 63~64 | 1000 | 189 | partial |
| yolo26n-pose.dxnn | 5 | 3 | 209.0 ±0.1 | 41.8 | 208 | 95.3 | 100.0 | 66~68 | 1000 | 184 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 123.8 ±0.0 | 30.9 | 122 | 96.6 | 100.0 | 64~68 | 1000 | 190 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 123.9 ±0.0 | 24.8 | 124 | 97.4 | 100.0 | 68~70 | 1000 | 198 | ok |
| yolo26m-pose.dxnn | 2 | 0/3 | 0.0 | 0.0 | 0 | 0.0 | 0.0 | 44~45 | 1000 | 0 | timeout |
| yolo26m-pose.dxnn | 1 | 2/3 | 86.2 ±0.0 | 86.2 | 94 | 92.7 | 100.0 | 44~45 | 1000 | 180 | partial |
| yolo26l-pose.dxnn | 2 | 3 | 64.8 ±0.0 | 32.4 | 76 | 96.0 | 100.0 | 73~76 | 1000 | 201 | ok |
| yolo26l-pose.dxnn | 3 | 3 | 63.5 ±1.6 | 21.2 | 69 | 96.9 | 100.0 | 76~79 | 1000 | 212 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 37.2 ±0.0 | 37.2 | 78 | 94.7 | 100.0 | 80~89 | 1000 | 331 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 35.0 ±0.4 | 17.5 | 78 | 96.4 | 100.0 | 80~81 | 1000 | 331 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 6 | 32.8 | 6+ | 34.9 |
| yolo26s-pose.dxnn | 4 | 31.0 | 4 | 30.9 |
| yolo26m-pose.dxnn | 2 | 43.1 | 1+ | 86.2 |
| yolo26l-pose.dxnn | 2 | 32.4 | 2 | 32.4 |
| yolo26x-pose.dxnn | 1 | 37.2 | 1 | 37.2 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 93.9 ±0.5 | 31.3 | 384 | 35.8 | 64.5 | 61~63 | 1000 | 285 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 94.3 ±0.1 | 23.6 | 397 | 36.0 | 63.9 | 63~64 | 1000 | 296 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 86.8 ±0.2 | 43.4 | 345 | 71.5 | 82.1 | 68~71 | 1000 | 289 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 86.8 ±0.3 | 28.9 | 346 | 72.2 | 84.8 | 72~74 | 1000 | 301 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 58.1 ±1.3 | 29.0 | 210 | 96.2 | 100.0 | 79~81 | 800~1000 | 318 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 61.6 ±0.0 | 61.6 | 212 | 93.3 | 100.0 | 78~90 | 800~1000 | 299 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 50.6 ±0.0 | 50.6 | 177 | 95.0 | 100.0 | 80~90 | 800~1000 | 304 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 46.7 ±1.0 | 23.4 | 163 | 96.1 | 100.0 | 81 | 800~1000 | 325 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 26.3 ±1.5 | 26.3 | 96 | 93.9 | 100.0 | 79~82 | 800~1000 | 376 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 2 | 3 | 80.8 ±0.5 | 40.4 | 292 | 28.7 | 89.4 | 58~59 | 1000 | 321 | ok |
| yolo26n-seg.dxnn | 3 | 3 | 79.1 ±1.9 | 26.4 | 304 | 28.2 | 88.3 | 60~61 | 1000 | 335 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 78.7 ±0.7 | 39.3 | 289 | 58.9 | 95.1 | 65~68 | 1000 | 336 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 76.9 ±2.0 | 25.6 | 290 | 57.9 | 92.7 | 68~70 | 1000 | 352 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 57.2 ±0.5 | 28.6 | 202 | 96.0 | 100.0 | 81~82 | 800~1000 | 362 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 61.7 ±0.0 | 61.7 | 216 | 93.8 | 100.0 | 80~91 | 800~1000 | 330 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 50.6 ±0.0 | 50.6 | 173 | 94.5 | 100.0 | 79~91 | 800~1000 | 340 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 47.0 ±1.0 | 23.5 | 156 | 96.3 | 100.0 | 80~82 | 800~1000 | 365 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 26.0 ±1.8 | 26.0 | 102 | 94.0 | 100.0 | 79~83 | 600~1000 | 384 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 31.3 | 2 | 40.4 |
| yolo26s-seg.dxnn | 2 | 43.4 | 2 | 39.3 |
| yolo26m-seg.dxnn | 1 | 61.6 | 1 | 61.7 |
| yolo26l-seg.dxnn | 1 | 50.6 | 1 | 50.6 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 71.5 ±0.0 | 35.8 | 121 | 96.2 | 100.0 | 63~64 | 1000 | 204 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 71.5 ±0.0 | 23.8 | 119 | 96.7 | 100.0 | 65~67 | 1000 | 217 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 41.2 ±0.0 | 41.2 | 90 | 94.3 | 100.0 | 65~73 | 1000 | 205 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 41.2 ±0.0 | 20.6 | 87 | 96.7 | 100.0 | 67~69 | 1000 | 222 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 30.1 ±0.0 | 30.1 | 79 | 96.0 | 100.0 | 77~86 | 1000 | 225 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 28.9 ±0.8 | 14.5 | 82 | 96.4 | 100.0 | 78 | 1000 | 245 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 22.5 ±0.0 | 22.5 | 70 | 94.2 | 100.0 | 71~75 | 1000 | 236 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.6 ±0.4 | 12.6 | 55 | 94.8 | 100.0 | 77~80 | 1000 | 344 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 71.5 ±0.0 | 35.8 | 122 | 95.6 | 100.0 | 62~65 | 1000 | 197 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 71.5 ±0.0 | 23.8 | 123 | 96.4 | 100.0 | 65~67 | 1000 | 210 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 41.2 ±0.0 | 41.2 | 89 | 94.4 | 100.0 | 67~73 | 1000 | 194 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 41.2 ±0.0 | 20.6 | 96 | 96.4 | 100.0 | 67~69 | 1000 | 225 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 30.1 ±0.0 | 30.1 | 88 | 96.3 | 100.0 | 77~86 | 1000 | 223 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.1 ±0.9 | 14.6 | 85 | 96.6 | 100.0 | 77~79 | 1000 | 239 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 22.5 ±0.0 | 22.5 | 94 | 94.2 | 100.0 | 70~74 | 1000 | 234 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.6 ±0.4 | 12.6 | 57 | 95.0 | 100.0 | 78~80 | 1000 | 344 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 35.8 | 2 | 35.8 |
| yolo26s-obb.dxnn | 1 | 41.2 | 1 | 41.2 |
| yolo26m-obb.dxnn | 1 | 30.1 | 1 | 30.1 |

---
*Report generated by dx_stream benchmark tool*
