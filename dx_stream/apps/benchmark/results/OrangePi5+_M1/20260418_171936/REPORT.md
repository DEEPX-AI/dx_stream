# YOLO26 Benchmark Report

**Generated:** 2026-04-19 09:04:27 (Local)

## Test Timing

| # | Type | Start | End | Duration |
|---|------|-------|-----|----------|
| 1 | run | 2026-04-18 17:19:36 | 2026-04-19 09:04:27 | 15h 44m 50s |

## Executive Summary

### Object Detection

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n.dxnn | ON | 39.21 | 181.1 | 170.2 | 5 |
| yolo26n.dxnn | OFF | 36.21 | 237.6 | 96.7 | 3 |
| yolo26s.dxnn | ON | 50.20 | 135.2 | 136.1 | 4 |
| yolo26s.dxnn | OFF | 39.72 | 139.9 | 97.1 | 3 |
| yolo26m.dxnn | ON | 59.98 | 96.3 | 83.3 | 2 |
| yolo26m.dxnn | OFF | 50.74 | 96.4 | 80.8 | 2 |
| yolo26l.dxnn | ON | 68.92 | 71.5 | 57.6 | 1 |
| yolo26l.dxnn | OFF | 58.68 | 70.8 | 58.1 | 1 |
| yolo26x.dxnn | ON | 94.22 | 39.5 | 27.5 | — |
| yolo26x.dxnn | OFF | 86.77 | 39.5 | 27.3 | — |

### Pose Estimation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-pose.dxnn | ON | 29.85 | 221.1 | 224.9 | 7 |
| yolo26n-pose.dxnn | OFF | 26.61 | 232.5 | 231.6 | 7 |
| yolo26s-pose.dxnn | ON | 43.07 | 134.7 | 134.3 | 4 |
| yolo26s-pose.dxnn | OFF | 33.08 | 134.8 | 134.4 | 4 |
| yolo26m-pose.dxnn | ON | 47.63 | 93.4 | 73.5 | 2 |
| yolo26m-pose.dxnn | OFF | 40.95 | 92.9 | 76.2 | 2 |
| yolo26l-pose.dxnn | ON | 61.28 | 69.5 | 57.4 | 1 |
| yolo26l-pose.dxnn | OFF | 49.34 | 69.0 | 58.2 | 1 |
| yolo26x-pose.dxnn | ON | 87.53 | 38.9 | 27.4 | — |
| yolo26x-pose.dxnn | OFF | 76.91 | 38.9 | 27.8 | — |

### Segmentation

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-seg.dxnn | ON | 52.95 | 128.4 | 115.0 | 3 |
| yolo26n-seg.dxnn | OFF | 49.17 | 166.6 | 87.5 | 2 |
| yolo26s-seg.dxnn | ON | 67.44 | 103.0 | 100.2 | 2 |
| yolo26s-seg.dxnn | OFF | 59.14 | 108.9 | 84.8 | 2 |
| yolo26m-seg.dxnn | ON | 79.30 | 60.0 | 39.8 | 1 |
| yolo26m-seg.dxnn | OFF | 73.83 | 61.9 | 42.8 | 1 |
| yolo26l-seg.dxnn | ON | 89.14 | 53.3 | 38.2 | 1 |
| yolo26l-seg.dxnn | OFF | 82.32 | 52.5 | 36.9 | 1 |
| yolo26x-seg.dxnn | ON | 129.55 | 27.8 | 22.7 | — |
| yolo26x-seg.dxnn | OFF | 123.48 | 30.2 | 24.9 | — |

### Oriented BBox (OBB)

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-obb.dxnn | ON | 50.88 | 81.1 | 81.2 | 2 |
| yolo26n-obb.dxnn | OFF | 40.49 | 80.6 | 80.3 | 2 |
| yolo26s-obb.dxnn | ON | 68.56 | 46.8 | 46.8 | 1 |
| yolo26s-obb.dxnn | OFF | 61.02 | 46.8 | 46.9 | 1 |
| yolo26m-obb.dxnn | ON | 90.61 | 34.1 | 34.3 | 1 |
| yolo26m-obb.dxnn | OFF | 79.27 | 34.2 | 34.3 | 1 |
| yolo26l-obb.dxnn | ON | 109.58 | 24.9 | 25.1 | — |
| yolo26l-obb.dxnn | OFF | 99.49 | 25.1 | 25.0 | — |
| yolo26x-obb.dxnn | ON | 181.77 | 14.4 | 13.8 | — |
| yolo26x-obb.dxnn | OFF | 172.03 | 14.4 | 13.7 | — |

### Classification

| Model | ORT | Latency (ms) | Throughput (FPS) | E2E FPS | Max Channels |
|-------|-----|:------------:|:----------------:|:-------:|:------------:|
| yolo26n-cls.dxnn | ON | 2.84 | 3411.2 | 1071.3 | — |
| yolo26n-cls.dxnn | OFF | 2.85 | 3409.0 | 1071.6 | — |
| yolo26s-cls.dxnn | ON | 3.53 | 1935.5 | 1076.8 | — |
| yolo26s-cls.dxnn | OFF | 3.43 | 1933.7 | 1068.1 | — |
| yolo26m-cls.dxnn | ON | 4.12 | 1363.1 | 1072.1 | — |
| yolo26m-cls.dxnn | OFF | 3.94 | 1362.0 | 1073.8 | — |
| yolo26l-cls.dxnn | ON | 5.50 | 857.1 | 848.1 | — |
| yolo26l-cls.dxnn | OFF | 5.66 | 859.0 | 849.3 | — |
| yolo26x-cls.dxnn | ON | 7.99 | 464.4 | 461.5 | — |
| yolo26x-cls.dxnn | OFF | 8.35 | 463.5 | 461.2 | — |

## Environment

| Item | Value |
|------|-------|
| Hostname | orangepi5plus |
| OS | Debian GNU/Linux 12 (bookworm) |
| Kernel | 6.1.43-rockchip-rk3588 |
| CPU | Cortex-A55 |
| CPU Cores | 8 |
| RAM | 15.6 GB |
| NPU SKU | M1 |
| NPU RT | v3.3.0 |
| NPU Driver (RT) | v2.4.0 |
| NPU Driver (PCIe) | v2.2.0 |
| NPU Firmware | v2.5.6 |
| NPU Memory | LPDDR5x 6000 Mbps, 3.92GiB |
| NPU Board | M.2, Rev 1.0 |
| NPU PCIe | Gen3 X4 [01:00:00] |

### Tools

| Tool | Available | Version |
|------|-----------|---------|
| run_model | Yes | DXRT v3.3.0 run_model |
| gst-launch-1.0 | Yes | gst-launch-1.0 version 1.22.0 |
| gst-inspect-1.0 | Yes | gst-inspect-1.0 version 1.22.0 |
| dxtop | Yes | DX-TOP 1.1.0 |
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
| yolo26n.dxnn | 181.1 ±1.9 | 232 | 52.8 | 89.2 | 55~60 | 1000 | ok |
| yolo26s.dxnn | 135.2 ±0.4 | 201 | 86.6 | 98.4 | 66~71 | 1000 | ok |
| yolo26m.dxnn | 96.3 ±0.2 | 175 | 87.7 | 100.0 | 72~80 | 1000 | ok |
| yolo26l.dxnn | 71.5 ±0.2 | 162 | 91.5 | 100.0 | 73~81 | 1000 | ok |
| yolo26x.dxnn | 39.5 ±1.2 | 116 | 88.7 | 100.0 | 76~83 | 800~1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n.dxnn | 237.6 ±0.1 | 183 | 88.7 | 99.3 | 65~69 | 1000 | ok |
| yolo26s.dxnn | 139.9 ±0.4 | 144 | 89.4 | 100.0 | 67~72 | 1000 | ok |
| yolo26m.dxnn | 96.4 ±0.7 | 126 | 89.4 | 100.0 | 74~82 | 800~1000 | ok |
| yolo26l.dxnn | 70.8 ±0.4 | 124 | 91.3 | 100.0 | 73~80 | 1000 | ok |
| yolo26x.dxnn | 39.5 ±1.8 | 87 | 88.6 | 100.0 | 76~83 | 600~1000 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 221.1 ±1.3 | 223 | 83.6 | 94.1 | 65~70 | 1000 | ok |
| yolo26s-pose.dxnn | 134.7 ±0.2 | 184 | 91.0 | 100.0 | 68~74 | 1000 | ok |
| yolo26m-pose.dxnn | 93.4 ±1.3 | 160 | 91.3 | 100.0 | 74~82 | 800~1000 | ok |
| yolo26l-pose.dxnn | 69.5 ±0.4 | 131 | 90.1 | 100.0 | 73~80 | 1000 | ok |
| yolo26x-pose.dxnn | 38.9 ±1.3 | 104 | 88.7 | 100.0 | 76~83 | 800~1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-pose.dxnn | 232.5 ±0.3 | 159 | 91.0 | 100.0 | 67~72 | 1000 | ok |
| yolo26s-pose.dxnn | 134.8 ±0.2 | 135 | 89.4 | 100.0 | 68~73 | 1000 | ok |
| yolo26m-pose.dxnn | 92.9 ±1.3 | 126 | 88.8 | 100.0 | 74~82 | 600~1000 | ok |
| yolo26l-pose.dxnn | 69.0 ±0.4 | 102 | 88.7 | 100.0 | 73~80 | 1000 | ok |
| yolo26x-pose.dxnn | 38.9 ±1.2 | 62 | 87.5 | 100.0 | 76~83 | 800~1000 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 128.4 ±1.2 | 296 | 45.1 | 77.8 | 62~67 | 1000 | ok |
| yolo26s-seg.dxnn | 103.0 ±0.8 | 248 | 81.8 | 94.4 | 70~77 | 1000 | ok |
| yolo26m-seg.dxnn | 60.0 ±9.0 | 172 | 87.1 | 100.0 | 79~84 | 400~1000 | ok |
| yolo26l-seg.dxnn | 53.3 ±2.6 | 164 | 88.6 | 100.0 | 76~83 | 800~1000 | ok |
| yolo26x-seg.dxnn | 27.8 ±2.1 | 110 | 87.1 | 100.0 | 78~82 | 600~1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-seg.dxnn | 166.6 ±0.4 | 235 | 72.4 | 85.7 | 67~73 | 1000 | ok |
| yolo26s-seg.dxnn | 108.9 ±0.3 | 178 | 88.1 | 100.0 | 70~77 | 1000 | ok |
| yolo26m-seg.dxnn | 61.9 ±8.5 | 142 | 88.1 | 100.0 | 79~82 | 400~1000 | ok |
| yolo26l-seg.dxnn | 52.5 ±3.2 | 129 | 88.8 | 100.0 | 77~82 | 600~1000 | ok |
| yolo26x-seg.dxnn | 30.2 ±0.9 | 92 | 88.6 | 100.0 | 72~78 | 800~1000 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 81.1 ±0.1 | 160 | 92.1 | 100.0 | 59~60 | 1000 | ok |
| yolo26s-obb.dxnn | 46.8 ±0.1 | 125 | 90.6 | 100.0 | 60~62 | 1000 | ok |
| yolo26m-obb.dxnn | 34.1 ±0.2 | 105 | 88.9 | 100.0 | 66~70 | 1000 | ok |
| yolo26l-obb.dxnn | 24.9 ±0.1 | 84 | 87.6 | 100.0 | 64~68 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.0 | 50 | 87.7 | 100.0 | 67~71 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-obb.dxnn | 80.6 ±0.1 | 131 | 89.3 | 100.0 | 59~61 | 1000 | ok |
| yolo26s-obb.dxnn | 46.8 ±0.1 | 83 | 88.7 | 100.0 | 60~63 | 1000 | ok |
| yolo26m-obb.dxnn | 34.2 ±0.2 | 62 | 89.8 | 100.0 | 66~72 | 1000 | ok |
| yolo26l-obb.dxnn | 25.1 ±0.1 | 45 | 88.1 | 100.0 | 65~69 | 1000 | ok |
| yolo26x-obb.dxnn | 14.4 ±0.1 | 27 | 87.2 | 100.0 | 68~72 | 1000 | ok |

#### Classification

**ORT = ON**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3411.2 ±4.8 | 149 | 84.4 | 93.5 | 57 | 1000 | ok |
| yolo26s-cls.dxnn | 1935.5 ±2.0 | 107 | 88.8 | 96.9 | 56~58 | 1000 | ok |
| yolo26m-cls.dxnn | 1363.1 ±0.4 | 82 | 89.2 | 97.4 | 63~68 | 1000 | ok |
| yolo26l-cls.dxnn | 857.1 ±0.4 | 69 | 90.0 | 98.4 | 62~64 | 1000 | ok |
| yolo26x-cls.dxnn | 464.4 ±0.6 | 42 | 90.6 | 99.6 | 64~68 | 1000 | ok |

**ORT = OFF**

| Model | FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | Status |
|-------|-----|------|----------|----------|-------------|---------|--------|
| yolo26n-cls.dxnn | 3409.0 ±5.4 | 149 | 84.5 | 93.5 | 54~56 | 1000 | ok |
| yolo26s-cls.dxnn | 1933.7 ±1.2 | 107 | 88.9 | 96.5 | 56~58 | 1000 | ok |
| yolo26m-cls.dxnn | 1362.0 ±1.2 | 81 | 89.9 | 97.6 | 65~68 | 1000 | ok |
| yolo26l-cls.dxnn | 859.0 ±0.2 | 66 | 89.1 | 98.4 | 62~64 | 1000 | ok |
| yolo26x-cls.dxnn | 463.5 ±0.9 | 47 | 90.8 | 99.3 | 64~67 | 1000 | ok |

### Latency (Single-Core, Sync)

#### Object Detection

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n.dxnn | 25.5 | 39.21 | 32.83 | 6.37 | 46 | ok |
| yolo26s.dxnn | 19.9 | 50.20 | 45.04 | 5.16 | 55 | ok |
| yolo26m.dxnn | 16.7 | 59.98 | 51.96 | 8.02 | 55 | ok |
| yolo26l.dxnn | 14.5 | 68.92 | 62.76 | 6.16 | 57 | ok |
| yolo26x.dxnn | 10.6 | 94.22 | 88.67 | 5.54 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n.dxnn | 27.6 | 36.21 | 36.21 | 55 | ok |
| yolo26s.dxnn | 25.2 | 39.72 | 39.72 | 55 | ok |
| yolo26m.dxnn | 19.7 | 50.74 | 50.74 | 57 | ok |
| yolo26l.dxnn | 17.0 | 58.68 | 58.68 | 57 | ok |
| yolo26x.dxnn | 11.5 | 86.77 | 86.77 | 59 | ok |

#### Pose Estimation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-pose.dxnn | 33.5 | 29.85 | 26.81 | 3.05 | 55 | ok |
| yolo26s-pose.dxnn | 23.2 | 43.07 | 39.84 | 3.23 | 55 | ok |
| yolo26m-pose.dxnn | 21.0 | 47.63 | 43.72 | 3.92 | 56 | ok |
| yolo26l-pose.dxnn | 16.3 | 61.28 | 57.94 | 3.33 | 57 | ok |
| yolo26x-pose.dxnn | 11.4 | 87.53 | 84.22 | 3.31 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-pose.dxnn | 37.6 | 26.61 | 26.61 | 56 | ok |
| yolo26s-pose.dxnn | 30.2 | 33.08 | 33.08 | 56 | ok |
| yolo26m-pose.dxnn | 24.4 | 40.95 | 40.95 | 57 | ok |
| yolo26l-pose.dxnn | 20.3 | 49.34 | 49.34 | 57 | ok |
| yolo26x-pose.dxnn | 13.0 | 76.91 | 76.91 | 59 | ok |

#### Segmentation

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-seg.dxnn | 18.9 | 52.95 | 46.98 | 5.98 | 54 | ok |
| yolo26s-seg.dxnn | 14.8 | 67.44 | 61.63 | 5.81 | 56 | ok |
| yolo26m-seg.dxnn | 12.6 | 79.30 | 72.60 | 6.71 | 57 | ok |
| yolo26l-seg.dxnn | 11.2 | 89.14 | 83.31 | 5.83 | 56 | ok |
| yolo26x-seg.dxnn | 7.7 | 129.55 | 122.01 | 7.55 | 58 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-seg.dxnn | 20.3 | 49.17 | 49.17 | 56 | ok |
| yolo26s-seg.dxnn | 16.9 | 59.14 | 59.14 | 56 | ok |
| yolo26m-seg.dxnn | 13.5 | 73.83 | 73.83 | 57 | ok |
| yolo26l-seg.dxnn | 12.1 | 82.32 | 82.32 | 57 | ok |
| yolo26x-seg.dxnn | 8.1 | 123.48 | 123.48 | 54 | ok |

#### Oriented BBox (OBB)

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-obb.dxnn | 19.7 | 50.88 | 46.65 | 4.24 | 52 | ok |
| yolo26s-obb.dxnn | 14.6 | 68.56 | 63.83 | 4.72 | 53 | ok |
| yolo26m-obb.dxnn | 11.0 | 90.61 | 86.15 | 4.47 | 54 | ok |
| yolo26l-obb.dxnn | 9.1 | 109.58 | 104.97 | 4.61 | 53 | ok |
| yolo26x-obb.dxnn | 5.5 | 181.77 | 177.50 | 4.27 | 55 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-obb.dxnn | 24.7 | 40.49 | 40.49 | 54 | ok |
| yolo26s-obb.dxnn | 16.4 | 61.02 | 61.02 | 55 | ok |
| yolo26m-obb.dxnn | 12.6 | 79.27 | 79.27 | 55 | ok |
| yolo26l-obb.dxnn | 10.1 | 99.49 | 99.49 | 55 | ok |
| yolo26x-obb.dxnn | 5.8 | 172.03 | 172.03 | 56 | ok |

#### Classification

**ORT = ON**

| Model | FPS | Total ms | NPU ms | CPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|--------|-------------|--------|
| yolo26n-cls.dxnn | 352.5 | 2.84 | 2.84 | N/A | 54 | ok |
| yolo26s-cls.dxnn | 283.3 | 3.53 | 3.53 | N/A | 49 | ok |
| yolo26m-cls.dxnn | 242.9 | 4.12 | 4.12 | N/A | 50 | ok |
| yolo26l-cls.dxnn | 181.8 | 5.50 | 5.50 | N/A | 54 | ok |
| yolo26x-cls.dxnn | 125.2 | 7.99 | 7.99 | N/A | 54 | ok |

**ORT = OFF**

| Model | FPS | Total ms | NPU ms | NPU Temp °C | Status |
|-------|-----|----------|--------|-------------|--------|
| yolo26n-cls.dxnn | 350.6 | 2.85 | 2.85 | 50 | ok |
| yolo26s-cls.dxnn | 291.7 | 3.43 | 3.43 | 50 | ok |
| yolo26m-cls.dxnn | 253.9 | 3.94 | 3.94 | 54 | ok |
| yolo26l-cls.dxnn | 176.7 | 5.66 | 5.66 | 54 | ok |
| yolo26x-cls.dxnn | 119.8 | 8.35 | 8.35 | 54 | ok |

## E2E Pipeline (Single-Stream)

### Object Detection

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | mppvideodec | 3455 | 3 | 170.2 ±1.0 | 20.30 | 292 | 46.6 | 81.1 | 60~62 | 1000 | 148 | ok |
| yolo26s.dxnn | mppvideodec | 3455 | 3 | 136.1 ±0.7 | 25.39 | 241 | 84.2 | 99.5 | 72~75 | 1000 | 162 | ok |
| yolo26m.dxnn | mppvideodec | 3455 | 3 | 83.3 ±9.7 | 41.48 | 188 | 90.9 | 100.0 | 83~84 | 400~1000 | 183 | ok |
| yolo26l.dxnn | mppvideodec | 3455 | 3 | 57.6 ±5.2 | 59.99 | 164 | 93.4 | 100.0 | 84~85 | 400~1000 | 193 | ok |
| yolo26x.dxnn | mppvideodec | 3455 | 3 | 27.5 ±0.2 | 125.62 | 104 | 94.6 | 100.0 | 83~84 | 400~1000 | 314 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | mppvideodec | 3455 | 3 | 96.7 ±5.6 | 35.73 | 226 | 25.0 | 82.4 | 63 | 1000 | 164 | ok |
| yolo26s.dxnn | mppvideodec | 3455 | 3 | 97.1 ±5.3 | 35.59 | 228 | 49.0 | 83.8 | 69~70 | 1000 | 179 | ok |
| yolo26m.dxnn | mppvideodec | 3455 | 3 | 80.8 ±9.2 | 42.79 | 220 | 92.2 | 100.0 | 84~85 | 400~1000 | 199 | ok |
| yolo26l.dxnn | mppvideodec | 3455 | 3 | 58.1 ±5.6 | 59.41 | 186 | 93.8 | 100.0 | 84 | 400~1000 | 203 | ok |
| yolo26x.dxnn | mppvideodec | 3455 | 3 | 27.3 ±0.1 | 126.63 | 116 | 94.7 | 100.0 | 83~84 | 400~1000 | 314 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n.dxnn | 170.2 | 96.7 | +73.5 | +76.0% |
| yolo26s.dxnn | 136.1 | 97.1 | +39.0 | +40.2% |
| yolo26m.dxnn | 83.3 | 80.8 | +2.5 | +3.1% |
| yolo26l.dxnn | 57.6 | 58.1 | -0.6 | -1.0% |
| yolo26x.dxnn | 27.5 | 27.3 | +0.2 | +0.8% |

### Pose Estimation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 224.9 ±1.3 | 15.36 | 285 | 77.7 | 96.1 | 69~71 | 1000 | 138 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 134.3 ±0.2 | 25.72 | 202 | 88.4 | 100.0 | 74~78 | 1000 | 153 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 73.5 ±7.2 | 47.01 | 160 | 92.7 | 100.0 | 84 | 400~1000 | 175 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 57.4 ±5.1 | 60.18 | 139 | 93.8 | 100.0 | 84~85 | 400~1000 | 184 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 27.4 ±0.3 | 126.01 | 94 | 94.6 | 100.0 | 83~84 | 400~1000 | 325 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | mppvideodec | 3455 | 3 | 231.6 ±0.4 | 14.92 | 211 | 83.9 | 100.0 | 70~72 | 1000 | 130 | ok |
| yolo26s-pose.dxnn | mppvideodec | 3455 | 3 | 134.4 ±0.3 | 25.70 | 172 | 87.6 | 100.0 | 73~77 | 1000 | 144 | ok |
| yolo26m-pose.dxnn | mppvideodec | 3455 | 3 | 76.2 ±8.1 | 45.35 | 131 | 92.8 | 100.0 | 83~84 | 400~1000 | 166 | ok |
| yolo26l-pose.dxnn | mppvideodec | 3455 | 3 | 58.2 ±5.1 | 59.40 | 116 | 93.5 | 100.0 | 83~84 | 400~1000 | 174 | ok |
| yolo26x-pose.dxnn | mppvideodec | 3455 | 3 | 27.8 ±0.6 | 124.49 | 70 | 95.3 | 100.0 | 83~84 | 400~1000 | 325 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-pose.dxnn | 224.9 | 231.6 | -6.7 | -2.9% |
| yolo26s-pose.dxnn | 134.3 | 134.4 | -0.1 | -0.1% |
| yolo26m-pose.dxnn | 73.5 | 76.2 | -2.7 | -3.5% |
| yolo26l-pose.dxnn | 57.4 | 58.2 | -0.8 | -1.3% |
| yolo26x-pose.dxnn | 27.4 | 27.8 | -0.3 | -1.2% |

### Segmentation

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 115.0 ±1.3 | 30.04 | 372 | 38.9 | 67.5 | 68~71 | 1000 | 240 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 100.2 ±0.9 | 34.49 | 337 | 77.5 | 89.5 | 80~85 | 800~1000 | 257 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 39.8 ±0.1 | 86.78 | 174 | 96.1 | 100.0 | 85 | 400~800 | 282 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 38.2 ±1.3 | 90.35 | 166 | 94.8 | 100.0 | 83~84 | 400~1000 | 292 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 22.7 ±0.8 | 152.29 | 119 | 94.4 | 100.0 | 82~83 | 400~1000 | 362 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | mppvideodec | 3455 | 3 | 87.5 ±0.1 | 39.48 | 281 | 27.6 | 87.2 | 69~70 | 1000 | 280 | ok |
| yolo26s-seg.dxnn | mppvideodec | 3455 | 3 | 84.8 ±0.1 | 40.74 | 271 | 57.3 | 93.0 | 77~81 | 1000 | 296 | ok |
| yolo26m-seg.dxnn | mppvideodec | 3455 | 3 | 42.8 ±0.8 | 80.82 | 184 | 95.0 | 100.0 | 84 | 400~800 | 314 | ok |
| yolo26l-seg.dxnn | mppvideodec | 3455 | 3 | 36.9 ±0.7 | 93.64 | 168 | 94.5 | 100.0 | 84 | 400~1000 | 316 | ok |
| yolo26x-seg.dxnn | mppvideodec | 3455 | 3 | 24.9 ±0.1 | 138.86 | 130 | 94.6 | 100.0 | 81~82 | 600~1000 | 368 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-seg.dxnn | 115.0 | 87.5 | +27.5 | +31.5% |
| yolo26s-seg.dxnn | 100.2 | 84.8 | +15.4 | +18.1% |
| yolo26m-seg.dxnn | 39.8 | 42.8 | -2.9 | -6.9% |
| yolo26l-seg.dxnn | 38.2 | 36.9 | +1.3 | +3.6% |
| yolo26x-seg.dxnn | 22.7 | 24.9 | -2.2 | -8.8% |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 81.2 ±0.1 | 32.53 | 175 | 90.7 | 100.0 | 60~62 | 1000 | 162 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 46.8 ±0.0 | 56.36 | 132 | 94.1 | 100.0 | 63~65 | 1000 | 182 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 34.3 ±0.1 | 76.88 | 114 | 92.9 | 100.0 | 60~74 | 1000 | 204 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 25.1 ±0.0 | 105.29 | 96 | 94.7 | 100.0 | 74~76 | 1000 | 215 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 13.8 ±0.1 | 191.62 | 55 | 93.2 | 100.0 | 80~81 | 800~1000 | 334 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | mppvideodec | 2640 | 3 | 80.3 ±0.0 | 32.89 | 195 | 91.0 | 100.0 | 60~61 | 1000 | 160 | ok |
| yolo26s-obb.dxnn | mppvideodec | 2640 | 3 | 46.9 ±0.0 | 56.32 | 138 | 93.2 | 100.0 | 63~65 | 1000 | 178 | ok |
| yolo26m-obb.dxnn | mppvideodec | 2640 | 3 | 34.3 ±0.1 | 76.87 | 124 | 94.7 | 100.0 | 75~80 | 1000 | 198 | ok |
| yolo26l-obb.dxnn | mppvideodec | 2640 | 3 | 25.0 ±0.1 | 105.41 | 101 | 94.5 | 100.0 | 75~78 | 1000 | 209 | ok |
| yolo26x-obb.dxnn | mppvideodec | 2640 | 3 | 13.7 ±0.2 | 192.12 | 56 | 92.2 | 100.0 | 80 | 800~1000 | 334 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-obb.dxnn | 81.2 | 80.3 | +0.9 | +1.1% |
| yolo26s-obb.dxnn | 46.8 | 46.9 | -0.0 | -0.1% |
| yolo26m-obb.dxnn | 34.3 | 34.3 | +0.0 | +0.0% |
| yolo26l-obb.dxnn | 25.1 | 25.0 | +0.0 | +0.1% |
| yolo26x-obb.dxnn | 13.8 | 13.7 | +0.0 | +0.3% |

### Classification

**ORT = ON**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 1071.3 ±7.5 | 3.23 | 193 | 14.9 | 53.4 | 50~51 | 1000 | 55 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 1076.8 ±1.3 | 3.21 | 191 | 25.7 | 68.4 | 51~52 | 1000 | 67 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 1072.1 ±8.6 | 3.22 | 190 | 42.2 | 85.0 | 59 | 1000 | 91 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 848.1 ±2.9 | 4.07 | 168 | 52.2 | 98.1 | 58 | 1000 | 103 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 461.5 ±2.0 | 7.49 | 130 | 68.1 | 99.2 | 62~63 | 1000 | 179 | ok |

**ORT = OFF**

| Model | Decoder | Frames | Runs | Avg FPS | Avg Duration (s) | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|--------|------|---------|------------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-cls.dxnn | mppvideodec | 3455 | 3 | 1071.6 ±8.7 | 3.22 | 193 | 14.2 | 51.9 | 50 | 1000 | 55 | ok |
| yolo26s-cls.dxnn | mppvideodec | 3455 | 3 | 1068.1 ±3.1 | 3.23 | 192 | 26.6 | 69.5 | 51~52 | 1000 | 67 | ok |
| yolo26m-cls.dxnn | mppvideodec | 3455 | 3 | 1073.8 ±6.0 | 3.22 | 190 | 43.9 | 85.1 | 59~60 | 1000 | 91 | ok |
| yolo26l-cls.dxnn | mppvideodec | 3455 | 3 | 849.3 ±2.9 | 4.07 | 169 | 47.1 | 98.3 | 58 | 1000 | 103 | ok |
| yolo26x-cls.dxnn | mppvideodec | 3455 | 3 | 461.2 ±3.4 | 7.49 | 132 | 73.2 | 99.2 | 61~62 | 1000 | 179 | ok |

**ORT Comparison – E2E FPS**

| Model | ORT ON | ORT OFF | Delta | Delta % |
|-------|--------|---------|-------|---------|
| yolo26n-cls.dxnn | 1071.3 | 1071.6 | -0.3 | -0.0% |
| yolo26s-cls.dxnn | 1076.8 | 1068.1 | +8.7 | +0.8% |
| yolo26m-cls.dxnn | 1072.1 | 1073.8 | -1.7 | -0.2% |
| yolo26l-cls.dxnn | 848.1 | 849.3 | -1.3 | -0.1% |
| yolo26x-cls.dxnn | 461.5 | 461.2 | +0.3 | +0.1% |

## E2E Pipeline (Multi-Stream)

### Object Detection

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 5 | 3 | 168.5 ±0.6 | 33.7 | 291 | 51.0 | 80.5 | 69~74 | 1000 | 177 | ok |
| yolo26n.dxnn | 6 | 3 | 169.3 ±0.2 | 28.2 | 277 | 51.2 | 81.3 | 75~76 | 1000 | 182 | ok |
| yolo26s.dxnn | 4 | 3 | 136.4 ±0.6 | 34.1 | 245 | 92.6 | 100.0 | 82~85 | 800~1000 | 186 | ok |
| yolo26s.dxnn | 5 | 3 | 133.3 ±1.9 | 26.7 | 242 | 92.9 | 99.7 | 85 | 800~1000 | 192 | ok |
| yolo26m.dxnn | 2 | 3 | 67.0 ±0.2 | 33.5 | 173 | 94.5 | 100.0 | 84~85 | 400~1000 | 196 | ok |
| yolo26m.dxnn | 3 | 3 | 65.5 ±0.1 | 21.8 | 173 | 95.4 | 100.0 | 85 | 400~1000 | 202 | ok |
| yolo26l.dxnn | 1 | 3 | 57.6 ±5.2 | 57.6 | 164 | 93.4 | 100.0 | 84~85 | 400~1000 | 193 | ok |
| yolo26l.dxnn | 2 | 3 | 51.5 ±1.1 | 25.8 | 156 | 96.2 | 100.0 | 84 | 400~1000 | 204 | ok |
| yolo26x.dxnn | 1 | 3 | 27.5 ±0.2 | 27.5 | 104 | 94.6 | 100.0 | 83~84 | 400~1000 | 314 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n.dxnn | 3 | 3 | 99.3 ±0.2 | 33.1 | 210 | 26.7 | 82.0 | 63~64 | 1000 | 187 | ok |
| yolo26n.dxnn | 4 | 3 | 100.3 ±0.3 | 25.1 | 221 | 26.9 | 82.3 | 64 | 1000 | 196 | ok |
| yolo26s.dxnn | 3 | 3 | 99.9 ±0.6 | 33.3 | 227 | 53.5 | 83.9 | 73~75 | 1000 | 202 | ok |
| yolo26s.dxnn | 4 | 3 | 100.0 ±0.5 | 25.0 | 228 | 53.6 | 83.9 | 75~76 | 1000 | 211 | ok |
| yolo26m.dxnn | 2 | 3 | 67.0 ±0.3 | 33.5 | 200 | 95.5 | 100.0 | 84~85 | 400~1000 | 211 | ok |
| yolo26m.dxnn | 3 | 3 | 65.1 ±0.8 | 21.7 | 197 | 96.8 | 100.0 | 85 | 400~800 | 220 | ok |
| yolo26l.dxnn | 1 | 3 | 58.1 ±5.6 | 58.1 | 186 | 93.8 | 100.0 | 84 | 400~1000 | 203 | ok |
| yolo26l.dxnn | 2 | 3 | 50.8 ±0.7 | 25.4 | 171 | 95.9 | 100.0 | 84~85 | 400~1000 | 214 | ok |
| yolo26x.dxnn | 1 | 3 | 27.3 ±0.1 | 27.3 | 116 | 94.7 | 100.0 | 83~84 | 400~1000 | 314 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n.dxnn | 5 | 33.7 | 3 | 33.1 |
| yolo26s.dxnn | 4 | 34.1 | 3 | 33.3 |
| yolo26m.dxnn | 2 | 33.5 | 2 | 33.5 |
| yolo26l.dxnn | 1 | 57.6 | 1 | 58.1 |

### Pose Estimation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 224.1 ±0.8 | 32.0 | 295 | 89.3 | 95.9 | 81~85 | 800~1000 | 185 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 217.6 ±3.2 | 27.2 | 287 | 91.4 | 98.7 | 86 | 600~1000 | 191 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 129.7 ±3.3 | 32.4 | 205 | 95.5 | 100.0 | 84~86 | 800~1000 | 182 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 123.9 ±1.0 | 24.8 | 199 | 95.9 | 100.0 | 85~86 | 600~1000 | 188 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 65.0 ±0.6 | 32.5 | 152 | 95.5 | 100.0 | 84 | 400~1000 | 189 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 63.8 ±0.1 | 21.3 | 151 | 96.3 | 100.0 | 84~85 | 400~1000 | 198 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 57.4 ±5.1 | 57.4 | 139 | 93.8 | 100.0 | 84~85 | 400~1000 | 184 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 51.7 ±0.4 | 25.9 | 135 | 95.8 | 100.0 | 84 | 400~1000 | 198 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 27.4 ±0.3 | 27.4 | 94 | 94.6 | 100.0 | 83~84 | 400~1000 | 325 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-pose.dxnn | 7 | 3 | 231.9 ±0.1 | 33.1 | 219 | 95.4 | 100.0 | 81~84 | 1000 | 184 | ok |
| yolo26n-pose.dxnn | 8 | 3 | 231.4 ±1.3 | 28.9 | 216 | 96.0 | 100.0 | 84~85 | 800~1000 | 189 | ok |
| yolo26s-pose.dxnn | 4 | 3 | 132.8 ±1.9 | 33.2 | 175 | 96.3 | 100.0 | 83~85 | 800~1000 | 177 | ok |
| yolo26s-pose.dxnn | 5 | 3 | 130.0 ±1.4 | 26.0 | 173 | 96.7 | 100.0 | 85 | 800~1000 | 185 | ok |
| yolo26m-pose.dxnn | 2 | 3 | 66.8 ±1.4 | 33.4 | 127 | 95.9 | 100.0 | 84 | 400~1000 | 180 | ok |
| yolo26m-pose.dxnn | 3 | 3 | 64.5 ±1.1 | 21.5 | 123 | 96.8 | 100.0 | 83~84 | 400~1000 | 189 | ok |
| yolo26l-pose.dxnn | 1 | 3 | 58.2 ±5.1 | 58.2 | 116 | 93.5 | 100.0 | 83~84 | 400~1000 | 174 | ok |
| yolo26l-pose.dxnn | 2 | 3 | 53.0 ±0.5 | 26.5 | 113 | 96.2 | 100.0 | 84~85 | 400~1000 | 188 | ok |
| yolo26x-pose.dxnn | 1 | 3 | 27.8 ±0.6 | 27.8 | 70 | 95.3 | 100.0 | 83~84 | 400~1000 | 325 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-pose.dxnn | 7 | 32.0 | 7 | 33.1 |
| yolo26s-pose.dxnn | 4 | 32.4 | 4 | 33.2 |
| yolo26m-pose.dxnn | 2 | 32.5 | 2 | 33.4 |
| yolo26l-pose.dxnn | 1 | 57.4 | 1 | 58.2 |

### Segmentation

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 3 | 3 | 115.4 ±0.2 | 38.5 | 391 | 41.9 | 67.2 | 76~80 | 1000 | 273 | ok |
| yolo26n-seg.dxnn | 4 | 3 | 115.4 ±0.1 | 28.8 | 373 | 42.1 | 68.1 | 81~82 | 1000 | 280 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 77.2 ±0.6 | 25.7 | 270 | 91.1 | 100.0 | 85~86 | 400~1000 | 293 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 77.8 ±0.0 | 38.9 | 271 | 89.7 | 100.0 | 85~86 | 400~1000 | 281 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 39.8 ±0.1 | 39.8 | 174 | 96.1 | 100.0 | 85 | 400~800 | 282 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 40.0 ±1.2 | 20.0 | 176 | 97.0 | 100.0 | 84~86 | 400~1000 | 303 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 38.2 ±1.3 | 38.2 | 166 | 94.8 | 100.0 | 83~84 | 400~1000 | 292 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 36.4 ±0.2 | 18.2 | 160 | 95.7 | 100.0 | 84 | 400~1000 | 312 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 22.7 ±0.8 | 22.7 | 119 | 94.4 | 100.0 | 82~83 | 400~1000 | 362 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-seg.dxnn | 2 | 3 | 87.2 ±0.2 | 43.6 | 285 | 28.3 | 87.0 | 71~73 | 1000 | 304 | ok |
| yolo26n-seg.dxnn | 3 | 3 | 87.1 ±0.3 | 29.0 | 284 | 28.4 | 87.9 | 73~74 | 1000 | 318 | ok |
| yolo26s-seg.dxnn | 2 | 3 | 83.1 ±2.4 | 41.5 | 277 | 67.3 | 100.0 | 84~85 | 400~1000 | 319 | ok |
| yolo26s-seg.dxnn | 3 | 3 | 78.6 ±2.3 | 26.2 | 271 | 88.7 | 100.0 | 86 | 400~1000 | 334 | ok |
| yolo26m-seg.dxnn | 1 | 3 | 42.8 ±0.8 | 42.8 | 184 | 95.0 | 100.0 | 84 | 400~800 | 314 | ok |
| yolo26m-seg.dxnn | 2 | 3 | 41.7 ±0.4 | 20.9 | 179 | 96.2 | 100.0 | 84 | 400~1000 | 329 | ok |
| yolo26l-seg.dxnn | 1 | 3 | 36.9 ±0.7 | 36.9 | 168 | 94.5 | 100.0 | 84 | 400~1000 | 316 | ok |
| yolo26l-seg.dxnn | 2 | 3 | 36.0 ±0.6 | 18.0 | 165 | 95.9 | 100.0 | 84 | 400~1000 | 338 | ok |
| yolo26x-seg.dxnn | 1 | 3 | 24.9 ±0.1 | 24.9 | 130 | 94.6 | 100.0 | 81~82 | 600~1000 | 368 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-seg.dxnn | 3 | 38.5 | 2 | 43.6 |
| yolo26s-seg.dxnn | 2 | 38.9 | 2 | 41.5 |
| yolo26m-seg.dxnn | 1 | 39.8 | 1 | 42.8 |
| yolo26l-seg.dxnn | 1 | 38.2 | 1 | 36.9 |

### Oriented BBox (OBB)

**ORT = ON**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 81.0 ±0.0 | 40.5 | 179 | 95.3 | 100.0 | 63~64 | 1000 | 182 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 81.1 ±0.1 | 27.0 | 178 | 96.3 | 100.0 | 65 | 1000 | 194 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.8 ±0.0 | 46.8 | 132 | 94.1 | 100.0 | 63~65 | 1000 | 182 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.8 ±0.1 | 23.4 | 134 | 95.9 | 100.0 | 68~69 | 1000 | 201 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.3 ±0.1 | 34.3 | 114 | 92.9 | 100.0 | 60~74 | 1000 | 204 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 34.3 ±0.1 | 17.2 | 118 | 96.2 | 100.0 | 79~80 | 800~1000 | 221 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.1 ±0.0 | 25.1 | 96 | 94.7 | 100.0 | 74~76 | 1000 | 215 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.8 ±0.1 | 13.8 | 55 | 93.2 | 100.0 | 80~81 | 800~1000 | 334 | ok |

**ORT = OFF**

| Model | Streams | Runs | E2E FPS | Per-Ch FPS | CPU% | NPU Avg% | NPU Max% | NPU Temp °C | NPU MHz | RSS MiB | Status |
|-------|---------|------|---------|------------|------|----------|----------|-------------|---------|---------|--------|
| yolo26n-obb.dxnn | 2 | 3 | 80.3 ±0.1 | 40.2 | 198 | 95.7 | 100.0 | 62~63 | 1000 | 175 | ok |
| yolo26n-obb.dxnn | 3 | 3 | 80.4 ±0.0 | 26.8 | 197 | 96.0 | 100.0 | 62~63 | 1000 | 189 | ok |
| yolo26s-obb.dxnn | 1 | 3 | 46.9 ±0.0 | 46.9 | 138 | 93.2 | 100.0 | 63~65 | 1000 | 178 | ok |
| yolo26s-obb.dxnn | 2 | 3 | 46.8 ±0.1 | 23.4 | 143 | 96.3 | 100.0 | 66~69 | 1000 | 199 | ok |
| yolo26m-obb.dxnn | 1 | 3 | 34.3 ±0.1 | 34.3 | 124 | 94.7 | 100.0 | 75~80 | 1000 | 198 | ok |
| yolo26m-obb.dxnn | 2 | 3 | 34.3 ±0.1 | 17.1 | 126 | 95.7 | 100.0 | 79~80 | 800~1000 | 217 | ok |
| yolo26l-obb.dxnn | 1 | 3 | 25.0 ±0.1 | 25.0 | 101 | 94.5 | 100.0 | 75~78 | 1000 | 209 | ok |
| yolo26x-obb.dxnn | 1 | 3 | 13.7 ±0.2 | 13.7 | 56 | 92.2 | 100.0 | 80 | 800~1000 | 334 | ok |

**Channel Capacity Summary** (max streams where per-channel FPS ≥ 30)

| Model | ORT ON Capacity | Per-Ch FPS | ORT OFF Capacity | Per-Ch FPS |
|-------|-----------------|------------|------------------|------------|
| yolo26n-obb.dxnn | 2 | 40.5 | 2 | 40.2 |
| yolo26s-obb.dxnn | 1 | 46.8 | 1 | 46.9 |
| yolo26m-obb.dxnn | 1 | 34.3 | 1 | 34.3 |

---
*Report generated by dx_stream benchmark tool*
