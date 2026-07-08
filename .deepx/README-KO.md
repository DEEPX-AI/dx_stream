# .deepx/ — dx_stream Agent-Driven 지식 베이스

> DEEPX NPU 가속기를 위한 GStreamer 기반 비디오 분석 프레임워크인 dx_stream의
> dx-agent-dev 개발을 위한 자체 완결형 지식 베이스.

## Quick Start

```bash
# 파이프라인 스크립트 검증
python3 .deepx/scripts/validate_app.py dx_stream/pipelines/single_network/object_detection/run_yolo26n.sh

# .deepx/ 무결성 검증
python3 .deepx/scripts/validate_framework.py

# 플랫폼 config 생성 (suite-level dx-agent-gen)
dx-agent-gen generate    # 단일 repo
bash ../../.deepx/tools/scripts/run_all.sh generate    # suite 전체 (5개 repo 전부)
```

## Context Routing Table

작업이 포함하는 내용에 따라, **오직** 일치하는 행만 읽으십시오:

| 작업이 언급하는 경우... | 다음 파일을 읽으십시오 |
|---|---|
| **Pipeline, detection, classification** | `skills/dx-agent-stream-build-pipeline/SKILL.md`, `toolsets/dx-stream-elements.md` |
| **MQTT, Kafka, message broker** | `skills/dx-agent-stream-build-mqtt-kafka/SKILL.md`, `toolsets/dx-stream-elements.md` |
| **Multi-model, cascaded, tiled** | `skills/dx-agent-stream-build-pipeline/SKILL.md`, `toolsets/dx-stream-metadata.md` |
| **Model, download, registry** | `skills/dx-agent-stream-model-management/SKILL.md`, `toolsets/model-registry.md` |
| **Validation, testing** | `skills/dx-agent-stream-validate/SKILL.md`, `instructions/testing-patterns.md` |
| **Architecture, structure** | `instructions/architecture.md` |
| **Coding rules, conventions** | `instructions/coding-standards.md` |
| **GStreamer elements, properties** | `toolsets/dx-stream-elements.md` |
| **Metadata, pydxs, DXObjectMeta** | `toolsets/dx-stream-metadata.md` |
| **Orchestration, lifecycle** | `instructions/orchestration.md` |
| **Agent protocols, routing** | `instructions/agent-protocols.md` |
| **Brainstorm, plan, design** | `skills/dx-agent-brainstorm/SKILL.md` |
| **TDD, validation, incremental** | `skills/dx-agent-tdd/SKILL.md` |
| **Completion, verify, evidence** | `skills/dx-agent-verify/SKILL.md` |
| **항상 읽기 (모든 작업)** | `memory/common_pitfalls.md`, `instructions/coding-standards.md` |

## Skills

| Command | 설명 |
|---------|-------------|
| `/dx-agent-stream-build-pipeline` | GStreamer 파이프라인 app 빌드 (6 카테고리: single-model, multi-model, cascaded, tiled, parallel, broker) |
| `/dx-agent-stream-build-mqtt-kafka` | MQTT/Kafka 메시지 broker 파이프라인 app 빌드 |
| `/dx-agent-stream-model-management` | .dxnn 모델 다운로드, 등록, 설정 |
| `/dx-agent-stream-validate` | 파이프라인 검증 체크 실행 |
| `/dx-agent-brainstorm` | 모든 코드 생성 이전에 브레인스토밍 및 계획 수립 (process skill) |
| `/dx-agent-tdd` | 테스트 주도 개발 — 생성 직후 각 파일 즉시 검증 (process skill) |
| `/dx-agent-verify` | 완료 주장 전 검증 — 단정 이전에 증거 제시, 필수 산출물 강제 (process skill) |

## 디렉토리 구조

```
.deepx/
  README.md                              # 이 파일 — 마스터 인덱스
  agents/
    dx-stream-builder.md                 # 마스터 라우터 agent
    dx-pipeline-builder.md               # 파이프라인 구성 전문가
    dx-model-manager.md                  # 모델 operation agent
    dx-validator.md                      # 파이프라인 및 프레임워크 검증 agent
  instructions/
    architecture.md                      # dx_stream v2.3.0 아키텍처 개요
    coding-standards.md                  # 파이프라인 구성 규칙 및 컨벤션
    gstreamer-pipeline.md                # 13 element + 6 파이프라인 패턴
    testing-patterns.md                  # GStreamer 디버그 및 테스트 패턴
    agent-protocols.md                   # 11개 agent 간 프로토콜
    orchestration.md                     # 5-phase 파이프라인 라이프사이클
  skills/
    dx-agent-stream-build-pipeline/
      SKILL.md                           # 전체 파이프라인 빌드 skill
    dx-agent-stream-build-mqtt-kafka/
      SKILL.md                           # Broker 통합 skill
    dx-agent-stream-model-management/
      SKILL.md                           # 모델 관리 skill
    dx-agent-stream-validate/
      SKILL.md                           # 파이프라인 검증 skill
    dx-agent-brainstorm/
      SKILL.md                           # Process skill
    dx-agent-tdd/
      SKILL.md                           # Process skill
    dx-agent-verify/
      SKILL.md                           # Process skill
  toolsets/
    dx-stream-elements.md                # 전체 13-element property 레퍼런스
    dx-stream-metadata.md                # pydxs metadata API 레퍼런스
    dx-engine-api.md                     # DX-RT inference engine 레퍼런스
    model-registry.md                    # model_list.json 레퍼런스 (14개 모델)
  memory/
    MEMORY.md                            # Memory 인덱스
    common_pitfalls.md                   # 알려진 10개 pitfall [DX_STREAM] + [UNIVERSAL]
    pipeline_optimization.md             # 성능 최적화 패턴
    platform_api.md                      # DX-RT 플랫폼 API 레퍼런스
  knowledge/
    knowledge_base.yaml                  # Best practice, instruction, recipe (YAML)
  prompts/
    new-stream-pipeline.md               # 프롬프트 템플릿: 새 파이프라인 app
    new-mqtt-kafka-app.md                # 프롬프트 템플릿: 새 broker app
    orchestrated-build.md                # 프롬프트 템플릿: orchestrated multi-step 빌드
  contextual-rules/
    stream-pipelines.md                  # 파이프라인 개발 컨텍스트 규칙
    postprocess.md                       # Postprocess 라이브러리 컨텍스트 규칙
    tests.md                             # 테스트 개발 컨텍스트 규칙
  scripts/
    validate_app.py                      # 파이프라인 app 검증기 (11개 체크)
    validate_framework.py                # .deepx/ 무결성 체커 (8 카테고리)
  templates/
    en/                                  # 영문 `.tmpl` 파일 (dx-agent-gen 처리)
    ko/                                  # 한국어 `.tmpl` 파일 (dx-agent-gen 처리)
```

> 플랫폼 파일 생성은 suite-root `tools/` 디렉토리에 위치한 suite-level **`dx-agent-gen`** CLI에서
> 처리됩니다 (사용법은 `dx-agent-gen --help` 실행).

## 지원 도구

이 지식 베이스는 다음 5개 AI 코딩 도구를 지원합니다:

- **Claude Code** — `CLAUDE.md` + `.claude/agents/`, `.claude/skills/`
- **GitHub Copilot** — `.github/copilot-instructions.md` + `.github/agents/`, `.github/skills/`
- **Cursor** — `.cursor/rules/*.mdc`
- **OpenCode** — `AGENTS.md` + `.opencode/agents/`, `opencode.json`
- **Codex CLI** — `AGENTS.md` + `.codex/skills/dx-codex-identity/SKILL.md` (자동 identity), 그리고 `.deepx/skills/*/SKILL.md` 직접 참조

## 13 GStreamer Elements

| Element | 카테고리 | 용도 |
|---------|----------|---------|
| `DxPreprocess` | Inference | NPU용 프레임 resize, normalize, 색상 변환 |
| `DxInfer` | Inference | NPU에서 .dxnn 모델 inference 실행 |
| `DxPostprocess` | Inference | Tensor를 detection/classification으로 디코딩 |
| `DxTracker` | Analysis | Multi-object tracking (OC-SORT, persistent ID) |
| `DxOsd` | Display | Bounding box, label, keypoint 그리기 |
| `DxGather` | Routing | Secondary mode 분기에 대한 N-to-1 merge |
| `DxInputSelector` | Routing | N-to-1 round-robin 입력 (multi-stream) |
| `DxOutputSelector` | Routing | 1-to-N 출력 demux (multi-stream) |
| `DxRate` | Rate Control | 프레임 레이트 제한 (RTSP에 필수) |
| `DxMsgConv` | Messaging | DXObjectMeta를 JSON으로 직렬화 |
| `DxMsgBroker` | Messaging | JSON을 Kafka 또는 MQTT에 publish |
| `DxScale` | Transform | 비디오 프레임 resize |
| `DxConvert` | Transform | 색공간 변환 |

## 6 Pipeline Categories

| 카테고리 | 패턴 | 핵심 Element |
|----------|---------|-------------|
| **Single-model** | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` | Core 3 + OSD |
| **Multi-model** | 서로 다른 preprocess-id로 DxInfer 단계 체인 | 다중 inference |
| **Cascaded** | `DxPostprocess ! tee ! DxPreprocess(secondary) ! DxInfer ! DxGather` | Secondary mode |
| **Parallel** | `DxInputSelector ! shared DxInfer ! DxOutputSelector` | Multi-stream |
| **Broker** | `DxPostprocess ! DxMsgConv ! DxMsgBroker` | MQTT/Kafka publish |

## 핵심 컨벤션

1. **Preprocess-ID 매칭**: DxPreprocess와 DxInfer는 동일한 `preprocess-id`를 공유해야 함
2. **Queue element**: 모든 dx_stream element 쌍 사이에 `queue` 배치
3. **RTSP에 대한 DxRate**: RTSP/가변 레이트 소스 뒤에 항상 DxRate 삽입
4. **절대 경로**: `model-path`와 `library-file-path`는 반드시 절대 경로여야 함
5. **DxMsgBroker 이전의 DxMsgConv**: Publish 이전에 항상 직렬화
6. **DxPostprocess 이후의 DxTracker**: Tracker는 raw tensor가 아니라 DXObjectMeta를 필요로 함
7. **모델 자동 다운로드**: Shell 스크립트는 `setup.sh` 다운로드 로직을 포함해야 함
8. **Headless 체크**: 비디오 sink 사용 전 `$DISPLAY` 테스트

## 지원 모델 (14개)

| Task | 모델 |
|------|--------|
| Object Detection | YoloV5S, YoloV5S_PPU, yolo26n, YoloV7, YoloV8N, YoloV9S, YoloXS, YOLOV11N |
| Face Detection | SCRFD500M, YOLOv5s_Face |
| Pose Estimation | yolo26n-pose, yolov8m_pose |
| Segmentation | yolo26n-seg |
| Classification | EfficientNet_Lite0 |

## 하드웨어

| 아키텍처 | 값 |
|---|---|
| DX-M1 | `dx_m1` |

## 검증

```bash
# 파이프라인 스크립트 검증 (static + property 체크)
python3 .deepx/scripts/validate_app.py <script.sh>

# Smoke test 포함 검증 (NPU 필요)
python3 .deepx/scripts/validate_app.py <script.sh> --smoke-test

# .deepx/ 무결성 검증
python3 .deepx/scripts/validate_framework.py

# 플랫폼 config 생성 (단일 repo 또는 suite 전체)
dx-agent-gen generate
bash ../../.deepx/tools/scripts/run_all.sh generate
```

## Memory

`memory/`에 지속 지식이 있습니다. 작업 시작 시 읽고, 새 패턴을 학습하거나
pitfall을 발견할 때 업데이트하십시오. Memory 인덱스는 `memory/MEMORY.md`를 참조하십시오.
