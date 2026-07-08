# YOLO26 × DEEPX NPU 벤치마크 분석 보고서

## 요약

아래 표는 **최신 벤치마크 결과**를 기준으로, **FHD 30fps 비디오 입력**에서 각 환경이 **가장 가벼운 nano 모델**로 보여준 대표적인 실사용 성능을 요약한다. 수치는 **nano 모델에 대해 ORT ON/OFF 중 더 높은 결과**를 사용했다.

정확도와 FPS는 trade-off 관계이므로, 이 요약을 넘어서는 모델 크기 선택과 더 큰 모델을 포함한 절대 최대 성능은 본문의 상세 벤치마크 결과를 참고하면 된다.

| 환경 | OD | Pose | Seg | OBB |
|------|----|------|-----|-----|
| i7 | 243.5 FPS / 8ch | 230.0 FPS / 7ch | 188.2 FPS / 6ch | 80.2 FPS / 2ch |
| AIBox | 188.4 FPS / 6ch | 209.6 FPS / 7ch | 115.2 FPS / 3ch | 80.0 FPS / 2ch |
| Biostar H1 | 498.3 FPS / 16ch | 557.8 FPS / 19ch | 363.0 FPS / 12ch | 307.9 FPS / 10ch |
| OrangePi 5 Plus | 170.2 FPS / 5ch | 231.6 FPS / 7ch | 115.0 FPS / 3ch | 81.2 FPS / 2ch |
| Radxa Rock 5B+ | 151.0 FPS / 4ch | 215.6 FPS / 7ch | 97.8 FPS / 3ch | 75.0 FPS / 2ch |
| RPi 5 | 86.6 FPS / 2ch | 124.7 FPS / 4ch | 48.7 FPS / 1ch | 79.4 FPS / 2ch |

> 표기: `single-stream 최대 E2E FPS / multi-stream 최대 채널 수(30fps/channel 기준)`
>
> Classification은 일반적인 E2E 비디오 분석 및 멀티스트림 배포 시나리오를 대표하기에는 적합성이 낮아 본 요약 표에서는 제외했다. 


## 목차

1. [개요](#1-개요)
2. [실험 환경](#2-실험-환경)
3. [벤치마크 설계](#3-벤치마크-설계)
4. [성능 결정 요인 분석](#4-성능-결정-요인-분석)
5. [환경별 실사용 가이드](#5-환경별-실사용-가이드)
6. [결론](#6-결론)
7. [부록](#7-부록)

---

## 1. 개요

### 1.1 문서 목적

본 문서는 DEEPX M1/H1 NPU에서 YOLO26 모델군의 추론 성능을 **6가지 호스트 환경** 에서 체계적으로 측정한 결과를 분석한다. 단순히 숫자를 나열하는 것이 아니라, **"왜 이 환경에서 이런 성능이 나오는가"** 에 대한 원인을 추적하고, 실사용에서 고려해야 할 사항을 정리하는 것이 목적이다.

### 1.2 측정 대상

- **모델**: YOLO26 계열 25개 모델
  - 5가지 크기: nano(n), small(s), medium(m), large(l), xlarge(x)
  - 5가지 태스크: Object Detection, Pose Estimation, Segmentation, Oriented BBox (OBB), Classification
- **NPU**: DEEPX M1 (단일 칩) 및 H1 (4× M1 패키지)
- **측정 항목**: Latency, Throughput, E2E Pipeline FPS, Multi-Stream 채널 수용량

### 1.3 핵심 발견 미리보기

1. **같은 NPU라도 호스트에 따라 최대 2.6배의 성능 차이가 발생**한다. 병목 지점은 환경마다 다르다.
2. **PCIe 레인/세대** 가 가벼운 모델의 NPU 포화를 결정하는 핵심 변수다.
3. Segmentation과 같은 **CPU 후처리가 무거운 태스크**에서는 호스트 CPU 성능이 E2E 성능의 결정적 요인이 된다.
4. ARM 환경에서 ORT OFF의 무거운 후처리는 GStreamer **backpressure 메커니즘을 통해 NPU 자체를 굶기는** 현상을 유발한다.

---

## 2. 실험 환경

### 2.1 환경 사양 비교

| 환경 | CPU | 코어 | RAM | NPU | PCIe | OS |
|------|-----|:----:|:---:|-----|------|----|
| **i7** | Intel i7-14700K | 20 (28T) | 62.5 GB | M1 × 1 | Gen3 ×4 | Ubuntu 24.04 |
| **AIBox** | Intel N97 | 4 | 7.5 GB | M1 × 1 | Gen3 ×2 | Ubuntu 24.04 |
| **Biostar** | AMD Ryzen 5 9600X | 6 (12T) | 30.5 GB | H1 × 1 (M1 ×4) | Gen3 ×4 | Ubuntu 22.04 |
| **OrangePi** | Cortex-A55 | 8 | 15.6 GB | M1 × 1 | Gen3 ×4 | Debian 12 |
| **Radxa** | RK3588 | 8 | 7.8 GB | M1 × 1 | Gen3 ×2 | Debian 12 |
| **RPi 5** | Cortex-A76 | 4 | 7.9 GB | M1 × 1 | **Gen2 ×1** | Debian 12 |

### 2.2 환경 특성 요약

**i7**:
Intel i7-14700K 기반의 데스크톱 x86 환경이다. `vah264dec` 하드웨어 디코딩을 사용하며, 단일 M1 기준으로는 CPU와 디코더 여유가 충분해 host-side 병목이 상대적으로 작다. 따라서 이 환경은 single-M1 기준 성능 reference로 해석하기 가장 쉽다.

**AIBox**:
Intel N97 4코어 기반의 저전력 x86 환경이다. `vah264dec` 하드웨어 디코딩을 사용하지만, CPU 자원이 제한적이어서 Segmentation과 같이 후처리가 무거운 태스크에서는 host-side 병목이 빠르게 드러난다. PCIe Gen3 ×2는 충분한 편이지만, CPU 4코어 제약 때문에 i7처럼 여유 있는 환경은 아니다.

**Biostar**:
AMD Ryzen 5 9600X + H1(4× M1) 조합의 고성능 데스크톱 환경이다. `vaapidecodebin` 하드웨어 디코딩을 사용하고 CPU 자체도 강하지만, H1의 raw throughput이 워낙 높아 가벼운 모델에서는 오히려 디코딩, 전처리, 후처리, 결과 배출 같은 host-side 경로가 먼저 병목이 된다. 즉, "병목이 없다"기보다 **NPU가 너무 빨라서 다른 단계가 병목으로 드러나는 환경**으로 보는 것이 맞다.

**OrangePi**:
Cortex-A55 8코어 기반 ARM SBC다. `mppvideodec` 하드웨어 디코딩을 사용하므로 RPi 5처럼 소프트웨어 디코딩 병목이 전면에 나오지는 않는다. 다만 CPU 연산력이 강하지 않아, ORT OFF의 raw tensor 후처리나 heavy model에서는 host-side 부담과 backpressure가 쉽게 나타난다.

**Radxa**:
RK3588 기반 ARM SBC다. OrangePi와 마찬가지로 `mppvideodec` 하드웨어 디코딩을 사용한다. PCIe는 Gen3 ×2로 OrangePi보다 좁지만, 실사용에서는 대역폭 자체보다 ARM CPU의 후처리 처리 여력과 host-side pipeline 비용이 더 먼저 드러나는 경우가 많다.

**RPi 5**:
Cortex-A76 4코어 기반 ARM SBC다. 이 환경은 `avdec_h264` 소프트웨어 디코딩을 사용하며, 동시에 PCIe가 **Gen2 ×1**로 매우 좁다. 따라서 다른 ARM 보드와 달리 디코딩 부담과 PCIe 병목이 함께 겹치며, 특히 light model에서는 NPU 활용률이 매우 낮게 나타난다.

### 2.3 PCIe 대역폭 비교

PCIe 대역폭은 호스트와 NPU 간 데이터 전송 속도의 물리적 상한이다.

| 구성 | 단방향 대역폭 (이론) | 비고 |
|------|:-------------------:|------|
| Gen3 ×4 | ~3.9 GB/s | i7, Biostar, OrangePi |
| Gen3 ×2 | ~2.0 GB/s | AIBox, Radxa |
| Gen2 ×1 | ~0.5 GB/s | RPi 5 |

Gen3 ×4 대비 RPi 5(Gen2 ×1)는 이론 대역폭이 **약 1/8** 수준이다. 이 차이가 가벼운 모델에서 극적으로 드러난다.

---

## 3. 벤치마크 설계

### 3.1 4계층 측정 구조

벤치마크는 순수 NPU 성능부터 실제 파이프라인 성능까지 4단계로 측정한다.

**1단계: Model Latency** — NPU 추론 지연 시간
- `run_model -m <model> -l 300` (동기, 단일 코어)
- 하나의 입력을 보내고 결과가 돌아올 때까지의 시간
- ORT ON일 때: Total = NPU ms + CPU ms (ORT CPU offload 연산 시간 포함)

**2단계: Model Throughput** — NPU 추론 처리량
- `run_model -m <model> -t 30` (비동기, 멀티코어)
- 6개의 요청을 동시에 in-flight로 유지하며 30초간 처리한 프레임 수
- 디코딩, 전/후처리 없이 **동일한 텐서를 반복 제출** → 순수 NPU + PCIe 성능

**3단계: E2E Single-Stream** — 단일 스트림 파이프라인 처리량
- GStreamer 파이프라인 토폴로지: `decodebin ! dxpreprocess ! queue ! dxinfer ! queue ! dxpostprocess ! queue ! fakesink`
- 모든 queue는 `leaky=no`로 설정되어, downstream이 느려지면 **backpressure가 upstream으로 전파**된다
- `dxinfer` 내부에는 비동기 push thread와 크기 제한이 있는 push_queue(`MAX_PUSH_QUEUE_SIZE=5 × 디바이스 수`)가 있다
- 실제 동영상(1080p H.264, 30 FPS)을 입력으로 사용
- 디코딩, 전처리, 후처리의 CPU 비용이 모두 반영된 **실사용 FPS**

**4단계: Multi-Stream** — 다채널 동시 처리 수용량
- **하나의 GStreamer 파이프라인** 안에서 `dxinputselector`/`dxoutputselector`로 N개 스트림을 멀티플렉싱
- N개의 입력 분기(`urisourcebin ! decodebin ! queue`)가 공유 추론 경로(`dxpreprocess ! dxinfer ! dxpostprocess`)를 거쳐 N개의 출력 분기(`queue ! fakesink`)로 디멀티플렉싱
- 채널당 FPS가 **30fps 이상**인 최대 동시 스트림 수(`Max Channels`)를 기록
- 실제 배포 시 필요한 **최대 동시 처리 용량**을 직접 확인

### 3.2 설계 시 고려 사항

**열적 안정성 (Thermal Stability)**

NPU 온도가 특정 임계점(약 85~90°C 부근)에 도달하면 클럭이 강제로 낮아져(thermal throttling) 성능이 급격히 떨어진다. 이를 방지하기 위해:

- **각 모델 × ORT 조합 블록 시작 시점에 한 번만** cooldown을 수행한다.
- cooldown 시작 시 처음 측정한 idle 온도 기준으로 **idle + 10°C 이하** 가 될 때까지 최대 300초 대기한다.
- cooldown은 **Latency 앞에만** 적용되며, Latency와 Throughput 사이, Throughput과 E2E 사이, E2E와 Multi-Stream 사이에는 별도의 cooldown이 없다.

**측정 순서 설계**

실제 코드가 수행하는 순서는 **모델별** 로 다음과 같다:

1. 모델 선택
2. ORT ON 블록 시작
3. `cooldown` 수행
4. Latency warmup 1회 후 Latency 측정
5. Throughput warmup 1회 후 Throughput 측정
6. E2E Single-Stream warmup 1회 후 E2E 측정
7. Multi-Stream sweep 수행
8. ORT OFF 블록 시작
9. 다시 `cooldown` 수행
10. 같은 순서를 반복

즉, ORT ON과 ORT OFF는 같은 모델 안에서 **독립적인 실행 블록** 으로 취급되며, 각 블록의 시작점만 냉각 상태에 가깝게 맞춘다. 이후 Latency → Throughput → E2E → Multi-Stream은 **연속으로 이어 실행** 된다. 이는 Throughput과 E2E를 이미 어느 정도 가열된 상태에서 측정하게 하여 실제 장시간 동작 조건에 더 가깝게 맞추기 위한 설계다.


**ORT CPU Offload 비교**

모든 모델에 대해 ORT(OnnxRuntime) CPU Offload가 ON인 경우와 OFF인 경우를 모두 측정한다. ORT ON은 모델의 일부 연산을 NPU가 아닌 CPU에서 실행한다. 단, CPU 연산 시간만큼 Latency가 늘어나고, CPU 부하가 증가한다.

**반복 측정과 통계**

- Latency: warmup 1회 후, `-l 300` 루프 기반 측정 1회
- Throughput: warmup 1회 후, 30초 측정 3회
- E2E Single-Stream: warmup 1회 후, 3회 측정
- Multi-Stream: **각 stream count마다** warmup 1회 후, 3회 측정
- warmup 실행은 결과에 포함하지 않음
- E2E Single-Stream과 Multi-Stream의 warmup/run은 timeout 발생 시 1회 재시도한다.

### 3.3 Multi-Stream 수용량 측정

Multi-Stream은 단순히 1, 2, 3, ... N을 끝까지 모두 측정하는 방식이 아니다. 실제 구현은 다음과 같이 동작한다.

1. 먼저 Single-Stream E2E FPS를 기준으로 시작 stream count를 추정한다.
2. `1ch` 결과는 별도 파이프라인을 다시 실행하지 않고, **Single-Stream 결과를 재사용** 한다.
3. 이후 추정된 시작점부터 stream count를 증가 또는 감소시키며 경계(boundary)를 찾는다.
4. 채널당 FPS가 **30fps 이상** 인 최대 stream count를 `Max Channels`로 기록한다.

즉, Multi-Stream은 "전 구간 brute-force sweep"이 아니라, **single-stream 기반 boundary search** 에 가깝다. 이는 총 측정 시간을 크게 줄이면서도 실사용 관점에서 필요한 용량 지점을 빠르게 찾기 위한 설계다.

---


## 4. 성능 결정 요인 분석

### 4.1 Model Throughput

`run_model`로 측정한 Model Throughput은 디코딩·전/후처리 없이 **동일한 입력 텐서를 반복 제출**하여 NPU의 순수 처리량을 측정한 값이다. 같은 DX-M1 칩에 같은 모델을 올려도, 호스트 환경에 따라 이 수치가 크게 달라지는 경우가 있다.

#### PCIe 대역폭과 모델 크기

NPU 추론은 다음 과정을 거친다:
1. 호스트 → NPU: 입력 텐서 전송 (PCIe DMA)
2. NPU: 추론 실행
3. NPU → 호스트: 출력 텐서 전송 (PCIe DMA)

**무거운 모델**(medium–xlarge)은 NPU 연산 시간이 40–80 ms로, 데이터 전송 시간(~0.5 ms)이 전체의 1% 미만이다. 따라서 PCIe 대역폭이 성능에 미치는 영향은 무시할 수 있으며, 실제로 M1 단일 칩 환경에서 OD medium 모델의 NPU Throughput(ORT OFF)이 91–96 FPS로 거의 동일하게 수렴한다.

**가벼운 모델**(nano)은 NPU 연산 시간이 ~5 ms로 짧아, 데이터 전송 시간의 비중이 상대적으로 커진다. 이 때문에 같은 OD nano 모델이라도 환경에 따라 최대 2.6배 차이(RPi 5: 91.5 FPS vs i7: 242.1 FPS, ORT OFF)가 발생한다.

#### RPi 5의 사례: Gen2 ×1의 한계

RPi 5에서 OD nano Throughput은 91.5 FPS(ORT OFF)인 반면, 같은 M1 칩의 i7에서는 242.1 FPS다. RPi 5 OD nano의 NPU Avg%가 **24.2%**에 불과하다는 점이 이를 뒷받침한다 — NPU가 70% 이상 idle인 상태로, PCIe Gen2 ×1(~0.5 GB/s)이 데이터를 충분히 빠르게 전달하지 못해 NPU가 입력을 기다리는 시간이 대부분이다.

반면 OD xlarge 모델은 RPi 5에서도 NPU Avg% 88% 이상으로, PCIe가 충분하다. 연산 시간이 길어서 전송 시간이 상대적으로 무시할 수 있기 때문이다.

#### 환경별 NPU 활용률 비교 (OD nano, ORT OFF, Throughput 측정)

| 환경 | PCIe | Throughput | NPU Avg% | 해석 |
|------|------|:---------:|:--------:|------|
| i7 | Gen3 ×4 | 242.1 | 91.2% | PCIe 충분, NPU 고활용 |
| AIBox | Gen3 ×2 | 222.0 | 80.6% | PCIe 충분, host submit 오버헤드 잔존 |
| OrangePi | Gen3 ×4 | 237.6 | 88.7% | PCIe 충분, NPU 고활용 |
| Radxa | Gen3 ×2 | 222.4 | 87.5% | PCIe 충분, host/runtime 오버헤드 잔존 |
| RPi 5 | Gen2 ×1 | 91.5 | 24.2% | **PCIe 심각한 병목** |
| Biostar | Gen3 ×4 (×4칩) | 929.3 | 92.7% | 4칩 병렬, 높은 활용률 |

RPi 5를 제외한 M1 단일 칩 환경에서 OD nano Throughput이 222–242 범위로 비슷한 것은, PCIe Gen3 이상에서는 **대역폭 부족이 더 이상 1차 병목이 아니기 때문**이다. 남는 차이는 host CPU의 요청 제출 오버헤드, 플랫폼/드라이버 오버헤드, 런타임 스케줄링 차이에서 발생한다.

### 4.2 E2E 파이프라인 성능

E2E(End-to-End) 파이프라인은 실제 사용 환경을 모사한 측정으로, GStreamer 파이프라인을 통해 실제 동영상(1080p H.264)을 처리한다. Model Throughput과 달리, 호스트의 **디코딩**, **전처리**, **후처리** 비용이 모두 반영되므로, 호스트 환경의 영향을 크게 받는다.

#### 파이프라인 처리 단계

E2E 파이프라인의 주요 처리 단계와 각 단계의 실행 위치는 환경에 따라 다르다:

| 단계 | 처리 내용 | 실행 위치 |
|------|----------|----------|
| **디코딩** | H.264 디코딩 | HW 가속(VA-API, MPP) 또는 SW(`avdec_h264`, CPU) — 환경별 상이 |
| **전처리** (dxpreprocess) | 리사이즈, 색공간 변환, 텐서 레이아웃 변환 | Rockchip 환경: RGA HW 가속, 그 외: libyuv(CPU) |
| **추론** (dxinfer) | dxnn 모델 추론 | NPU |
| **후처리** (dxpostprocess) | ORT ON/OFF에 따라 내용이 크게 다름 (아래 상세) | CPU |

디코딩 방식은 환경에 따라 다르며, 이것이 E2E 성능에 미치는 영향이 상당하다:

| 환경 | 디코더 | 가속 방식 |
|------|--------|----------|
| i7 | `vah264dec` | VA-API HW 가속 |
| AIBox | `vah264dec` | VA-API HW 가속 |
| Biostar | `vaapidecodebin` | VA-API HW 가속 |
| OrangePi | `mppvideodec` | Rockchip MPP HW 가속 |
| Radxa | `mppvideodec` | Rockchip MPP HW 가속 |
| RPi 5 | `avdec_h264` | **소프트웨어 디코딩** |

RPi 5만 소프트웨어 디코딩을 사용하며, 이로 인해 CPU 자원의 상당 부분이 디코딩에 소비된다.

#### ORT ON/OFF에 따른 후처리 비용

후처리(`dxpostprocess`)의 비용은 **ORT ON/OFF 여부와 태스크**에 따라 크게 달라진다.

**ORT ON** 시, `dxinfer`가 CPU offload 레이어까지 실행된 정제된 출력을 반환하므로, `dxpostprocess`는 가벼운 파싱만 수행한다:

| 태스크 | NPU 출력 형태 | dxpostprocess 처리 | 상대 비용 |
|--------|-------------|-------------------|:---------:|
| Classification | `[1, 1000]` scores | argmax | 극히 낮음 |
| Object Detection | `[1, N, 6]` (x1,y1,x2,y2,score,cls) | confidence 필터링 + 좌표 스케일링 | 낮음 |
| Pose Estimation | `[1, N, 57]` (bbox + 17×3 keypoints) | confidence 필터링 + 좌표 스케일링 | 낮음 |
| OBB | `[1, N, 7]` (cx,cy,w,h,score,cls,angle) | confidence 필터링 + AABB 변환 | 낮음 |
| Segmentation | `[1, 300, 38]` + `[1, 32, 160, 160]` proto | confidence 필터링 + **mask 생성**(coeff × proto, bilinear interpolation) | **높음** |

**ORT OFF** 시, NPU는 raw 멀티스케일 텐서를 그대로 반환하므로, `dxpostprocess`가 디코딩과 파싱을 모두 수행해야 한다:

| 태스크 | NPU 출력 형태 | dxpostprocess 처리 | 상대 비용 |
|--------|-------------|-------------------|:---------:|
| Object Detection | 6개 텐서 (cv2 bbox 3 + cv3 class 3, 80×80/40×40/20×20) | 3스케일 그리드 순회 + sigmoid + bbox decode | **중간** |
| Pose Estimation | 9개 텐서 (cv2 3 + cv4 kpt 3 + cv3 cls 3) | 3스케일 순회 + sigmoid + bbox/kpt decode (1 class → 비교적 가벼움) | 중간 |
| OBB | 9개 텐서 (cv2 3 + cv4 angle 3 + cv3 cls 3) | 3스케일 순회 + sigmoid + rotated bbox decode | 중간 |
| Segmentation | 10개 텐서 (cv2 3 + cv4 coef 3 + cv3 cls 3 + proto 1) | 3스케일 순회 + sigmoid + bbox/coef decode + **mask 생성** | **매우 높음** |

Segmentation의 mask 생성은 ORT ON/OFF 공통으로 수행되며, 매 프레임마다 검출된 객체별로 160×160 prototype에 대한 coefficient 내적 + bilinear interpolation을 수행해야 하므로 CPU 부하가 다른 태스크 대비 수배에 달한다. ORT OFF에서는 여기에 멀티스케일 디코딩 비용까지 더해진다.

#### ORT ON/OFF가 E2E에 미치는 영향

핵심은 ORT ON/OFF가 E2E에 미치는 영향이 **환경과 태스크에 따라 정반대**가 될 수 있다는 것이다. "어느 쪽이 무조건 우위"가 아니라, **"ORT CPU offload 비용 vs raw 텐서 후처리 비용" 중 어느 쪽이 파이프라인 병목을 더 키우는가**의 문제다.

#### Backpressure 메커니즘

E2E 파이프라인에서 CPU 병목은 단순히 "CPU가 바빠서 느려진다"가 아니라, GStreamer의 **backpressure 메커니즘**을 통해 NPU 활용률 자체를 떨어뜨린다. 모든 queue가 `leaky=no`로 설정되어 있으므로, downstream 처리 지연이 upstream으로 역류한다.

경로:
1. `dxpostprocess`의 `transform_ip()`가 동기적으로 후처리를 수행 — 이 함수가 느리면 downstream이 정체
2. `dxinfer` → `dxpostprocess` 사이의 queue가 가득 참
3. `dxinfer`의 push thread가 `gst_pad_push()`에서 블로킹
4. push_queue(`MAX_PUSH_QUEUE_SIZE=5 × 디바이스 수`)가 가득 차서 `primary_mode_infer()`가 새 추론을 제출하지 못함
5. **NPU에 새 입력이 공급되지 않아 NPU가 idle 상태에 빠짐**

즉, downstream의 느린 후처리가 pipeline을 역류하여 NPU를 굶기는 것이다.

#### 환경별 E2E 분석

##### i7 — CPU 여유가 충분한 레퍼런스 환경

i7(20코어/28스레드)은 HW 디코딩(`vah264dec`) + libyuv 전처리 + 풍부한 CPU 자원을 갖춘 환경으로, host-side 병목이 가장 적다.

| 태스크 | 모델 | ORT ON E2E | ORT OFF E2E | 분석 |
|--------|------|:----------:|:-----------:|------|
| OD | nano | 243.5 | 239.9 | 사실상 동일. raw 후처리도 x86 20코어에서 충분히 소화 |
| Pose | nano | 230.0 | 229.7 | 사실상 동일. 1 class라 후처리 비용 무시 |
| Seg | nano | 188.2 | 186.3 | 사실상 동일. mask 연산도 20코어에서 여유 |

**결론**: i7에서는 ORT ON/OFF가 E2E에 거의 영향을 미치지 않는다. CPU가 어떤 방식이든 충분히 처리할 수 있기 때문이다.

##### AIBox — 제한된 CPU에서의 균형

AIBox(N97, 4코어)는 HW 디코딩(`vah264dec`)을 사용하지만 CPU가 4코어(최대 CPU% = 400%)로 제한되어, 태스크에 따라 병목 패턴이 달라진다.

| 태스크 | 모델 | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | 분석 |
|--------|------|:-----------------:|:-------------------:|------|
| OD | nano | 172.7 (282) | **188.4** (307) | OFF가 +9%. raw 후처리도 x86에서 소화 가능 |
| Pose | nano | 182.9 (253) | **209.6** (207) | OFF가 +15%. 1 class로 raw 파싱 가벼움 |
| Seg | nano | 106.3 (360) | **115.2** (369) | OFF가 +8%. ON/OFF 모두 CPU 90–92% |

AIBox는 4코어 제약 때문에 모든 태스크에서 CPU가 높게 사용된다. ORT ON은 dxinfer 내부에서 추가 CPU 연산을 수행하므로 NPU에 프레임을 제출하는 속도가 느려지고, 결과적으로 E2E가 낮아진다. 특히 Seg에서는 ON/OFF 모두 CPU%가 360–369(전체의 90–92%)에 달해 CPU가 거의 포화 상태다.

**결론**: AIBox에서는 ORT OFF가 전반적으로 유리하다. 다만 태스크가 무거워지면 CPU 총량에 주의가 필요하다.

##### Biostar H1 — NPU가 너무 빨라 호스트가 병목

Biostar(Ryzen 5 9600X, 6코어/12스레드, H1=4× M1)는 한 가지 특수한 문제를 가지고 있다: **4개 NPU의 합산 Throughput이 너무 높아 호스트가 공급을 따라가지 못한다.**

| 태스크 | 모델 | Throughput (ORT OFF) | ORT ON E2E | ORT OFF E2E | E2E/Thr 비율 |
|--------|------|:---------:|:----------:|:-----------:|:------------:|
| OD | nano | 929.3 | **498.3** | 463.1 | 50–54% |
| Pose | nano | 888.8 | 542.7 | **557.8** | 61–63% |
| Seg | nano | 609.7 | **363.0** | 323.0 | 53–60% |

6코어(12스레드) CPU로도 4× NPU에 프레임을 충분히 공급하지 못해, 가벼운 모델에서 E2E가 Throughput의 50–63% 수준에 그친다. OD와 Seg에서 ORT ON이 더 높은 이유는, 4× NPU의 높은 프레임률에서 raw 텐서 후처리 부담이 ORT CPU offload 비용보다 크기 때문이다. Pose는 1 class라 raw 파싱이 가벼워 ORT OFF가 약간 높다.

**결론**: H1 환경에서는 가벼운 모델일수록 host 공급 한계가 두드러지며, medium 이상 모델에서 E2E/Throughput 비율이 개선된다.

##### OrangePi / Radxa — ARM CPU에서의 ORT OFF Backpressure

OrangePi(A55 8코어)와 Radxa(RK3588 8코어)는 `mppvideodec`으로 **HW 디코딩**, `dxpreprocess`에서 **RGA HW 전처리**를 사용하므로, RPi 5와 달리 디코딩/전처리 자체는 CPU 부담이 작다. 그러나 ARM CPU의 연산력이 제한적이어서, **ORT OFF의 raw 텐서 후처리에서 backpressure가 발생**하는 태스크가 있다.

**Object Detection:**

| 환경 | 모델 | ORT | E2E FPS | CPU% | NPU Avg% |
|------|------|-----|:-------:|:----:|:--------:|
| OrangePi | OD nano | **ON** | **170.2** | 292 | 46.6% |
| OrangePi | OD nano | OFF | 96.7 | 226 | 25.0% |
| OrangePi | OD small | **ON** | **136.1** | 241 | 84.2% |
| OrangePi | OD small | OFF | 97.1 | 228 | 49.0% |
| Radxa | OD nano | **ON** | **151.0** | 256 | 44.3% |
| Radxa | OD nano | OFF | 95.8 | 188 | 25.2% |
| Radxa | OD small | **ON** | **130.9** | 227 | 89.3% |
| Radxa | OD small | OFF | 95.0 | 193 | 50.0% |

ORT OFF에서 CPU%가 **낮음에도** NPU Avg%가 절반으로 떨어진다. 이는 CPU 경합이 아니라 **backpressure**에 의한 것이다: `dxpostprocess`가 6개의 raw 멀티스케일 텐서(80×80, 40×40, 20×20 각 bbox+class)를 디코딩하는 데 시간이 걸려, 파이프라인 전체가 정체되고 NPU가 굶는다. ORT ON은 `[1, N, 6]` 컴팩트 출력을 넘기므로 후처리가 가벼워 backpressure가 발생하지 않는다.

**Pose Estimation:**

| 환경 | 모델 | ORT ON E2E | ORT OFF E2E | 분석 |
|------|------|:----------:|:-----------:|------|
| OrangePi | Pose nano | 224.9 | **231.6** | OFF가 +3% |
| Radxa | Pose nano | 215.6 | 215.0 | 사실상 동일 |

Pose는 OD와 정반대로 **ORT OFF가 더 좋다**. Pose 모델은 class가 1개(person)뿐이어서, ORT OFF의 raw 텐서(cv3 class 텐서가 `[1,1,H,W]`)에서 sigmoid 연산이 80-class OD 대비 1/80 수준으로 가볍다. 따라서 ORT CPU offload의 추가 비용이 오히려 부담이 된다.

**Segmentation:**

| 환경 | 모델 | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | 분석 |
|------|------|:-----------------:|:-------------------:|------|
| OrangePi | Seg nano | **115.0** (372) | 87.5 (281) | ON이 +31% |
| Radxa | Seg nano | **97.8** (370) | 78.4 (280) | ON이 +25% |

Seg는 ORT ON이 확실히 유리하다. mask 생성 자체가 CPU 집약적인 데다, ORT OFF에서는 10개 raw 텐서 디코딩까지 더해져 postprocess 부담이 심해지기 때문이다.

**ARM 환경 결론**:
- **OD**: ORT ON 필수 (raw 텐서 backpressure 회피)
- **Pose**: ORT OFF 권장 (1 class로 raw 파싱 가벼움, Radxa에서는 ON/OFF 동등)
- **Seg**: ORT ON 권장 (mask + raw 텐서의 이중 부담)
- 무거운 모델(medium–xlarge)에서는 NPU 연산 시간이 지배적이므로 ORT ON/OFF 차이가 작아진다

##### RPi 5 — 소프트웨어 디코딩과 PCIe 이중 병목

RPi 5(A76 4코어)는 두 가지 고유한 제약을 동시에 가진다:
1. **PCIe Gen2 ×1**: 가벼운 모델의 NPU 활용률을 27%까지 낮춤
2. **소프트웨어 디코딩** (`avdec_h264`): CPU 자원의 상당 부분을 디코딩에 소비

이 조합에서 ORT ON은 CPU 자원 경쟁을 심화시켜 **모든 태스크에서 E2E를 악화**시킨다:

| 태스크 | 모델 | ORT ON E2E (CPU%) | ORT OFF E2E (CPU%) | 차이 |
|--------|------|:-----------------:|:-------------------:|:----:|
| OD | nano | 66.6 (336) | **86.6** (310) | +30% |
| Pose | nano | 83.7 (344) | **124.7** (311) | +49% |
| Seg | nano | 47.1 (337) | **48.7** (216) | +3% |

RPi 5에서 ORT ON이 항상 나쁜 이유:
- 소프트웨어 디코딩이 4코어 CPU의 상당 부분을 점유
- ORT CPU offload가 남은 CPU를 추가 소비 → 디코딩/전처리와 경합
- 전체 파이프라인 처리 속도 저하

반면 ORT OFF는 CPU offload 단계가 없어 소프트웨어 디코딩에 더 많은 CPU를 할당할 수 있으므로 E2E가 높아진다.

주목할 점은 **OD에서 nano–medium까지 E2E FPS가 비슷한 현상**이다:

| 태스크 | 모델 | Throughput | ORT OFF E2E | NPU Avg% |
|--------|------|:---------:|:----------:|:--------:|
| OD | nano | 91.5 | 86.6 | 22.8% |
| OD | small | 91.4 | 86.3 | 44.5% |
| OD | medium | 91.3 | 86.0 | 78.2% |
| OD | large | 70.3 | 71.0 | 93.1% |

Throughput(`run_model`)에서 이미 PCIe 병목이 반영되어 nano–medium이 84–91 FPS 범위이지만, E2E는 그보다 **더 낮은** ~86 FPS로 수렴한다. 이는 PCIe가 아닌 **호스트 측 파이프라인 처리 — 소프트웨어 디코딩(`avdec_h264`) + 전처리(libyuv) + 후처리 — 가 4코어 CPU에서 병목**이기 때문이다. nano의 NPU Avg%가 24.2%에 불과한 것이 이를 뒷받침한다 — NPU는 충분히 여유가 있지만 호스트가 프레임을 공급하지 못해 굶고 있다.

#### 실사용 시사점: 모델 크기 선택

호스트 파이프라인 병목이 E2E 천장을 결정하므로, **Throughput이 천장보다 높은 모델 범위에서는 모델을 키워도 E2E 손해가 없다** — 즉 정확도만 올라간다. 단, Throughput이 천장 아래로 떨어지는 지점부터는 NPU가 병목이 되어 E2E가 하락한다.

| 태스크 | 모델 | Throughput | E2E | 병목 | 시사점 |
|--------|------|:---------:|:---:|------|--------|
| OD | nano | 91.5 | 86.6 | 호스트 | Thr > 천장(~86) → FPS 손해 없음 |
| OD | small | 91.4 | 86.3 | 호스트 | 〃 |
| OD | medium | 91.3 | 86.0 | 호스트 | Thr ≈ 천장 → FPS 손해 거의 없음 |
| OD | large | 70.3 | 71.0 | **NPU** | Thr < 천장 → NPU가 병목, FPS 하락 |
| Pose | nano | 135.3 | 124.7 | 호스트 | Thr > 천장(~125) → FPS 손해 없음 |
| Pose | small | 130.4 | 121.1 | 호스트 | 〃 |
| Pose | medium | 93.1 | **93.5** | **NPU** | Thr < 천장 → **FPS 25% 하락** |
| Seg | nano | 48.7 | 48.7 | 호스트/NPU 혼재 | Thr ≈ 천장 → FPS 손해 거의 없음 |
| Seg | medium | 48.7 | 48.7 | NPU 혼재 | Thr ≈ E2E → 여전히 비슷 |

**결론**:
- **OD**: nano–medium까지 E2E ~86으로 동일 → **medium 사용 권장** (FPS 손해 없이 정확도 향상)
- **Pose**: nano–small까지 E2E ~121–125로 유사, medium부터 93.5로 하락 → **small 사용 권장** (medium은 FPS 25% 하락)
- **Seg**: nano–large까지 E2E ~49로 비슷 → **medium 사용 가능** (NPU 연산이 느려 모델 크기 영향 작음)
- 전체적으로 RPi 5에서는 **ORT OFF** 권장

#### E2E ORT ON/OFF 분석 종합

| 환경 | OD | Pose | Seg | 핵심 병목 |
|------|:--:|:----:|:---:|----------|
| i7 | 무관 | 무관 | 무관 | 없음 (reference) |
| AIBox | OFF | OFF | OFF | CPU 코어 수 (4) |
| Biostar | **ON** | OFF | **ON** | host 공급 한계 (4× NPU) |
| OrangePi | **ON** | OFF | **ON** | ARM raw 텐서 후처리 |
| Radxa | **ON** | OFF | **ON** | ARM raw 텐서 후처리 |
| RPi 5 | **OFF** | **OFF** | **OFF** | SW 디코딩 + PCIe |

> **참고**: 위 표는 가벼운 모델(nano~small) 기준이다. medium 이상에서는 NPU 연산 시간이 지배적이므로 ORT ON/OFF 차이가 크지 않으며, OBB와 Classification은 환경 간 경향이 OD/Seg과 유사하다.

### 4.3 Thermal Throttling

NPU 온도가 특정 임계점(약 85~90°C 부근)에 도달하면, 펌웨어가 클럭 주파수를 1000 MHz에서 강제로 하향한다. 온도가 충분히 내려갈 때까지 이 상태가 유지되며, 주파수 하향 구간에서는 추론 성능이 떨어진다.

#### OrangePi에서의 쓰로틀링 사례

OrangePi는 소형 ARM SBC로 방열 조건이 제한적이다. 단일 스트림 E2E 측정에서도 medium 이상 모델에서 NPU 온도가 83~85°C에 도달하며, **측정값의 편차(σ)가 급격히 커진다**:

| 태스크 | 모델 | ORT | E2E FPS | NPU Temp | σ 비율 |
|--------|------|-----|:-------:|:--------:|:------:|
| OD | medium | ON | 83.3 ±9.7 | 83~85°C | 11.7% |
| Pose | medium | ON | 73.5 ±7.2 | 83~85°C | 9.8% |
| Pose | large | ON | 57.4 ±5.1 | 84~85°C | 8.9% |

반면 같은 환경에서 가벼운 모델(OD nano, ORT ON)은 온도가 63~66°C에 머물러 σ가 1% 미만이다.


#### 실사용 시사점

- 장시간 연속 운영 시에는 NPU 온도를 모니터링(`dxtop`)하고 적절한 방열 대책(방열판, 팬)을 확보해야 한다
- 소형 SBC(OrangePi, AIBox 등)에서 medium 이상 모델을 장시간 사용할 경우, 쓰로틀링에 의한 성능 저하를 고려하여 설계해야 한다
- Biostar 등 데스크톱급 환경에서도 다채널 heavy model 운영 시 온도가 높아질 수 있으며, 방열 설계가 중요하다
---

## 5. 환경별 실사용 가이드

### 5.1 환경 선택 기준

```
모델 선택 기준:
┌─ 처리 채널 수 ≥ 8?   → Biostar H1 (4× M1)
├─ 처리 채널 수 4~7?    → i7 + M1 또는 AIBox + M1
├─ 처리 채널 수 1~3?    → ARM SBC (OrangePi/Radxa) 가능
└─ 저전력/소형 필수?     → RPi 5 (단, 가벼운 모델 제한)
```

### 5.2 환경별 권장 사항

#### i7 (데스크톱, M1 × 1, Gen3 ×4)

- **강점**: CPU 자원 풍부, PCIe 대역폭 충분, 모든 태스크에서 안정적 성능
- **약점**: 소비 전력이 높고 폼팩터가 큼
- **적합**: 개발/테스트 환경, PoC, 단일 NPU로 최대 성능이 필요한 경우
- **주의**: ORT ON/OFF 차이가 거의 없어 어느 쪽이든 무방

| 태스크 | 권장 모델 | Max Ch (30fps) |
|--------|----------|:--------------:|
| Object Detection | nano–small | 4–8 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 3–6 |
| OBB | nano | 2 |

#### AIBox (N97, M1 × 1, Gen3 ×2)

- **강점**: 저전력 (TDP 12W), 소형, 팬리스 가능
- **약점**: CPU 4코어로 제한, Segmentation 후처리에서 CPU 병목
- **적합**: 에지 배포, 소형 디바이스, 1–5채널 운영
- **주의**: seg 태스크에서는 CPU 병목에 유의. ORT ON 사용 시 CPU 부하 증가 주의

| 태스크 | 권장 모델 | Max Ch (30fps) |
|--------|----------|:--------------:|
| Object Detection | nano–small | 4–6 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 3 |
| OBB | nano | 2 |

#### Biostar (Ryzen 5 9600X, H1 × 1, Gen3 ×4)

- **강점**: H1(4× M1) 패키지로 절대 성능 최고, CPU 6코어(12스레드)로 여유
- **약점**: 비용, H1 전용 보드 필요
- **적합**: 다채널 영상 분석 서버, 중앙 집중형 배포
- **주의**: 가벼운 모델의 E2E는 CPU가 4× NPU를 충분히 공급하지 못해 Throughput 대비 하락

| 태스크 | 권장 모델 | Max Ch (30fps) |
|--------|----------|:--------------:|
| Object Detection | nano–xlarge | 5–17 |
| Pose Estimation | nano–xlarge | 5–19 |
| Segmentation | nano–xlarge | 3–12 |
| OBB | nano–large | 1–10 |

#### OrangePi 5 Plus (A55 8코어, M1 × 1, Gen3 ×4)

- **강점**: Arm SBC 중 PCIe 대역폭이 넓음 (Gen3 ×4), 가격 대비 성능 우수
- **약점**: CPU 연산력 제한 (A55는 저전력 코어), ORT OFF 시 E2E 역설 현상
- **적합**: 에지 디바이스, 1–3채널 가벼운 태스크
- **주의**: ORT ON 사용 권장 (OD에서 E2E FPS 높음). 무거운 모델(large–xlarge) 사용 시 CPU 병목 심각

| 태스크 | 권장 모델 | Max Ch (30fps) |
|--------|----------|:--------------:|
| Object Detection | nano–small | 3–5 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 2–3 |

#### Radxa Rock 5B+ (RK3588 8코어, M1 × 1, Gen3 ×2)

- **강점**: 소형, 저전력, ARM 생태계
- **약점**: Gen3 ×2로 대역폭 제한, OrangePi와 유사한 CPU 제한
- **적합**: 에지 디바이스, 저전력 운영
- **주의**: OrangePi와 유사한 특성. Gen3 ×2는 가벼운 모델에서 약간의 대역폭 제한 있으나 실사용에 큰 영향은 없음

| 태스크 | 권장 모델 | Max Ch (30fps) |
|--------|----------|:--------------:|
| Object Detection | nano–medium | 2–4 |
| Pose Estimation | nano–small | 4–7 |
| Segmentation | nano–small | 2–3 |

#### RPi 5 (A76 4코어, M1 × 1, Gen2 ×1)

- **강점**: 가장 넓은 생태계, 저렴, 초소형
- **약점**: **Gen2 ×1 PCIe** + **SW 디코딩**으로 인해 가벼운 모델에서 NPU 활용률이 매우 낮음 (OD nano: 24%), CPU 4코어 제한
- **적합**: PoC, 학습용, 가벼운 단일 채널 운영
- **주의**: SW 디코딩 + PCIe 이중 제약으로 E2E 천장이 태스크별로 제한됨. ORT OFF 필수

| 태스크 | 권장 모델 | Max Ch (30fps) | 근거 |
|--------|----------|:--------------:|------|
| Object Detection | medium | 2 | nano–medium E2E 동일(~86), medium이 정확도 유리 |
| Pose Estimation | small | 2–4 | nano–small E2E 유사(~121–125), medium부터 하락 |
| Segmentation | medium 이하 | 1 | nano–large E2E 비슷(~49), 모델 크기 영향 작음 |

RPi 5의 핵심 특성: SW 디코딩이 CPU의 상당 부분을 소비하므로, 태스크별 호스트 파이프라인 처리량이 E2E 천장을 결정한다. Throughput이 이 천장보다 높은 모델 범위에서는 **모델을 키워도 FPS 손해가 없으면서 정확도만 올라간다**. 다만 Pose는 medium에서 Throughput이 천장 아래로 떨어지므로 small까지 권장한다.

### 5.3 모델 선택 결정 트리

```
목표: 30fps × N채널 운영

1. 원하는 태스크 선택 (OD / Pose / Seg / OBB / Cls)
2. 환경별 Max Channels 테이블에서 N채널 가능한 모델 확인
3. 가능한 모델 중 가장 큰(정확한) 모델 선택
4. ORT ON/OFF 비교:
   - x86 환경: 보통 ORT OFF 선호 (CPU 여유)
   - ARM 환경: ORT ON이 유리할 수 있음 (출력 크기 감소 효과)
5. 열적 안정성 확인:
   - 연속 운영 시 NPU 온도 모니터링 (dxtop)
   - 고온 도달 시 쓰로틀링 → 방열 개선 또는 모델 사이즈 축소
```

---

## 6. 결론

### 6.1 핵심 요약

1. **NPU 자체 성능(Throughput)은 PCIe Gen3 이상의 M1 단일 칩 환경에서 거의 동일하다.** OD medium Throughput(ORT OFF)은 i7: 96.5, AIBox: 96.0, OrangePi: 96.4로 수렴한다. Radxa는 90.8로 약간 낮은데, 이는 Gen3 ×2 + ARM 호스트 오버헤드의 복합적 영향으로 보인다.

2. **PCIe 대역폭은 가벼운 모델의 Throughput에만 영향을 미친다.** RPi 5(Gen2 ×1)의 OD nano Throughput(91.5)은 i7(242.1) 대비 38%에 불과하지만, xlarge에서는 RPi 5(40.5)와 i7(40.3)이 동일하다. NPU 연산 시간이 길어질수록 PCIe 전송 시간의 비중이 무시 가능해지기 때문이다.

3. **E2E 파이프라인 성능은 호스트 환경에 크게 좌우되며, 병목 원인은 환경마다 다르다.**
   - **RPi 5**: SW 디코딩(`avdec_h264`) + PCIe 이중 제약. OD nano E2E(86.6)가 Throughput(91.5)보다 낮으며, 태스크별로 E2E 천장이 다름 (Pose 약 125, OD 약 86, Seg 약 49). CPU 호스트 처리량이 천장을 결정한다.
   - **OrangePi/Radxa**: HW 디코딩 + RGA 전처리를 사용하지만, ORT OFF의 raw 텐서 후처리에서 backpressure가 발생하여 NPU를 굶긴다. OrangePi OD nano E2E는 ORT ON(170.2) vs OFF(96.7)로 76% 차이.
   - **Biostar H1**: 4× NPU의 합산 Throughput이 매우 높아, 단일 파이프라인의 순차 처리 속도가 NPU 소비 속도를 따라잡지 못한다. OD nano에서 E2E(498.3)/Throughput(927.9) = 54%. 호스트 CPU 성능과 다채널 배포가 H1 활용의 핵심이다 (상세: 항목 5).

4. **Segmentation 태스크는 다른 태스크 대비 후처리 단계의 CPU 부하가 압도적으로 높다.** mask 생성(coefficient × prototype + bilinear interpolation)이 ORT ON/OFF 공통으로 매 프레임 수행되며, AIBox에서 Seg nano E2E 시 CPU%가 360–369(4코어의 90–92%)에 달한다. 이로 인해 CPU 코어가 제한된 환경에서 E2E가 크게 제한된다.

5. **H1(4× M1)은 추론 능력이 매우 뛰어나므로, NPU 성능을 최대한 끌어내려면 호스트 성능이 뒷받침되어야 한다.** H1의 Throughput은 단일 M1 대비 약 3.8배(OD nano: 929 vs 242)에 달한다. 그러나 Ryzen 5 9600X(6코어/12스레드)에서도 가벼운 모델의 단일 스트림 E2E는 Throughput의 61–63%에 그친다:

   | 태스크 | 모델 | Throughput | E2E | E2E/Thr | NPU Avg% |
   |--------|------|:---------:|:---:|:-------:|:--------:|
   | OD | nano | 927.9 | 498.3 | 54% | 26.4% |
   | OD | small | 536.4 | 497.5 | 93% | 65.6% |
   | OD | medium | 371.3 | 372.4 | 100% | 80.1% |
   | Pose | nano | 889.6 | 542.7 | 61% | 30.3% |
   | Seg | nano | 573.1 | 363.0 | 63% | 24.9% |
   | Seg | medium | 266.0 | 265.7 | 100% | 82.0% |

   > ORT ON 기준. 단일 스트림 E2E 측정.

   nano에서 NPU Avg%가 25–31%에 불과한 것은, 단일 GStreamer 파이프라인의 '디코딩 → 전처리 → NPU 제출' 순차 처리 속도가 4개 NPU의 소비 속도를 따라잡지 못하기 때문이다. CPU 연산 자원 자체는 여유가 있으나(CPU% 173–539 / 최대 1200%), **파이프라인의 구조적 직렬성**이 병목이다. medium 모델부터는 NPU 연산 시간이 충분히 길어 호스트가 따라잡으므로 E2E ≈ Throughput(~100%)이 된다.

   **실사용 시사점**: H1의 진가는 다채널 동시 처리에서 발휘된다. 여러 파이프라인이 병렬로 NPU에 프레임을 공급하면 단일 파이프라인의 직렬성 한계를 우회할 수 있으며, 실제로 Max Channels 결과에서 OD nano 17ch, Pose nano 19ch를 수용한다. H1 배포 시에는 이 처리량을 뒷받침할 수 있도록 **CPU 코어 수와 메모리 대역폭이 충분한 호스트**를 선택해야 하며, 특히 Seg처럼 후처리가 무거운 태스크에서는 nano 단일 스트림에서도 CPU%가 539(최대의 45%)에 달하는 점을 감안해야 한다.

6. **ORT ON/OFF 최적 설정은 환경 × 태스크 조합에 따라 달라진다.** 단일 규칙이 없으며, 환경별 E2E ORT ON/OFF 분석 종합(§4.2)을 참고해야 한다.

   | 환경 | OD | Pose | Seg | 핵심 요인 |
   |------|:--:|:----:|:---:|----------|
   | i7 | 무관 | 무관 | 무관 | CPU 여유 충분 |
   | AIBox | OFF | OFF | OFF | ORT ON의 CPU 경합 |
   | Biostar | **ON** | OFF | **ON** | 4×NPU 프레임률에서 raw 후처리 부담 |
   | OrangePi/Radxa | **ON** | OFF | **ON** | ARM CPU에서 raw 텐서 backpressure |
   | RPi 5 | **OFF** | **OFF** | **OFF** | SW 디코딩과 CPU 경합 |

### 6.2 권장 사항

**하드웨어 선택**:
- 다채널/고성능 → Biostar H1 (OD 17ch, Pose 19ch, Seg 12ch @30fps). 경량 모델 다채널 운영 시 호스트 CPU 12코어 이상 권장 — H1의 처리량을 뒷받침하지 못하면 NPU가 idle 상태에 빠진다 (§6.1 항목 5 참고)
- 단일·소수 채널 + x86 에지 → AIBox (OD 4–6ch, Pose 4–7ch @30fps)
- 단일·소수 채널 + ARM 에지 → OrangePi (OD 3–5ch) 또는 Radxa (OD 2–4ch)
- PoC/프로토타입 → RPi 5 (단, OD 2ch, Pose OFF 4ch, Seg 1ch이 한계)

**모델 선택**:
- FPS 여유가 있으면 가능한 큰 모델을 사용하여 정확도를 확보할 것
- RPi 5에서는 호스트 파이프라인이 E2E 천장을 결정하므로, 태스크별로 천장 이내의 가장 큰 모델 사용 권장 (OD: medium, Pose: small–nano, Seg: medium)
- Segmentation이 필요한 경우 CPU 코어 수를 고려하여 환경 선정

**ORT ON/OFF 설정**:
- i7/AIBox (x86, CPU 여유): ORT OFF 기본
- OrangePi/Radxa (ARM): OD·Seg는 ORT ON 필수, Pose는 ORT OFF
- RPi 5: **모든 태스크에서 ORT OFF** (SW 디코딩과 CPU 경합 방지)
- Biostar H1: OD·Seg는 ORT ON 유리, Pose는 ORT OFF
- medium 이상 모델에서는 ORT ON/OFF 차이가 작아지므로 어느 쪽이든 무방

**열 관리**:
- OrangePi에서 medium 이상 모델 E2E 시 NPU 83–85°C 도달, σ 9–12% 편차 발생
- AIBox에서 Seg 멀티스트림 시 σ 11–14% 편차 관찰
- 장시간 연속 운영 시 `dxtop`으로 NPU 온도 모니터링, 방열판/팬 확보 필수

---

## 7. 부록

### 7.1 벤치마크 재현 방법

```bash
# 전체 벤치마크 실행
python -m dx_stream.apps.benchmark run

# 특정 모델 크기/태스크만 실행
python -m dx_stream.apps.benchmark run --sizes n,s --task object_detection

# 기존 결과에서 실패한 항목만 재시도
python -m dx_stream.apps.benchmark run --resume results/<run_id> --retry-failed

# 대시보드 생성
python -m dx_stream.apps.benchmark dashboard dx_stream/apps/benchmark/results
```

### 7.2 측정 프로토콜 주요 파라미터

| 파라미터 | 값 | 비고 |
|----------|-----|------|
| Throughput 측정 시간 | 30초 | |
| Latency 루프 수 | 300 | 고정 루프, `-l` 모드 |
| Throughput 반복 횟수 | 3회 | |
| E2E 반복 횟수 | 3회 | |
| Warmup 횟수 | 1회 | 결과에 미포함 |
| Multi-Stream FPS 기준 | 30 fps | 채널당 최소 FPS |
| 쿨다운 목표 ΔT | 10°C | idle 대비 |
| 쿨다운 최대 대기 | 300초 | |

### 7.3 환경별 상세 벤치마크 결과

각 환경의 전체 벤치마크 결과는 아래 파일에서 확인할 수 있다:

| 환경 | 리포트 |
|------|--------|
| i7 | [`results/i7-14700K_M1/20260421_180106/REPORT.md`](results/i7-14700K_M1/20260421_180106/REPORT.md) |
| AIBox | [`results/DX-AIPlayer-N97_M1/20260421_201939/REPORT.md`](results/DX-AIPlayer-N97_M1/20260421_201939/REPORT.md) |
| Biostar | [`results/BIOSTAR_H1/20260421_201621/REPORT.md`](results/BIOSTAR_H1/20260421_201621/REPORT.md) |
| OrangePi | [`results/OrangePi5+_M1/20260418_171936/REPORT.md`](results/OrangePi5+_M1/20260418_171936/REPORT.md) |
| Radxa | [`results/ROCK5B+_M1/20260422_111610/REPORT.md`](results/ROCK5B+_M1/20260422_111610/REPORT.md) |
| RPi 5 | [`results/RPi_M1/20260421_205004/REPORT.md`](results/RPi_M1/20260421_205004/REPORT.md) |

### 7.4 용어 정리

| 용어 | 설명 |
|------|------|
| **Throughput** | run_model로 측정한 순수 NPU 비동기 처리량 (FPS) |
| **E2E FPS** | GStreamer 파이프라인에서의 실제 처리량 (디코딩~출력 전체 포함) |
| **Latency** | 단일 프레임의 추론 지연 시간 (ms) |
| **Max Channels** | 채널당 30fps 이상 유지 가능한 최대 동시 스트림 수 |
| **ORT** | ORT ON은 CPU offload 구간을 OnnxRuntime으로 함께 수행하고, ORT OFF는 해당 경로를 거치지 않고 NPU 결과만 반환 |
| **NPU Avg%** | 측정 중 NPU 평균 활용률 |
| **Backpressure** | GStreamer 파이프라인에서 downstream 처리 지연이 queue를 통해 upstream으로 전파되어 전체 파이프라인 속도를 제한하는 메커니즘 |
| **push_queue** | dxinfer 내부의 크기 제한 큐 (MAX_PUSH_QUEUE_SIZE=5 × 디바이스 수). 가득 차면 새 추론 요청 제출이 블로킹됨 |
| **Thermal Throttling** | NPU 온도가 특정 고온(약 85~90°C 부근)에 도달 시 클럭 강제 하향 |

---

*이 문서는 dx_stream 벤치마크 도구의 측정 결과를 기반으로 작성되었습니다.*
