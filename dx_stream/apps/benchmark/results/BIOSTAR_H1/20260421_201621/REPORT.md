# YOLO26 Benchmark Report

**Generated:** 2026-04-22 13:02:04 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-21 20:16:21 | 2026-04-22 13:02:04 | 16h 45m 42s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 11.74 | 927.9 | 498.3 | 17 |
| yolo26n.dxnn | OFF | 11.71 | 929.3 | 463.1 | 15 |
| yolo26s.dxnn | ON | 19.07 | 536.4 | 497.5 | 17 |
| yolo26s.dxnn | OFF | 18.79 | 536.6 | 458.1 | 14 |
| yolo26m.dxnn | ON | 26.18 | 371.3 | 372.4 | 12 |
| yolo26m.dxnn | OFF | 26.06 | 372.5 | 372.9 | 12 |
| yolo26l.dxnn | ON | 34.45 | 273.1 | 275.3 | 9 |
| yolo26l.dxnn | OFF | 34.12 | 273.3 | 274.9 | 9 |
| yolo26x.dxnn | ON | 61.72 | 156.8 | 159.0 | 5 |
| yolo26x.dxnn | OFF | 61.36 | 156.2 | 158.4 | 5 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 11.66 | 889.6 | 542.7 | 19 |
| yolo26n-pose.dxnn | OFF | 11.63 | 888.8 | 557.8 | 18 |
| yolo26s-pose.dxnn | ON | 18.84 | 516.7 | 516.1 | 17 |
| yolo26s-pose.dxnn | OFF | 18.51 | 516.3 | 515.0 | 17 |
| yolo26m-pose.dxnn | ON | 26.37 | 359.2 | 364.5 | 12 |
| yolo26m-pose.dxnn | OFF | 25.86 | 361.1 | 363.5 | 12 |
| yolo26l-pose.dxnn | ON | 34.56 | 265.8 | 269.2 | 8 |
| yolo26l-pose.dxnn | OFF | 34.10 | 265.8 | 269.0 | 8 |
| yolo26x-pose.dxnn | ON | 62.05 | 152.9 | 155.6 | 5 |
| yolo26x-pose.dxnn | OFF | 61.70 | 152.7 | 155.4 | 5 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 19.68 | 573.1 | 363.0 | 12 |
| yolo26n-seg.dxnn | OFF | 19.29 | 609.7 | 323.0 | 10 |
| yolo26s-seg.dxnn | ON | 28.25 | 419.7 | 360.5 | 12 |
| yolo26s-seg.dxnn | OFF | 27.84 | 420.0 | 317.0 | 10 |
| yolo26m-seg.dxnn | ON | 42.31 | 266.0 | 265.7 | 8 |
| yolo26m-seg.dxnn | OFF | 41.70 | 265.2 | 265.0 | 8 |
| yolo26l-seg.dxnn | ON | 50.42 | 209.7 | 211.5 | 7 |
| yolo26l-seg.dxnn | OFF | 50.17 | 209.7 | 211.8 | 7 |
| yolo26x-seg.dxnn | ON | 89.47 | 118.4 | 119.9 | 3 |
| yolo26x-seg.dxnn | OFF | 89.06 | 118.6 | 119.8 | 3 |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 25.68 | 308.2 | 307.0 | 10 |
| yolo26n-obb.dxnn | OFF | 25.26 | 307.6 | 307.9 | 10 |
| yolo26s-obb.dxnn | ON | 43.24 | 178.5 | 178.0 | 5 |
| yolo26s-obb.dxnn | OFF | 42.91 | 178.5 | 177.8 | 5 |
| yolo26m-obb.dxnn | ON | 60.79 | 130.8 | 130.9 | 4 |
| yolo26m-obb.dxnn | OFF | 60.52 | 130.7 | 130.8 | 4 |
| yolo26l-obb.dxnn | ON | 82.57 | 95.5 | 95.9 | 3 |
| yolo26l-obb.dxnn | OFF | 81.86 | 95.2 | 95.7 | 3 |
| yolo26x-obb.dxnn | ON | 154.07 | 55.1 | 55.3 | 1 |
| yolo26x-obb.dxnn | OFF | 153.67 | 55.2 | 55.3 | 1 |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 0.99 | 14005.5 | 784.9 | — |
| yolo26n-cls.dxnn | OFF | 1.01 | 14000.5 | 782.9 | — |
| yolo26s-cls.dxnn | ON | 1.66 | 7645.6 | 780.2 | — |
| yolo26s-cls.dxnn | OFF | 1.76 | 7644.1 | 777.8 | — |
| yolo26m-cls.dxnn | ON | 2.30 | 5396.1 | 776.2 | — |
| yolo26m-cls.dxnn | OFF | 2.30 | 5394.5 | 775.0 | — |
| yolo26l-cls.dxnn | ON | 3.62 | 3382.8 | 761.5 | — |
| yolo26l-cls.dxnn | OFF | 3.66 | 3384.3 | 764.2 | — |
| yolo26x-cls.dxnn | ON | 6.34 | 1818.3 | 741.8 | — |
| yolo26x-cls.dxnn | OFF | 6.30 | 1819.4 | 740.8 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | deepx-B650MT |
| OS | Ubuntu 22.04.5 LTS |
| Kernel | 6.8.0-107-generic |
| CPU | AMD Ryzen 5 9600X 6-Core Processor |
| CPU Cores | 12 |
| RAM | 30.5 GB |
| NPU SKU | H1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | H1, Rev 0.0 |
| NPU PCIe | Gen3 X4 [04:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.3.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.20.3 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.20.3 |
| dxtop | Yes | DX-TOP 1.1.0 |
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
| yolo26n.dxnn | 927.9 ±1.2 | 249 | 93.0 | 100.0 | 46~49 | 1000 | ok |
| yolo26s.dxnn | 536.4 ±0.0 | 132 | 91.8 | 100.0 | 55~57 | 1000 | ok |
| yolo26m.dxnn | 371.3 ±0.4 | 89 | 90.7 | 100.0 | 59~62 | 1000 | ok |
| yolo26l.dxnn | 273.1 ±0.1 | 66 | 91.6 | 100.0 | 63~66 | 1000 | ok |
| yolo26x.dxnn | 156.8 ±0.4 | 39 | 91.1 | 100.0 | 63~65 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 929.3 ±0.3 | 166 | 92.7 | 100.0 | 54~56 | 1000 | ok |
| yolo26s.dxnn | 536.6 ±0.4 | 91 | 93.0 | 100.0 | 59~60 | 1000 | ok |
| yolo26m.dxnn | 372.5 ±0.1 | 61 | 92.3 | 100.0 | 63~65 | 1000 | ok |
| yolo26l.dxnn | 273.3 ±0.1 | 45 | 92.0 | 100.0 | 62~65 | 1000 | ok |
| yolo26x.dxnn | 156.2 ±0.1 | 26 | 92.5 | 100.0 | 61~64 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 889.6 ±0.4 | 157 | 92.6 | 100.0 | 60~61 | 1000 | ok |
| yolo26s-pose.dxnn | 516.7 ±0.3 | 86 | 91.6 | 100.0 | 57~59 | 1000 | ok |
| yolo26m-pose.dxnn | 359.2 ±0.6 | 59 | 91.8 | 100.0 | 60~63 | 1000 | ok |
| yolo26l-pose.dxnn | 265.8 ±0.3 | 45 | 90.7 | 100.0 | 62~65 | 1000 | ok |
| yolo26x-pose.dxnn | 152.9 ±0.3 | 27 | 90.8 | 100.0 | 61~64 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 888.8 ±0.8 | 99 | 92.2 | 100.0 | 57~59 | 1000 | ok |
| yolo26s-pose.dxnn | 516.3 ±0.1 | 55 | 92.3 | 100.0 | 60~61 | 1000 | ok |
| yolo26m-pose.dxnn | 361.1 ±1.0 | 38 | 89.9 | 100.0 | 63~65 | 1000 | ok |
| yolo26l-pose.dxnn | 265.8 ±0.3 | 29 | 90.3 | 100.0 | 61~63 | 1000 | ok |
| yolo26x-pose.dxnn | 152.7 ±1.0 | 17 | 89.7 | 100.0 | 60~63 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 573.1 ±3.4 | 519 | 56.5 | 88.3 | 57~58 | 1000 | ok |
| yolo26s-seg.dxnn | 419.7 ±0.2 | 356 | 91.7 | 100.0 | 56~59 | 1000 | ok |
| yolo26m-seg.dxnn | 266.0 ±0.6 | 213 | 91.5 | 100.0 | 61~65 | 1000 | ok |
| yolo26l-seg.dxnn | 209.7 ±0.4 | 167 | 93.1 | 100.0 | 64~66 | 1000 | ok |
| yolo26x-seg.dxnn | 118.4 ±1.2 | 90 | 89.5 | 100.0 | 62~65 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 609.7 ±5.3 | 434 | 61.4 | 90.3 | 55~57 | 1000 | ok |
| yolo26s-seg.dxnn | 420.0 ±0.4 | 290 | 92.8 | 100.0 | 60~62 | 1000 | ok |
| yolo26m-seg.dxnn | 265.2 ±0.8 | 177 | 90.5 | 100.0 | 64~67 | 1000 | ok |
| yolo26l-seg.dxnn | 209.7 ±0.2 | 140 | 90.4 | 100.0 | 63~66 | 1000 | ok |
| yolo26x-seg.dxnn | 118.6 ±0.0 | 76 | 89.5 | 100.0 | 60~63 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 308.2 ±0.2 | 54 | 92.0 | 100.0 | 56~57 | 1000 | ok |
| yolo26s-obb.dxnn | 178.5 ±0.1 | 33 | 91.4 | 100.0 | 54~55 | 1000 | ok |
| yolo26m-obb.dxnn | 130.8 ±0.2 | 24 | 88.8 | 100.0 | 55~58 | 1000 | ok |
| yolo26l-obb.dxnn | 95.5 ±0.1 | 20 | 88.6 | 100.0 | 56~58 | 1000 | ok |
| yolo26x-obb.dxnn | 55.1 ±0.1 | 13 | 87.4 | 100.0 | 56~58 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 307.6 ±0.3 | 37 | 92.0 | 100.0 | 53~54 | 1000 | ok |
| yolo26s-obb.dxnn | 178.5 ±0.0 | 22 | 92.6 | 100.0 | 54~55 | 1000 | ok |
| yolo26m-obb.dxnn | 130.7 ±0.2 | 16 | 92.3 | 100.0 | 56~59 | 1000 | ok |
| yolo26l-obb.dxnn | 95.2 ±0.3 | 14 | 91.4 | 100.0 | 55~57 | 1000 | ok |
| yolo26x-obb.dxnn | 55.2 ±0.1 | 8 | 86.8 | 100.0 | 55~58 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 14005.5 ±3.6 | 64 | 88.6 | 96.5 | 53 | 1000 | ok |
| yolo26s-cls.dxnn | 7645.6 ±1.6 | 36 | 89.2 | 98.5 | 54~55 | 1000 | ok |
| yolo26m-cls.dxnn | 5396.1 ±1.8 | 25 | 91.0 | 99.0 | 56~58 | 1000 | ok |
| yolo26l-cls.dxnn | 3382.8 ±1.5 | 16 | 90.0 | 99.4 | 55~56 | 1000 | ok |
| yolo26x-cls.dxnn | 1818.3 ±2.8 | 9 | 91.0 | 100.0 | 56~58 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 14000.5 ±11.9 | 64 | 88.8 | 97.1 | 53 | 1000 | ok |
| yolo26s-cls.dxnn | 7644.1 ±3.4 | 36 | 89.8 | 98.4 | 54~55 | 1000 | ok |
| yolo26m-cls.dxnn | 5394.5 ±5.5 | 25 | 89.9 | 99.0 | 56~58 | 1000 | ok |
| yolo26l-cls.dxnn | 3384.3 ±3.5 | 16 | 90.8 | 99.5 | 55~57 | 1000 | ok |
| yolo26x-cls.dxnn | 1819.4 ±1.0 | 9 | 91.7 | 99.9 | 56~58 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 85.2 | 11.74 | 11.48 | 0.26 | 42 | ok |
| yolo26s.dxnn | 52.5 | 19.07 | 18.75 | 0.32 | 52 | ok |
| yolo26m.dxnn | 38.2 | 26.18 | 25.86 | 0.32 | 55 | ok |
| yolo26l.dxnn | 29.0 | 34.45 | 34.10 | 0.35 | 60 | ok |
| yolo26x.dxnn | 16.2 | 61.72 | 61.37 | 0.35 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 85.4 | 11.71 | 11.71 | 52 | ok |
| yolo26s.dxnn | 53.2 | 18.79 | 18.79 | 56 | ok |
| yolo26m.dxnn | 38.4 | 26.06 | 26.06 | 59 | ok |
| yolo26l.dxnn | 29.3 | 34.12 | 34.12 | 58 | ok |
| yolo26x.dxnn | 16.3 | 61.36 | 61.36 | 57 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 85.8 | 11.66 | 11.41 | 0.25 | 59 | ok |
| yolo26s-pose.dxnn | 53.1 | 18.84 | 18.57 | 0.27 | 54 | ok |
| yolo26m-pose.dxnn | 37.9 | 26.37 | 26.09 | 0.28 | 55 | ok |
| yolo26l-pose.dxnn | 28.9 | 34.56 | 34.27 | 0.28 | 59 | ok |
| yolo26x-pose.dxnn | 16.1 | 62.05 | 61.76 | 0.28 | 56 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 86.0 | 11.63 | 11.63 | 55 | ok |
| yolo26s-pose.dxnn | 54.0 | 18.51 | 18.51 | 57 | ok |
| yolo26m-pose.dxnn | 38.7 | 25.86 | 25.86 | 59 | ok |
| yolo26l-pose.dxnn | 29.3 | 34.10 | 34.10 | 57 | ok |
| yolo26x-pose.dxnn | 16.2 | 61.70 | 61.70 | 56 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 50.8 | 19.68 | 19.29 | 0.39 | 56 | ok |
| yolo26s-seg.dxnn | 35.4 | 28.25 | 27.88 | 0.37 | 52 | ok |
| yolo26m-seg.dxnn | 23.6 | 42.31 | 41.91 | 0.40 | 55 | ok |
| yolo26l-seg.dxnn | 19.8 | 50.42 | 50.01 | 0.40 | 59 | ok |
| yolo26x-seg.dxnn | 11.2 | 89.47 | 89.06 | 0.40 | 57 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 51.9 | 19.29 | 19.29 | 53 | ok |
| yolo26s-seg.dxnn | 35.9 | 27.84 | 27.84 | 56 | ok |
| yolo26m-seg.dxnn | 24.0 | 41.70 | 41.70 | 60 | ok |
| yolo26l-seg.dxnn | 19.9 | 50.17 | 50.17 | 59 | ok |
| yolo26x-seg.dxnn | 11.2 | 89.06 | 89.06 | 54 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 38.9 | 25.68 | 25.40 | 0.28 | 55 | ok |
| yolo26s-obb.dxnn | 23.1 | 43.24 | 42.96 | 0.28 | 51 | ok |
| yolo26m-obb.dxnn | 16.5 | 60.79 | 60.53 | 0.26 | 51 | ok |
| yolo26l-obb.dxnn | 12.1 | 82.57 | 82.25 | 0.32 | 52 | ok |
| yolo26x-obb.dxnn | 6.5 | 154.07 | 153.73 | 0.33 | 51 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 39.6 | 25.26 | 25.26 | 51 | ok |
| yolo26s-obb.dxnn | 23.3 | 42.91 | 42.91 | 51 | ok |
| yolo26m-obb.dxnn | 16.5 | 60.52 | 60.52 | 52 | ok |
| yolo26l-obb.dxnn | 12.2 | 81.86 | 81.86 | 51 | ok |
| yolo26x-obb.dxnn | 6.5 | 153.67 | 153.67 | 51 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 1010.5 | 0.99 | 0.99 | N/A | 52 | ok |
| yolo26s-cls.dxnn | 603.3 | 1.66 | 1.66 | N/A | 52 | ok |
| yolo26m-cls.dxnn | 435.4 | 2.30 | 2.30 | N/A | 52 | ok |
| yolo26l-cls.dxnn | 276.0 | 3.62 | 3.62 | N/A | 52 | ok |
| yolo26x-cls.dxnn | 157.7 | 6.34 | 6.34 | N/A | 52 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 987.2 | 1.01 | 1.01 | 51 | ok |
| yolo26s-cls.dxnn | 566.9 | 1.76 | 1.76 | 52 | ok |
| yolo26m-cls.dxnn | 434.1 | 2.30 | 2.30 | 51 | ok |
| yolo26l-cls.dxnn | 273.2 | 3.66 | 3.66 | 52 | ok |
| yolo26x-cls.dxnn | 158.8 | 6.30 | 6.30 | 52 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vaapidecodebin | 3455 | 3 | 498.3 ±0.9 | 6.93 | 219 | 26.4 | 63.5 | 49 | 1000 | 307 | ok |
| yolo26s.dxnn | vaapidecodebin | 3455 | 3 | 497.5 ±0.6 | 6.95 | 222 | 65.6 | 87.5 | 57 | 1000 | 394 | ok |
| yolo26m.dxnn | vaapidecodebin | 3455 | 3 | 372.4 ±0.6 | 9.28 | 150 | 80.1 | 100.0 | 63 | 1000 | 512 | ok |
| yolo26l.dxnn | vaapidecodebin | 3455 | 3 | 275.3 ±0.8 | 12.55 | 104 | 82.2 | 100.0 | 66~67 | 1000 | 527 | ok |
| yolo26x.dxnn | vaapidecodebin | 3455 | 3 | 159.0 ±0.1 | 21.73 | 53 | 86.5 | 100.0 | 67~68 | 1000 | 573 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | vaapidecodebin | 3455 | 3 | 463.1 ±0.7 | 7.46 | 245 | 25.6 | 74.5 | 55 | 1000 | 431 | ok |
| yolo26s.dxnn | vaapidecodebin | 3455 | 3 | 458.1 ±0.4 | 7.54 | 245 | 53.0 | 77.5 | 60 | 1000 | 493 | ok |
| yolo26m.dxnn | vaapidecodebin | 3455 | 3 | 372.9 ±0.4 | 9.27 | 194 | 80.8 | 100.0 | 66 | 1000 | 583 | ok |
| yolo26l.dxnn | vaapidecodebin | 3455 | 3 | 274.9 ±0.4 | 12.57 | 134 | 82.8 | 100.0 | 65 | 1000 | 594 | ok |
| yolo26x.dxnn | vaapidecodebin | 3455 | 3 | 158.4 ±0.4 | 21.81 | 72 | 87.4 | 100.0 | 65~67 | 1000 | 634 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 498.3 | 463.1 | +35.2 | +7.6% |
| yolo26s.dxnn | 497.5 | 458.1 | +39.4 | +8.6% |
| yolo26m.dxnn | 372.4 | 372.9 | -0.5 | -0.1% |
| yolo26l.dxnn | 275.3 | 274.9 | +0.4 | +0.1% |
| yolo26x.dxnn | 159.0 | 158.4 | +0.6 | +0.4% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vaapidecodebin | 3455 | 3 | 542.7 ±0.6 | 6.37 | 173 | 30.3 | 68.8 | 60 | 1000 | 300 | ok |
| yolo26s-pose.dxnn | vaapidecodebin | 3455 | 3 | 516.1 ±0.3 | 6.70 | 173 | 70.5 | 100.0 | 59~60 | 1000 | 430 | ok |
| yolo26m-pose.dxnn | vaapidecodebin | 3455 | 3 | 364.5 ±0.6 | 9.48 | 104 | 79.9 | 100.0 | 63~64 | 1000 | 461 | ok |
| yolo26l-pose.dxnn | vaapidecodebin | 3455 | 3 | 269.2 ±0.6 | 12.83 | 74 | 83.6 | 100.0 | 65 | 1000 | 474 | ok |
| yolo26x-pose.dxnn | vaapidecodebin | 3455 | 3 | 155.6 ±0.4 | 22.21 | 40 | 87.1 | 100.0 | 65~66 | 1000 | 526 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | vaapidecodebin | 3455 | 3 | 557.8 ±1.7 | 6.19 | 133 | 31.3 | 70.9 | 58 | 1000 | 288 | ok |
| yolo26s-pose.dxnn | vaapidecodebin | 3455 | 3 | 515.0 ±1.4 | 6.71 | 130 | 72.9 | 100.0 | 61 | 1000 | 458 | ok |
| yolo26m-pose.dxnn | vaapidecodebin | 3455 | 3 | 363.5 ±1.6 | 9.51 | 80 | 76.7 | 100.0 | 65~66 | 1000 | 490 | ok |
| yolo26l-pose.dxnn | vaapidecodebin | 3455 | 3 | 269.0 ±0.2 | 12.84 | 57 | 83.5 | 100.0 | 64 | 1000 | 505 | ok |
| yolo26x-pose.dxnn | vaapidecodebin | 3455 | 3 | 155.4 ±0.2 | 22.23 | 31 | 88.2 | 100.0 | 65~66 | 1000 | 545 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 542.7 | 557.8 | -15.1 | -2.7% |
| yolo26s-pose.dxnn | 516.1 | 515.0 | +1.1 | +0.2% |
| yolo26m-pose.dxnn | 364.5 | 363.5 | +1.0 | +0.3% |
| yolo26l-pose.dxnn | 269.2 | 269.0 | +0.2 | +0.1% |
| yolo26x-pose.dxnn | 155.6 | 155.4 | +0.2 | +0.1% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vaapidecodebin | 3455 | 3 | 363.0 ±0.3 | 9.52 | 539 | 24.9 | 76.9 | 58 | 1000 | 555 | ok |
| yolo26s-seg.dxnn | vaapidecodebin | 3455 | 3 | 360.5 ±1.9 | 9.58 | 537 | 57.9 | 79.9 | 59 | 1000 | 747 | ok |
| yolo26m-seg.dxnn | vaapidecodebin | 3455 | 3 | 265.7 ±0.3 | 13.01 | 303 | 82.0 | 100.0 | 66~67 | 1000 | 812 | ok |
| yolo26l-seg.dxnn | vaapidecodebin | 3455 | 3 | 211.5 ±0.9 | 16.34 | 228 | 83.7 | 100.0 | 67~68 | 1000 | 826 | ok |
| yolo26x-seg.dxnn | vaapidecodebin | 3455 | 3 | 119.9 ±0.5 | 28.82 | 121 | 90.5 | 100.0 | 68~70 | 1000 | 876 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | vaapidecodebin | 3455 | 3 | 323.0 ±0.2 | 10.70 | 431 | 22.5 | 83.0 | 56 | 1000 | 667 | ok |
| yolo26s-seg.dxnn | vaapidecodebin | 3455 | 3 | 317.0 ±2.0 | 10.90 | 420 | 47.8 | 88.9 | 61~62 | 1000 | 782 | ok |
| yolo26m-seg.dxnn | vaapidecodebin | 3455 | 3 | 265.0 ±1.5 | 13.04 | 329 | 78.1 | 100.0 | 68~69 | 1000 | 910 | ok |
| yolo26l-seg.dxnn | vaapidecodebin | 3455 | 3 | 211.8 ±0.4 | 16.31 | 247 | 82.6 | 100.0 | 67~68 | 1000 | 926 | ok |
| yolo26x-seg.dxnn | vaapidecodebin | 3455 | 3 | 119.8 ±0.6 | 28.83 | 131 | 88.7 | 100.0 | 66~68 | 1000 | 972 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 363.0 | 323.0 | +40.0 | +12.4% |
| yolo26s-seg.dxnn | 360.5 | 317.0 | +43.6 | +13.7% |
| yolo26m-seg.dxnn | 265.7 | 265.0 | +0.6 | +0.2% |
| yolo26l-seg.dxnn | 211.5 | 211.8 | -0.3 | -0.1% |
| yolo26x-seg.dxnn | 119.9 | 119.8 | +0.0 | +0.0% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vaapidecodebin | 2640 | 3 | 307.0 ±1.1 | 8.60 | 117 | 76.6 | 100.0 | 56 | 1000 | 478 | ok |
| yolo26s-obb.dxnn | vaapidecodebin | 2640 | 3 | 178.0 ±0.1 | 14.83 | 62 | 84.1 | 100.0 | 55~56 | 1000 | 497 | ok |
| yolo26m-obb.dxnn | vaapidecodebin | 2640 | 3 | 130.9 ±0.1 | 20.16 | 46 | 88.8 | 100.0 | 60~61 | 1000 | 529 | ok |
| yolo26l-obb.dxnn | vaapidecodebin | 2640 | 3 | 95.9 ±0.0 | 27.54 | 34 | 90.8 | 100.0 | 60~61 | 1000 | 549 | ok |
| yolo26x-obb.dxnn | vaapidecodebin | 2640 | 3 | 55.3 ±0.1 | 47.75 | 20 | 88.9 | 100.0 | 62~65 | 1000 | 597 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | vaapidecodebin | 2640 | 3 | 307.9 ±0.4 | 8.57 | 121 | 78.3 | 100.0 | 54 | 1000 | 506 | ok |
| yolo26s-obb.dxnn | vaapidecodebin | 2640 | 3 | 177.8 ±0.4 | 14.85 | 65 | 83.0 | 100.0 | 55~56 | 1000 | 528 | ok |
| yolo26m-obb.dxnn | vaapidecodebin | 2640 | 3 | 130.8 ±0.1 | 20.18 | 48 | 86.0 | 100.0 | 60~61 | 1000 | 558 | ok |
| yolo26l-obb.dxnn | vaapidecodebin | 2640 | 3 | 95.7 ±0.1 | 27.58 | 36 | 89.2 | 100.0 | 59~61 | 1000 | 575 | ok |
| yolo26x-obb.dxnn | vaapidecodebin | 2640 | 3 | 55.3 ±0.1 | 47.74 | 21 | 88.5 | 100.0 | 62~65 | 1000 | 624 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 307.0 | 307.9 | -0.9 | -0.3% |
| yolo26s-obb.dxnn | 178.0 | 177.8 | +0.2 | +0.1% |
| yolo26m-obb.dxnn | 130.9 | 130.8 | +0.1 | +0.1% |
| yolo26l-obb.dxnn | 95.9 | 95.7 | +0.2 | +0.2% |
| yolo26x-obb.dxnn | 55.3 | 55.3 | -0.0 | -0.0% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vaapidecodebin | 3455 | 3 | 784.9 ±2.8 | 4.40 | 41 | 3.0 | 15.6 | 52 | 1000 | 165 | ok |
| yolo26s-cls.dxnn | vaapidecodebin | 3455 | 3 | 780.2 ±3.8 | 4.43 | 40 | 4.9 | 28.7 | 54 | 1000 | 181 | ok |
| yolo26m-cls.dxnn | vaapidecodebin | 3455 | 3 | 776.2 ±8.9 | 4.45 | 42 | 8.3 | 40.7 | 57 | 1000 | 202 | ok |
| yolo26l-cls.dxnn | vaapidecodebin | 3455 | 3 | 761.5 ±4.9 | 4.54 | 43 | 14.1 | 64.3 | 55 | 1000 | 213 | ok |
| yolo26x-cls.dxnn | vaapidecodebin | 3455 | 3 | 741.8 ±5.7 | 4.66 | 44 | 24.5 | 59.6 | 56~57 | 1000 | 306 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | vaapidecodebin | 3455 | 3 | 782.9 ±1.2 | 4.41 | 41 | 2.5 | 15.6 | 52~53 | 1000 | 169 | ok |
| yolo26s-cls.dxnn | vaapidecodebin | 3455 | 3 | 777.8 ±5.2 | 4.44 | 41 | 6.0 | 28.5 | 54 | 1000 | 187 | ok |
| yolo26m-cls.dxnn | vaapidecodebin | 3455 | 3 | 775.0 ±2.9 | 4.46 | 41 | 8.7 | 40.3 | 56~57 | 1000 | 197 | ok |
| yolo26l-cls.dxnn | vaapidecodebin | 3455 | 3 | 764.2 ±6.6 | 4.52 | 42 | 13.7 | 64.0 | 55 | 1000 | 212 | ok |
| yolo26x-cls.dxnn | vaapidecodebin | 3455 | 3 | 740.8 ±15.3 | 4.66 | 44 | 23.0 | 59.1 | 57 | 1000 | 309 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 784.9 | 782.9 | +2.0 | +0.3% |
| yolo26s-cls.dxnn | 780.2 | 777.8 | +2.5 | +0.3% |
| yolo26m-cls.dxnn | 776.2 | 775.0 | +1.2 | +0.2% |
| yolo26l-cls.dxnn | 761.5 | 764.2 | -2.8 | -0.4% |
| yolo26x-cls.dxnn | 741.8 | 740.8 | +1.1 | +0.1% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 16 | 3 | 540.5 ±0.7 | 33.8 | 312 | 39.2 | 78.3 | 54~57 | 1000 | 671 | ok |
| yolo26n.dxnn | 17 | 2/3 | 540.1 ±0.7 | 31.8 | 314 | 38.9 | 75.3 | 59~60 | 1000 | 685 | partial |
| yolo26s.dxnn | 16 | 3 | 532.3 ±0.3 | 33.3 | 317 | 93.7 | 100.0 | 64~68 | 1000 | 745 | ok |
| yolo26s.dxnn | 17 | 2/3 | 532.1 ±0.1 | 31.3 | 316 | 94.2 | 100.0 | 70~71 | 1000 | 764 | partial |
| yolo26m.dxnn | 12 | 3 | 373.4 ±0.1 | 31.1 | 175 | 96.1 | 100.0 | 71~76 | 1000 | 722 | ok |
| yolo26m.dxnn | 13 | 3 | 373.8 ±0.2 | 28.8 | 176 | 96.2 | 100.0 | 79~81 | 1000 | 745 | ok |
| yolo26l.dxnn | 9 | 3 | 276.2 ±0.1 | 30.7 | 115 | 96.4 | 100.0 | 73~76 | 1000 | 686 | ok |
| yolo26l.dxnn | 10 | 3 | 276.0 ±0.3 | 27.6 | 116 | 96.5 | 100.0 | 78~79 | 1000 | 681 | ok |
| yolo26x.dxnn | 5 | 3 | 158.3 ±0.6 | 31.6 | 57 | 95.8 | 100.0 | 74~78 | 1000 | 650 | ok |
| yolo26x.dxnn | 6 | 3 | 158.5 ±0.2 | 26.4 | 58 | 96.2 | 100.0 | 79~80 | 1000 | 665 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 15 | 3 | 454.6 ±0.8 | 30.3 | 259 | 31.0 | 88.3 | 57~58 | 1000 | 771 | ok |
| yolo26n.dxnn | 16 | 3 | 451.9 ±0.8 | 28.2 | 260 | 30.7 | 91.4 | 59~60 | 1000 | 791 | ok |
| yolo26s.dxnn | 15 | 3 | 449.8 ±1.7 | 30.0 | 262 | 67.5 | 82.6 | 64~66 | 1000 | 826 | ok |
| yolo26s.dxnn | 14 | 3 | 448.7 ±0.8 | 32.0 | 262 | 66.8 | 83.0 | 67~68 | 1000 | 811 | ok |
| yolo26m.dxnn | 12 | 3 | 374.2 ±0.3 | 31.2 | 222 | 96.5 | 100.0 | 74~78 | 1000 | 812 | ok |
| yolo26m.dxnn | 13 | 3 | 374.4 ±0.3 | 28.8 | 222 | 96.5 | 100.0 | 81~82 | 1000 | 831 | ok |
| yolo26l.dxnn | 9 | 3 | 276.0 ±0.1 | 30.7 | 148 | 96.8 | 100.0 | 72~76 | 1000 | 767 | ok |
| yolo26l.dxnn | 10 | 3 | 275.9 ±0.3 | 27.6 | 148 | 96.9 | 100.0 | 78~80 | 1000 | 799 | ok |
| yolo26x.dxnn | 5 | 3 | 158.4 ±0.4 | 31.7 | 76 | 95.8 | 100.0 | 74~77 | 1000 | 730 | ok |
| yolo26x.dxnn | 6 | 3 | 158.1 ±0.9 | 26.4 | 77 | 96.7 | 100.0 | 80~81 | 800~1000 | 753 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 17+ | 31.8 | 15 | 30.3 |
| yolo26s.dxnn | 17+ | 31.3 | 14 | 32.0 |
| yolo26m.dxnn | 12 | 31.1 | 12 | 31.2 |
| yolo26l.dxnn | 9 | 30.7 | 9 | 30.7 |
| yolo26x.dxnn | 5 | 31.6 | 5 | 31.7 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 18 | 3 | 598.2 ±2.2 | 33.2 | 244 | 49.0 | 74.5 | 61~62 | 1000 | 705 | ok |
| yolo26n-pose.dxnn | 19 | 3 | 599.0 ±0.7 | 31.5 | 245 | 49.1 | 75.1 | 58~62 | 1000 | 711 | ok |
| yolo26n-pose.dxnn | 20 | 3 | 596.8 ±1.2 | 29.8 | 246 | 48.8 | 74.4 | 63~65 | 1000 | 740 | ok |
| yolo26s-pose.dxnn | 17 | 3 | 517.2 ±0.0 | 30.4 | 206 | 95.9 | 100.0 | 66~69 | 1000 | 723 | ok |
| yolo26s-pose.dxnn | 18 | 3 | 517.7 ±1.0 | 28.8 | 208 | 96.4 | 100.0 | 72~73 | 1000 | 745 | ok |
| yolo26m-pose.dxnn | 12 | 3 | 363.4 ±0.5 | 30.3 | 121 | 96.3 | 100.0 | 72~77 | 1000 | 676 | ok |
| yolo26m-pose.dxnn | 13 | 3 | 364.1 ±0.6 | 28.0 | 122 | 96.7 | 100.0 | 79~81 | 1000 | 695 | ok |
| yolo26l-pose.dxnn | 8 | 3 | 269.4 ±0.3 | 33.7 | 84 | 96.1 | 100.0 | 72~75 | 1000 | 613 | ok |
| yolo26l-pose.dxnn | 9 | 3 | 269.4 ±0.3 | 29.9 | 84 | 96.4 | 100.0 | 77~78 | 1000 | 642 | ok |
| yolo26x-pose.dxnn | 5 | 3 | 155.7 ±0.3 | 31.1 | 43 | 97.0 | 100.0 | 73~77 | 1000 | 608 | ok |
| yolo26x-pose.dxnn | 6 | 3 | 155.5 ±0.2 | 25.9 | 44 | 97.0 | 100.0 | 78~79 | 1000 | 626 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 18 | 2/3 | 615.4 ±0.1 | 34.2 | 194 | 50.4 | 76.4 | 59~60 | 1000 | 729 | partial |
| yolo26n-pose.dxnn | 17 | 2/3 | 613.8 ±0.1 | 36.1 | 193 | 50.3 | 76.3 | 62~63 | 1000 | 696 | partial |
| yolo26n-pose.dxnn | 16 | 3 | 613.9 ±1.2 | 38.4 | 192 | 50.7 | 76.7 | 64 | 1000 | 698 | ok |
| yolo26s-pose.dxnn | 17 | 3 | 517.9 ±0.9 | 30.5 | 161 | 96.4 | 100.0 | 66~69 | 1000 | 767 | ok |
| yolo26s-pose.dxnn | 18 | 0/3 | 0.0 | 0.0 | 0 | 0.0 | 0.0 | N/A | — | 0 | error |
| yolo26m-pose.dxnn | 12 | 3 | 363.6 ±0.4 | 30.3 | 95 | 96.4 | 100.0 | 73~77 | 1000 | 724 | ok |
| yolo26m-pose.dxnn | 13 | 3 | 363.6 ±0.3 | 28.0 | 97 | 96.5 | 100.0 | 79~81 | 1000 | 746 | ok |
| yolo26l-pose.dxnn | 8 | 3 | 268.9 ±0.1 | 33.6 | 65 | 96.5 | 100.0 | 71~74 | 1000 | 653 | ok |
| yolo26l-pose.dxnn | 9 | 3 | 269.2 ±0.4 | 29.9 | 66 | 96.6 | 100.0 | 76~77 | 1000 | 679 | ok |
| yolo26x-pose.dxnn | 5 | 3 | 155.7 ±0.3 | 31.1 | 34 | 96.8 | 100.0 | 73~76 | 1000 | 633 | ok |
| yolo26x-pose.dxnn | 6 | 3 | 155.6 ±0.2 | 25.9 | 34 | 97.0 | 100.0 | 78~79 | 1000 | 652 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 19 | 31.5 | 18+ | 34.2 |
| yolo26s-pose.dxnn | 17 | 30.4 | 17+ | 30.5 |
| yolo26m-pose.dxnn | 12 | 30.3 | 12 | 30.3 |
| yolo26l-pose.dxnn | 8 | 33.7 | 8 | 33.6 |
| yolo26x-pose.dxnn | 5 | 31.1 | 5 | 31.1 |

> **+** 표시: 마지막 측정 스트림에서도 기준 FPS를 만족함. 스위프가 FPS 임계값 미달전에 중단된 경우로, 실제 최대 처리 가능 스트림 수는 더 클 수 있음.

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 12 | 3 | 371.6 ±1.4 | 31.0 | 639 | 32.4 | 76.4 | 59~61 | 1000 | 1017 | ok |
| yolo26n-seg.dxnn | 13 | 3 | 371.4 ±0.7 | 28.6 | 644 | 32.4 | 79.4 | 61~62 | 1000 | 1051 | ok |
| yolo26s-seg.dxnn | 12 | 3 | 366.6 ±1.9 | 30.6 | 633 | 74.4 | 90.4 | 66~69 | 1000 | 1043 | ok |
| yolo26s-seg.dxnn | 13 | 3 | 367.7 ±1.0 | 28.3 | 639 | 74.8 | 89.7 | 72 | 1000 | 1068 | ok |
| yolo26m-seg.dxnn | 8 | 3 | 266.9 ±0.4 | 33.4 | 329 | 96.0 | 100.0 | 76~82 | 800~1000 | 980 | ok |
| yolo26m-seg.dxnn | 9 | 3 | 249.2 ±2.5 | 27.7 | 302 | 95.3 | 100.0 | 84 | 600~1000 | 1014 | ok |
| yolo26l-seg.dxnn | 7 | 3 | 212.1 ±0.2 | 30.3 | 239 | 96.4 | 100.0 | 76~80 | 800~1000 | 985 | ok |
| yolo26l-seg.dxnn | 8 | 3 | 208.7 ±0.7 | 26.1 | 238 | 96.0 | 100.0 | 82~83 | 800~1000 | 999 | ok |
| yolo26x-seg.dxnn | 3 | 3 | 120.6 ±0.1 | 40.2 | 123 | 95.0 | 100.0 | 76~79 | 1000 | 938 | ok |
| yolo26x-seg.dxnn | 4 | 3 | 116.1 ±0.6 | 29.0 | 121 | 94.8 | 100.0 | 81 | 600~1000 | 967 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 10 | 3 | 317.2 ±0.8 | 31.7 | 440 | 26.1 | 83.0 | 58~59 | 1000 | 998 | ok |
| yolo26n-seg.dxnn | 11 | 3 | 315.7 ±0.7 | 28.7 | 438 | 25.8 | 83.9 | 60 | 1000 | 1033 | ok |
| yolo26s-seg.dxnn | 10 | 3 | 310.4 ±0.2 | 31.0 | 433 | 54.5 | 87.5 | 65~67 | 1000 | 1063 | ok |
| yolo26s-seg.dxnn | 11 | 3 | 309.1 ±0.7 | 28.1 | 432 | 54.1 | 87.4 | 68~69 | 1000 | 1150 | ok |
| yolo26m-seg.dxnn | 8 | 3 | 265.8 ±1.1 | 33.2 | 355 | 95.4 | 100.0 | 77~82 | 800~1000 | 1114 | ok |
| yolo26m-seg.dxnn | 9 | 3 | 246.1 ±8.8 | 27.4 | 324 | 93.6 | 100.0 | 83~84 | 400~1000 | 1120 | ok |
| yolo26l-seg.dxnn | 7 | 3 | 212.2 ±0.0 | 30.3 | 260 | 96.3 | 100.0 | 76~79 | 1000 | 1084 | ok |
| yolo26l-seg.dxnn | 8 | 3 | 211.7 ±0.4 | 26.5 | 262 | 96.4 | 100.0 | 82 | 800~1000 | 1123 | ok |
| yolo26x-seg.dxnn | 3 | 3 | 120.4 ±0.5 | 40.1 | 134 | 95.2 | 100.0 | 74~78 | 1000 | 1045 | ok |
| yolo26x-seg.dxnn | 4 | 3 | 117.8 ±2.2 | 29.4 | 133 | 95.1 | 100.0 | 80~81 | 600~1000 | 1072 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 12 | 31.0 | 10 | 31.7 |
| yolo26s-seg.dxnn | 12 | 30.6 | 10 | 31.0 |
| yolo26m-seg.dxnn | 8 | 33.4 | 8 | 33.2 |
| yolo26l-seg.dxnn | 7 | 30.3 | 7 | 30.3 |
| yolo26x-seg.dxnn | 3 | 40.2 | 3 | 40.1 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 10 | 3 | 308.6 ±0.5 | 30.9 | 132 | 95.7 | 100.0 | 58~60 | 1000 | 675 | ok |
| yolo26n-obb.dxnn | 11 | 3 | 309.0 ±0.3 | 28.1 | 134 | 95.8 | 100.0 | 61~62 | 1000 | 697 | ok |
| yolo26s-obb.dxnn | 5 | 3 | 178.6 ±0.1 | 35.7 | 67 | 96.0 | 100.0 | 59~61 | 1000 | 589 | ok |
| yolo26s-obb.dxnn | 6 | 3 | 178.8 ±0.1 | 29.8 | 68 | 95.8 | 100.0 | 63~64 | 1000 | 612 | ok |
| yolo26m-obb.dxnn | 4 | 3 | 131.1 ±0.2 | 32.8 | 49 | 95.5 | 100.0 | 66~69 | 1000 | 611 | ok |
| yolo26m-obb.dxnn | 5 | 3 | 131.2 ±0.1 | 26.2 | 48 | 96.5 | 100.0 | 71~72 | 1000 | 624 | ok |
| yolo26l-obb.dxnn | 3 | 3 | 96.1 ±0.1 | 32.0 | 35 | 96.2 | 100.0 | 65~67 | 1000 | 603 | ok |
| yolo26l-obb.dxnn | 4 | 3 | 96.0 ±0.1 | 24.0 | 36 | 96.6 | 100.0 | 69~70 | 1000 | 624 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 55.3 ±0.1 | 55.3 | 20 | 88.9 | 100.0 | 62~65 | 1000 | 597 | ok |
| yolo26x-obb.dxnn | 2 | 3 | 55.4 ±0.0 | 27.7 | 21 | 91.6 | 100.0 | 69~71 | 1000 | 630 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 10 | 3 | 308.6 ±0.3 | 30.9 | 137 | 95.6 | 100.0 | 57~59 | 1000 | 703 | ok |
| yolo26n-obb.dxnn | 11 | 3 | 308.4 ±0.2 | 28.0 | 137 | 95.5 | 100.0 | 61~62 | 1000 | 728 | ok |
| yolo26s-obb.dxnn | 5 | 3 | 178.6 ±0.1 | 35.7 | 71 | 95.7 | 100.0 | 59~61 | 1000 | 620 | ok |
| yolo26s-obb.dxnn | 6 | 3 | 178.7 ±0.1 | 29.8 | 71 | 95.9 | 100.0 | 62~64 | 1000 | 645 | ok |
| yolo26m-obb.dxnn | 4 | 3 | 131.2 ±0.1 | 32.8 | 51 | 95.6 | 100.0 | 66~69 | 1000 | 638 | ok |
| yolo26m-obb.dxnn | 5 | 3 | 131.2 ±0.1 | 26.2 | 51 | 96.5 | 100.0 | 71~72 | 1000 | 654 | ok |
| yolo26l-obb.dxnn | 3 | 3 | 96.0 ±0.0 | 32.0 | 37 | 95.5 | 100.0 | 65~67 | 1000 | 625 | ok |
| yolo26l-obb.dxnn | 4 | 3 | 96.0 ±0.1 | 24.0 | 37 | 96.8 | 100.0 | 69~70 | 1000 | 655 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 55.3 ±0.1 | 55.3 | 21 | 88.5 | 100.0 | 62~65 | 1000 | 624 | ok |
| yolo26x-obb.dxnn | 2 | 3 | 55.4 ±0.0 | 27.7 | 22 | 91.6 | 100.0 | 69~71 | 1000 | 652 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 10 | 30.9 | 10 | 30.9 |
| yolo26s-obb.dxnn | 5 | 35.7 | 5 | 35.7 |
| yolo26m-obb.dxnn | 4 | 32.8 | 4 | 32.8 |
| yolo26l-obb.dxnn | 3 | 32.0 | 3 | 32.0 |
| yolo26x-obb.dxnn | 1 | 55.3 | 1 | 55.3 |

---
*Report generated by dx_stream benchmark tool*
