# YOLO26 Benchmark Report

**Generated:** 2026-04-15 17:57:44 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-06 16:14:00 | 2026-04-07 08:36:24 | 16h 22m 24s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-1.dxnn | ON | 53.28 | 172.7 | 161.4 | 5 |
| yolo26n-1.dxnn | OFF | 38.80 | 180.3 | 98.8 | 3 |
| yolo26s-1.dxnn | ON | 65.76 | 112.0 | 112.7 | 3 |
| yolo26s-1.dxnn | OFF | 45.84 | 120.4 | 98.8 | 3 |
| yolo26m-1.dxnn | ON | 82.43 | 83.2 | 74.9 | 2 |
| yolo26m-1.dxnn | OFF | 53.80 | 85.1 | 75.7 | 2 |
| yolo26l-1.dxnn | ON | 82.94 | 62.6 | 56.3 | 1 |
| yolo26l-1.dxnn | OFF | 65.84 | 62.8 | 57.5 | 1 |
| yolo26x-1.dxnn | ON | 125.46 | 35.4 | 28.0 | — |
| yolo26x-1.dxnn | OFF | 96.74 | 35.5 | 27.9 | — |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 42.32 | 202.4 | 201.7 | 6 |
| yolo26n-pose.dxnn | OFF | 29.83 | 218.3 | 219.0 | 7 |
| yolo26s-pose.dxnn | ON | 48.67 | 129.3 | 129.8 | 4 |
| yolo26s-pose.dxnn | OFF | 36.91 | 131.4 | 131.1 | 4 |
| yolo26m-pose.dxnn | ON | 60.05 | 91.2 | 78.4 | 2 |
| yolo26m-pose.dxnn | OFF | 46.24 | 91.5 | 80.5 | 2 |
| yolo26l-pose.dxnn | ON | 70.48 | 68.3 | 59.7 | 1 |
| yolo26l-pose.dxnn | OFF | 53.04 | 68.4 | 60.6 | 1 |
| yolo26x-pose.dxnn | ON | 102.21 | 38.5 | 29.2 | — |
| yolo26x-pose.dxnn | OFF | 80.47 | 38.5 | 29.5 | — |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 68.32 | 114.8 | 105.6 | 3 |
| yolo26n-seg.dxnn | OFF | 49.74 | 138.3 | 85.8 | 2 |
| yolo26s-seg.dxnn | ON | 81.37 | 84.4 | 88.4 | 2 |
| yolo26s-seg.dxnn | OFF | 60.41 | 92.3 | 84.2 | 2 |
| yolo26m-seg.dxnn | ON | 110.94 | 60.1 | 41.1 | 1 |
| yolo26m-seg.dxnn | OFF | 76.50 | 61.8 | 39.4 | 1 |
| yolo26l-seg.dxnn | ON | 111.02 | 51.5 | 35.3 | 1 |
| yolo26l-seg.dxnn | OFF | 85.05 | 51.0 | 35.4 | 1 |
| yolo26x-seg.dxnn | ON | 149.12 | 26.4 | 17.9 | — |
| yolo26x-seg.dxnn | OFF | 127.93 | 26.9 | 17.7 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 64.45 | 76.6 | 76.6 | 2 |
| yolo26n-obb.dxnn | OFF | 49.30 | 76.7 | 76.5 | 2 |
| yolo26s-obb.dxnn | ON | 86.09 | 44.4 | 44.4 | 1 |
| yolo26s-obb.dxnn | OFF | 67.78 | 44.4 | 44.5 | 1 |
| yolo26m-obb.dxnn | ON | 104.94 | 32.0 | 31.5 | 1 |
| yolo26m-obb.dxnn | OFF | 87.15 | 32.0 | 31.5 | 1 |
| yolo26l-obb.dxnn | ON | 128.13 | 24.0 | 22.9 | — |
| yolo26l-obb.dxnn | OFF | 107.50 | 24.0 | 23.3 | — |
| yolo26x-obb.dxnn | ON | 203.79 | 13.9 | 11.4 | — |
| yolo26x-obb.dxnn | OFF | 181.52 | 13.9 | 11.2 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 2.32 | 3229.0 | 1050.3 | — |
| yolo26n-cls.dxnn | OFF | 2.50 | 3232.9 | 1050.7 | — |
| yolo26s-cls.dxnn | ON | 3.14 | 1888.3 | 1044.8 | — |
| yolo26s-cls.dxnn | OFF | 3.28 | 1882.6 | 1050.0 | — |
| yolo26m-cls.dxnn | ON | 3.86 | 1345.7 | 1060.0 | — |
| yolo26m-cls.dxnn | OFF | 4.10 | 1346.1 | 1055.8 | — |
| yolo26l-cls.dxnn | ON | 5.36 | 849.3 | 842.2 | — |
| yolo26l-cls.dxnn | OFF | 5.46 | 848.8 | 840.6 | — |
| yolo26x-cls.dxnn | ON | 8.17 | 457.6 | 455.4 | — |
| yolo26x-cls.dxnn | OFF | 7.97 | 457.8 | 453.9 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | OrangePi5+ |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.1.43-rockchip-rk3588 |
| CPU | Cortex-A55 |
| CPU Cores | 8 |
| RAM | 15.6 GB |
| NPU SKU | M1 × 1 |
| NPU RT | v3.2.0 |
| NPU Driver (RT) | v2.1.0 |
| NPU Driver (PCIe) | v2.0.1 |
| NPU Firmware | v2.5.2 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X4 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.2.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.0 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.0 |
| dxtop | Yes | DX-TOP 1.0.1 |
| ffprobe | Yes | ffprobe version 5.1.3-4 Copyright (c) 2007-2022 the FFmpeg d... |

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
| yolo26n-1.dxnn | 172.7 ±1.6 | 247 | 57.6 | 92.6 | 56~62 | 1000 | ok |
| yolo26s-1.dxnn | 112.0 ±0.2 | 193 | 79.9 | 94.9 | 66~71 | 1000 | ok |
| yolo26m-1.dxnn | 83.2 ±0.1 | 175 | 86.4 | 100.0 | 71~79 | 1000 | ok |
| yolo26l-1.dxnn | 62.6 ±0.0 | 160 | 90.8 | 100.0 | 70~78 | 1000 | ok |
| yolo26x-1.dxnn | 35.4 ±0.1 | 118 | 88.6 | 100.0 | 73~81 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-1.dxnn | 180.3 ±1.1 | 188 | 70.7 | 86.7 | 64~68 | 1000 | ok |
| yolo26s-1.dxnn | 120.4 ±0.0 | 150 | 87.6 | 99.4 | 67~72 | 1000 | ok |
| yolo26m-1.dxnn | 85.1 ±0.0 | 127 | 92.5 | 100.0 | 72~80 | 1000 | ok |
| yolo26l-1.dxnn | 62.8 ±0.1 | 111 | 91.3 | 100.0 | 71~77 | 1000 | ok |
| yolo26x-1.dxnn | 35.5 ±0.0 | 74 | 89.5 | 100.0 | 73~80 | 800~1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 202.4 ±1.2 | 234 | 75.8 | 89.8 | 64~68 | 1000 | ok |
| yolo26s-pose.dxnn | 129.3 ±0.3 | 190 | 87.1 | 100.0 | 66~72 | 1000 | ok |
| yolo26m-pose.dxnn | 91.2 ±0.2 | 178 | 90.3 | 100.0 | 73~81 | 1000 | ok |
| yolo26l-pose.dxnn | 68.3 ±0.0 | 136 | 90.1 | 100.0 | 72~79 | 1000 | ok |
| yolo26x-pose.dxnn | 38.5 ±0.9 | 104 | 89.4 | 100.0 | 75~82 | 800~1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 218.3 ±0.3 | 182 | 87.4 | 98.9 | 65~70 | 1000 | ok |
| yolo26s-pose.dxnn | 131.4 ±0.0 | 141 | 90.7 | 100.0 | 67~72 | 1000 | ok |
| yolo26m-pose.dxnn | 91.5 ±0.1 | 121 | 92.3 | 100.0 | 73~81 | 1000 | ok |
| yolo26l-pose.dxnn | 68.4 ±0.0 | 112 | 92.3 | 100.0 | 72~79 | 1000 | ok |
| yolo26x-pose.dxnn | 38.5 ±0.9 | 66 | 89.5 | 100.0 | 75~82 | 800~1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 114.8 ±2.3 | 329 | 42.3 | 73.9 | 61~65 | 1000 | ok |
| yolo26s-seg.dxnn | 84.4 ±0.3 | 237 | 65.5 | 87.0 | 67~72 | 1000 | ok |
| yolo26m-seg.dxnn | 60.1 ±6.0 | 180 | 87.9 | 100.0 | 77~83 | 400~1000 | ok |
| yolo26l-seg.dxnn | 51.5 ±2.5 | 165 | 88.5 | 100.0 | 75~83 | 600~1000 | ok |
| yolo26x-seg.dxnn | 26.4 ±3.1 | 112 | 87.0 | 100.0 | 79~83 | 600~1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 138.3 ±0.6 | 251 | 54.8 | 82.1 | 65~70 | 1000 | ok |
| yolo26s-seg.dxnn | 92.3 ±0.6 | 197 | 71.5 | 89.4 | 68~74 | 1000 | ok |
| yolo26m-seg.dxnn | 61.8 ±5.5 | 151 | 88.5 | 100.0 | 77~83 | 600~1000 | ok |
| yolo26l-seg.dxnn | 51.0 ±3.5 | 131 | 88.0 | 100.0 | 77~83 | 600~1000 | ok |
| yolo26x-seg.dxnn | 26.9 ±2.7 | 89 | 87.3 | 100.0 | 78~83 | 600~1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.6 ±0.0 | 163 | 89.6 | 100.0 | 64~68 | 1000 | ok |
| yolo26s-obb.dxnn | 44.4 ±0.1 | 122 | 90.1 | 100.0 | 64~68 | 1000 | ok |
| yolo26m-obb.dxnn | 32.0 ±0.0 | 106 | 88.6 | 100.0 | 68~74 | 1000 | ok |
| yolo26l-obb.dxnn | 24.0 ±0.0 | 84 | 90.2 | 100.0 | 70~75 | 1000 | ok |
| yolo26x-obb.dxnn | 13.9 ±0.0 | 55 | 85.5 | 100.0 | 72~78 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 76.7 ±0.0 | 104 | 91.6 | 100.0 | 64~67 | 1000 | ok |
| yolo26s-obb.dxnn | 44.4 ±0.0 | 88 | 88.8 | 100.0 | 63~66 | 1000 | ok |
| yolo26m-obb.dxnn | 32.0 ±0.1 | 64 | 88.8 | 100.0 | 69~74 | 1000 | ok |
| yolo26l-obb.dxnn | 24.0 ±0.0 | 47 | 89.3 | 100.0 | 69~75 | 1000 | ok |
| yolo26x-obb.dxnn | 13.9 ±0.0 | 30 | 85.7 | 100.0 | 74~80 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3229.0 ±2.1 | 134 | 85.2 | 94.7 | 60~63 | 1000 | ok |
| yolo26s-cls.dxnn | 1888.3 ±0.3 | 97 | 89.9 | 97.3 | 62~65 | 1000 | ok |
| yolo26m-cls.dxnn | 1345.7 ±3.9 | 76 | 89.8 | 98.3 | 69~74 | 1000 | ok |
| yolo26l-cls.dxnn | 849.3 ±0.2 | 62 | 91.2 | 99.1 | 66~70 | 1000 | ok |
| yolo26x-cls.dxnn | 457.6 ±0.1 | 41 | 91.5 | 99.9 | 68~74 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3232.9 ±4.3 | 133 | 85.9 | 94.8 | 61~63 | 1000 | ok |
| yolo26s-cls.dxnn | 1882.6 ±4.7 | 98 | 89.4 | 97.7 | 62~65 | 1000 | ok |
| yolo26m-cls.dxnn | 1346.1 ±3.9 | 77 | 88.7 | 98.4 | 68~74 | 1000 | ok |
| yolo26l-cls.dxnn | 848.8 ±0.1 | 60 | 90.1 | 99.1 | 66~70 | 1000 | ok |
| yolo26x-cls.dxnn | 457.8 ±0.1 | 39 | 91.4 | 99.8 | 68~74 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-1.dxnn | 18.8 | 53.28 | 35.54 | 17.74 | 46 | ok |
| yolo26s-1.dxnn | 15.2 | 65.76 | 50.01 | 15.76 | 55 | ok |
| yolo26m-1.dxnn | 12.1 | 82.43 | 65.01 | 17.42 | 56 | ok |
| yolo26l-1.dxnn | 12.1 | 82.94 | 65.03 | 17.90 | 56 | ok |
| yolo26x-1.dxnn | 8.0 | 125.46 | 98.37 | 27.09 | 57 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-1.dxnn | 25.8 | 38.80 | 38.80 | 55 | ok |
| yolo26s-1.dxnn | 21.8 | 45.84 | 45.84 | 56 | ok |
| yolo26m-1.dxnn | 18.6 | 53.80 | 53.80 | 56 | ok |
| yolo26l-1.dxnn | 15.2 | 65.84 | 65.84 | 56 | ok |
| yolo26x-1.dxnn | 10.3 | 96.74 | 96.74 | 58 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 23.6 | 42.32 | 33.11 | 9.22 | 54 | ok |
| yolo26s-pose.dxnn | 20.5 | 48.67 | 39.28 | 9.39 | 55 | ok |
| yolo26m-pose.dxnn | 16.7 | 60.05 | 50.79 | 9.26 | 56 | ok |
| yolo26l-pose.dxnn | 14.2 | 70.48 | 60.26 | 10.21 | 56 | ok |
| yolo26x-pose.dxnn | 9.8 | 102.21 | 90.65 | 11.56 | 57 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 33.5 | 29.83 | 29.83 | 55 | ok |
| yolo26s-pose.dxnn | 27.1 | 36.91 | 36.91 | 56 | ok |
| yolo26m-pose.dxnn | 21.6 | 46.24 | 46.24 | 56 | ok |
| yolo26l-pose.dxnn | 18.9 | 53.04 | 53.04 | 57 | ok |
| yolo26x-pose.dxnn | 12.4 | 80.47 | 80.47 | 58 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 14.6 | 68.32 | 51.30 | 17.02 | 54 | ok |
| yolo26s-seg.dxnn | 12.3 | 81.37 | 64.97 | 16.40 | 55 | ok |
| yolo26m-seg.dxnn | 9.0 | 110.94 | 77.13 | 33.81 | 56 | ok |
| yolo26l-seg.dxnn | 9.0 | 111.02 | 86.84 | 24.18 | 56 | ok |
| yolo26x-seg.dxnn | 6.7 | 149.12 | 129.56 | 19.56 | 59 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 20.1 | 49.74 | 49.74 | 55 | ok |
| yolo26s-seg.dxnn | 16.6 | 60.41 | 60.41 | 56 | ok |
| yolo26m-seg.dxnn | 13.1 | 76.50 | 76.50 | 57 | ok |
| yolo26l-seg.dxnn | 11.8 | 85.05 | 85.05 | 57 | ok |
| yolo26x-seg.dxnn | 7.8 | 127.93 | 127.93 | 59 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 15.5 | 64.45 | 50.92 | 13.52 | 55 | ok |
| yolo26s-obb.dxnn | 11.6 | 86.09 | 68.13 | 17.96 | 55 | ok |
| yolo26m-obb.dxnn | 9.5 | 104.94 | 89.15 | 15.79 | 56 | ok |
| yolo26l-obb.dxnn | 7.8 | 128.13 | 114.14 | 13.98 | 58 | ok |
| yolo26x-obb.dxnn | 4.9 | 203.79 | 187.10 | 16.69 | 59 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 20.3 | 49.30 | 49.30 | 55 | ok |
| yolo26s-obb.dxnn | 14.8 | 67.78 | 67.78 | 55 | ok |
| yolo26m-obb.dxnn | 11.5 | 87.15 | 87.15 | 56 | ok |
| yolo26l-obb.dxnn | 9.3 | 107.50 | 107.50 | 58 | ok |
| yolo26x-obb.dxnn | 5.5 | 181.52 | 181.52 | 61 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 430.3 | 2.32 | 2.32 | N/A | 54 | ok |
| yolo26s-cls.dxnn | 318.4 | 3.14 | 3.14 | N/A | 55 | ok |
| yolo26m-cls.dxnn | 258.9 | 3.86 | 3.86 | N/A | 55 | ok |
| yolo26l-cls.dxnn | 186.7 | 5.36 | 5.36 | N/A | 55 | ok |
| yolo26x-cls.dxnn | 122.4 | 8.17 | 8.17 | N/A | 55 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 400.7 | 2.50 | 2.50 | 55 | ok |
| yolo26s-cls.dxnn | 305.0 | 3.28 | 3.28 | 55 | ok |
| yolo26m-cls.dxnn | 244.1 | 4.10 | 4.10 | 55 | ok |
| yolo26l-cls.dxnn | 183.0 | 5.46 | 5.46 | 55 | ok |
| yolo26x-cls.dxnn | 125.5 | 7.97 | 7.97 | 55 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | mppvideodec | 3455 | 3 | 161.4 ±1.2 | 21.41 | 302 | 52.2 | 90.5 | 63~66 | 1000 | 159 | ok |
| yolo26s-1.dxnn | mppvideodec | 3455 | 3 | 112.7 ±0.4 | 30.66 | 229 | 80.8 | 96.8 | 72~76 | 1000 | 173 | ok |
| yolo26m-1.dxnn | mppvideodec | 3455 | 3 | 74.9 ±5.6 | 46.13 | 189 | 90.7 | 100.0 | 84~85 | 600~1000 | 194 | ok |
| yolo26l-1.dxnn | mppvideodec | 3455 | 3 | 56.3 ±4.4 | 61.41 | 166 | 93.5 | 100.0 | 83~84 | 600~1000 | 204 | ok |
| yolo26x-1.dxnn | mppvideodec | 3455 | 3 | 28.0 ±0.1 | 123.57 | 113 | 95.2 | 100.0 | 84~85 | 600~1000 | 318 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | mppvideodec | 3455 | 3 | 98.8 ±0.4 | 34.96 | 233 | 28.7 | 91.6 | 64~65 | 1000 | 173 | ok |
| yolo26s-1.dxnn | mppvideodec | 3455 | 3 | 98.8 ±0.6 | 34.96 | 230 | 62.1 | 94.6 | 71~74 | 1000 | 191 | ok |
| yolo26m-1.dxnn | mppvideodec | 3455 | 3 | 75.7 ±6.0 | 45.63 | 221 | 93.4 | 100.0 | 83~85 | 600~1000 | 207 | ok |
| yolo26l-1.dxnn | mppvideodec | 3455 | 3 | 57.5 ±4.0 | 60.08 | 194 | 94.1 | 100.0 | 83~84 | 600~1000 | 214 | ok |
| yolo26x-1.dxnn | mppvideodec | 3455 | 3 | 27.9 ±0.5 | 123.65 | 134 | 94.6 | 100.0 | 84~85 | 400~1000 | 318 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-1.dxnn | 161.4 | 98.8 | +62.6 | +63.3% |
| yolo26s-1.dxnn | 112.7 | 98.8 | +13.8 | +14.0% |
| yolo26m-1.dxnn | 74.9 | 75.7 | -0.8 | -1.1% |
| yolo26l-1.dxnn | 56.3 | 57.5 | -1.2 | -2.2% |
| yolo26x-1.dxnn | 28.0 | 27.9 | +0.0 | +0.1% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 201.7 ±0.2 | 17.13 | 302 | 74.1 | 88.1 | 67~70 | 1000 | 146 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 129.8 ±0.2 | 26.63 | 216 | 88.7 | 100.0 | 73~76 | 1000 | 163 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 78.4 ±7.2 | 44.09 | 173 | 92.4 | 100.0 | 83~84 | 400~1000 | 184 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 59.7 ±4.9 | 57.91 | 148 | 94.2 | 100.0 | 84 | 600~1000 | 194 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 29.2 ±0.5 | 118.37 | 98 | 95.1 | 100.0 | 83~84 | 400~1000 | 328 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 219.0 ±1.1 | 15.77 | 235 | 83.3 | 99.2 | 69~70 | 1000 | 140 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 131.1 ±0.1 | 26.36 | 181 | 89.7 | 100.0 | 72~75 | 1000 | 152 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 80.5 ±6.5 | 42.93 | 141 | 93.4 | 100.0 | 83~84 | 600~1000 | 174 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 60.6 ±4.7 | 57.02 | 116 | 93.2 | 100.0 | 83~84 | 600~1000 | 184 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 29.5 ±0.6 | 117.03 | 78 | 95.0 | 100.0 | 83~84 | 400~1000 | 328 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 201.7 | 219.0 | -17.4 | -7.9% |
| yolo26s-pose.dxnn | 129.8 | 131.1 | -1.3 | -1.0% |
| yolo26m-pose.dxnn | 78.4 | 80.5 | -2.1 | -2.6% |
| yolo26l-pose.dxnn | 59.7 | 60.6 | -0.9 | -1.5% |
| yolo26x-pose.dxnn | 29.2 | 29.5 | -0.3 | -1.1% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 105.6 ±1.7 | 32.70 | 419 | 37.9 | 67.3 | 66~70 | 1000 | 255 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 88.4 ±0.9 | 39.09 | 333 | 67.7 | 85.2 | 76~81 | 1000 | 270 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 41.1 ±0.6 | 84.07 | 186 | 92.7 | 100.0 | 84 | 400~800 | 299 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 35.3 ±0.7 | 97.93 | 171 | 93.2 | 100.0 | 83~84 | 400~1000 | 314 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 17.9 ±0.4 | 193.60 | 108 | 92.1 | 100.0 | 83 | 400~1000 | 376 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 85.8 ±0.3 | 40.28 | 311 | 28.4 | 82.5 | 67~69 | 1000 | 292 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 84.2 ±0.4 | 41.04 | 304 | 60.0 | 93.3 | 76~79 | 1000 | 308 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 39.4 ±10.4 | 87.67 | 179 | 73.2 | 100.0 | 77~84 | 400~1000 | 320 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 35.4 ±0.4 | 97.60 | 171 | 93.7 | 100.0 | 84 | 400~1000 | 321 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 17.7 ±0.1 | 195.53 | 109 | 92.0 | 100.0 | 83 | 400~1000 | 386 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 105.6 | 85.8 | +19.9 | +23.2% |
| yolo26s-seg.dxnn | 88.4 | 84.2 | +4.2 | +5.0% |
| yolo26m-seg.dxnn | 41.1 | 39.4 | +1.7 | +4.3% |
| yolo26l-seg.dxnn | 35.3 | 35.4 | -0.1 | -0.3% |
| yolo26x-seg.dxnn | 17.9 | 17.7 | +0.2 | +1.0% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 76.6 ±0.0 | 34.47 | 187 | 90.9 | 100.0 | 68~71 | 1000 | 183 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 44.4 ±0.0 | 59.45 | 137 | 94.3 | 100.0 | 69~70 | 1000 | 198 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 31.5 ±0.6 | 83.92 | 111 | 93.0 | 100.0 | 80~83 | 600~1000 | 224 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 22.9 ±0.5 | 115.25 | 97 | 93.3 | 100.0 | 83~84 | 600~1000 | 235 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 11.4 ±0.1 | 231.57 | 57 | 90.3 | 100.0 | 84 | 400~1000 | 341 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 76.5 ±0.1 | 34.51 | 207 | 91.4 | 100.0 | 68~71 | 1000 | 178 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 44.5 ±0.0 | 59.40 | 143 | 94.4 | 100.0 | 68~71 | 1000 | 191 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 31.5 ±0.5 | 83.80 | 126 | 93.3 | 100.0 | 81~83 | 600~1000 | 218 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 23.3 ±0.5 | 113.52 | 107 | 94.2 | 100.0 | 82~84 | 600~1000 | 229 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 11.2 ±0.1 | 236.24 | 57 | 90.3 | 100.0 | 84 | 400~1000 | 341 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 76.6 | 76.5 | +0.1 | +0.1% |
| yolo26s-obb.dxnn | 44.4 | 44.5 | -0.0 | -0.1% |
| yolo26m-obb.dxnn | 31.5 | 31.5 | -0.0 | -0.1% |
| yolo26l-obb.dxnn | 22.9 | 23.3 | -0.4 | -1.5% |
| yolo26x-obb.dxnn | 11.4 | 11.2 | +0.2 | +2.0% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 1050.3 ±8.4 | 3.29 | 187 | 17.2 | 53.0 | 57~58 | 1000 | 55 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 1044.8 ±4.3 | 3.31 | 186 | 29.8 | 67.7 | 59~60 | 1000 | 68 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 1060.0 ±7.5 | 3.26 | 184 | 37.6 | 85.4 | 66 | 1000 | 91 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 842.2 ±2.2 | 4.10 | 161 | 58.0 | 98.8 | 64~65 | 1000 | 104 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 455.4 ±0.9 | 7.59 | 126 | 71.4 | 99.8 | 69 | 1000 | 180 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 1050.7 ±5.9 | 3.29 | 185 | 18.4 | 52.2 | 58 | 1000 | 55 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 1050.0 ±6.2 | 3.29 | 185 | 27.9 | 68.2 | 60 | 1000 | 68 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 1055.8 ±6.0 | 3.27 | 184 | 48.0 | 84.5 | 66 | 1000 | 91 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 840.6 ±1.3 | 4.11 | 161 | 53.9 | 99.0 | 64~65 | 1000 | 104 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 453.9 ±1.6 | 7.61 | 124 | 68.9 | 100.0 | 68~69 | 1000 | 180 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 1050.3 | 1050.7 | -0.3 | -0.0% |
| yolo26s-cls.dxnn | 1044.8 | 1050.0 | -5.2 | -0.5% |
| yolo26m-cls.dxnn | 1060.0 | 1055.8 | +4.2 | +0.4% |
| yolo26l-cls.dxnn | 842.2 | 840.6 | +1.5 | +0.2% |
| yolo26x-cls.dxnn | 455.4 | 453.9 | +1.5 | +0.3% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 5 | 3 | 160.9 ±1.2 | 32.2 | 310 | 56.8 | 89.8 | 74~78 | 1000 | 189 | ok |
| yolo26n-1.dxnn | 6 | 3 | 160.8 ±0.5 | 26.8 | 312 | 57.1 | 89.4 | 78~80 | 1000 | 193 | ok |
| yolo26s-1.dxnn | 3 | 3 | 114.2 ±0.3 | 38.0 | 235 | 86.2 | 97.1 | 81~84 | 1000 | 192 | ok |
| yolo26s-1.dxnn | 4 | 3 | 111.5 ±1.9 | 27.9 | 231 | 87.0 | 98.4 | 85 | 600~1000 | 198 | ok |
| yolo26m-1.dxnn | 2 | 3 | 63.5 ±1.6 | 31.7 | 174 | 94.5 | 100.0 | 84~85 | 400~1000 | 206 | ok |
| yolo26m-1.dxnn | 3 | 3 | 61.3 ±0.5 | 20.4 | 172 | 94.5 | 100.0 | 84 | 400~1000 | 213 | ok |
| yolo26l-1.dxnn | 1 | 3 | 56.3 ±4.4 | 56.3 | 166 | 93.5 | 100.0 | 84~91 | 600~1000 | 204 | ok |
| yolo26l-1.dxnn | 2 | 3 | 51.3 ±0.1 | 25.7 | 160 | 96.4 | 100.0 | 84~85 | 600~1000 | 216 | ok |
| yolo26x-1.dxnn | 1 | 3 | 28.0 ±0.1 | 28.0 | 113 | 95.2 | 100.0 | 84~85 | 600~1000 | 318 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-1.dxnn | 3 | 3 | 98.8 ±0.1 | 33.0 | 237 | 30.0 | 91.6 | 66~67 | 1000 | 201 | ok |
| yolo26n-1.dxnn | 4 | 3 | 99.0 ±0.2 | 24.8 | 242 | 30.3 | 91.5 | 68 | 1000 | 209 | ok |
| yolo26s-1.dxnn | 3 | 3 | 98.3 ±0.1 | 32.8 | 237 | 64.5 | 92.8 | 77~79 | 1000 | 215 | ok |
| yolo26s-1.dxnn | 4 | 3 | 98.1 ±0.1 | 24.5 | 236 | 64.5 | 89.7 | 79~81 | 1000 | 224 | ok |
| yolo26m-1.dxnn | 2 | 3 | 63.6 ±1.1 | 31.8 | 203 | 95.7 | 100.0 | 84~85 | 400~1000 | 218 | ok |
| yolo26m-1.dxnn | 3 | 3 | 62.9 ±1.1 | 21.0 | 202 | 95.6 | 100.0 | 84~85 | 400~1000 | 228 | ok |
| yolo26l-1.dxnn | 1 | 3 | 57.5 ±4.0 | 57.5 | 194 | 94.1 | 100.0 | 84~91 | 400~1000 | 214 | ok |
| yolo26l-1.dxnn | 2 | 3 | 51.1 ±0.7 | 25.6 | 181 | 96.4 | 100.0 | 84 | 400~1000 | 221 | ok |
| yolo26x-1.dxnn | 1 | 3 | 27.9 ±0.5 | 27.9 | 134 | 94.6 | 100.0 | 84~85 | 400~1000 | 318 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-1.dxnn | 5 | 32.2 | 3 | 33.0 |
| yolo26s-1.dxnn | 3 | 38.0 | 3 | 32.8 |
| yolo26m-1.dxnn | 2 | 31.7 | 2 | 31.8 |
| yolo26l-1.dxnn | 1 | 56.3 | 1 | 57.5 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 6 | 3 | 200.6 ±0.2 | 33.4 | 316 | 80.1 | 87.7 | 78~82 | 1000 | 189 | ok |
| yolo26n-pose.dxnn | 7 | 3 | 201.0 ±0.4 | 28.7 | 314 | 81.0 | 87.9 | 83~84 | 1000 | 198 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 128.2 ±2.1 | 32.1 | 220 | 95.5 | 100.0 | 82~85 | 800~1000 | 191 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 124.2 ±1.1 | 24.8 | 216 | 95.7 | 100.0 | 85 | 800~1000 | 197 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 67.5 ±1.8 | 33.8 | 164 | 95.4 | 100.0 | 83~84 | 400~1000 | 200 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 65.9 ±0.9 | 22.0 | 163 | 95.5 | 100.0 | 83~84 | 400~1000 | 206 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 59.7 ±4.9 | 59.7 | 148 | 94.2 | 100.0 | 83~92 | 400~1000 | 194 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 54.1 ±0.5 | 27.1 | 137 | 96.7 | 100.0 | 83~84 | 400~1000 | 209 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 29.2 ±0.5 | 29.2 | 98 | 95.1 | 100.0 | 83~84 | 400~1000 | 328 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 219.6 ±0.3 | 31.4 | 244 | 94.5 | 99.3 | 80~83 | 1000 | 196 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 219.8 ±0.1 | 27.5 | 245 | 94.7 | 99.9 | 83~84 | 1000 | 198 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 130.9 ±0.3 | 32.7 | 184 | 96.7 | 100.0 | 81~84 | 800~1000 | 187 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 129.2 ±0.4 | 25.8 | 182 | 96.4 | 100.0 | 84 | 800~1000 | 196 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 69.9 ±2.8 | 34.9 | 131 | 95.9 | 100.0 | 83~85 | 400~1000 | 188 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 68.2 ±0.1 | 22.7 | 131 | 96.6 | 100.0 | 84 | 400~1000 | 197 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 54.7 ±0.6 | 27.3 | 108 | 96.1 | 100.0 | 84 | 600~1000 | 196 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 60.6 ±4.7 | 60.6 | 116 | 93.2 | 100.0 | 83~92 | 600~1000 | 184 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 29.5 ±0.6 | 29.5 | 78 | 95.0 | 100.0 | 83~84 | 400~1000 | 328 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 6 | 33.4 | 7 | 31.4 |
| yolo26s-pose.dxnn | 4 | 32.1 | 4 | 32.7 |
| yolo26m-pose.dxnn | 2 | 33.8 | 2 | 34.9 |
| yolo26l-pose.dxnn | 1 | 59.7 | 1 | 60.6 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 105.0 ±0.6 | 35.0 | 429 | 39.6 | 67.8 | 75~78 | 1000 | 286 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 103.9 ±1.8 | 26.0 | 426 | 39.5 | 67.7 | 79~80 | 1000 | 296 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 84.8 ±2.8 | 42.4 | 321 | 77.3 | 98.5 | 85~86 | 400~1000 | 290 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 77.4 ±1.4 | 25.8 | 299 | 85.4 | 99.8 | 85 | 400~1000 | 307 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 41.1 ±0.6 | 41.1 | 186 | 92.7 | 100.0 | 84~92 | 400~800 | 299 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 39.4 ±0.4 | 19.7 | 178 | 95.3 | 100.0 | 84 | 400~800 | 325 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 35.3 ±0.7 | 35.3 | 171 | 93.2 | 100.0 | 83~92 | 400~1000 | 314 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 33.9 ±0.3 | 16.9 | 167 | 95.0 | 100.0 | 84 | 400~1000 | 327 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 17.9 ±0.4 | 17.9 | 108 | 92.1 | 100.0 | 83 | 400~1000 | 376 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 2 | 3 | 85.9 ±0.7 | 42.9 | 308 | 29.3 | 84.3 | 70~72 | 1000 | 317 | ok |
| yolo26n-seg.dxnn | 3 | 3 | 85.4 ±0.1 | 28.5 | 316 | 29.7 | 86.5 | 72~74 | 1000 | 331 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 82.8 ±1.7 | 41.4 | 310 | 65.7 | 99.0 | 82~85 | 600~1000 | 333 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 77.8 ±1.8 | 25.9 | 295 | 85.8 | 100.0 | 85 | 400~1000 | 348 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 39.4 ±10.4 | 39.4 | 179 | 73.2 | 100.0 | 83~92 | 400~1000 | 320 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 40.1 ±0.3 | 20.1 | 187 | 95.5 | 100.0 | 84 | 400~1000 | 342 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 35.4 ±0.4 | 35.4 | 171 | 93.7 | 100.0 | 83~93 | 400~800 | 321 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 34.1 ±0.1 | 17.0 | 172 | 95.1 | 100.0 | 84 | 400~800 | 344 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 17.7 ±0.1 | 17.7 | 109 | 92.0 | 100.0 | 83 | 400~1000 | 386 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 35.0 | 2 | 42.9 |
| yolo26s-seg.dxnn | 2 | 42.4 | 2 | 41.4 |
| yolo26m-seg.dxnn | 1 | 41.1 | 1 | 39.4 |
| yolo26l-seg.dxnn | 1 | 35.3 | 1 | 35.4 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.5 ±0.0 | 38.3 | 191 | 94.3 | 100.0 | 73~76 | 1000 | 202 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.5 ±0.0 | 25.5 | 191 | 96.5 | 100.0 | 76~78 | 1000 | 214 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.4 ±0.0 | 44.4 | 137 | 94.3 | 100.0 | 70~78 | 1000 | 198 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.4 ±0.0 | 22.2 | 140 | 96.8 | 100.0 | 71~72 | 1000 | 218 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.5 ±0.6 | 31.5 | 111 | 93.0 | 100.0 | 82~92 | 400~1000 | 224 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.7 ±1.2 | 14.9 | 112 | 94.7 | 100.0 | 83~84 | 400~1000 | 241 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 22.9 ±0.5 | 22.9 | 97 | 93.3 | 100.0 | 83~84 | 600~1000 | 235 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 11.4 ±0.1 | 11.4 | 57 | 90.3 | 100.0 | 84 | 400~1000 | 341 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 76.5 ±0.0 | 38.3 | 210 | 95.5 | 100.0 | 72~75 | 1000 | 196 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 76.5 ±0.0 | 25.5 | 203 | 96.4 | 100.0 | 75~76 | 1000 | 205 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 44.5 ±0.0 | 44.5 | 143 | 94.4 | 100.0 | 72~79 | 1000 | 191 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 44.4 ±0.0 | 22.2 | 147 | 96.8 | 100.0 | 72~73 | 1000 | 207 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 31.5 ±0.5 | 31.5 | 126 | 93.3 | 100.0 | 82~92 | 600~1000 | 218 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 29.5 ±0.9 | 14.7 | 125 | 94.5 | 100.0 | 83~84 | 600~1000 | 234 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 23.3 ±0.5 | 23.3 | 107 | 94.2 | 100.0 | 82~84 | 600~1000 | 229 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 11.2 ±0.1 | 11.2 | 57 | 90.3 | 100.0 | 84 | 400~1000 | 341 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 38.3 | 2 | 38.3 |
| yolo26s-obb.dxnn | 1 | 44.4 | 1 | 44.5 |
| yolo26m-obb.dxnn | 1 | 31.5 | 1 | 31.5 |

---
*Report generated by dx_stream benchmark tool*
