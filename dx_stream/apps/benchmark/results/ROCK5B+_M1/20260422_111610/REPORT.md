# YOLO26 Benchmark Report

**Generated:** 2026-04-23 06:08:20 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-22 11:16:10 | 2026-04-23 06:08:20 | 18h 52m 10s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 37.83 | 163.9 | 151.0 | 4 |
| yolo26n.dxnn | OFF | 36.79 | 222.4 | 95.8 | 3 |
| yolo26s.dxnn | ON | 47.07 | 131.3 | 130.9 | 4 |
| yolo26s.dxnn | OFF | 32.55 | 131.1 | 95.0 | 3 |
| yolo26m.dxnn | ON | 54.68 | 91.2 | 91.5 | 2 |
| yolo26m.dxnn | OFF | 52.72 | 90.8 | 90.7 | 2 |
| yolo26l.dxnn | ON | 69.24 | 66.6 | 66.7 | 2 |
| yolo26l.dxnn | OFF | 62.38 | 66.8 | 66.8 | 2 |
| yolo26x.dxnn | ON | 100.04 | 38.6 | 36.0 | 1 |
| yolo26x.dxnn | OFF | 92.27 | 38.2 | 35.2 | 1 |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 27.65 | 218.2 | 215.6 | 7 |
| yolo26n-pose.dxnn | OFF | 26.18 | 216.0 | 215.0 | 7 |
| yolo26s-pose.dxnn | ON | 39.87 | 126.3 | 126.2 | 4 |
| yolo26s-pose.dxnn | OFF | 33.99 | 126.3 | 126.1 | 4 |
| yolo26m-pose.dxnn | ON | 44.87 | 88.1 | 88.1 | 2 |
| yolo26m-pose.dxnn | OFF | 39.40 | 87.9 | 87.7 | 2 |
| yolo26l-pose.dxnn | ON | 55.37 | 65.1 | 64.7 | 1 |
| yolo26l-pose.dxnn | OFF | 51.31 | 64.9 | 64.9 | 1 |
| yolo26x-pose.dxnn | ON | 85.85 | 37.5 | 35.4 | 1 |
| yolo26x-pose.dxnn | OFF | 81.32 | 37.5 | 35.7 | 1 |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 36.45 | 111.9 | 97.8 | 3 |
| yolo26n-seg.dxnn | OFF | 34.94 | 144.7 | 78.4 | 2 |
| yolo26s-seg.dxnn | ON | 49.96 | 97.2 | 94.3 | 3 |
| yolo26s-seg.dxnn | OFF | 47.60 | 99.6 | 76.3 | 2 |
| yolo26m-seg.dxnn | ON | 67.24 | 65.4 | 60.5 | 1 |
| yolo26m-seg.dxnn | OFF | 58.29 | 65.5 | 60.4 | 1 |
| yolo26l-seg.dxnn | ON | 82.95 | 51.8 | 45.2 | 1 |
| yolo26l-seg.dxnn | OFF | 71.12 | 52.1 | 44.7 | 1 |
| yolo26x-seg.dxnn | ON | 122.76 | 29.6 | 22.1 | — |
| yolo26x-seg.dxnn | OFF | 110.20 | 29.2 | 21.9 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 49.90 | 75.0 | 75.0 | 2 |
| yolo26n-obb.dxnn | OFF | 44.91 | 74.9 | 74.9 | 2 |
| yolo26s-obb.dxnn | ON | 72.56 | 43.5 | 43.5 | 1 |
| yolo26s-obb.dxnn | OFF | 64.63 | 43.6 | 43.5 | 1 |
| yolo26m-obb.dxnn | ON | 90.56 | 31.9 | 32.0 | 1 |
| yolo26m-obb.dxnn | OFF | 85.96 | 31.9 | 31.1 | 1 |
| yolo26l-obb.dxnn | ON | 112.24 | 23.3 | 21.3 | — |
| yolo26l-obb.dxnn | OFF | 106.35 | 23.4 | 21.7 | — |
| yolo26x-obb.dxnn | ON | 191.61 | 13.5 | 12.1 | — |
| yolo26x-obb.dxnn | OFF | 186.36 | 13.6 | 12.1 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 1.38 | 3496.8 | 966.7 | — |
| yolo26n-cls.dxnn | OFF | 1.42 | 3496.7 | 992.6 | — |
| yolo26s-cls.dxnn | ON | 1.97 | 1895.1 | 990.0 | — |
| yolo26s-cls.dxnn | OFF | 1.99 | 1895.6 | 952.3 | — |
| yolo26m-cls.dxnn | ON | 2.62 | 1330.0 | 980.7 | — |
| yolo26m-cls.dxnn | OFF | 2.81 | 1329.6 | 978.8 | — |
| yolo26l-cls.dxnn | ON | 3.90 | 840.1 | 808.0 | — |
| yolo26l-cls.dxnn | OFF | 4.21 | 838.7 | 812.4 | — |
| yolo26x-cls.dxnn | ON | 6.67 | 449.6 | 448.9 | — |
| yolo26x-cls.dxnn | OFF | 6.71 | 450.1 | 447.3 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | rock-5b-plus |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.1.84-6-rk2410 |
| CPU | - |
| CPU Cores | 8 |
| RAM | 7.8 GB |
| NPU SKU | M1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5 5600 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X2 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.3.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.9 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.9 |
| dxtop | Yes | DX-TOP 1.1.0 |
| ffprobe | Yes | ffprobe version 5.1.8-0+deb12u1 Copyright (c) 2007-2025 the ... |

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
| yolo26n.dxnn | 163.9 ±2.1 | 203 | 48.9 | 83.4 | 40~44 | 1000 | ok |
| yolo26s.dxnn | 131.3 ±0.1 | 177 | 88.8 | 100.0 | 63~66 | 1000 | ok |
| yolo26m.dxnn | 91.2 ±0.5 | 131 | 91.1 | 100.0 | 65~69 | 1000 | ok |
| yolo26l.dxnn | 66.6 ±0.1 | 105 | 89.8 | 100.0 | 67~70 | 1000 | ok |
| yolo26x.dxnn | 38.6 ±0.0 | 79 | 89.4 | 100.0 | 69~72 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 222.4 ±1.5 | 142 | 87.5 | 100.0 | 60~62 | 1000 | ok |
| yolo26s.dxnn | 131.1 ±0.1 | 92 | 91.9 | 100.0 | 69~70 | 1000 | ok |
| yolo26m.dxnn | 90.8 ±0.1 | 80 | 89.2 | 100.0 | 68~71 | 1000 | ok |
| yolo26l.dxnn | 66.8 ±0.1 | 77 | 89.6 | 100.0 | 67~70 | 1000 | ok |
| yolo26x.dxnn | 38.2 ±0.0 | 57 | 88.8 | 100.0 | 69~72 | 1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 218.2 ±0.4 | 186 | 88.7 | 99.6 | 66~67 | 1000 | ok |
| yolo26s-pose.dxnn | 126.3 ±0.4 | 134 | 91.9 | 100.0 | 65~67 | 1000 | ok |
| yolo26m-pose.dxnn | 88.1 ±0.0 | 112 | 91.8 | 100.0 | 66~69 | 1000 | ok |
| yolo26l-pose.dxnn | 65.1 ±0.2 | 97 | 89.0 | 100.0 | 68~71 | 1000 | ok |
| yolo26x-pose.dxnn | 37.5 ±0.1 | 70 | 88.6 | 100.0 | 68~72 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 216.0 ±0.8 | 114 | 91.2 | 100.0 | 68~70 | 1000 | ok |
| yolo26s-pose.dxnn | 126.3 ±0.3 | 103 | 92.4 | 100.0 | 65~67 | 1000 | ok |
| yolo26m-pose.dxnn | 87.9 ±0.3 | 89 | 90.9 | 100.0 | 69~72 | 1000 | ok |
| yolo26l-pose.dxnn | 64.9 ±0.0 | 83 | 89.9 | 100.0 | 68~71 | 1000 | ok |
| yolo26x-pose.dxnn | 37.5 ±0.0 | 55 | 89.0 | 100.0 | 69~73 | 1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 111.9 ±1.2 | 286 | 41.4 | 69.6 | 62~63 | 1000 | ok |
| yolo26s-seg.dxnn | 97.2 ±2.1 | 241 | 80.2 | 94.8 | 60~63 | 1000 | ok |
| yolo26m-seg.dxnn | 65.4 ±0.5 | 154 | 90.1 | 100.0 | 68~73 | 1000 | ok |
| yolo26l-seg.dxnn | 51.8 ±0.2 | 118 | 88.1 | 100.0 | 70~74 | 1000 | ok |
| yolo26x-seg.dxnn | 29.6 ±0.1 | 81 | 89.1 | 100.0 | 71~75 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 144.7 ±0.7 | 229 | 62.7 | 77.9 | 61~64 | 1000 | ok |
| yolo26s-seg.dxnn | 99.6 ±0.2 | 146 | 89.6 | 100.0 | 66~68 | 1000 | ok |
| yolo26m-seg.dxnn | 65.5 ±0.4 | 107 | 90.2 | 100.0 | 69~73 | 1000 | ok |
| yolo26l-seg.dxnn | 52.1 ±0.1 | 84 | 89.1 | 100.0 | 73~76 | 1000 | ok |
| yolo26x-seg.dxnn | 29.2 ±0.2 | 64 | 88.5 | 100.0 | 71~75 | 1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 75.0 ±0.1 | 110 | 88.8 | 100.0 | 65~66 | 1000 | ok |
| yolo26s-obb.dxnn | 43.5 ±0.0 | 91 | 88.3 | 100.0 | 61~63 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.1 | 76 | 88.3 | 100.0 | 63~66 | 1000 | ok |
| yolo26l-obb.dxnn | 23.3 ±0.1 | 56 | 88.4 | 100.0 | 71~74 | 1000 | ok |
| yolo26x-obb.dxnn | 13.5 ±0.0 | 44 | 86.0 | 100.0 | 68~71 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 74.9 ±0.2 | 86 | 91.4 | 100.0 | 61~63 | 1000 | ok |
| yolo26s-obb.dxnn | 43.6 ±0.1 | 74 | 90.9 | 100.0 | 61~63 | 1000 | ok |
| yolo26m-obb.dxnn | 31.9 ±0.1 | 54 | 89.0 | 100.0 | 66~69 | 1000 | ok |
| yolo26l-obb.dxnn | 23.4 ±0.0 | 46 | 88.6 | 100.0 | 70~72 | 1000 | ok |
| yolo26x-obb.dxnn | 13.6 ±0.0 | 30 | 86.3 | 100.0 | 69~72 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3496.8 ±1.1 | 118 | 86.4 | 95.4 | 63 | 1000 | ok |
| yolo26s-cls.dxnn | 1895.1 ±1.4 | 67 | 87.1 | 97.1 | 56~58 | 1000 | ok |
| yolo26m-cls.dxnn | 1330.0 ±1.6 | 76 | 88.7 | 98.1 | 59~62 | 1000 | ok |
| yolo26l-cls.dxnn | 840.1 ±1.5 | 62 | 88.7 | 98.5 | 59~61 | 1000 | ok |
| yolo26x-cls.dxnn | 449.6 ±0.4 | 49 | 91.4 | 99.4 | 59~63 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3496.7 ±11.2 | 117 | 86.4 | 96.1 | 56~58 | 1000 | ok |
| yolo26s-cls.dxnn | 1895.6 ±1.6 | 70 | 88.1 | 97.7 | 56~59 | 1000 | ok |
| yolo26m-cls.dxnn | 1329.6 ±2.3 | 77 | 91.5 | 98.3 | 60~63 | 1000 | ok |
| yolo26l-cls.dxnn | 838.7 ±1.2 | 61 | 88.9 | 98.6 | 58~61 | 1000 | ok |
| yolo26x-cls.dxnn | 450.1 ±0.6 | 49 | 90.3 | 99.5 | 60~63 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 26.4 | 37.83 | 33.48 | 4.35 | 34 | ok |
| yolo26s.dxnn | 21.2 | 47.07 | 41.79 | 5.29 | 59 | ok |
| yolo26m.dxnn | 18.3 | 54.68 | 50.01 | 4.67 | 60 | ok |
| yolo26l.dxnn | 14.4 | 69.24 | 60.49 | 8.75 | 62 | ok |
| yolo26x.dxnn | 10.0 | 100.04 | 90.71 | 9.32 | 63 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 27.2 | 36.79 | 36.79 | 57 | ok |
| yolo26s.dxnn | 30.7 | 32.55 | 32.55 | 65 | ok |
| yolo26m.dxnn | 19.0 | 52.72 | 52.72 | 63 | ok |
| yolo26l.dxnn | 16.0 | 62.38 | 62.38 | 63 | ok |
| yolo26x.dxnn | 10.8 | 92.27 | 92.27 | 63 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 36.2 | 27.65 | 25.56 | 2.08 | 63 | ok |
| yolo26s-pose.dxnn | 25.1 | 39.87 | 35.28 | 4.59 | 61 | ok |
| yolo26m-pose.dxnn | 22.3 | 44.87 | 40.55 | 4.32 | 60 | ok |
| yolo26l-pose.dxnn | 18.1 | 55.37 | 50.74 | 4.63 | 63 | ok |
| yolo26x-pose.dxnn | 11.6 | 85.85 | 81.60 | 4.25 | 63 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 38.2 | 26.18 | 26.18 | 66 | ok |
| yolo26s-pose.dxnn | 29.4 | 33.99 | 33.99 | 62 | ok |
| yolo26m-pose.dxnn | 25.4 | 39.40 | 39.40 | 64 | ok |
| yolo26l-pose.dxnn | 19.5 | 51.31 | 51.31 | 63 | ok |
| yolo26x-pose.dxnn | 12.3 | 81.32 | 81.32 | 64 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 27.4 | 36.45 | 33.28 | 3.17 | 61 | ok |
| yolo26s-seg.dxnn | 20.0 | 49.96 | 46.01 | 3.95 | 56 | ok |
| yolo26m-seg.dxnn | 14.9 | 67.24 | 63.11 | 4.13 | 60 | ok |
| yolo26l-seg.dxnn | 12.1 | 82.95 | 70.85 | 12.11 | 63 | ok |
| yolo26x-seg.dxnn | 8.1 | 122.76 | 115.08 | 7.68 | 64 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 28.6 | 34.94 | 34.94 | 58 | ok |
| yolo26s-seg.dxnn | 21.0 | 47.60 | 47.60 | 62 | ok |
| yolo26m-seg.dxnn | 17.2 | 58.29 | 58.29 | 63 | ok |
| yolo26l-seg.dxnn | 14.1 | 71.12 | 71.12 | 67 | ok |
| yolo26x-seg.dxnn | 9.1 | 110.20 | 110.20 | 64 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 20.0 | 49.90 | 44.90 | 5.00 | 63 | ok |
| yolo26s-obb.dxnn | 13.8 | 72.56 | 66.63 | 5.93 | 58 | ok |
| yolo26m-obb.dxnn | 11.0 | 90.56 | 84.32 | 6.25 | 58 | ok |
| yolo26l-obb.dxnn | 8.9 | 112.24 | 105.49 | 6.74 | 66 | ok |
| yolo26x-obb.dxnn | 5.2 | 191.61 | 187.09 | 4.52 | 63 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 22.3 | 44.91 | 44.91 | 59 | ok |
| yolo26s-obb.dxnn | 15.5 | 64.63 | 64.63 | 58 | ok |
| yolo26m-obb.dxnn | 11.6 | 85.96 | 85.96 | 61 | ok |
| yolo26l-obb.dxnn | 9.4 | 106.35 | 106.35 | 65 | ok |
| yolo26x-obb.dxnn | 5.4 | 186.36 | 186.36 | 64 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 725.7 | 1.38 | 1.38 | N/A | 62 | ok |
| yolo26s-cls.dxnn | 508.2 | 1.97 | 1.97 | N/A | 52 | ok |
| yolo26m-cls.dxnn | 382.1 | 2.62 | 2.62 | N/A | 53 | ok |
| yolo26l-cls.dxnn | 256.3 | 3.90 | 3.90 | N/A | 54 | ok |
| yolo26x-cls.dxnn | 149.9 | 6.67 | 6.67 | N/A | 53 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 704.1 | 1.42 | 1.42 | 54 | ok |
| yolo26s-cls.dxnn | 502.4 | 1.99 | 1.99 | 53 | ok |
| yolo26m-cls.dxnn | 355.9 | 2.81 | 2.81 | 55 | ok |
| yolo26l-cls.dxnn | 237.4 | 4.21 | 4.21 | 54 | ok |
| yolo26x-cls.dxnn | 149.0 | 6.71 | 6.71 | 55 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | mppvideodec | 3455 | 3 | 151.0 ±0.9 | 22.88 | 256 | 44.3 | 74.5 | 47~50 | 1000 | 147 | ok |
| yolo26s.dxnn | mppvideodec | 3455 | 3 | 130.9 ±0.5 | 26.38 | 227 | 89.3 | 100.0 | 67~69 | 1000 | 162 | ok |
| yolo26m.dxnn | mppvideodec | 3455 | 3 | 91.5 ±0.2 | 37.76 | 167 | 91.1 | 100.0 | 72~76 | 1000 | 183 | ok |
| yolo26l.dxnn | mppvideodec | 3455 | 3 | 66.7 ±0.1 | 51.81 | 127 | 93.7 | 100.0 | 73~77 | 1000 | 192 | ok |
| yolo26x.dxnn | mppvideodec | 3455 | 3 | 36.0 ±1.5 | 96.09 | 84 | 95.4 | 100.0 | 79~83 | 600~1000 | 313 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | mppvideodec | 3455 | 3 | 95.8 ±0.8 | 36.08 | 188 | 25.2 | 81.6 | 61~62 | 1000 | 164 | ok |
| yolo26s.dxnn | mppvideodec | 3455 | 3 | 95.0 ±0.1 | 36.35 | 193 | 50.0 | 82.2 | 69~70 | 1000 | 180 | ok |
| yolo26m.dxnn | mppvideodec | 3455 | 3 | 90.7 ±0.1 | 38.11 | 206 | 91.9 | 100.0 | 73~76 | 1000 | 197 | ok |
| yolo26l.dxnn | mppvideodec | 3455 | 3 | 66.8 ±0.2 | 51.73 | 154 | 93.6 | 100.0 | 73~76 | 1000 | 210 | ok |
| yolo26x.dxnn | mppvideodec | 3455 | 3 | 35.2 ±1.4 | 98.28 | 108 | 95.0 | 100.0 | 81~83 | 800~1000 | 313 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 151.0 | 95.8 | +55.3 | +57.7% |
| yolo26s.dxnn | 130.9 | 95.0 | +35.9 | +37.8% |
| yolo26m.dxnn | 91.5 | 90.7 | +0.9 | +0.9% |
| yolo26l.dxnn | 66.7 | 66.8 | -0.1 | -0.2% |
| yolo26x.dxnn | 36.0 | 35.2 | +0.8 | +2.2% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 215.6 ±1.4 | 16.03 | 282 | 83.2 | 98.5 | 67~68 | 1000 | 138 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 126.2 ±0.1 | 27.38 | 165 | 89.6 | 100.0 | 68~69 | 1000 | 153 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 88.1 ±0.3 | 39.23 | 119 | 91.6 | 100.0 | 72~75 | 1000 | 176 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 64.7 ±1.0 | 53.43 | 98 | 93.6 | 100.0 | 74~78 | 1000 | 184 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 35.4 ±1.3 | 97.59 | 73 | 95.1 | 100.0 | 79~83 | 800~1000 | 325 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 215.0 ±0.2 | 16.07 | 179 | 83.1 | 100.0 | 69~70 | 1000 | 129 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 126.1 ±0.1 | 27.39 | 120 | 90.0 | 100.0 | 67~69 | 1000 | 143 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 87.7 ±0.1 | 39.38 | 101 | 93.2 | 100.0 | 74~77 | 1000 | 164 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 64.9 ±0.4 | 53.22 | 93 | 94.5 | 100.0 | 74~77 | 1000 | 175 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 35.7 ±1.1 | 96.87 | 71 | 95.8 | 100.0 | 78~82 | 800~1000 | 325 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 215.6 | 215.0 | +0.5 | +0.3% |
| yolo26s-pose.dxnn | 126.2 | 126.1 | +0.1 | +0.0% |
| yolo26m-pose.dxnn | 88.1 | 87.7 | +0.3 | +0.4% |
| yolo26l-pose.dxnn | 64.7 | 64.9 | -0.3 | -0.4% |
| yolo26x-pose.dxnn | 35.4 | 35.7 | -0.3 | -0.8% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 97.8 ±0.9 | 35.31 | 370 | 34.6 | 62.3 | 63~64 | 1000 | 239 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 94.3 ±1.6 | 36.65 | 329 | 77.1 | 88.4 | 66~69 | 1000 | 256 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 60.5 ±2.7 | 57.12 | 214 | 94.2 | 100.0 | 79~83 | 800~1000 | 282 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 45.2 ±4.1 | 76.51 | 138 | 94.2 | 100.0 | 81~84 | 400~1000 | 291 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 22.1 ±1.4 | 156.26 | 89 | 95.2 | 100.0 | 84 | 400~1000 | 361 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 78.4 ±0.2 | 44.05 | 280 | 26.0 | 84.1 | 63 | 1000 | 280 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 76.3 ±0.4 | 45.27 | 269 | 53.6 | 91.2 | 68~69 | 1000 | 295 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 60.4 ±3.0 | 57.24 | 197 | 93.8 | 100.0 | 78~83 | 800~1000 | 306 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 44.7 ±3.4 | 77.33 | 148 | 94.0 | 100.0 | 81~84 | 600~1000 | 322 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 21.9 ±1.4 | 158.13 | 96 | 94.6 | 100.0 | 83~84 | 400~1000 | 366 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 97.8 | 78.4 | +19.4 | +24.7% |
| yolo26s-seg.dxnn | 94.3 | 76.3 | +18.0 | +23.5% |
| yolo26m-seg.dxnn | 60.5 | 60.4 | +0.1 | +0.2% |
| yolo26l-seg.dxnn | 45.2 | 44.7 | +0.5 | +1.1% |
| yolo26x-seg.dxnn | 22.1 | 21.9 | +0.3 | +1.2% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 75.0 ±0.2 | 35.18 | 131 | 91.4 | 100.0 | 67~68 | 1000 | 166 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 43.5 ±0.0 | 60.70 | 92 | 93.4 | 100.0 | 65~68 | 1000 | 181 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 32.0 ±0.1 | 82.50 | 75 | 94.9 | 100.0 | 72~77 | 1000 | 204 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 21.3 ±0.3 | 124.06 | 58 | 96.4 | 100.0 | 80~83 | 1000 | 215 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 12.1 ±0.4 | 218.82 | 47 | 95.4 | 100.0 | 82~84 | 600~1000 | 333 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 74.9 ±0.1 | 35.24 | 135 | 92.9 | 100.0 | 64~66 | 1000 | 171 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 43.5 ±0.0 | 60.68 | 105 | 93.3 | 100.0 | 65~68 | 1000 | 184 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 31.1 ±1.3 | 84.87 | 81 | 94.7 | 100.0 | 74~79 | 1000 | 204 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 21.7 ±0.8 | 121.78 | 60 | 96.0 | 100.0 | 78~81 | 1000 | 212 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 12.1 ±0.4 | 218.57 | 46 | 95.3 | 100.0 | 82~83 | 600~1000 | 333 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 75.0 | 74.9 | +0.1 | +0.2% |
| yolo26s-obb.dxnn | 43.5 | 43.5 | -0.0 | -0.0% |
| yolo26m-obb.dxnn | 32.0 | 31.1 | +0.9 | +2.9% |
| yolo26l-obb.dxnn | 21.3 | 21.7 | -0.4 | -1.8% |
| yolo26x-obb.dxnn | 12.1 | 12.1 | -0.0 | -0.2% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 966.7 ±47.9 | 3.57 | 153 | 13.0 | 46.3 | 61 | 1000 | 54 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 990.0 ±32.9 | 3.49 | 154 | 28.2 | 68.4 | 56 | 1000 | 67 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 980.7 ±4.5 | 3.52 | 153 | 37.8 | 82.6 | 61 | 1000 | 90 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 808.0 ±7.3 | 4.28 | 136 | 55.6 | 98.2 | 60 | 1000 | 103 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 448.9 ±0.1 | 7.70 | 100 | 71.7 | 99.3 | 62~63 | 1000 | 179 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 992.6 ±27.6 | 3.48 | 154 | 13.8 | 45.6 | 56 | 1000 | 54 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 952.3 ±16.1 | 3.63 | 156 | 30.5 | 66.8 | 58 | 1000 | 67 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 978.8 ±22.9 | 3.53 | 153 | 40.0 | 84.2 | 61~62 | 1000 | 90 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 812.4 ±14.7 | 4.25 | 134 | 52.2 | 98.4 | 60 | 1000 | 103 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 447.3 ±0.9 | 7.72 | 99 | 72.1 | 99.3 | 63~64 | 1000 | 179 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 966.7 | 992.6 | -25.9 | -2.6% |
| yolo26s-cls.dxnn | 990.0 | 952.3 | +37.7 | +4.0% |
| yolo26m-cls.dxnn | 980.7 | 978.8 | +1.9 | +0.2% |
| yolo26l-cls.dxnn | 808.0 | 812.4 | -4.4 | -0.5% |
| yolo26x-cls.dxnn | 448.9 | 447.3 | +1.6 | +0.4% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 5 | 3 | 149.3 ±0.5 | 29.9 | 267 | 46.5 | 73.3 | 59~64 | 1000 | 174 | ok |
| yolo26n.dxnn | 4 | 3 | 148.4 ±0.6 | 37.1 | 258 | 46.0 | 72.0 | 66~67 | 1000 | 170 | ok |
| yolo26s.dxnn | 4 | 3 | 127.1 ±4.9 | 31.8 | 211 | 96.3 | 100.0 | 76~79 | 1000 | 186 | ok |
| yolo26s.dxnn | 5 | 3 | 121.0 ±0.2 | 24.2 | 201 | 97.4 | 100.0 | 81~82 | 1000 | 191 | ok |
| yolo26m.dxnn | 3 | 3 | 82.2 ±2.1 | 27.4 | 152 | 96.4 | 100.0 | 81~84 | 800~1000 | 201 | ok |
| yolo26m.dxnn | 2 | 3 | 80.1 ±1.4 | 40.0 | 151 | 95.2 | 100.0 | 84 | 800~1000 | 194 | ok |
| yolo26l.dxnn | 2 | 3 | 61.8 ±0.4 | 30.9 | 117 | 95.9 | 100.0 | 80~82 | 1000 | 205 | ok |
| yolo26l.dxnn | 3 | 3 | 60.3 ±0.9 | 20.1 | 119 | 96.6 | 100.0 | 83 | 800~1000 | 210 | ok |
| yolo26x.dxnn | 1 | 3 | 36.0 ±1.5 | 36.0 | 84 | 95.4 | 100.0 | 79~83 | 600~1000 | 313 | ok |
| yolo26x.dxnn | 2 | 3 | 32.0 ±0.6 | 16.0 | 80 | 96.6 | 100.0 | 83~84 | 400~1000 | 313 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 3 | 3 | 95.9 ±0.6 | 32.0 | 188 | 26.2 | 81.6 | 63~64 | 1000 | 189 | ok |
| yolo26n.dxnn | 4 | 3 | 95.9 ±0.5 | 24.0 | 194 | 26.4 | 81.6 | 65~67 | 1000 | 196 | ok |
| yolo26s.dxnn | 3 | 3 | 95.5 ±0.2 | 31.8 | 197 | 52.8 | 83.7 | 72~73 | 1000 | 203 | ok |
| yolo26s.dxnn | 4 | 3 | 96.0 ±0.1 | 24.0 | 190 | 53.6 | 83.8 | 73 | 1000 | 212 | ok |
| yolo26m.dxnn | 3 | 3 | 82.2 ±1.8 | 27.4 | 187 | 96.4 | 100.0 | 81~84 | 800~1000 | 223 | ok |
| yolo26m.dxnn | 2 | 3 | 80.6 ±1.7 | 40.3 | 181 | 95.7 | 100.0 | 83~84 | 600~1000 | 214 | ok |
| yolo26l.dxnn | 2 | 3 | 61.6 ±0.4 | 30.8 | 145 | 96.2 | 100.0 | 80~82 | 1000 | 223 | ok |
| yolo26l.dxnn | 3 | 3 | 60.1 ±0.8 | 20.0 | 149 | 96.7 | 100.0 | 83~84 | 600~1000 | 232 | ok |
| yolo26x.dxnn | 1 | 3 | 35.2 ±1.4 | 35.2 | 108 | 95.0 | 100.0 | 81~83 | 800~1000 | 313 | ok |
| yolo26x.dxnn | 2 | 3 | 32.5 ±0.5 | 16.3 | 105 | 96.5 | 100.0 | 84 | 400~1000 | 313 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 4 | 37.1 | 3 | 32.0 |
| yolo26s.dxnn | 4 | 31.8 | 3 | 31.8 |
| yolo26m.dxnn | 2 | 40.0 | 2 | 40.3 |
| yolo26l.dxnn | 2 | 30.9 | 2 | 30.8 |
| yolo26x.dxnn | 1 | 36.0 | 1 | 35.2 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 213.9 ±3.1 | 30.6 | 268 | 92.9 | 99.0 | 73~78 | 1000 | 187 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 201.0 ±0.9 | 25.1 | 259 | 95.4 | 99.9 | 80~81 | 1000 | 190 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 126.5 ±0.2 | 31.6 | 169 | 96.6 | 100.0 | 74~77 | 1000 | 182 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 119.9 ±2.2 | 24.0 | 163 | 97.5 | 100.0 | 78~79 | 1000 | 188 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 82.0 ±1.5 | 41.0 | 111 | 95.9 | 100.0 | 79~83 | 800~1000 | 189 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 75.7 ±2.1 | 25.2 | 108 | 96.5 | 100.0 | 84 | 600~1000 | 196 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 59.9 ±0.5 | 29.9 | 91 | 96.2 | 100.0 | 80~83 | 800~1000 | 198 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 64.7 ±1.0 | 64.7 | 98 | 93.6 | 100.0 | 74~78 | 1000 | 184 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 35.4 ±1.3 | 35.4 | 73 | 95.1 | 100.0 | 79~83 | 800~1000 | 325 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 31.3 ±0.3 | 15.7 | 72 | 96.6 | 100.0 | 83~84 | 400~1000 | 325 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 216.3 ±0.1 | 30.9 | 192 | 96.6 | 100.0 | 74~76 | 1000 | 182 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 211.9 ±4.5 | 26.5 | 187 | 97.2 | 100.0 | 77~78 | 1000 | 190 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 126.4 ±0.1 | 31.6 | 124 | 96.4 | 100.0 | 73~76 | 1000 | 178 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 125.5 ±1.3 | 25.1 | 123 | 97.3 | 100.0 | 76~77 | 1000 | 185 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 81.4 ±0.9 | 40.7 | 87 | 96.1 | 100.0 | 80~82 | 800~1000 | 180 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 76.5 ±2.9 | 25.5 | 93 | 96.5 | 100.0 | 83~84 | 600~1000 | 189 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 59.9 ±0.4 | 30.0 | 86 | 96.4 | 100.0 | 81~83 | 800~1000 | 188 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 64.9 ±0.4 | 64.9 | 93 | 94.5 | 100.0 | 74~77 | 1000 | 175 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 35.7 ±1.1 | 35.7 | 71 | 95.8 | 100.0 | 78~82 | 800~1000 | 325 | ok |
| yolo26x-pose.dxnn | 2 | 3 | 32.7 ±0.2 | 16.3 | 69 | 95.9 | 100.0 | 83 | 600~1000 | 325 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 7 | 30.6 | 7 | 30.9 |
| yolo26s-pose.dxnn | 4 | 31.6 | 4 | 31.6 |
| yolo26m-pose.dxnn | 2 | 41.0 | 2 | 40.7 |
| yolo26l-pose.dxnn | 1 | 64.7 | 1 | 64.9 |
| yolo26x-pose.dxnn | 1 | 35.4 | 1 | 35.7 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 97.3 ±0.1 | 32.5 | 387 | 35.2 | 67.1 | 65~67 | 1000 | 268 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 97.6 ±0.3 | 24.4 | 376 | 35.6 | 65.2 | 68~69 | 1000 | 280 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 92.2 ±0.6 | 30.7 | 345 | 78.9 | 89.2 | 74~78 | 1000 | 294 | ok |
| yolo26s-seg.dxnn | 4 | 3 | 91.0 ±1.5 | 22.8 | 317 | 89.4 | 96.0 | 80~81 | 1000 | 299 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 47.7 ±1.7 | 23.9 | 160 | 95.7 | 100.0 | 83~84 | 400~1000 | 302 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 60.5 ±2.7 | 60.5 | 214 | 94.2 | 100.0 | 79~83 | 800~1000 | 282 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 45.2 ±4.1 | 45.2 | 138 | 94.2 | 100.0 | 81~84 | 400~1000 | 291 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 34.3 ±0.6 | 17.2 | 124 | 95.8 | 100.0 | 84 | 400~1000 | 312 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 22.1 ±1.4 | 22.1 | 89 | 95.2 | 100.0 | 84 | 400~1000 | 361 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 2 | 3 | 78.5 ±0.1 | 39.3 | 281 | 26.3 | 84.0 | 63~64 | 1000 | 303 | ok |
| yolo26n-seg.dxnn | 3 | 3 | 78.1 ±0.5 | 26.0 | 263 | 26.6 | 83.1 | 64~65 | 1000 | 318 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 76.1 ±0.2 | 38.1 | 271 | 54.9 | 91.8 | 71~73 | 1000 | 319 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 76.2 ±0.2 | 25.4 | 271 | 55.3 | 92.7 | 73~74 | 1000 | 333 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 48.1 ±3.4 | 24.1 | 151 | 95.8 | 100.0 | 84~85 | 400~1000 | 345 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 60.4 ±3.0 | 60.4 | 197 | 93.8 | 100.0 | 78~83 | 800~1000 | 306 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 44.7 ±3.4 | 44.7 | 148 | 94.0 | 100.0 | 81~84 | 600~1000 | 322 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 38.0 ±1.1 | 19.0 | 131 | 96.1 | 100.0 | 84 | 400~1000 | 346 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 21.9 ±1.4 | 21.9 | 96 | 94.6 | 100.0 | 83~84 | 400~1000 | 366 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 32.5 | 2 | 39.3 |
| yolo26s-seg.dxnn | 3 | 30.7 | 2 | 38.1 |
| yolo26m-seg.dxnn | 1 | 60.5 | 1 | 60.4 |
| yolo26l-seg.dxnn | 1 | 45.2 | 1 | 44.7 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 75.0 ±0.0 | 37.5 | 133 | 94.8 | 100.0 | 71~74 | 1000 | 181 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 75.0 ±0.1 | 25.0 | 135 | 96.2 | 100.0 | 73~74 | 1000 | 194 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 43.5 ±0.0 | 43.5 | 92 | 93.4 | 100.0 | 65~68 | 1000 | 181 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 43.6 ±0.0 | 21.8 | 94 | 95.8 | 100.0 | 71~73 | 1000 | 198 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 32.0 ±0.1 | 32.0 | 75 | 94.9 | 100.0 | 72~77 | 1000 | 204 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.0 ±0.2 | 14.5 | 73 | 95.9 | 100.0 | 80~81 | 1000 | 220 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 21.3 ±0.3 | 21.3 | 58 | 96.4 | 100.0 | 80~83 | 1000 | 215 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.1 ±0.4 | 12.1 | 47 | 95.4 | 100.0 | 82~84 | 600~1000 | 333 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 74.9 ±0.0 | 37.4 | 136 | 95.3 | 100.0 | 67~69 | 1000 | 177 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 74.9 ±0.1 | 25.0 | 139 | 96.2 | 100.0 | 70~72 | 1000 | 191 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 43.5 ±0.0 | 43.5 | 105 | 93.3 | 100.0 | 65~68 | 1000 | 184 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 43.6 ±0.0 | 21.8 | 103 | 96.4 | 100.0 | 70~73 | 1000 | 202 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.1 ±1.3 | 31.1 | 81 | 94.7 | 100.0 | 74~79 | 1000 | 204 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 27.0 ±1.1 | 13.5 | 76 | 95.7 | 100.0 | 84 | 400~1000 | 219 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 21.7 ±0.8 | 21.7 | 60 | 96.0 | 100.0 | 78~81 | 1000 | 212 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 12.1 ±0.4 | 12.1 | 46 | 95.3 | 100.0 | 82~83 | 600~1000 | 333 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 37.5 | 2 | 37.4 |
| yolo26s-obb.dxnn | 1 | 43.5 | 1 | 43.5 |
| yolo26m-obb.dxnn | 1 | 32.0 | 1 | 31.1 |

---
*Report generated by dx_stream benchmark tool*
