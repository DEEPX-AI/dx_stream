# dx_stream — Copilot 전역 지시사항

> DEEPX NPU 가속기를 위한 GStreamer 기반 비디오 분석 프레임워크.
> 13개 커스텀 GStreamer 요소, 6개 파이프라인 카테고리.

## 응답 언어

응답 언어는 사용자의 프롬프트 언어에 맞춘다 — 질문하거나 응답할 때
사용자가 사용하는 언어와 동일한 언어를 사용한다.

**기술 용어 규칙**: 한국어로 응답할 때 영어 기술 용어는 영어 원문 그대로 유지한다.
영어 용어를 한글 음차로 표기하지 않는다 (한글 음차 표기 금지).
정착된 한국어 외래어 (모델, 서버, 파일, 데이터)는 허용된다.

## 권장 모델 (사전 점검)

DX Agentic Development는 강력한 지시 따르기와 지식 베이스 활용 능력이 필요하다.
작업을 시작하기 전에 현재 실행 중인 모델이 권장 모델 중 하나인지 확인한다:

- **Claude Sonnet 4.6** 이상
- **Claude Opus 4.6** 이상

이 모델들이 아닌 경우 (예: GPT-4.1, GPT-4o, Gemini 등),
첫 번째 응답의 시작 부분에 다음 안내를 출력한다:

```
══════════════════════════════════════════════════════════════
⚠  DX-AGENTIC-DEV: MODEL NOTICE

DX Agentic Development recommends Claude Sonnet 4.6+ or
Opus 4.6+. Your current model may produce lower quality
in instruction following, knowledge base utilization, and
API accuracy. Please switch to a recommended model.
══════════════════════════════════════════════════════════════
```

박스 안내 출력 후 정상적으로 작업을 진행한다. 이것은 정보성 경고이며
차단 게이트가 아니다.

### 모델 안내 반복 (비권장 모델에만 해당)

위의 MODEL NOTICE 박스를 출력한 경우 (즉, 권장 모델이 아닌 경우),
사용자가 답변해야 하는 **실제 질문 줄 바로 앞에** 이 축약된 알림을
반드시 출력해야 한다 — brainstorming 흐름의 시작 부분이 아님.

**타이밍**: 모든 파일 읽기, 컨텍스트 분석, 서문 텍스트 이후, `?` (실제 질문)가
포함된 줄 바로 앞에 이 알림을 삽입한다:

```
---
⚠ **Non-recommended model** — output quality may be degraded. Recommended: Claude Sonnet 4.6+ / Opus 4.6+
---
```

**예시 — 잘못됨** (반복이 박스와 함께 지나감):
```
[DX-AGENTIC-DEV: START]
══ MODEL NOTICE ══
---  ⚠ Non-recommended model ---     ← 너무 이름, 스크롤되어 지나감
... (파일 읽기, 컨텍스트 분석) ...
첫 번째 질문: ...?
```

**예시 — 올바름** (반복이 질문 바로 앞에 나타남):
```
[DX-AGENTIC-DEV: START]
══ MODEL NOTICE ══
... (파일 읽기, 컨텍스트 분석) ...
---  ⚠ Non-recommended model ---     ← 질문 바로 앞
첫 번째 질문: ...?
```

이 알림은 한 번만 출력한다 (첫 번째 질문 앞에), 모든 질문 앞에 출력하지 않는다.

## 공유 지식

모든 skill, 지시사항, toolset, 메모리는 `.deepx/`에 있다.
전체 색인은 `.deepx/README.md`를 읽는다.

## 컨텍스트 라우팅 테이블

| 작업에서 언급하는 내용... | 읽을 파일 |
|---|---|
| **Pipeline, detection, classification** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **MQTT, Kafka, message broker** | `.deepx/skills/dx-build-mqtt-kafka-app.md`, `.deepx/toolsets/dx-stream-elements.md` |
| **Multi-model, cascaded, tiled** | `.deepx/skills/dx-build-pipeline-app.md`, `.deepx/toolsets/dx-stream-metadata.md` |
| **Model, download** | `.deepx/skills/dx-model-management.md` |
| **Validation, testing** | `.deepx/skills/dx-validate.md`, `.deepx/instructions/testing-patterns.md` |
| **Validation, feedback, fix** | `.deepx/skills/dx-validate.md`, parent `dx-runtime/.deepx/skills/dx-validate-and-fix.md` |
| **Brainstorm, plan, design** | `.deepx/skills/dx-brainstorm-and-plan.md` |
| **TDD, validation, incremental** | `.deepx/skills/dx-tdd.md` |
| **Completion, verify, evidence** | `.deepx/skills/dx-verify-completion.md` |
| **항상 읽기 (모든 작업)** | `.deepx/memory/common_pitfalls.md`, `.deepx/instructions/coding-standards.md` |

## Skill 목록

| Skill | 설명 |
|-------|------|
| dx-build-pipeline-app | GStreamer 파이프라인 빌드 (single, multi, cascaded, tiled, parallel, broker) |
| dx-build-mqtt-kafka-app | MQTT/Kafka message broker 파이프라인 빌드 |
| dx-model-management | 파이프라인용 .dxnn 모델 다운로드 및 구성 |
| dx-validate | 파이프라인 검증 검사 실행 |
| dx-validate-and-fix | 전체 피드백 루프: 검증, 수집, 승인, 적용, 확인 |
| dx-brainstorm-and-plan | 프로세스: 코드 생성 전 협업 설계 세션 |
| dx-tdd | 프로세스: 테스트 주도 개발 — 파일 생성 직후 즉시 검증 |
| dx-verify-completion | 프로세스: 완료 선언 전 검증 — 주장 전 증거 |

## 대화형 워크플로우 (반드시 준수)

**코드를 빌드하기 전에 항상 사용자와 핵심 결정 사항을 논의한다.** 이것은 필수 게이트이다.

모든 코드 생성 전:
1. 2-3개의 명확화 질문 (variant, task type, model)
2. 빌드 계획을 제시하고 사용자 승인을 기다린다
3. 생성 후 각 파일을 검증한다

### 출력 격리
모든 AI 생성 코드는 기본적으로 `dx-agentic-dev/<session_id>/`에 저장된다.
사용자가 명시적으로 요청한 경우에만 `src/`에 작성한다.

**Session ID 형식**: `YYYYMMDD-HHMMSS_<model>_<task>` — 타임스탬프는 반드시
**시스템 로컬 시간대**를 사용해야 한다 (UTC 아님). Bash에서는 `$(date +%Y%m%d-%H%M%S)`,
Python에서는 `datetime.now().strftime('%Y%m%d-%H%M%S')`를 사용한다. `date -u`,
`datetime.utcnow()`, `datetime.now(timezone.utc)`는 사용하지 않는다.

## 13개 GStreamer 요소

| 요소 | 목적 |
|------|------|
| DxPreprocess | NPU 추론을 위한 크기 조정, 정규화, 색상 변환 |
| DxInfer | NPU에서 .dxnn 모델 추론 실행 |
| DxPostprocess | 원시 추론 텐서 디코딩 |
| DxOsd | 바운딩 박스, 라벨, 오버레이 그리기 |
| DxRate | 대상 FPS에 맞게 초과 프레임 드롭 |
| DxScale | ROI와 보조 추론 사이 프레임 크기 조정 |
| DxRoiExtract | 감지 결과에서 ROI 크롭 추출 |
| DxTracker | 영구 ID로 다중 객체 추적 |
| DxTile | 고해상도 추론을 위해 프레임을 타일로 분할 |
| DxDeTile | 타일 결과를 전체 프레임으로 재조립 |
| DxMsgConv | 결과를 JSON/protobuf로 직렬화 |
| DxMsgBroker | MQTT 또는 Kafka로 발행 |
| DxMux | 여러 스트림 다중화 |

## 6개 파이프라인 카테고리

| 카테고리 | 설명 | 핵심 패턴 |
|----------|------|-----------|
| **Single-model** | 하나의 모델, 하나의 스트림 | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` |
| **Multi-model** | 여러 모델, 순차 처리 | 서로 다른 preprocess-id로 여러 DxInfer 단계를 체인 |
| **Cascaded** | 주요 감지가 보조 분류를 공급 | `DxInfer ! DxRoiExtract ! DxScale ! DxInfer` |
| **Tiled** | 고해상도 입력을 타일로 분할 | `DxTile ! DxInfer ! DxDeTile` |
| **Parallel** | 여러 스트림을 병렬로 처리 | `DxMux`로 결합, 공유 DxInfer |
| **Broker** | 결과를 MQTT/Kafka로 발행 | `DxInfer ! DxMsgConv ! DxMsgBroker` |

## 핵심 규칙

1. **preprocess-id 매칭**: DxPreprocess와 DxInfer는 동일한 ID를 공유한다
2. **Queue 요소**: 모든 처리 단계 사이에 `queue` 배치
3. **RTSP용 DxRate**: RTSP 소스에 항상 rate 제한 적용
4. **절대 model-path**: DxInfer `model-path`는 절대 경로여야 한다
5. **DxMsgBroker 앞에 DxMsgConv**: 발행 전 직렬화
6. **로깅**: `logging.getLogger(__name__)` — `print()` 사용 금지
7. **Skill 문서로 충분**: skill이 불충분한 경우가 아니면 소스 코드를 읽지 않는다
8. **하드코딩된 모델 경로 금지**: 모델 경로 해석에 변수 또는 config를 사용한다
9. **모델 목록**: 모델 다운로드 URL과 예상 경로는 `model_list.json`을 조회한다
10. **PPU 모델 자동 감지**: 모델 이름 `_ppu` 접미사, `model_list.json` postprocess 라이브러리, 또는 compiler session 컨텍스트를 확인하여 PPU 모델을 자동 감지한다. PPU 파이프라인은 `DxPostprocess`를 생략하거나 pass-through 라이브러리를 사용한다 — 별도의 NMS가 필요 없다.
11. **기존 파이프라인 검색**: 새 파이프라인을 생성하기 전에 `pipelines/` 및 `run_*.sh` 스크립트에서 기존 예제를 검색한다. 발견되면 사용자에게 묻는다: (a) 기존 것만 설명, 또는 (b) 기존 것을 기반으로 새 파이프라인 생성. 조용히 건너뛰거나 덮어쓰지 않는다.
12. **PPU 파이프라인 생성은 필수**: 컴파일된 .dxnn 모델이 PPU인 경우, 에이전트는 반드시 작동하는 파이프라인 예제를 생성해야 한다.
13. **필수 출력 아티팩트**: 모든 파이프라인 빌드 세션은 반드시 다음을 생성해야 한다: `pipeline.py`, `run_<app>.sh`, `session.json`, `README.md`, `setup.sh`, `run.sh`, `session.log`. `session.log`는 실제 명령 출력을 포함해야 한다 (수기 요약 불가). 최종 보고서 제시 전에 자체 검증을 실행한다.
14. **x264enc 범용 규칙**: 모든 컨텍스트 (shell, Python, 파이프라인 문자열)의 모든 `x264enc`는 반드시 `bitrate=4000 speed-preset=ultrafast tune=zerolatency`를 포함해야 한다 — bare `x264enc`는 데드락을 유발한다 (pitfall #14)

## 파이프라인 템플릿

```
source ! queue ! DxPreprocess preprocess-id=0 ! queue ! DxInfer preprocess-id=0 model-path=/path/to/model.dxnn ! queue ! DxPostprocess ! queue ! DxOsd ! queue ! sink
```

## 빠른 참조

```bash
./install.sh                         # GStreamer 플러그인 바인딩 설치
./setup.sh                           # 샘플 모델 및 비디오 다운로드
dxrt-cli -s                          # NPU 사용 가능 여부 확인
gst-inspect-1.0 dxinfer              # DxInfer 플러그인 확인
pytest test/                         # 단위 테스트 실행
```

## 플레이스홀더 코드 금지 (필수)

스텁/플레이스홀더 코드를 절대 생성하지 않는다. 여기에는 다음이 포함된다:
- 주석 처리된 임포트: `# from dxnn_sdk import InferenceEngine`
- 가짜 결과: `result = np.zeros(...)`
- TODO 마커: `# TODO: implement actual inference`
- 실제 async 구현 없이 "sync 버전과 유사"

모든 생성 코드는 지식 베이스의 실제 API를 사용하여 기능적이어야 한다.
필요한 SDK/API를 모르는 경우, 먼저 관련 skill 문서를 읽는다.

## 실험적 기능 — 금지

실험적이거나 존재하지 않는 기능을 제안, 제시 또는 구현하지 않는다. 여기에는 다음이 포함된다:
- "웹 기반 비주얼 컴패니언" (web-based visual companion)
- 로컬 URL 기반 다이어그램 뷰어 또는 대시보드
- 사용자가 시각화를 위해 로컬 URL을 열어야 하는 기능
- 현재 toolset에 존재하지 않는 모든 기능

**Superpowers brainstorming skill 오버라이드**: superpowers `brainstorming` skill에는
"Visual Companion" 단계 (체크리스트의 2단계)가 포함되어 있다. 이 단계는 DEEPX 프로젝트에서
반드시 건너뛰어야 한다. visual companion은 우리 환경에 존재하지 않는다.
brainstorming 체크리스트에서 "Offer visual companion"이라고 하면, 건너뛰고
"Ask clarifying questions" (3단계)로 바로 진행한다.

기능이 존재하지 않으면 존재하는 것처럼 행동하지 않는다. 검증되고 문서화된
기능만 사용한다.

**Autopilot / 자율 모드 오버라이드**: 사용자가 부재 중일 때 (autopilot 모드,
auto-response "work autonomously", 또는 `--yolo` 플래그), brainstorming skill의
"Ask clarifying questions" 단계는 "지식 베이스 규칙에 따라 기본 결정을 내린다"로
대체해야 한다. `ask_user`를 호출하지 않는다 — 지식 베이스 기본값을 사용하여
brainstorming spec을 바로 작성한다. 이후의 모든 게이트 (spec 검토, 계획, TDD,
필수 아티팩트, 실행 검증)는 예외 없이 그대로 적용된다.

## 브레인스토밍 — 계획 전 Spec (필수 게이트)

superpowers `brainstorming` skill 또는 `/dx-brainstorm-and-plan` 사용 시:

1. **Spec 문서는 필수** — `writing-plans`로 전환하기 전에 spec 문서를
   `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`에 반드시 작성해야 한다.
   spec을 건너뛰고 바로 plan 작성으로 가는 것은 위반이다.
2. **사용자 승인 게이트는 필수** — spec 작성 후 사용자가 검토하고 승인해야
   plan 작성으로 진행할 수 있다. 관련 없는 사용자 응답 (예: 다른 질문에 답변)을
   spec 승인으로 취급하지 않는다.
3. **Plan 문서는 spec을 참조해야 한다** — plan 헤더에 승인된 spec 문서 링크가
   포함되어야 한다.
4. **`/dx-brainstorm-and-plan` 우선** — 일반 superpowers `brainstorming` skill 대신
   프로젝트 수준의 brainstorming skill을 사용한다. 프로젝트 수준 skill에는
   도메인 특화 질문과 사전 점검이 있다.

## 하드웨어

| 아키텍처 | 값 |
|---|---|
| DX-M1 | `dx_m1` |

## 자율 모드 보호 (필수)

사용자가 부재 중일 때 — autopilot 모드, `--yolo` 플래그, 또는 시스템 auto-response
"The user is not available to respond" — 다음 규칙이 적용된다:

1. **"자율적으로 작업하라"는 "묻지 않고 모든 규칙을 따르라"이지, "규칙을 건너뛰라"가 아니다.**
   모든 필수 게이트가 여전히 적용된다: brainstorming spec, plan, TDD, 필수 아티팩트,
   실행 검증, 자체 검증 확인.
2. **`ask_user`를 호출하지 않는다** — 지식 베이스 기본값과 문서화된 모범 사례를 사용하여
   결정을 내린다. autopilot에서 `ask_user`를 호출하면 턴이 낭비되며
   auto-response는 어떤 게이트도 우회할 권한을 부여하지 않는다.
3. **사용자 승인 게이트 적응** — autopilot에서 spec 승인 게이트는
   spec을 작성하고 지식 베이스에 대해 자체 검토하는 것으로 충족된다.
   spec 자체를 건너뛰지 않는다.
4. **setup.sh 우선** — 애플리케이션 코드를 작성하기 전에 인프라 아티팩트
   (`setup.sh`, `config.json`)를 먼저 생성한다. 이것은 autopilot에서 특히 중요한데
   누락된 종속성을 잡아줄 사람이 없기 때문이다.
5. **실행 검증은 선택 사항이 아니다** — 생성된 코드를 실행하고 완료 선언 전에
   작동하는지 확인한다. autopilot에서는 오류를 잡아줄 사용자가 없다.
6. **시간 예산 인식** — autopilot 세션에는 시간 제약이 있을 수 있습니다.
   행동을 효율적으로 계획하세요:
   - 컴파일 (ONNX → DXNN)은 5분 이상 걸릴 수 있습니다 — 일찍 시작하세요.
   - 시간이 부족하면 실행 검증보다 산출물 생성을 우선시하세요 — 테스트되지 않은
     완전한 파일 세트가 테스트된 불완전한 세트보다 낫습니다.
   - 우선순위: `setup.sh` > `run.sh` > 앱 코드 > `verify.py` > session.log.
   - **컴파일 병렬 워크플로 (HARD GATE)** — `dxcom` 또는 `dx_com.compile()`을
     bash 명령으로 시작한 후 기다리지 마세요. 즉시 모든 필수 산출물을 생성하세요:
     factory, 앱 코드, setup.sh, run.sh, verify.py. `.dxnn` 출력은 다른 모든
     산출물이 생성된 후에만 확인하세요. **이 규칙 위반 시 세션 실패입니다.**
   - **컴파일 대기를 위한 sleep-poll 금지** — `.dxnn` 파일을 폴링하기 위해
     `sleep`을 루프에서 사용하지 마세요. 금지된 패턴:
     `for i in ...; do sleep N; ls *.dxnn; done`,
     `while ! ls *.dxnn; do sleep N; done`,
     대기 사이에 반복되는 `ls *.dxnn` / `test -f *.dxnn` 확인.
     대신: 다른 모든 산출물을 먼저 생성한 후 `.dxnn` 파일이 존재하는지 한 번만
     확인하세요. 아직 존재하지 않으면 컴파일이 완료될 것이라는 가정하에 실행
     검증으로 진행하세요.
   - **필수 산출물은 컴파일과 독립적** — `setup.sh`, `run.sh`, `verify.py`,
     factory, 앱 코드는 `.dxnn` 파일이 존재할 필요가 없습니다. 알려진 모델 이름
     (예: `yolo26n.dxnn`)을 플레이스홀더 경로로 사용하여 생성하세요. 실행 검증만
     실제 `.dxnn`이 필요합니다.
7. **파일 읽기 도구 호출 최소화** — 컨텍스트에 이미 로드된 지침 파일, 에이전트 문서,
   스킬 문서를 다시 읽지 마세요. 불필요한 `cat` / `bash` 읽기마다 5-15초가
   낭비됩니다. 시스템 프롬프트와 대화 이력에 있는 지식을 사용하세요.

## 메모리

영구 지식은 `.deepx/memory/`에 있다. 작업 시작 시 읽고, 학습 시 업데이트한다.

## 규칙 충돌 해결 (HARD GATE)

사용자의 요청이 HARD GATE 규칙과 충돌하는 경우 에이전트는 반드시:

1. **사용자의 의도 인정** — 사용자가 원하는 것을 이해하고 있음을 보여주세요.
2. **충돌 설명** — 구체적인 규칙과 그 존재 이유를 인용하세요.
3. **올바른 대안 제시** — 프레임워크 내에서 사용자의 목표를 달성하는 방법을
   보여주세요. 예를 들어, 사용자가 직접 `InferenceEngine.run()` 사용을 요청하면
   IFactory 패턴이 동일한 API를 래핑함을 설명하고 factory 기반 대안을 제시하세요.
4. **올바른 접근 방식으로 진행** — 규칙 위반 요청에 묵인하지 마세요.
   "옵션 A vs 옵션 B"로 제시하지 마세요.

**일반적인 충돌 패턴** (실제 세션에서):
- 사용자가 "`InferenceEngine.Run()` 사용하라"고 함 → IFactory 패턴을 사용해야 함
  (engine 호출은 `run_inference()` 메서드 내에)
- 사용자가 "demo.py를 복제하고 onnxruntime을 교체하라"고 함 → 사용자 스크립트를
  복제하는 대신 `src/python_example/`에서 skeleton-first를 사용해야 함
- 사용자가 "demo_dxnn_sync.py를 생성하라"고 함 → 독립형 스크립트가 아닌
  SyncRunner를 사용한 `<model>_sync.py` 명명 규칙을 사용해야 함
- 사용자가 "`run_async()`를 직접 사용하라"고 함 → 수동 async 루프가 아닌
  AsyncRunner를 사용해야 함

**이 규칙은 명시적 사용자 재정의를 대체하지 않습니다**: 충돌을 안내받은 후
사용자가 명시적으로 "규칙을 이해하고 있으며, 직접 API 사용으로 진행하라"고 하면
따르세요. 하지만 에이전트는 먼저 충돌을 설명해야 합니다 — 묵인은 항상 위반입니다.

## Git 작업 — 사용자가 처리

작업 종료 시 git 브랜치 작업 (merge, PR, push, cleanup)에 대해 묻지 않는다.
사용자가 모든 git 작업을 직접 처리한다. "merge to main", "create PR",
"delete branch"와 같은 옵션을 절대 제시하지 않는다 — 그냥 작업을 완료한다.

## Git 안전 — Superpowers 아티팩트

**`docs/superpowers/` 아래 파일을 절대 `git add`하거나 `git commit`하지 않는다.**
이들은 superpowers skill 시스템에서 생성된 임시 계획 아티팩트 (spec, plan)이다.
`.gitignore` 처리되어 있지만 일부 도구가 `git add -f`로 `.gitignore`를 우회할 수 있다.
파일 생성은 괜찮지만 — 커밋은 금지이다.

## 세션 센티널 (자동화 테스트를 위한 필수 사항)

사용자 프롬프트를 처리할 때, 테스트 하네스의 자동화된 세션 경계 감지를 위해
다음 정확한 마커를 출력한다:

- **응답의 첫 번째 줄**: `[DX-AGENTIC-DEV: START]`
- **모든 작업 완료 후 마지막 줄**: `[DX-AGENTIC-DEV: DONE (output-dir: <relative_path>)]`
  여기서 `<relative_path>`는 세션 출력 디렉토리이다 (예: `dx-agentic-dev/20260409-143022_yolo26n_detection/`)

규칙:
1. **중요 — `[DX-AGENTIC-DEV: START]`를 첫 번째 응답의 절대 첫 줄로 출력한다.**
   이것은 다른 텍스트, 도구 호출, 또는 추론보다 앞에 나와야 한다.
   사용자가 "그냥 진행하라" 또는 "자체 판단으로 하라"고 지시하더라도
   START 센티널은 협상 불가이다 — 자동화 테스트가 이것 없이는 실패한다.
2. 모든 작업, 검증, 파일 생성이 완료된 후 맨 마지막 줄에 `[DX-AGENTIC-DEV: DONE (output-dir: <path>)]`를 출력한다
3. **상위 에이전트의 handoff/routing으로 호출된 sub-agent**인 경우
   이 센티널을 출력하지 않는다 — 최상위 에이전트만 출력한다
4. 사용자가 세션에서 여러 프롬프트를 보내면 각 프롬프트에 대해 START/DONE을 출력한다
5. DONE의 `output-dir`은 프로젝트 루트에서 세션 출력 디렉토리까지의 상대 경로여야 한다.
   파일이 생성되지 않은 경우 `(output-dir: ...)` 부분을 생략한다.
6. **계획 아티팩트만 생성한 후에는 절대 DONE을 출력하지 않는다** (spec, plan, 설계
   문서). DONE은 모든 결과물이 생성되었음을 의미한다 — 구현 코드, 스크립트,
   config, 검증 결과. brainstorming 또는 계획 단계를 완료했지만 실제 코드를
   아직 구현하지 않은 경우 DONE을 출력하지 않는다. 대신 구현을 진행하거나
   사용자에게 진행 방법을 묻는다.
7. **DONE 전 필수 결과물 확인**: DONE을 출력하기 전에 세션 디렉토리에 모든
   필수 결과물이 존재하는지 확인한다. 필수 파일이 누락된 경우 DONE을 출력하기
   전에 생성한다. 각 하위 프로젝트는 skill 문서에 자체 필수 파일 목록을
   정의한다 (예: `dx-build-pipeline-app.md` File Creation Checklist).
8. **세션 HTML 내보내기 안내** (Copilot CLI 전용): DONE 센티널 줄 바로 앞에 다음을
   출력한다: `To save this session as HTML, type: /share html`
   — 이것은 사용자가 전체 대화를 보존할 수 있음을 알려준다. `/share html` 명령은
   GitHub Copilot CLI에만 해당된다; Claude Code, Copilot Chat (VS Code), 또는
   OpenCode에서는 작동하지 않는다. 테스트 하네스 (`test.sh`)가 내보낸 HTML 파일을
   세션 출력 디렉토리에 자동으로 복사한다.

## Plan 출력 (필수)

plan 문서를 생성할 때 (예: writing-plans 또는 brainstorming skill을 통해),
파일 저장 직후 **항상 대화 출력에 전체 plan 내용을 출력한다**.
파일 경로만 언급하지 않는다 — 사용자가 별도의 파일을 열지 않고도
프롬프트에서 직접 plan을 검토할 수 있어야 한다.
