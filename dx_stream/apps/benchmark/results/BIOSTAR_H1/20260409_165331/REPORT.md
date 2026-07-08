# YOLO26 Benchmark Report

**Generated:** 2026-04-16 17:33:11 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-09 16:53:31 | 2026-04-10 11:05:01 | 18h 11m 30s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 15.04 | 803.1 | 488.1 | 17 |
| yolo26n-1.dxnn | OFF | 14.41 | 806.2 | 442.8 | 14 |
| yolo26s-1.dxnn | ON | 22.86 | 480.7 | 473.9 | 15 |
| yolo26s-1.dxnn | OFF | 22.10 | 480.9 | 437.6 | 14 |
| yolo26m-1.dxnn | ON | 31.11 | 330.5 | 329.6 | 11 |
| yolo26m-1.dxnn | OFF | 30.23 | 330.8 | 329.9 | 11 |
| yolo26l-1.dxnn | ON | 40.01 | 244.9 | 244.2 | 8 |
| yolo26l-1.dxnn | OFF | 39.12 | 244.8 | 244.2 | 8 |
| yolo26x-1.dxnn | ON | 70.02 | 137.8 | 137.7 | 4 |
| yolo26x-1.dxnn | OFF | 69.09 | 137.7 | 137.6 | 4 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 13.65 | 868.9 | 539.3 | 17 |
| yolo26n-pose.dxnn | OFF | 12.92 | 868.5 | 554.5 | 18 |
| yolo26s-pose.dxnn | ON | 20.78 | 511.3 | 508.4 | 17 |
| yolo26s-pose.dxnn | OFF | 19.96 | 511.5 | 508.8 | 16 |
| yolo26m-pose.dxnn | ON | 28.81 | 356.2 | 355.0 | 11 |
| yolo26m-pose.dxnn | OFF | 27.99 | 356.3 | 354.9 | 11 |
| yolo26l-pose.dxnn | ON | 37.11 | 266.9 | 266.3 | 8 |
| yolo26l-pose.dxnn | OFF | 36.22 | 266.9 | 266.2 | 8 |
| yolo26x-pose.dxnn | ON | 65.46 | 151.7 | 151.6 | 5 |
| yolo26x-pose.dxnn | OFF | 64.55 | 151.7 | 151.5 | 5 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 21.86 | 557.6 | 353.2 | 11 |
| yolo26n-seg.dxnn | OFF | 20.77 | 571.3 | 320.3 | 10 |
| yolo26s-seg.dxnn | ON | 30.13 | 410.1 | 351.1 | 11 |
| yolo26s-seg.dxnn | OFF | 29.53 | 411.2 | 314.4 | 10 |
| yolo26m-seg.dxnn | ON | 44.93 | 253.4 | 252.6 | 8 |
| yolo26m-seg.dxnn | OFF | 43.68 | 253.6 | 252.7 | 8 |
| yolo26l-seg.dxnn | ON | 53.40 | 208.3 | 207.7 | 6 |
| yolo26l-seg.dxnn | OFF | 52.02 | 208.1 | 207.6 | 6 |
| yolo26x-seg.dxnn | ON | 93.07 | 114.7 | 114.6 | 3 |
| yolo26x-seg.dxnn | OFF | 91.88 | 114.7 | 114.5 | 3 |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 28.27 | 296.9 | 295.3 | 9 |
| yolo26n-obb.dxnn | OFF | 27.45 | 296.9 | 295.1 | 9 |
| yolo26s-obb.dxnn | ON | 46.67 | 170.4 | 169.8 | 5 |
| yolo26s-obb.dxnn | OFF | 45.88 | 170.3 | 169.9 | 5 |
| yolo26m-obb.dxnn | ON | 65.57 | 123.5 | 123.2 | 4 |
| yolo26m-obb.dxnn | OFF | 64.63 | 123.5 | 123.1 | 4 |
| yolo26l-obb.dxnn | ON | 86.71 | 92.9 | 92.7 | 3 |
| yolo26l-obb.dxnn | OFF | 85.93 | 92.9 | 92.7 | 3 |
| yolo26x-obb.dxnn | ON | 161.29 | 53.4 | 53.5 | 1 |
| yolo26x-obb.dxnn | OFF | 160.40 | 53.4 | 53.5 | 1 |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.01 | 13193.0 | 791.7 | — |
| yolo26n-cls.dxnn | OFF | 1.08 | 13189.9 | 792.3 | — |
| yolo26s-cls.dxnn | ON | 1.74 | 7463.8 | 791.2 | — |
| yolo26s-cls.dxnn | OFF | 1.66 | 7462.8 | 782.2 | — |
| yolo26m-cls.dxnn | ON | 2.34 | 5338.5 | 783.8 | — |
| yolo26m-cls.dxnn | OFF | 2.29 | 5338.4 | 786.5 | — |
| yolo26l-cls.dxnn | ON | 3.63 | 3360.1 | 758.3 | — |
| yolo26l-cls.dxnn | OFF | 3.72 | 3360.3 | 774.4 | — |
| yolo26x-cls.dxnn | ON | 6.34 | 1800.1 | 753.9 | — |
| yolo26x-cls.dxnn | OFF | 6.36 | 1800.3 | 756.8 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | BIOSTAR |
| OS | Ubuntu 22.04.5 LTS |
| Kernel | 6.8.0-106-generic |
| CPU | AMD Ryzen 5 9600X 6-Core Processor |
| CPU Cores | 12 |
| RAM | 30.5 GB |
| NPU SKU | H1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.1.0 |
| NPU Driver (PCIe) | v2.0.1 |
| NPU Firmware | v2.5.0 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | H1, Rev 0.0 |
| NPU PCIe | Gen3 X4 [04:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.2.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.20.3 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.20.3 |
| dxtop | Yes | DX-TOP 1.0.1 |
| ffprobe | Yes | ffprobe version 4.4.2-0ubuntu0.22.04.1 Copyright (c) 2007-20... |

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
| yolo26n-1.dxnn | 803.1 ±0.4 | 255 | 85.1 | 94.9 | 45~48 | 1000 | ok |
| yolo26s-1.dxnn | 480.7 ±0.1 | 129 | 91.0 | 100.0 | 55~58 | 1000 | ok |
| yolo26m-1.dxnn | 330.5 ±0.0 | 86 | 91.2 | 100.0 | 57~60 | 1000 | ok |
| yolo26l-1.dxnn | 244.9 ±0.0 | 65 | 90.7 | 100.0 | 61~63 | 1000 | ok |
| yolo26x-1.dxnn | 137.8 ±0.1 | 39 | 87.9 | 100.0 | 60~63 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 806.2 ±0.2 | 162 | 85.1 | 94.3 | 56~57 | 1000 | ok |
| yolo26s-1.dxnn | 480.9 ±0.1 | 90 | 92.2 | 100.0 | 59~60 | 1000 | ok |
| yolo26m-1.dxnn | 330.8 ±0.2 | 61 | 92.3 | 100.0 | 61~63 | 1000 | ok |
| yolo26l-1.dxnn | 244.8 ±0.0 | 46 | 90.0 | 100.0 | 60~62 | 1000 | ok |
| yolo26x-1.dxnn | 137.7 ±0.0 | 28 | 89.8 | 100.0 | 56~59 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 868.9 ±0.0 | 170 | 92.6 | 100.0 | 57~58 | 1000 | ok |
| yolo26s-pose.dxnn | 511.3 ±0.1 | 96 | 91.8 | 100.0 | 55~57 | 1000 | ok |
| yolo26m-pose.dxnn | 356.2 ±0.1 | 66 | 91.7 | 100.0 | 58~61 | 1000 | ok |
| yolo26l-pose.dxnn | 266.9 ±0.0 | 50 | 90.2 | 100.0 | 59~61 | 1000 | ok |
| yolo26x-pose.dxnn | 151.7 ±0.0 | 33 | 88.0 | 100.0 | 58~61 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 868.5 ±0.1 | 111 | 92.7 | 100.0 | 54~56 | 1000 | ok |
| yolo26s-pose.dxnn | 511.5 ±0.1 | 64 | 92.0 | 100.0 | 57~59 | 1000 | ok |
| yolo26m-pose.dxnn | 356.3 ±0.2 | 45 | 91.0 | 100.0 | 60~62 | 1000 | ok |
| yolo26l-pose.dxnn | 266.9 ±0.0 | 34 | 91.6 | 100.0 | 58~61 | 1000 | ok |
| yolo26x-pose.dxnn | 151.7 ±0.0 | 22 | 87.9 | 100.0 | 59~61 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 557.6 ±1.7 | 520 | 59.1 | 83.4 | 56~57 | 1000 | ok |
| yolo26s-seg.dxnn | 410.1 ±0.1 | 367 | 90.3 | 100.0 | 55~58 | 1000 | ok |
| yolo26m-seg.dxnn | 253.4 ±0.1 | 207 | 90.5 | 100.0 | 59~63 | 1000 | ok |
| yolo26l-seg.dxnn | 208.3 ±0.0 | 169 | 90.2 | 100.0 | 62~65 | 1000 | ok |
| yolo26x-seg.dxnn | 114.7 ±0.0 | 92 | 88.2 | 100.0 | 62~65 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 571.3 ±7.2 | 442 | 61.4 | 85.9 | 54~55 | 1000 | ok |
| yolo26s-seg.dxnn | 411.2 ±0.1 | 299 | 91.2 | 100.0 | 53~56 | 1000 | ok |
| yolo26m-seg.dxnn | 253.6 ±0.0 | 175 | 91.0 | 100.0 | 63~66 | 1000 | ok |
| yolo26l-seg.dxnn | 208.1 ±0.0 | 143 | 93.1 | 100.0 | 62~65 | 1000 | ok |
| yolo26x-seg.dxnn | 114.7 ±0.0 | 79 | 88.2 | 100.0 | 61~64 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 296.9 ±0.1 | 59 | 93.0 | 100.0 | 58~59 | 1000 | ok |
| yolo26s-obb.dxnn | 170.4 ±0.0 | 37 | 90.3 | 100.0 | 54~55 | 1000 | ok |
| yolo26m-obb.dxnn | 123.5 ±0.0 | 29 | 90.6 | 100.0 | 55~58 | 1000 | ok |
| yolo26l-obb.dxnn | 92.9 ±0.1 | 22 | 90.9 | 100.0 | 56~59 | 1000 | ok |
| yolo26x-obb.dxnn | 53.4 ±0.0 | 14 | 86.3 | 100.0 | 56~59 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 296.9 ±0.0 | 41 | 90.9 | 100.0 | 53~55 | 1000 | ok |
| yolo26s-obb.dxnn | 170.3 ±0.0 | 26 | 90.6 | 100.0 | 54~55 | 1000 | ok |
| yolo26m-obb.dxnn | 123.5 ±0.0 | 20 | 88.8 | 100.0 | 56~59 | 1000 | ok |
| yolo26l-obb.dxnn | 92.9 ±0.0 | 15 | 89.4 | 100.0 | 56~58 | 1000 | ok |
| yolo26x-obb.dxnn | 53.4 ±0.0 | 9 | 85.3 | 100.0 | 56~59 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 13193.0 ±2.8 | 59 | 90.8 | 98.3 | 54 | 1000 | ok |
| yolo26s-cls.dxnn | 7463.8 ±0.9 | 35 | 91.5 | 99.0 | 54~55 | 1000 | ok |
| yolo26m-cls.dxnn | 5338.5 ±2.3 | 25 | 90.9 | 99.4 | 56~58 | 1000 | ok |
| yolo26l-cls.dxnn | 3360.1 ±1.0 | 16 | 92.4 | 99.8 | 55~57 | 1000 | ok |
| yolo26x-cls.dxnn | 1800.1 ±0.2 | 9 | 90.3 | 100.0 | 56~58 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 13189.9 ±3.1 | 60 | 90.2 | 98.3 | 53~54 | 1000 | ok |
| yolo26s-cls.dxnn | 7462.8 ±0.2 | 34 | 91.6 | 99.1 | 54~55 | 1000 | ok |
| yolo26m-cls.dxnn | 5338.4 ±1.8 | 25 | 92.0 | 99.5 | 56~58 | 1000 | ok |
| yolo26l-cls.dxnn | 3360.3 ±0.5 | 16 | 91.6 | 99.8 | 55~57 | 1000 | ok |
| yolo26x-cls.dxnn | 1800.3 ±0.3 | 9 | 91.5 | 100.0 | 56~58 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 66.5 | 15.04 | 14.03 | 1.01 | 41 | ok |
| yolo26s-1.dxnn | 43.7 | 22.86 | 21.97 | 0.89 | 52 | ok |
| yolo26m-1.dxnn | 32.1 | 31.11 | 30.11 | 1.00 | 53 | ok |
| yolo26l-1.dxnn | 25.0 | 40.01 | 39.01 | 1.00 | 58 | ok |
| yolo26x-1.dxnn | 14.3 | 70.02 | 69.02 | 1.00 | 56 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 69.4 | 14.41 | 14.41 | 54 | ok |
| yolo26s-1.dxnn | 45.3 | 22.10 | 22.10 | 56 | ok |
| yolo26m-1.dxnn | 33.1 | 30.23 | 30.23 | 58 | ok |
| yolo26l-1.dxnn | 25.6 | 39.12 | 39.12 | 56 | ok |
| yolo26x-1.dxnn | 14.5 | 69.09 | 69.09 | 51 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 73.3 | 13.65 | 12.82 | 0.83 | 55 | ok |
| yolo26s-pose.dxnn | 48.1 | 20.78 | 19.89 | 0.89 | 52 | ok |
| yolo26m-pose.dxnn | 34.7 | 28.81 | 27.93 | 0.88 | 54 | ok |
| yolo26l-pose.dxnn | 26.9 | 37.11 | 36.17 | 0.94 | 55 | ok |
| yolo26x-pose.dxnn | 15.3 | 65.46 | 64.54 | 0.92 | 54 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 77.4 | 12.92 | 12.92 | 52 | ok |
| yolo26s-pose.dxnn | 50.1 | 19.96 | 19.96 | 55 | ok |
| yolo26m-pose.dxnn | 35.7 | 27.99 | 27.99 | 56 | ok |
| yolo26l-pose.dxnn | 27.6 | 36.22 | 36.22 | 54 | ok |
| yolo26x-pose.dxnn | 15.5 | 64.55 | 64.55 | 54 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 45.8 | 21.86 | 20.71 | 1.15 | 54 | ok |
| yolo26s-seg.dxnn | 33.2 | 30.13 | 29.10 | 1.03 | 51 | ok |
| yolo26m-seg.dxnn | 22.3 | 44.93 | 43.78 | 1.16 | 53 | ok |
| yolo26l-seg.dxnn | 18.7 | 53.40 | 52.23 | 1.18 | 58 | ok |
| yolo26x-seg.dxnn | 10.7 | 93.07 | 91.90 | 1.17 | 57 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 48.1 | 20.77 | 20.77 | 51 | ok |
| yolo26s-seg.dxnn | 33.9 | 29.53 | 29.53 | 49 | ok |
| yolo26m-seg.dxnn | 22.9 | 43.68 | 43.68 | 58 | ok |
| yolo26l-seg.dxnn | 19.2 | 52.02 | 52.02 | 57 | ok |
| yolo26x-seg.dxnn | 10.9 | 91.88 | 91.88 | 56 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 35.4 | 28.27 | 27.34 | 0.92 | 57 | ok |
| yolo26s-obb.dxnn | 21.4 | 46.67 | 45.80 | 0.87 | 51 | ok |
| yolo26m-obb.dxnn | 15.2 | 65.57 | 64.64 | 0.94 | 51 | ok |
| yolo26l-obb.dxnn | 11.5 | 86.71 | 85.74 | 0.97 | 52 | ok |
| yolo26x-obb.dxnn | 6.2 | 161.29 | 160.32 | 0.97 | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 36.4 | 27.45 | 27.45 | 51 | ok |
| yolo26s-obb.dxnn | 21.8 | 45.88 | 45.88 | 51 | ok |
| yolo26m-obb.dxnn | 15.5 | 64.63 | 64.63 | 52 | ok |
| yolo26l-obb.dxnn | 11.6 | 85.93 | 85.93 | 52 | ok |
| yolo26x-obb.dxnn | 6.2 | 160.40 | 160.40 | 51 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 990.3 | 1.01 | 1.01 | N/A | 53 | ok |
| yolo26s-cls.dxnn | 575.8 | 1.74 | 1.74 | N/A | 52 | ok |
| yolo26m-cls.dxnn | 427.2 | 2.34 | 2.34 | N/A | 51 | ok |
| yolo26l-cls.dxnn | 275.4 | 3.63 | 3.63 | N/A | 52 | ok |
| yolo26x-cls.dxnn | 157.8 | 6.34 | 6.34 | N/A | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 929.2 | 1.08 | 1.08 | 52 | ok |
| yolo26s-cls.dxnn | 603.7 | 1.66 | 1.66 | 52 | ok |
| yolo26m-cls.dxnn | 436.5 | 2.29 | 2.29 | 52 | ok |
| yolo26l-cls.dxnn | 269.2 | 3.72 | 3.72 | 52 | ok |
| yolo26x-cls.dxnn | 157.2 | 6.36 | 6.36 | 52 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vaapidecodebin | 3455 | 3 | 488.1 ±1.1 | 7.08 | 254 | 32.3 | 68.8 | 48~49 | 1000 | 351 | ok |
| yolo26s-1.dxnn | vaapidecodebin | 3455 | 3 | 473.9 ±0.1 | 7.29 | 268 | 66.8 | 100.0 | 58 | 1000 | 514 | ok |
| yolo26m-1.dxnn | vaapidecodebin | 3455 | 3 | 329.6 ±0.1 | 10.48 | 149 | 78.5 | 100.0 | 61 | 1000 | 545 | ok |
| yolo26l-1.dxnn | vaapidecodebin | 3455 | 3 | 244.2 ±0.0 | 14.14 | 99 | 80.4 | 100.0 | 64 | 1000 | 562 | ok |
| yolo26x-1.dxnn | vaapidecodebin | 3455 | 3 | 137.7 ±0.0 | 25.09 | 52 | 86.6 | 100.0 | 64~66 | 1000 | 605 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | vaapidecodebin | 3455 | 3 | 442.8 ±0.1 | 7.80 | 287 | 28.5 | 59.2 | 56~57 | 1000 | 466 | ok |
| yolo26s-1.dxnn | vaapidecodebin | 3455 | 3 | 437.6 ±1.1 | 7.89 | 290 | 66.4 | 88.5 | 60 | 1000 | 571 | ok |
| yolo26m-1.dxnn | vaapidecodebin | 3455 | 3 | 329.9 ±0.1 | 10.47 | 188 | 80.1 | 100.0 | 63~64 | 1000 | 610 | ok |
| yolo26l-1.dxnn | vaapidecodebin | 3455 | 3 | 244.2 ±0.0 | 14.15 | 127 | 86.3 | 100.0 | 62~63 | 1000 | 622 | ok |
| yolo26x-1.dxnn | vaapidecodebin | 3455 | 3 | 137.6 ±0.1 | 25.11 | 68 | 87.6 | 100.0 | 61~63 | 1000 | 661 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 488.1 | 442.8 | +45.2 | +10.2% |
| yolo26s-1.dxnn | 473.9 | 437.6 | +36.3 | +8.3% |
| yolo26m-1.dxnn | 329.6 | 329.9 | -0.2 | -0.1% |
| yolo26l-1.dxnn | 244.2 | 244.2 | +0.0 | +0.0% |
| yolo26x-1.dxnn | 137.7 | 137.6 | +0.1 | +0.1% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vaapidecodebin | 3455 | 3 | 539.3 ±2.5 | 6.41 | 194 | 34.9 | 74.0 | 57 | 1000 | 316 | ok |
| yolo26s-pose.dxnn | vaapidecodebin | 3455 | 3 | 508.4 ±0.1 | 6.80 | 195 | 75.9 | 100.0 | 57~58 | 1000 | 459 | ok |
| yolo26m-pose.dxnn | vaapidecodebin | 3455 | 3 | 355.0 ±0.1 | 9.73 | 113 | 78.7 | 100.0 | 61~62 | 1000 | 492 | ok |
| yolo26l-pose.dxnn | vaapidecodebin | 3455 | 3 | 266.3 ±0.0 | 12.98 | 79 | 82.4 | 100.0 | 62 | 1000 | 504 | ok |
| yolo26x-pose.dxnn | vaapidecodebin | 3455 | 3 | 151.6 ±0.0 | 22.80 | 44 | 88.0 | 100.0 | 63~64 | 1000 | 554 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vaapidecodebin | 3455 | 3 | 554.5 ±1.0 | 6.23 | 154 | 38.9 | 74.9 | 55 | 1000 | 310 | ok |
| yolo26s-pose.dxnn | vaapidecodebin | 3455 | 3 | 508.8 ±0.1 | 6.79 | 149 | 71.8 | 100.0 | 59 | 1000 | 492 | ok |
| yolo26m-pose.dxnn | vaapidecodebin | 3455 | 3 | 354.9 ±0.3 | 9.73 | 89 | 78.1 | 100.0 | 62~63 | 1000 | 523 | ok |
| yolo26l-pose.dxnn | vaapidecodebin | 3455 | 3 | 266.2 ±0.1 | 12.98 | 63 | 82.0 | 100.0 | 61~62 | 1000 | 535 | ok |
| yolo26x-pose.dxnn | vaapidecodebin | 3455 | 3 | 151.5 ±0.1 | 22.80 | 35 | 87.9 | 100.0 | 63~64 | 1000 | 575 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 539.3 | 554.5 | -15.2 | -2.7% |
| yolo26s-pose.dxnn | 508.4 | 508.8 | -0.4 | -0.1% |
| yolo26m-pose.dxnn | 355.0 | 354.9 | +0.0 | +0.0% |
| yolo26l-pose.dxnn | 266.3 | 266.2 | +0.0 | +0.0% |
| yolo26x-pose.dxnn | 151.6 | 151.5 | +0.0 | +0.0% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vaapidecodebin | 3455 | 3 | 353.2 ±0.6 | 9.78 | 571 | 25.7 | 74.7 | 56 | 1000 | 615 | ok |
| yolo26s-seg.dxnn | vaapidecodebin | 3455 | 3 | 351.1 ±0.6 | 9.84 | 574 | 56.9 | 83.2 | 58 | 1000 | 808 | ok |
| yolo26m-seg.dxnn | vaapidecodebin | 3455 | 3 | 252.6 ±0.1 | 13.68 | 304 | 82.3 | 100.0 | 64~65 | 1000 | 851 | ok |
| yolo26l-seg.dxnn | vaapidecodebin | 3455 | 3 | 207.7 ±0.1 | 16.63 | 231 | 83.4 | 100.0 | 66~67 | 1000 | 868 | ok |
| yolo26x-seg.dxnn | vaapidecodebin | 3455 | 3 | 114.6 ±0.0 | 30.16 | 115 | 88.5 | 100.0 | 68~70 | 1000 | 910 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vaapidecodebin | 3455 | 3 | 320.3 ±0.9 | 10.79 | 459 | 24.2 | 86.8 | 55 | 1000 | 693 | ok |
| yolo26s-seg.dxnn | vaapidecodebin | 3455 | 3 | 314.4 ±0.8 | 10.99 | 453 | 49.9 | 92.1 | 56~57 | 1000 | 832 | ok |
| yolo26m-seg.dxnn | vaapidecodebin | 3455 | 3 | 252.7 ±0.0 | 13.67 | 325 | 82.7 | 100.0 | 67 | 1000 | 953 | ok |
| yolo26l-seg.dxnn | vaapidecodebin | 3455 | 3 | 207.6 ±0.1 | 16.64 | 249 | 83.1 | 100.0 | 65~67 | 1000 | 966 | ok |
| yolo26x-seg.dxnn | vaapidecodebin | 3455 | 3 | 114.5 ±0.1 | 30.17 | 125 | 87.6 | 100.0 | 67~69 | 1000 | 1005 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 353.2 | 320.3 | +32.9 | +10.3% |
| yolo26s-seg.dxnn | 351.1 | 314.4 | +36.7 | +11.7% |
| yolo26m-seg.dxnn | 252.6 | 252.7 | -0.2 | -0.1% |
| yolo26l-seg.dxnn | 207.7 | 207.6 | +0.1 | +0.0% |
| yolo26x-seg.dxnn | 114.6 | 114.5 | +0.0 | +0.0% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vaapidecodebin | 2640 | 3 | 295.3 ±0.2 | 8.94 | 122 | 77.3 | 100.0 | 59 | 1000 | 548 | ok |
| yolo26s-obb.dxnn | vaapidecodebin | 2640 | 3 | 169.8 ±0.1 | 15.54 | 65 | 84.6 | 100.0 | 56 | 1000 | 574 | ok |
| yolo26m-obb.dxnn | vaapidecodebin | 2640 | 3 | 123.2 ±0.1 | 21.43 | 47 | 86.7 | 100.0 | 59~60 | 1000 | 603 | ok |
| yolo26l-obb.dxnn | vaapidecodebin | 2640 | 3 | 92.7 ±0.0 | 28.47 | 37 | 88.1 | 100.0 | 60~62 | 1000 | 622 | ok |
| yolo26x-obb.dxnn | vaapidecodebin | 2640 | 3 | 53.5 ±0.0 | 49.39 | 22 | 88.9 | 100.0 | 63~66 | 1000 | 664 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vaapidecodebin | 2640 | 3 | 295.1 ±0.6 | 8.95 | 126 | 77.1 | 100.0 | 55 | 1000 | 577 | ok |
| yolo26s-obb.dxnn | vaapidecodebin | 2640 | 3 | 169.9 ±0.1 | 15.54 | 68 | 83.4 | 100.0 | 55~56 | 1000 | 601 | ok |
| yolo26m-obb.dxnn | vaapidecodebin | 2640 | 3 | 123.1 ±0.0 | 21.44 | 49 | 87.2 | 100.0 | 60~61 | 1000 | 634 | ok |
| yolo26l-obb.dxnn | vaapidecodebin | 2640 | 3 | 92.7 ±0.0 | 28.49 | 38 | 88.9 | 100.0 | 60~62 | 1000 | 652 | ok |
| yolo26x-obb.dxnn | vaapidecodebin | 2640 | 3 | 53.5 ±0.0 | 49.39 | 22 | 89.3 | 100.0 | 63~66 | 1000 | 694 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 295.3 | 295.1 | +0.2 | +0.1% |
| yolo26s-obb.dxnn | 169.8 | 169.9 | -0.0 | -0.0% |
| yolo26m-obb.dxnn | 123.2 | 123.1 | +0.1 | +0.0% |
| yolo26l-obb.dxnn | 92.7 | 92.7 | +0.1 | +0.1% |
| yolo26x-obb.dxnn | 53.5 | 53.5 | -0.0 | -0.0% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vaapidecodebin | 3455 | 3 | 791.7 ±0.8 | 4.36 | 39 | 3.5 | 16.6 | 53~54 | 1000 | 166 | ok |
| yolo26s-cls.dxnn | vaapidecodebin | 3455 | 3 | 791.2 ±0.5 | 4.37 | 39 | 6.0 | 29.5 | 54 | 1000 | 179 | ok |
| yolo26m-cls.dxnn | vaapidecodebin | 3455 | 3 | 783.8 ±1.1 | 4.41 | 40 | 8.6 | 41.5 | 57 | 1000 | 196 | ok |
| yolo26l-cls.dxnn | vaapidecodebin | 3455 | 3 | 758.3 ±26.4 | 4.56 | 42 | 14.1 | 65.9 | 55~56 | 1000 | 263 | ok |
| yolo26x-cls.dxnn | vaapidecodebin | 3455 | 3 | 753.9 ±2.1 | 4.58 | 43 | 24.6 | 59.6 | 57 | 1000 | 260 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vaapidecodebin | 3455 | 3 | 792.3 ±3.0 | 4.36 | 39 | 2.6 | 16.5 | 53 | 1000 | 164 | ok |
| yolo26s-cls.dxnn | vaapidecodebin | 3455 | 3 | 782.2 ±14.8 | 4.42 | 39 | 6.1 | 29.6 | 54 | 1000 | 242 | ok |
| yolo26m-cls.dxnn | vaapidecodebin | 3455 | 3 | 786.5 ±1.3 | 4.39 | 40 | 8.7 | 41.3 | 57 | 1000 | 193 | ok |
| yolo26l-cls.dxnn | vaapidecodebin | 3455 | 3 | 774.4 ±3.9 | 4.46 | 41 | 14.1 | 65.8 | 56 | 1000 | 207 | ok |
| yolo26x-cls.dxnn | vaapidecodebin | 3455 | 3 | 756.8 ±12.1 | 4.57 | 43 | 26.1 | 59.3 | 57 | 1000 | 310 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 791.7 | 792.3 | -0.6 | -0.1% |
| yolo26s-cls.dxnn | 791.2 | 782.2 | +9.0 | +1.1% |
| yolo26m-cls.dxnn | 783.8 | 786.5 | -2.6 | -0.3% |
| yolo26l-cls.dxnn | 758.3 | 774.4 | -16.1 | -2.1% |
| yolo26x-cls.dxnn | 753.9 | 756.8 | -3.0 | -0.4% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 16 | 3 | 516.2 ±0.5 | 32.3 | 358 | 44.1 | 72.7 | 54~58 | 1000 | 744 | ok |
| yolo26n-1.dxnn | 17 | 3 | 511.6 ±2.4 | 30.1 | 360 | 43.7 | 74.3 | 60~62 | 1000 | 749 | ok |
| yolo26n-1.dxnn | 18 | 3 | 512.3 ±1.1 | 28.5 | 360 | 43.8 | 73.9 | 63 | 1000 | 779 | ok |
| yolo26s-1.dxnn | 15 | 3 | 472.8 ±0.4 | 31.5 | 329 | 94.8 | 100.0 | 64~68 | 1000 | 772 | ok |
| yolo26s-1.dxnn | 16 | 3 | 472.7 ±0.1 | 29.6 | 326 | 94.8 | 100.0 | 71~72 | 1000 | 776 | ok |
| yolo26m-1.dxnn | 10 | 3 | 330.6 ±0.1 | 33.1 | 170 | 96.2 | 100.0 | 69~73 | 1000 | 704 | ok |
| yolo26m-1.dxnn | 11 | 3 | 330.5 ±0.1 | 30.1 | 171 | 96.5 | 100.0 | 76~78 | 1000 | 730 | ok |
| yolo26m-1.dxnn | 12 | 3 | 330.7 ±0.0 | 27.6 | 172 | 96.5 | 100.0 | 79 | 1000 | 757 | ok |
| yolo26l-1.dxnn | 8 | 3 | 244.7 ±0.1 | 30.6 | 109 | 96.3 | 100.0 | 70~74 | 1000 | 693 | ok |
| yolo26l-1.dxnn | 9 | 3 | 244.6 ±0.0 | 27.2 | 109 | 96.3 | 100.0 | 75~76 | 1000 | 720 | ok |
| yolo26x-1.dxnn | 4 | 2/3 | 137.7 ±0.3 | 34.4 | 54 | 95.5 | 100.0 | 53~71 | 1000 | 669 | partial |
| yolo26x-1.dxnn | 3 | 3 | 137.8 ±0.1 | 45.9 | 55 | 93.9 | 100.0 | 62~68 | 1000 | 649 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 14 | 3 | 434.5 ±1.4 | 31.0 | 303 | 34.5 | 69.2 | 58~59 | 1000 | 791 | ok |
| yolo26n-1.dxnn | 15 | 3 | 433.2 ±0.4 | 28.9 | 303 | 34.4 | 69.7 | 60~61 | 1000 | 802 | ok |
| yolo26s-1.dxnn | 14 | 3 | 429.5 ±0.4 | 30.7 | 308 | 80.2 | 92.4 | 65~67 | 1000 | 874 | ok |
| yolo26s-1.dxnn | 15 | 3 | 430.6 ±1.8 | 28.7 | 309 | 81.0 | 90.9 | 60~65 | 1000 | 890 | ok |
| yolo26m-1.dxnn | 10 | 3 | 330.5 ±0.2 | 33.0 | 210 | 96.4 | 100.0 | 70~75 | 1000 | 812 | ok |
| yolo26m-1.dxnn | 11 | 3 | 330.6 ±0.2 | 30.1 | 212 | 96.4 | 100.0 | 77~79 | 1000 | 818 | ok |
| yolo26m-1.dxnn | 12 | 3 | 330.7 ±0.1 | 27.6 | 213 | 96.6 | 100.0 | 79~80 | 1000 | 848 | ok |
| yolo26l-1.dxnn | 8 | 3 | 244.7 ±0.1 | 30.6 | 138 | 96.0 | 100.0 | 69~73 | 1000 | 772 | ok |
| yolo26l-1.dxnn | 9 | 3 | 244.6 ±0.0 | 27.2 | 139 | 96.7 | 100.0 | 75~76 | 1000 | 799 | ok |
| yolo26x-1.dxnn | 4 | 3 | 137.9 ±0.0 | 34.5 | 71 | 94.8 | 100.0 | 69~73 | 1000 | 735 | ok |
| yolo26x-1.dxnn | 5 | 3 | 137.9 ±0.0 | 27.6 | 71 | 95.3 | 100.0 | 75~77 | 1000 | 759 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 17 | 30.1 | 14 | 31.0 |
| yolo26s-1.dxnn | 15 | 31.5 | 14 | 30.7 |
| yolo26m-1.dxnn | 11 | 30.1 | 11 | 30.1 |
| yolo26l-1.dxnn | 8 | 30.6 | 8 | 30.6 |
| yolo26x-1.dxnn | 4+ | 34.4 | 4 | 34.5 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 17 | 2/3 | 586.7 ±1.6 | 34.5 | 276 | 51.7 | 77.7 | 52~59 | 1000 | 714 | partial |
| yolo26n-pose.dxnn | 16 | 3 | 589.5 ±2.0 | 36.8 | 277 | 51.6 | 78.0 | 56~59 | 1000 | 691 | ok |
| yolo26s-pose.dxnn | 16 | 3 | 510.8 ±0.4 | 31.9 | 235 | 96.6 | 100.0 | 64~68 | 1000 | 728 | ok |
| yolo26s-pose.dxnn | 17 | 3 | 510.8 ±0.3 | 30.1 | 239 | 96.6 | 100.0 | 70~71 | 1000 | 757 | ok |
| yolo26s-pose.dxnn | 18 | 3 | 510.9 ±0.1 | 28.4 | 238 | 97.0 | 100.0 | 72 | 1000 | 773 | ok |
| yolo26m-pose.dxnn | 11 | 3 | 356.0 ±0.1 | 32.4 | 130 | 96.7 | 100.0 | 70~74 | 1000 | 681 | ok |
| yolo26m-pose.dxnn | 12 | 3 | 355.9 ±0.1 | 29.6 | 132 | 96.9 | 100.0 | 76~78 | 1000 | 711 | ok |
| yolo26l-pose.dxnn | 8 | 3 | 266.8 ±0.1 | 33.4 | 88 | 96.4 | 100.0 | 68~72 | 1000 | 646 | ok |
| yolo26l-pose.dxnn | 9 | 3 | 266.7 ±0.1 | 29.6 | 88 | 96.7 | 100.0 | 74~75 | 1000 | 675 | ok |
| yolo26x-pose.dxnn | 5 | 3 | 151.8 ±0.0 | 30.4 | 46 | 95.6 | 100.0 | 71~75 | 1000 | 639 | ok |
| yolo26x-pose.dxnn | 6 | 3 | 151.8 ±0.0 | 25.3 | 47 | 96.3 | 100.0 | 77~78 | 1000 | 650 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 18 | 2/3 | 603.1 ±1.6 | 33.5 | 225 | 53.5 | 79.2 | 58~59 | 1000 | 768 | partial |
| yolo26n-pose.dxnn | 17 | 3 | 603.6 ±1.5 | 35.5 | 226 | 53.3 | 79.2 | 56~61 | 1000 | 764 | ok |
| yolo26s-pose.dxnn | 16 | 2/3 | 510.9 ±0.1 | 31.9 | 185 | 97.2 | 100.0 | 64~66 | 1000 | 777 | partial |
| yolo26s-pose.dxnn | 15 | 3 | 511.0 ±0.1 | 34.1 | 186 | 96.6 | 100.0 | 68~69 | 1000 | 748 | ok |
| yolo26m-pose.dxnn | 11 | 3 | 356.0 ±0.2 | 32.4 | 105 | 96.4 | 100.0 | 70~73 | 1000 | 730 | ok |
| yolo26m-pose.dxnn | 12 | 3 | 356.0 ±0.1 | 29.7 | 104 | 96.8 | 100.0 | 76~77 | 1000 | 757 | ok |
| yolo26l-pose.dxnn | 8 | 3 | 266.8 ±0.0 | 33.4 | 71 | 96.3 | 100.0 | 68~72 | 1000 | 686 | ok |
| yolo26l-pose.dxnn | 9 | 3 | 266.7 ±0.1 | 29.6 | 72 | 96.4 | 100.0 | 74~75 | 1000 | 716 | ok |
| yolo26x-pose.dxnn | 5 | 3 | 151.8 ±0.0 | 30.4 | 38 | 95.9 | 100.0 | 71~74 | 1000 | 665 | ok |
| yolo26x-pose.dxnn | 6 | 3 | 151.8 ±0.0 | 25.3 | 38 | 96.2 | 100.0 | 76~77 | 1000 | 682 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 17+ | 34.5 | 18+ | 33.5 |
| yolo26s-pose.dxnn | 17 | 30.1 | 16+ | 31.9 |
| yolo26m-pose.dxnn | 11 | 32.4 | 11 | 32.4 |
| yolo26l-pose.dxnn | 8 | 33.4 | 8 | 33.4 |
| yolo26x-pose.dxnn | 5 | 30.4 | 5 | 30.4 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 11 | 3 | 352.3 ±0.9 | 32.0 | 678 | 32.2 | 80.5 | 57~59 | 1000 | 1030 | ok |
| yolo26n-seg.dxnn | 12 | 3 | 359.8 ±2.8 | 30.0 | 676 | 32.7 | 76.0 | 60~61 | 1000 | 1060 | ok |
| yolo26s-seg.dxnn | 11 | 3 | 353.6 ±3.1 | 32.1 | 665 | 72.4 | 87.3 | 64~68 | 1000 | 1054 | ok |
| yolo26s-seg.dxnn | 12 | 2/3 | 349.3 ±0.9 | 29.1 | 667 | 72.0 | 86.8 | 49~70 | 1000 | 1082 | partial |
| yolo26m-seg.dxnn | 8 | 3 | 253.2 ±0.1 | 31.6 | 328 | 96.6 | 100.0 | 74~80 | 1000 | 1022 | ok |
| yolo26m-seg.dxnn | 9 | 3 | 246.3 ±4.0 | 27.4 | 319 | 95.9 | 100.0 | 83~84 | 600~1000 | 1052 | ok |
| yolo26l-seg.dxnn | 6 | 3 | 208.1 ±0.0 | 34.7 | 245 | 95.8 | 100.0 | 74~78 | 1000 | 985 | ok |
| yolo26l-seg.dxnn | 7 | 3 | 206.6 ±0.7 | 29.5 | 246 | 95.9 | 100.0 | 80~82 | 800~1000 | 1036 | ok |
| yolo26x-seg.dxnn | 3 | 3 | 114.2 ±0.8 | 38.1 | 117 | 94.0 | 100.0 | 76~79 | 800~1000 | 972 | ok |
| yolo26x-seg.dxnn | 4 | 3 | 110.7 ±0.7 | 27.7 | 117 | 93.1 | 100.0 | 82 | 600~1000 | 1006 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 10 | 3 | 314.5 ±0.6 | 31.4 | 475 | 27.4 | 85.5 | 57~58 | 1000 | 1035 | ok |
| yolo26n-seg.dxnn | 11 | 3 | 310.0 ±1.1 | 28.2 | 487 | 27.0 | 84.5 | 59~60 | 1000 | 1070 | ok |
| yolo26s-seg.dxnn | 10 | 3 | 308.0 ±1.5 | 30.8 | 468 | 56.1 | 89.7 | 62~65 | 1000 | 1141 | ok |
| yolo26s-seg.dxnn | 11 | 3 | 303.7 ±0.7 | 27.6 | 474 | 55.2 | 87.4 | 67~68 | 1000 | 1192 | ok |
| yolo26m-seg.dxnn | 8 | 3 | 253.1 ±0.3 | 31.6 | 351 | 97.0 | 100.0 | 76~81 | 800~1000 | 1154 | ok |
| yolo26m-seg.dxnn | 9 | 3 | 246.1 ±3.7 | 27.4 | 339 | 95.5 | 100.0 | 83 | 600~1000 | 1166 | ok |
| yolo26l-seg.dxnn | 6 | 3 | 208.0 ±0.1 | 34.7 | 262 | 95.8 | 100.0 | 74~78 | 1000 | 1107 | ok |
| yolo26l-seg.dxnn | 7 | 3 | 206.7 ±0.3 | 29.5 | 264 | 95.6 | 100.0 | 81~82 | 800~1000 | 1139 | ok |
| yolo26x-seg.dxnn | 3 | 3 | 114.6 ±0.0 | 38.2 | 128 | 94.3 | 100.0 | 76~79 | 1000 | 1070 | ok |
| yolo26x-seg.dxnn | 4 | 3 | 111.3 ±1.6 | 27.8 | 127 | 94.1 | 100.0 | 82~83 | 600~1000 | 1115 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 11 | 32.0 | 10 | 31.4 |
| yolo26s-seg.dxnn | 11 | 32.1 | 10 | 30.8 |
| yolo26m-seg.dxnn | 8 | 31.6 | 8 | 31.6 |
| yolo26l-seg.dxnn | 6 | 34.7 | 6 | 34.7 |
| yolo26x-seg.dxnn | 3 | 38.1 | 3 | 38.2 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 9 | 3 | 296.4 ±0.0 | 32.9 | 140 | 96.0 | 100.0 | 61~62 | 1000 | 743 | ok |
| yolo26n-obb.dxnn | 10 | 3 | 296.4 ±0.2 | 29.6 | 142 | 96.4 | 100.0 | 64~65 | 1000 | 737 | ok |
| yolo26s-obb.dxnn | 5 | 3 | 170.2 ±0.1 | 34.0 | 71 | 95.6 | 100.0 | 60~62 | 1000 | 666 | ok |
| yolo26s-obb.dxnn | 6 | 3 | 170.4 ±0.0 | 28.4 | 71 | 95.7 | 100.0 | 63~64 | 1000 | 685 | ok |
| yolo26m-obb.dxnn | 4 | 3 | 123.5 ±0.0 | 30.9 | 49 | 95.0 | 100.0 | 65~69 | 1000 | 683 | ok |
| yolo26m-obb.dxnn | 5 | 3 | 123.4 ±0.0 | 24.7 | 50 | 95.9 | 100.0 | 71~72 | 1000 | 694 | ok |
| yolo26l-obb.dxnn | 3 | 3 | 92.8 ±0.0 | 30.9 | 39 | 94.4 | 100.0 | 66~68 | 1000 | 676 | ok |
| yolo26l-obb.dxnn | 4 | 3 | 92.8 ±0.0 | 23.2 | 39 | 95.5 | 100.0 | 70~72 | 1000 | 700 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 53.5 ±0.0 | 53.5 | 22 | 88.9 | 100.0 | 65~81 | 1000 | 664 | ok |
| yolo26x-obb.dxnn | 2 | 3 | 53.5 ±0.0 | 26.7 | 23 | 91.3 | 100.0 | 70~72 | 1000 | 702 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 9 | 3 | 296.2 ±0.1 | 32.9 | 142 | 95.6 | 100.0 | 58~61 | 1000 | 769 | ok |
| yolo26n-obb.dxnn | 10 | 3 | 296.4 ±0.2 | 29.6 | 144 | 95.6 | 100.0 | 62~63 | 1000 | 771 | ok |
| yolo26s-obb.dxnn | 5 | 3 | 170.2 ±0.0 | 34.0 | 74 | 95.0 | 100.0 | 59~61 | 1000 | 698 | ok |
| yolo26s-obb.dxnn | 6 | 3 | 170.4 ±0.0 | 28.4 | 74 | 95.7 | 100.0 | 63~64 | 1000 | 717 | ok |
| yolo26m-obb.dxnn | 4 | 3 | 123.5 ±0.1 | 30.9 | 52 | 94.6 | 100.0 | 66~69 | 1000 | 716 | ok |
| yolo26m-obb.dxnn | 5 | 3 | 123.4 ±0.0 | 24.7 | 52 | 95.8 | 100.0 | 71~73 | 1000 | 725 | ok |
| yolo26l-obb.dxnn | 3 | 3 | 92.8 ±0.0 | 30.9 | 40 | 94.2 | 100.0 | 66~68 | 1000 | 702 | ok |
| yolo26l-obb.dxnn | 4 | 3 | 92.8 ±0.0 | 23.2 | 40 | 94.8 | 100.0 | 71~72 | 1000 | 730 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 53.5 ±0.0 | 53.5 | 22 | 89.3 | 100.0 | 65~81 | 1000 | 694 | ok |
| yolo26x-obb.dxnn | 2 | 3 | 53.5 ±0.0 | 26.7 | 24 | 91.4 | 100.0 | 70~72 | 1000 | 724 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 9 | 32.9 | 9 | 32.9 |
| yolo26s-obb.dxnn | 5 | 34.0 | 5 | 34.0 |
| yolo26m-obb.dxnn | 4 | 30.9 | 4 | 30.9 |
| yolo26l-obb.dxnn | 3 | 30.9 | 3 | 30.9 |
| yolo26x-obb.dxnn | 1 | 53.5 | 1 | 53.5 |

---
*Report generated by dx_stream benchmark tool*
