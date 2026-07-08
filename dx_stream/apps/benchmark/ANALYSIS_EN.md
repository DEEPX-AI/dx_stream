# YOLO26 × DEEPX NPU Benchmark Analysis Report

## Executive Summary

The table below summarizes the **latest benchmark results** as the representative practical performance each environment delivers with the **lightest nano model** on **FHD 30fps video input**. The figures use the **better result between ORT ON and ORT OFF for the nano model**.

Because accuracy and FPS are in a trade-off relationship, model-size decisions beyond this high-level view, including absolute peak performance with larger models, should be made by referring to the detailed benchmark results in the main body.

| Environment | OD | Pose | Seg | OBB |
|-------------|----|------|-----|-----|
| i7 | 243.5 FPS / 8ch | 230.0 FPS / 7ch | 188.2 FPS / 6ch | 80.2 FPS / 2ch |
| AIBox | 188.4 FPS / 6ch | 209.6 FPS / 7ch | 115.2 FPS / 3ch | 80.0 FPS / 2ch |
| Biostar H1 | 498.3 FPS / 16ch | 557.8 FPS / 19ch | 363.0 FPS / 12ch | 307.9 FPS / 10ch |
| OrangePi 5 Plus | 170.2 FPS / 5ch | 231.6 FPS / 7ch | 115.0 FPS / 3ch | 81.2 FPS / 2ch |
| Radxa Rock 5B+ | 151.0 FPS / 4ch | 215.6 FPS / 7ch | 97.8 FPS / 3ch | 75.0 FPS / 2ch |
| RPi 5 | 86.6 FPS / 2ch | 124.7 FPS / 4ch | 48.7 FPS / 1ch | 79.4 FPS / 2ch |

> Notation: `maximum single-stream E2E FPS / maximum multi-stream channel count at 30fps/channel`
>
> Classification is excluded from this summary table because it is less representative of common E2E video analytics and multi-stream deployment scenarios.


## Table of Contents

1. [Introduction](#1-introduction)
2. [Test Environments](#2-test-environments)
3. [Benchmark Design](#3-benchmark-design)
4. [Performance Bottleneck Analysis](#4-performance-bottleneck-analysis)
5. [Deployment Guide by Environment](#5-deployment-guide-by-environment)
6. [Conclusion](#6-conclusion)
7. [Appendix](#7-appendix)

---

## 1. Introduction

### 1.1 Document Purpose

This document analyzes the inference performance of the YOLO26 model family on DEEPX M1/H1 NPUs, systematically measured across **6 host environments**. Rather than simply listing numbers, the goal is to trace **"why this environment yields this performance"** and to outline the practical considerations for real-world deployment.

### 1.2 Measurement Scope

- **Models**: 25 models in the YOLO26 family
  - 5 sizes: nano(n), small(s), medium(m), large(l), xlarge(x)
  - 5 tasks: Object Detection, Pose Estimation, Segmentation, Oriented BBox (OBB), Classification
- **NPU**: DEEPX M1 (single chip) and H1 (4× M1 package)
- **Metrics**: Latency, Throughput, E2E Pipeline FPS, Multi-Stream channel capacity

### 1.3 Key Findings Preview

1. **The same NPU can exhibit up to 2.6× performance difference depending on the host.** The bottleneck location varies by environment.
2. **PCIe lanes/generation** are the key variable determining NPU saturation for lightweight models.
3. For tasks with **heavy CPU post-processing such as Segmentation**, host CPU performance becomes the decisive factor for E2E performance.
4. On ARM environments, heavy post-processing with ORT OFF can **starve the NPU itself through GStreamer's backpressure mechanism**.

---

## 2. Test Environments

### 2.1 Environment Specifications

| Environment | CPU | Cores | RAM | NPU | PCIe | OS |
|-------------|-----|:-----:|:---:|-----|------|----|
| **i7** | Intel i7-14700K | 20 (28T) | 62.5 GB | M1 × 1 | Gen3 ×4 | Ubuntu 24.04 |
| **AIBox** | Intel N97 | 4 | 7.5 GB | M1 × 1 | Gen3 ×2 | Ubuntu 24.04 |
| **Biostar** | AMD Ryzen 5 9600X | 6 (12T) | 30.5 GB | H1 × 1 (M1 ×4) | Gen3 ×4 | Ubuntu 22.04 |
| **OrangePi** | Cortex-A55 | 8 | 15.6 GB | M1 × 1 | Gen3 ×4 | Debian 12 |
| **Radxa** | RK3588 | 8 | 7.8 GB | M1 × 1 | Gen3 ×2 | Debian 12 |
| **RPi 5** | Cortex-A76 | 4 | 7.9 GB | M1 × 1 | **Gen2 ×1** | Debian 12 |

### 2.2 Environment Characteristics

**i7**:
A desktop x86 environment based on the Intel i7-14700K. It uses `vah264dec` hardware decoding, and with a single M1 the CPU and decoder headroom is ample, making host-side bottlenecks relatively minor. This makes it the easiest environment to interpret as a single-M1 performance reference.

**AIBox**:
A low-power x86 environment based on the Intel N97 (4 cores). It uses `vah264dec` hardware decoding, but the limited CPU resources cause host-side bottlenecks to surface quickly for post-processing-heavy tasks like Segmentation. PCIe Gen3 ×2 bandwidth is adequate, but the 4-core CPU constraint means it lacks the headroom of the i7.

**Biostar**:
A high-performance desktop environment with AMD Ryzen 5 9600X + H1 (4× M1). It uses `vaapidecodebin` hardware decoding and has a powerful CPU, but the H1's raw throughput is so high that host-side stages — decoding, pre-processing, post-processing, and result egress — become the bottleneck first for lightweight models. Rather than "no bottleneck," it is more accurate to say this is an environment where **the NPU is so fast that other stages are exposed as bottlenecks**.

**OrangePi**:
An ARM SBC based on Cortex-A55 (8 cores). It uses `mppvideodec` hardware decoding, so unlike RPi 5, software decoding bottlenecks do not dominate. However, the CPU compute power is limited, so host-side burden and backpressure readily appear with ORT OFF raw tensor post-processing or heavy models.

**Radxa**:
An ARM SBC based on RK3588. Like OrangePi, it uses `mppvideodec` hardware decoding. PCIe is Gen3 ×2, narrower than OrangePi, but in practice the ARM CPU's post-processing capacity and host-side pipeline costs tend to surface as bottlenecks before bandwidth itself becomes limiting.

**RPi 5**:
An ARM SBC based on Cortex-A76 (4 cores). This environment uses `avdec_h264` software decoding, and PCIe is **Gen2 ×1** — very narrow. Unlike other ARM boards, both the decoding burden and PCIe bottleneck overlap, causing notably low NPU utilization for lightweight models.

### 2.3 PCIe Bandwidth Comparison

PCIe bandwidth is the physical upper bound on data transfer speed between host and NPU.

| Configuration | Unidirectional Bandwidth (Theoretical) | Notes |
|---------------|:--------------------------------------:|-------|
| Gen3 ×4 | ~3.9 GB/s | i7, Biostar, OrangePi |
| Gen3 ×2 | ~2.0 GB/s | AIBox, Radxa |
| Gen2 ×1 | ~0.5 GB/s | RPi 5 |

RPi 5 (Gen2 ×1) has roughly **1/8th** the theoretical bandwidth of Gen3 ×4. This difference manifests dramatically with lightweight models.

---

## 3. Benchmark Design

### 3.1 Measurement Tiers (4-tier)

The benchmark measures from raw NPU performance to real-world pipeline performance across 4 tiers.

**Tier 1: Model Latency** — NPU inference latency
- `run_model -m <model> -l 300` (synchronous, single core)
- Time from sending one input to receiving the result
- With ORT ON: Total = NPU ms + CPU ms (includes ORT CPU offload compute time)

**Tier 2: Model Throughput** — NPU inference throughput
- `run_model -m <model> -t 30` (asynchronous, multi-core)
- Frames processed over 30 seconds while maintaining 6 concurrent in-flight requests
- No decoding or pre/post-processing — **same tensor submitted repeatedly** → pure NPU + PCIe performance

**Tier 3: E2E Single-Stream** — Single-stream pipeline throughput
- GStreamer pipeline topology: `decodebin ! dxpreprocess ! queue ! dxinfer ! queue ! dxpostprocess ! queue ! fakesink`
- All queues are set to `leaky=no`, so if downstream slows down, **backpressure propagates upstream**
- Inside `dxinfer`, there is an asynchronous push thread and a size-limited push_queue (`MAX_PUSH_QUEUE_SIZE=5 × device count`)
- Uses real video input (1080p H.264, 30 FPS)
- **Real-world FPS** reflecting the CPU costs of decoding, pre-processing, and post-processing

**Tier 4: Multi-Stream** — Multi-channel concurrent processing capacity
- A **single GStreamer pipeline** multiplexes N streams via `dxinputselector`/`dxoutputselector`
- N input branches (`urisourcebin ! decodebin ! queue`) feed into a shared inference path (`dxpreprocess ! dxinfer ! dxpostprocess`) and are demultiplexed to N output branches (`queue ! fakesink`)
- Records the maximum number of concurrent streams (`Max Channels`) where per-channel FPS stays **above 30fps**
- Directly measures the **maximum concurrent processing capacity** needed for real deployments

### 3.2 Design Considerations

**Thermal Stability**

When the NPU temperature reaches a certain threshold (approximately 85–90°C), the clock is forcibly reduced (thermal throttling), causing a sharp performance drop. To mitigate this:

- **Cooldown is performed once at the start of each model × ORT combination block.**
- Cooldown waits up to 300 seconds until the temperature drops to **idle + 10°C or below**, based on the idle temperature measured at the start.
- Cooldown is **applied only before Latency** — there is no separate cooldown between Latency and Throughput, between Throughput and E2E, or between E2E and Multi-Stream.

**Measurement Sequence**

The actual execution order for **each model** is as follows:

1. Select model
2. Begin ORT ON block
3. Perform `cooldown`
4. Latency warmup ×1, then Latency measurement
5. Throughput warmup ×1, then Throughput measurement
6. E2E Single-Stream warmup ×1, then E2E measurement
7. Multi-Stream sweep
8. Begin ORT OFF block
9. Perform `cooldown` again
10. Repeat the same sequence

ORT ON and ORT OFF are treated as **independent execution blocks** within the same model, with only the start of each block brought close to cool state. The subsequent Latency → Throughput → E2E → Multi-Stream measurements **run consecutively without breaks**. This design intentionally measures Throughput and E2E in an already-warmed state, more closely approximating real-world sustained operation conditions.


**ORT CPU Offload Comparison**

All models are measured with ORT (OnnxRuntime) CPU Offload both ON and OFF. ORT ON executes some of the model's operations on the CPU instead of the NPU. However, this adds CPU computation time to Latency and increases CPU load.

**Repetition and Statistics**

- Latency: 1 warmup, then 1 measurement with `-l 300` loop
- Throughput: 1 warmup, then 3 measurements of 30 seconds each
- E2E Single-Stream: 1 warmup, then 3 measurements
- Multi-Stream: **per stream count**, 1 warmup then 3 measurements
- Warmup runs are excluded from results
- E2E Single-Stream and Multi-Stream warmup/run are retried once if a timeout occurs.

### 3.3 Capacity Measurement

Multi-Stream does not simply measure 1, 2, 3, ... N exhaustively. The actual implementation works as follows:

1. The starting stream count is estimated based on Single-Stream E2E FPS.
2. The `1ch` result is **reused from the Single-Stream result** without running a separate pipeline.
3. From the estimated starting point, stream count is increased or decreased to find the boundary.
4. The maximum stream count where per-channel FPS stays **above 30fps** is recorded as `Max Channels`.

In other words, Multi-Stream is closer to a **single-stream-based boundary search** than a "full-range brute-force sweep." This design significantly reduces total measurement time while quickly finding the capacity point needed from a practical deployment perspective.

---


## 4. Performance Bottleneck Analysis

### 4.1 Model Throughput

Model Throughput measured by `run_model` is a pure NPU throughput metric obtained by **repeatedly submitting the same input tensor** without decoding or pre/post-processing. Even with the same DX-M1 chip and the same model, this value can vary significantly depending on the host environment.

#### PCIe Bandwidth and Model Size

NPU inference involves the following steps:
1. Host → NPU: Input tensor transfer (PCIe DMA)
2. NPU: Inference execution
3. NPU → Host: Output tensor transfer (PCIe DMA)

For **heavy models** (medium–xlarge), NPU compute time is 40–80 ms, making the data transfer time (~0.5 ms) less than 1% of the total. Therefore, PCIe bandwidth has negligible impact on performance. In practice, OD medium model NPU Throughput (ORT OFF) converges to nearly identical values of 91–96 FPS across all single-M1 environments.

For **lightweight models** (nano), NPU compute time is ~5 ms, so data transfer time becomes a proportionally larger share. This causes up to 2.6× difference for the same OD nano model across environments (RPi 5: 91.5 FPS vs i7: 242.1 FPS, ORT OFF).

#### RPi 5 Case Study: The Limits of Gen2 ×1

RPi 5's OD nano Throughput is 91.5 FPS (ORT OFF), while the same M1 chip on i7 achieves 242.1 FPS. The fact that RPi 5 OD nano's NPU Avg% is only **24.2%** corroborates this — the NPU is idle more than 70% of the time because PCIe Gen2 ×1 (~0.5 GB/s) cannot deliver data fast enough, leaving the NPU waiting for input most of the time.

In contrast, the OD xlarge model achieves NPU Avg% of 88%+ even on RPi 5, because the long compute time makes transfer time relatively negligible.

#### NPU Utilization by Environment (OD nano, ORT OFF, Throughput Measurement)

| Environment | PCIe | Throughput | NPU Avg% | Interpretation |
|-------------|------|:---------:|:--------:|----------------|
| i7 | Gen3 ×4 | 242.1 | 91.2% | PCIe sufficient, high NPU utilization |
| AIBox | Gen3 ×2 | 222.0 | 80.6% | PCIe sufficient, residual host submit overhead |
| OrangePi | Gen3 ×4 | 237.6 | 88.7% | PCIe sufficient, high NPU utilization |
| Radxa | Gen3 ×2 | 222.4 | 87.5% | PCIe sufficient, residual host/runtime overhead |
| RPi 5 | Gen2 ×1 | 91.5 | 24.2% | **Severe PCIe bottleneck** |
| Biostar | Gen3 ×4 (×4 chips) | 929.3 | 92.7% | 4-chip parallel, high utilization |

Excluding RPi 5, OD nano Throughput across single-M1 environments falls within the 222–242 range, because **bandwidth is no longer the primary bottleneck at PCIe Gen3 or above**. The remaining differences arise from host CPU request submission overhead, platform/driver overhead, and runtime scheduling variations.

### 4.2 E2E Pipeline Performance

E2E (End-to-End) pipeline measurement simulates real-world usage by processing actual video (1080p H.264) through a GStreamer pipeline. Unlike Model Throughput, the host's **decoding**, **pre-processing**, and **post-processing** costs are all reflected, making it heavily influenced by the host environment.

#### Pipeline Processing Stages

The main processing stages and their execution locations in the E2E pipeline vary by environment:

| Stage | Processing | Execution Location |
|-------|-----------|-------------------|
| **Decoding** | H.264 decoding | HW-accelerated (VA-API, MPP) or SW (`avdec_h264`, CPU) — varies by environment |
| **Pre-processing** (dxpreprocess) | Resize, color space conversion, tensor layout conversion | Rockchip environments: RGA HW acceleration, others: libyuv (CPU) |
| **Inference** (dxinfer) | dxnn model inference | NPU |
| **Post-processing** (dxpostprocess) | Varies significantly depending on ORT ON/OFF (details below) | CPU |

The decoding method differs by environment, and its impact on E2E performance is substantial:

| Environment | Decoder | Acceleration |
|-------------|---------|-------------|
| i7 | `vah264dec` | VA-API HW acceleration |
| AIBox | `vah264dec` | VA-API HW acceleration |
| Biostar | `vaapidecodebin` | VA-API HW acceleration |
| OrangePi | `mppvideodec` | Rockchip MPP HW acceleration |
| Radxa | `mppvideodec` | Rockchip MPP HW acceleration |
| RPi 5 | `avdec_h264` | **Software decoding** |

Only RPi 5 uses software decoding, which consumes a significant portion of CPU resources.

#### Post-processing Cost by ORT ON/OFF

The cost of post-processing (`dxpostprocess`) varies greatly depending on **ORT ON/OFF and the task**.

With **ORT ON**, `dxinfer` returns refined output that includes the CPU offload layers, so `dxpostprocess` performs only lightweight parsing:

| Task | NPU Output Format | dxpostprocess Processing | Relative Cost |
|------|-------------------|-------------------------|:-------------:|
| Classification | `[1, 1000]` scores | argmax | Negligible |
| Object Detection | `[1, N, 6]` (x1,y1,x2,y2,score,cls) | confidence filtering + coordinate scaling | Low |
| Pose Estimation | `[1, N, 57]` (bbox + 17×3 keypoints) | confidence filtering + coordinate scaling | Low |
| OBB | `[1, N, 7]` (cx,cy,w,h,score,cls,angle) | confidence filtering + AABB conversion | Low |
| Segmentation | `[1, 300, 38]` + `[1, 32, 160, 160]` proto | confidence filtering + **mask generation** (coeff × proto, bilinear interpolation) | **High** |

With **ORT OFF**, the NPU returns raw multi-scale tensors as-is, so `dxpostprocess` must perform both decoding and parsing:

| Task | NPU Output Format | dxpostprocess Processing | Relative Cost |
|------|-------------------|-------------------------|:-------------:|
| Object Detection | 6 tensors (cv2 bbox 3 + cv3 class 3, 80×80/40×40/20×20) | 3-scale grid traversal + sigmoid + bbox decode | **Medium** |
| Pose Estimation | 9 tensors (cv2 3 + cv4 kpt 3 + cv3 cls 3) | 3-scale traversal + sigmoid + bbox/kpt decode (1 class → relatively light) | Medium |
| OBB | 9 tensors (cv2 3 + cv4 angle 3 + cv3 cls 3) | 3-scale traversal + sigmoid + rotated bbox decode | Medium |
| Segmentation | 10 tensors (cv2 3 + cv4 coef 3 + cv3 cls 3 + proto 1) | 3-scale traversal + sigmoid + bbox/coef decode + **mask generation** | **Very high** |

Segmentation's mask generation is common to both ORT ON and OFF. It requires per-detected-object coefficient dot product on 160×160 prototypes + bilinear interpolation every frame, resulting in CPU load several times higher than other tasks. With ORT OFF, the multi-scale decoding cost is added on top of this.

#### Impact of ORT ON/OFF on E2E

The key insight is that the impact of ORT ON/OFF on E2E can be **opposite depending on the environment and task**. It is not a matter of "one side always wins," but rather **"which is more costly — ORT CPU offload or raw tensor post-processing — and which enlarges the pipeline bottleneck more."**

#### Backpressure Mechanism

In the E2E pipeline, CPU bottlenecks do not simply mean "the CPU is busy so things slow down" — they **reduce NPU utilization itself** through GStreamer's **backpressure mechanism**. Since all queues are set to `leaky=no`, downstream processing delays propagate upstream.

The path:
1. `dxpostprocess`'s `transform_ip()` performs post-processing synchronously — if this function is slow, downstream stalls
2. The queue between `dxinfer` → `dxpostprocess` fills up
3. `dxinfer`'s push thread blocks on `gst_pad_push()`
4. The push_queue (`MAX_PUSH_QUEUE_SIZE=5 × device count`) fills up, blocking `primary_mode_infer()` from submitting new inferences
5. **No new input is supplied to the NPU, causing it to go idle**

In other words, slow downstream post-processing propagates back through the pipeline and starves the NPU.

#### E2E Analysis by Environment

##### i7 — Reference Environment with Ample CPU Headroom

The i7 (20 cores/28 threads) has HW decoding (`vah264dec`) + libyuv pre-processing + abundant CPU resources, making host-side bottlenecks minimal.

| Task | Model | ORT ON E2E | ORT OFF E2E | Analysis |
|------|-------|:----------:|:-----------:|----------|
| OD | nano | 243.5 | 239.9 | Essentially identical. Raw post-processing easily handled by x86 20 cores |
| Pose | nano | 230.0 | 229.7 | Essentially identical. 1 class makes post-processing cost negligible |
| Seg | nano | 188.2 | 186.3 | Essentially identical. Mask computation is within 20-core headroom |

**Conclusion**: On i7, ORT ON/OFF has virtually no effect on E2E. The CPU can handle either approach with ease.

##### AIBox — Balancing on Limited CPU

AIBox (N97, 4 cores) uses HW decoding (`vah264dec`) but is limited to 4 CPU cores (max CPU% = 400%), so bottleneck patterns vary by task.

| Task | Model | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | Analysis |
|------|-------|:-----------------:|:-------------------:|----------|
| OD | nano | 172.7 (282) | **188.4** (307) | OFF is +9%. Raw post-processing manageable on x86 |
| Pose | nano | 182.9 (253) | **209.6** (207) | OFF is +15%. 1 class makes raw parsing light |
| Seg | nano | 106.3 (360) | **115.2** (369) | OFF is +8%. Both ON/OFF at 90–92% CPU |

AIBox pushes CPU usage high across all tasks due to the 4-core constraint. ORT ON performs additional CPU computation inside dxinfer, slowing the rate at which frames are submitted to the NPU, which in turn lowers E2E. For Seg in particular, CPU% reaches 360–369 (90–92% of total) under both ON and OFF, indicating near-saturation.

**Conclusion**: ORT OFF is generally favorable on AIBox. However, attention to total CPU capacity is needed as tasks get heavier.

##### Biostar H1 — When the NPU Is Too Fast for the Host

Biostar (Ryzen 5 9600X, 6 cores/12 threads, H1 = 4× M1) has a specific challenge: **the combined throughput of 4 NPUs is so high that the host cannot keep up with supply.**

| Task | Model | Throughput (ORT OFF) | ORT ON E2E | ORT OFF E2E | E2E/Thr Ratio |
|------|-------|:---------:|:----------:|:-----------:|:------------:|
| OD | nano | 929.3 | **498.3** | 463.1 | 50–54% |
| Pose | nano | 888.8 | 542.7 | **557.8** | 61–63% |
| Seg | nano | 609.7 | **363.0** | 323.0 | 53–60% |

Even 6 cores (12 threads) cannot supply frames fast enough to 4× NPUs, so E2E for lightweight models reaches only 50–63% of Throughput. ORT ON is higher for OD and Seg because at 4× NPU frame rates, the raw tensor post-processing burden exceeds the ORT CPU offload cost. Pose favors ORT OFF slightly because its 1-class raw parsing is lightweight.

**Conclusion**: With H1, the host supply limitation becomes more pronounced for lighter models. The E2E/Throughput ratio improves for medium and heavier models.

##### OrangePi / Radxa — ORT OFF Backpressure on ARM CPUs

OrangePi (A55, 8 cores) and Radxa (RK3588, 8 cores) use `mppvideodec` for **HW decoding** and `dxpreprocess` with **RGA HW pre-processing**, so unlike RPi 5, decoding/pre-processing itself does not burden the CPU. However, ARM CPU compute power is limited, causing **backpressure from ORT OFF raw tensor post-processing** for certain tasks.

**Object Detection:**

| Environment | Model | ORT | E2E FPS | CPU% | NPU Avg% |
|-------------|-------|-----|:-------:|:----:|:--------:|
| OrangePi | OD nano | **ON** | **170.2** | 292 | 46.6% |
| OrangePi | OD nano | OFF | 96.7 | 226 | 25.0% |
| OrangePi | OD small | **ON** | **136.1** | 241 | 84.2% |
| OrangePi | OD small | OFF | 97.1 | 228 | 49.0% |
| Radxa | OD nano | **ON** | **151.0** | 256 | 44.3% |
| Radxa | OD nano | OFF | 95.8 | 188 | 25.2% |
| Radxa | OD small | **ON** | **130.9** | 227 | 89.3% |
| Radxa | OD small | OFF | 95.0 | 193 | 50.0% |

With ORT OFF, NPU Avg% drops by half **despite CPU% being lower**. This is not due to CPU contention but to **backpressure**: `dxpostprocess` takes time to decode 6 raw multi-scale tensors (80×80, 40×40, 20×20 each for bbox+class), stalling the entire pipeline and starving the NPU. ORT ON passes compact `[1, N, 6]` output, making post-processing lightweight and avoiding backpressure.

**Pose Estimation:**

| Environment | Model | ORT ON E2E | ORT OFF E2E | Analysis |
|-------------|-------|:----------:|:-----------:|----------|
| OrangePi | Pose nano | 224.9 | **231.6** | OFF is +3% |
| Radxa | Pose nano | 215.6 | 215.0 | Essentially identical |

Pose shows the opposite of OD — **ORT OFF is better**. Pose models have only 1 class (person), so the sigmoid computation on ORT OFF raw tensors (cv3 class tensor is `[1,1,H,W]`) is ~1/80th the cost compared to 80-class OD. The additional ORT CPU offload cost becomes a net burden instead.

**Segmentation:**

| Environment | Model | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | Analysis |
|-------------|-------|:-----------------:|:-------------------:|----------|
| OrangePi | Seg nano | **115.0** (372) | 87.5 (281) | ON is +31% |
| Radxa | Seg nano | **97.8** (370) | 78.4 (280) | ON is +25% |

Seg clearly favors ORT ON. Mask generation is inherently CPU-intensive, and with ORT OFF the 10-tensor raw decoding cost on top makes post-processing burden severe.

**ARM Environment Conclusion**:
- **OD**: ORT ON required (avoids raw tensor backpressure)
- **Pose**: ORT OFF recommended (1-class raw parsing is lightweight; Radxa shows ON/OFF parity)
- **Seg**: ORT ON recommended (dual burden of masks + raw tensor decoding)
- For heavy models (medium–xlarge), NPU compute time dominates and ORT ON/OFF difference diminishes

##### RPi 5 — Dual Bottleneck: Software Decoding + PCIe

RPi 5 (A76, 4 cores) has two unique constraints simultaneously:
1. **PCIe Gen2 ×1**: Reduces NPU utilization to as low as 27% for lightweight models
2. **Software decoding** (`avdec_h264`): Consumes a significant portion of CPU resources

In this combination, ORT ON intensifies CPU resource contention and **degrades E2E across all tasks**:

| Task | Model | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | Difference |
|------|-------|:-----------------:|:-------------------:|:----------:|
| OD | nano | 66.6 (336) | **86.6** (310) | +30% |
| Pose | nano | 83.7 (344) | **124.7** (311) | +49% |
| Seg | nano | 47.1 (337) | **48.7** (216) | +3% |

Why ORT ON is always worse on RPi 5:
- Software decoding occupies a significant portion of the 4-core CPU
- ORT CPU offload consumes additional remaining CPU → contends with decoding/pre-processing
- Overall pipeline throughput degrades

ORT OFF, having no CPU offload stage, allows more CPU to be allocated to software decoding, resulting in higher E2E.

A notable observation is that **OD E2E FPS is similar from nano through medium**:

| Task | Model | Throughput | ORT OFF E2E | NPU Avg% |
|------|-------|:---------:|:----------:|:--------:|
| OD | nano | 91.5 | 86.6 | 22.8% |
| OD | small | 91.4 | 86.3 | 44.5% |
| OD | medium | 91.3 | 86.0 | 78.2% |
| OD | large | 70.3 | 71.0 | 93.1% |

Throughput (`run_model`) already reflects the PCIe bottleneck, with nano–medium in the 84–91 FPS range, but E2E converges even lower at ~86 FPS. This is because the bottleneck is not PCIe but **host-side pipeline processing — software decoding (`avdec_h264`) + pre-processing (libyuv) + post-processing — on the 4-core CPU**. The fact that nano's NPU Avg% is only 24.2% corroborates this — the NPU has ample headroom but is being starved because the host cannot supply frames fast enough.

#### Practical Implications: Model Size Selection

Since the host pipeline bottleneck determines the E2E ceiling, **within the range of models whose Throughput exceeds the ceiling, scaling up the model incurs no E2E penalty** — only accuracy improves. However, once Throughput drops below the ceiling, the NPU becomes the bottleneck and E2E declines.

| Task | Model | Throughput | E2E | Bottleneck | Implication |
|------|-------|:---------:|:---:|-----------|-----------|
| OD | nano | 91.5 | 86.6 | Host | Thr > ceiling (~86) → no FPS penalty |
| OD | small | 91.4 | 86.3 | Host | Same |
| OD | medium | 91.3 | 86.0 | Host | Thr ≈ ceiling → virtually no FPS penalty |
| OD | large | 70.3 | 71.0 | **NPU** | Thr < ceiling → NPU is bottleneck, FPS drops |
| Pose | nano | 135.3 | 124.7 | Host | Thr > ceiling (~125) → no FPS penalty |
| Pose | small | 130.4 | 121.1 | Host | Same |
| Pose | medium | 93.1 | **93.5** | **NPU** | Thr < ceiling → **FPS drops 25%** |
| Seg | nano | 48.7 | 48.7 | Host/NPU mixed | Thr ≈ ceiling → virtually no FPS penalty |
| Seg | medium | 48.7 | 48.7 | NPU mixed | Thr ≈ E2E → still similar |

**Conclusion**:
- **OD**: E2E is ~86 from nano to medium → **recommend medium** (no FPS penalty, better accuracy)
- **Pose**: E2E is ~121–125 from nano to small, drops to 93.5 at medium → **recommend small** (medium incurs 25% FPS drop)
- **Seg**: E2E is ~49 from nano to large → **medium is viable** (NPU compute is slow, so model size has little impact)
- Overall, **ORT OFF** is recommended on RPi 5

#### E2E ORT ON/OFF Summary

| Environment | OD | Pose | Seg | Primary Bottleneck |
|-------------|:--:|:----:|:---:|-------------------|
| i7 | Either | Either | Either | None (reference) |
| AIBox | OFF | OFF | OFF | CPU core count (4) |
| Biostar | **ON** | OFF | **ON** | Host supply limit (4× NPU) |
| OrangePi | **ON** | OFF | **ON** | ARM raw tensor post-processing |
| Radxa | **ON** | OFF | **ON** | ARM raw tensor post-processing |
| RPi 5 | **OFF** | **OFF** | **OFF** | SW decoding + PCIe |

> **Note**: The table above is based on lightweight models (nano–small). For medium and above, NPU compute time dominates and ORT ON/OFF differences are small. OBB and Classification follow trends similar to OD/Seg.

### 4.3 Thermal Throttling

When the NPU temperature reaches a certain threshold (approximately 85–90°C), the firmware forcibly reduces the clock frequency from 1000 MHz. This state is maintained until the temperature drops sufficiently, and inference performance degrades during the frequency-reduced interval.

#### Throttling Case Study on OrangePi

OrangePi is a compact ARM SBC with limited thermal dissipation. Even during single-stream E2E measurement, medium and larger models cause NPU temperature to reach 83–85°C, and **measurement variance (σ) increases sharply**:

| Task | Model | ORT | E2E FPS | NPU Temp | σ Ratio |
|------|-------|-----|:-------:|:--------:|:------:|
| OD | medium | ON | 83.3 ±9.7 | 83–85°C | 11.7% |
| Pose | medium | ON | 73.5 ±7.2 | 83–85°C | 9.8% |
| Pose | large | ON | 57.4 ±5.1 | 84–85°C | 8.9% |

In contrast, lightweight models (OD nano, ORT ON) on the same environment stay at 63–66°C with σ below 1%.


#### Practical Implications

- For sustained long-duration operation, monitor NPU temperature (`dxtop`) and ensure adequate thermal solutions (heatsinks, fans)
- When running medium or larger models for extended periods on compact SBCs (OrangePi, AIBox, etc.), factor in performance degradation from throttling
- Even desktop-class environments like Biostar can reach high temperatures during multi-channel heavy model operation, making thermal design important
---

## 5. Deployment Guide by Environment

### 5.1 Environment Selection Criteria

```
Selection Criteria:
┌─ Required channels ≥ 8?     → Biostar H1 (4× M1)
├─ Required channels 4–7?     → i7 + M1 or AIBox + M1
├─ Required channels 1–3?     → ARM SBC (OrangePi/Radxa) viable
└─ Low power/compact required? → RPi 5 (limited to lightweight models)
```

### 5.2 Recommendations by Environment

#### i7 (Desktop, M1 × 1, Gen3 ×4)

- **Strengths**: Abundant CPU resources, sufficient PCIe bandwidth, stable - performance across all tasks
- **Weaknesses**: High power consumption and large form factor
- **Best for**: Development/test environments, PoC, single NPU maximum performance
- **Note**: ORT ON/OFF difference is negligible — either is acceptable

| Task | Recommended Model | Max Ch (30fps) |
|------|------------------|:--------------:|
| Object Detection | nano–small | 4–8 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 3–6 |
| OBB | nano | 2 |

#### AIBox (N97, M1 × 1, Gen3 ×2)

- **Strengths**: Low power (TDP 12W), compact, fanless capable
- **Weaknesses**: Limited to 4 CPU cores, CPU bottleneck in Segmentation post-processing
- **Best for**: Edge deployment, compact devices, 1–5 channel operation
- **Note**: Watch for CPU bottlenecks in Seg tasks. Be aware of increased CPU load when using ORT ON

| Task | Recommended Model | Max Ch (30fps) |
|------|------------------|:--------------:|
| Object Detection | nano–small | 4–6 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 3 |
| OBB | nano | 2 |

#### Biostar (Ryzen 5 9600X, H1 × 1, Gen3 ×4)

- **Strengths**: Highest absolute performance with H1 (4× M1) package, 6-core (12-thread) CPU headroom
- **Weaknesses**: Cost, requires H1-specific board
- **Best for**: Multi-channel video analytics server, centralized deployment
- **Note**: E2E for lightweight models drops relative to Throughput because the CPU cannot fully supply 4× NPUs

| Task | Recommended Model | Max Ch (30fps) |
|------|------------------|:--------------:|
| Object Detection | nano–xlarge | 5–17 |
| Pose Estimation | nano–xlarge | 5–19 |
| Segmentation | nano–xlarge | 3–12 |
| OBB | nano–large | 1–10 |

#### OrangePi 5 Plus (A55 8-core, M1 × 1, Gen3 ×4)

- **Strengths**: Widest PCIe bandwidth among ARM SBCs (Gen3 ×4), excellent price-to-performance ratio
- **Weaknesses**: Limited CPU compute (A55 is a low-power core), E2E paradox with ORT OFF
- **Best for**: Edge devices, 1–3 channel lightweight tasks
- **Note**: ORT ON recommended (higher E2E FPS for OD). Severe CPU bottleneck with heavy models (large–xlarge)

| Task | Recommended Model | Max Ch (30fps) |
|------|------------------|:--------------:|
| Object Detection | nano–small | 3–5 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 2–3 |

#### Radxa Rock 5B+ (RK3588 8-core, M1 × 1, Gen3 ×2)

- **Strengths**: Compact, low power, ARM ecosystem
- **Weaknesses**: Bandwidth limited to Gen3 ×2, CPU limitations similar to OrangePi
- **Best for**: Edge devices, low-power operation
- **Note**: Similar characteristics to OrangePi. Gen3 ×2 slightly limits bandwidth for lightweight models, but the practical impact is minor

| Task | Recommended Model | Max Ch (30fps) |
|------|------------------|:--------------:|
| Object Detection | nano–medium | 2–4 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 2–3 |

#### RPi 5 (A76 4-core, M1 × 1, Gen2 ×1)

- **Strengths**: Broadest ecosystem, affordable, ultra-compact
- **Weaknesses**: **Gen2 ×1 PCIe** + **SW decoding** results in very low NPU utilization for lightweight models (OD nano: 24%), limited to 4 CPU cores
- **Best for**: PoC, learning/prototyping, single-channel lightweight operation
- **Note**: Dual constraint of SW decoding + PCIe limits E2E ceiling per task. ORT OFF required

| Task | Recommended Model | Max Ch (30fps) | Rationale |
|------|------------------|:--------------:|-----------|
| Object Detection | medium | 2 | nano–medium E2E identical (~86), medium has better accuracy |
| Pose Estimation | small | 2–4 | nano–small E2E similar (~121–125), drops from medium onward |
| Segmentation | medium or smaller | 1 | nano–large E2E similar (~49), model size has little impact |

RPi 5's key characteristic: SW decoding consumes a significant portion of the CPU, so the per-task host pipeline throughput determines the E2E ceiling. Within the range of models whose Throughput exceeds this ceiling, **scaling up the model incurs no FPS penalty while gaining accuracy**. Pose is the exception — Throughput drops below the ceiling at medium, so small is recommended.

### 5.3 Model Selection Decision Tree

```
Goal: 30fps × N-channel operation

1. Select desired task (OD / Pose / Seg / OBB / Cls)
2. Check the per-environment Max Channels table for models supporting N channels
3. Among viable models, select the largest (most accurate) one
4. Compare ORT ON/OFF:
   - x86 environments: Generally prefer ORT OFF (CPU headroom)
   - ARM environments: ORT ON may be advantageous (reduced output size effect)
5. Verify thermal stability:
   - Monitor NPU temperature during sustained operation (dxtop)
   - If high temperature reached → throttling → improve cooling or reduce model size
```

---

## 6. Conclusion

### 6.1 Key Takeaways

1. **NPU throughput (Throughput) is nearly identical across single-M1 environments with PCIe Gen3 or above.** OD medium Throughput (ORT OFF) converges to i7: 96.5, AIBox: 96.0, OrangePi: 96.4. Radxa is slightly lower at 90.8, likely due to the combined effect of Gen3 ×2 and ARM host overhead.

2. **PCIe bandwidth affects Throughput only for lightweight models.** RPi 5 (Gen2 ×1) OD nano Throughput (91.5) is only 38% of i7 (242.1), but for xlarge, RPi 5 (40.5) and i7 (40.3) are identical. As NPU compute time grows, the PCIe transfer time becomes negligible.

3. **E2E pipeline performance is heavily influenced by the host environment, and the bottleneck cause differs by environment.**
   - **RPi 5**: SW decoding (`avdec_h264`) + PCIe dual constraint. OD nano E2E (86.6) is lower than Throughput (91.5), with different E2E ceilings per task (Pose about 125, OD about 86, Seg about 49). CPU host throughput determines the ceiling.
   - **OrangePi/Radxa**: HW decoding + RGA pre-processing are used, but ORT OFF raw tensor post-processing causes backpressure that starves the NPU. OrangePi OD nano E2E is ORT ON (170.2) vs OFF (96.7) — a 76% gap.
   - **Biostar H1**: The combined throughput of 4× NPUs is so high that the sequential processing speed of a single pipeline cannot keep up with NPU consumption speed. For OD nano, E2E (498.3) / Throughput (927.9) = 54%. Host CPU performance and multi-channel deployment are the keys to leveraging H1 (details: item 5).

4. **Segmentation tasks have overwhelmingly higher CPU load in the post-processing stage compared to other tasks.** Mask generation (coefficient × prototype + bilinear interpolation) is performed every frame regardless of ORT ON/OFF, and on AIBox, Seg nano E2E CPU% reaches 360–369 (90–92% of 4 cores). This severely limits E2E in environments with limited CPU cores.

5. **H1 (4× M1) has exceptional inference capability, so host performance must keep pace to fully exploit it.** H1 Throughput is approximately 3.8× that of a single M1 (OD nano: 929 vs 242). However, even on Ryzen 5 9600X (6 cores/12 threads), single-stream E2E for lightweight models reaches only 61–63% of Throughput:

   | Task | Model | Throughput | E2E | E2E/Thr | NPU Avg% |
   |------|-------|:---------:|:---:|:-------:|:--------:|
   | OD | nano | 927.9 | 498.3 | 54% | 26.4% |
   | OD | small | 536.4 | 497.5 | 93% | 65.6% |
   | OD | medium | 371.3 | 372.4 | 100% | 80.1% |
   | Pose | nano | 889.6 | 542.7 | 61% | 30.3% |
   | Seg | nano | 573.1 | 363.0 | 63% | 24.9% |
   | Seg | medium | 266.0 | 265.7 | 100% | 82.0% |

   > ORT ON, single-stream E2E measurement.

   The fact that NPU Avg% is only 25–31% for nano models is because the sequential processing speed of a single GStreamer pipeline — 'decoding → pre-processing → NPU submission' — cannot keep up with the consumption speed of 4 NPUs. CPU compute resources themselves have headroom (CPU% 173–539 / max 1200%), but the **structural seriality of the pipeline** is the bottleneck. Starting from medium models, NPU compute time is long enough for the host to keep up, resulting in E2E ≈ Throughput (~100%).

   **Practical Implications**: H1's true value is realized in multi-channel concurrent processing. With multiple pipelines supplying frames to the NPU in parallel, the seriality limitation of a single pipeline can be bypassed. In practice, Max Channels results show OD nano at 17ch, Pose nano at 19ch. When deploying with H1, choose a **host with sufficient CPU cores and memory bandwidth** to support this throughput. For post-processing-heavy tasks like Seg, note that even a single-stream nano reaches CPU% of 539 (45% of max).

6. **The optimal ORT ON/OFF setting varies by environment × task combination.** There is no single rule — refer to the E2E ORT ON/OFF Summary (§4.2).

   | Environment | OD | Pose | Seg | Key Factor |
   |-------------|:--:|:----:|:---:|-----------|
   | i7 | Either | Either | Either | Ample CPU headroom |
   | AIBox | OFF | OFF | OFF | ORT ON CPU contention |
   | Biostar | **ON** | OFF | **ON** | Raw post-processing burden at 4×NPU frame rates |
   | OrangePi/Radxa | **ON** | OFF | **ON** | Raw tensor backpressure on ARM CPU |
   | RPi 5 | **OFF** | **OFF** | **OFF** | SW decoding and CPU contention |

### 6.2 Recommendations

**Hardware Selection**:
- Multi-channel/high performance → Biostar H1 (OD 17ch, Pose 19ch, Seg 12ch @30fps). For lightweight model multi-channel operation, 12+ host CPU cores recommended — if the host cannot match H1's throughput, NPUs will go idle (see §6.1, item 5)
- Single/few channels + x86 edge → AIBox (OD 4–6ch, Pose 4–7ch @30fps)
- Single/few channels + ARM edge → OrangePi (OD 3–5ch) or Radxa (OD 2–4ch)
- PoC/prototype → RPi 5 (limited to OD 2ch, Pose OFF 4ch, Seg 1ch)

**Model Selection**:
- When FPS headroom exists, use the largest model possible to maximize accuracy
- On RPi 5, host pipeline determines the E2E ceiling — recommend the largest model within the per-task ceiling (OD: medium, Pose: small–nano, Seg: medium)
- When Segmentation is needed, factor CPU core count into environment selection

**ORT ON/OFF Configuration**:
- i7/AIBox (x86, CPU headroom): ORT OFF by default
- OrangePi/Radxa (ARM): ORT ON required for OD and Seg, ORT OFF for Pose
- RPi 5: **ORT OFF for all tasks** (avoids contention with SW decoding)
- Biostar H1: ORT ON favorable for OD and Seg, ORT OFF for Pose
- For medium and larger models, ORT ON/OFF difference is small — either is acceptable

**Thermal Management**:
- On OrangePi, medium+ model E2E reaches NPU 83–85°C with σ 9–12% variance
- On AIBox, Seg Multi-Stream shows σ 11–14% variance
- For sustained long-duration operation, monitor NPU temperature with `dxtop` and ensure heatsinks/fans are in place

---

## 7. Appendix

### 7.1 Reproducing the Benchmark

```bash
# Run the full benchmark
python -m dx_stream.apps.benchmark run

# Run only specific model sizes/tasks
python -m dx_stream.apps.benchmark run --sizes n,s --task object_detection

# Retry only failed items from existing results
python -m dx_stream.apps.benchmark run --resume results/<run_id> --retry-failed

# Generate dashboard
python -m dx_stream.apps.benchmark dashboard dx_stream/apps/benchmark/results
```

### 7.2 Measurement Protocol Key Parameters

| Parameter | Value | Notes |
|-----------|-------|-------|
| Throughput measurement duration | 30 seconds | |
| Latency loop count | 300 | Fixed loop, `-l` mode |
| Throughput repetitions | 3 | |
| E2E repetitions | 3 | |
| Warmup count | 1 | Excluded from results |
| Multi-Stream FPS threshold | 30 fps | Minimum FPS per channel |
| Cooldown target ΔT | 10°C | Relative to idle |
| Cooldown max wait | 300 seconds | |

### 7.3 Detailed Benchmark Results by Environment

Full benchmark results for each environment are available in the following files:

| Environment | Report |
|-------------|--------|
| i7 | [`results/i7-14700K_M1/20260421_180106/REPORT.md`](results/i7-14700K_M1/20260421_180106/REPORT.md) |
| AIBox | [`results/DX-AIPlayer-N97_M1/20260421_201939/REPORT.md`](results/DX-AIPlayer-N97_M1/20260421_201939/REPORT.md) |
| Biostar | [`results/BIOSTAR_H1/20260421_201621/REPORT.md`](results/BIOSTAR_H1/20260421_201621/REPORT.md) |
| OrangePi | [`results/OrangePi5+_M1/20260418_171936/REPORT.md`](results/OrangePi5+_M1/20260418_171936/REPORT.md) |
| Radxa | [`results/ROCK5B+_M1/20260422_111610/REPORT.md`](results/ROCK5B+_M1/20260422_111610/REPORT.md) |
| RPi 5 | [`results/RPi_M1/20260421_205004/REPORT.md`](results/RPi_M1/20260421_205004/REPORT.md) |

### 7.4 Glossary

| Term | Description |
|------|-------------|
| **Throughput** | Pure NPU asynchronous throughput measured by run_model (FPS) |
| **E2E FPS** | Actual throughput in a GStreamer pipeline (includes full decoding-to-output chain) |
| **Latency** | Inference latency for a single frame (ms) |
| **Max Channels** | Maximum number of concurrent streams maintaining ≥30fps per channel |
| **ORT** | ORT ON executes CPU offload layers via OnnxRuntime alongside NPU; ORT OFF returns NPU results only without the offload path |
| **NPU Avg%** | Average NPU utilization during measurement |
| **Backpressure** | Mechanism in GStreamer pipelines where downstream processing delays propagate upstream through queues, limiting overall pipeline throughput |
| **push_queue** | Size-limited queue inside dxinfer (MAX_PUSH_QUEUE_SIZE=5 × device count). When full, new inference request submission is blocked |
| **Thermal Throttling** | Forced clock frequency reduction when NPU temperature reaches a certain threshold (approximately 85–90°C) |

---

*This document is based on measurement results from the dx_stream benchmark tool.*
