# DX-STREAM 에이전틱 개발 가이드

## 개요

이 가이드는 **dx_stream**과 DEEPX NPU 가속기를 사용하여 GStreamer 파이프라인
애플리케이션을 구축하기 위한 DEEPX agent-driven development (dx-agent-dev) 사용 방법을
설명합니다. 에이전트가 파이프라인 구성, 모델 관리, 검증을 처리하므로 자연어 요청에서
작동하는 파이프라인 스크립트까지 몇 분 만에 진행할 수 있습니다.

---

## 에이전트

4개의 에이전트가 협력하여 dx_stream 파이프라인을 빌드, 설정, 검증합니다.

| 에이전트 | 설명 | 라우팅 대상 |
|---|---|---|
| `dx-stream-builder` | 마스터 라우터 — 사용자 요청에서 파이프라인 유형을 분류하여 적절한 전문 에이전트에 디스패치 | `dx-pipeline-builder`, `dx-model-manager` |
| `dx-pipeline-builder` | 6개 카테고리(단일 모델, 다중 모델, 캐스케이드, 타일, 병렬, 브로커)의 GStreamer 파이프라인 앱 빌드 | — |
| `dx-model-manager` | 파이프라인에서 사용할 `.dxnn` 모델 다운로드 및 설정 | — |
| `dx-validator` | 생성된 파이프라인 스크립트와 `.deepx/` 프레임워크 무결성 검증 | — |

### 라우팅 흐름

```
사용자 요청
    └─▶ dx-stream-builder (분류 및 라우팅)
            ├─▶ dx-pipeline-builder (파이프라인 생성)
            ├─▶ dx-model-manager   (모델 해석)
            └─▶ dx-validator       (출력 검증)
```

---

## 스킬

#### 범용 SWE 프로세스

| 스킬 | 설명 |
|------|------|
| `dx-swe-brainstorm` | 파이프라인 생성 전 브레인스토밍 및 계획 수립 |
| `dx-swe-tdd` | 테스트 주도 개발 — 파일 생성 직후 즉시 검증 |
| `dx-swe-verify` | 완료 선언 전 검증 — 증거 기반 확인 |
| `dx-swe-writing-plans` | 스펙 또는 요구사항에서 구현 계획 작성 |
| `dx-swe-executing-plans` | 검토 체크포인트가 포함된 구현 계획 실행 |
| `dx-swe-debugging` | 체계적 디버깅 — 수정 제안 전 진단 |
| `dx-swe-parallel-agents` | 2개 이상의 독립 작업을 병렬 에이전트에 디스패치 |
| `dx-swe-subagent-dev` | 독립적 하위 에이전트 작업으로 구현 계획 실행 |
| `dx-swe-receiving-review` | 코드 리뷰 피드백을 기술적 엄밀성으로 수신 및 처리 |
| `dx-swe-requesting-review` | 작업이 요구사항을 충족하는지 코드 리뷰 요청 |
| `dx-skill-router` | 작업 분류에 따라 적절한 스킬로 라우팅 |

#### DEEPX 빌드

| 스킬 | 설명 |
|------|------|
| `dx-agent-stream-build-pipeline` | 6개 카테고리의 GStreamer 파이프라인 빌드: 단일 모델, 다중 모델, 캐스케이드, 타일, 병렬, 브로커 |
| `dx-agent-stream-build-mqtt-kafka` | 이벤트 퍼블리싱을 위한 MQTT 또는 Kafka 메시지 브로커 파이프라인 빌드 |
| `dx-agent-stream-model-management` | 대상 NPU 아키텍처용 `.dxnn` 모델 다운로드 및 설정 |
| `dx-agent-stream-validate` | 파이프라인 검증 검사 실행 (문법, 속성, 엘리먼트 순서) |

---

## 지원 AI 도구

dx_stream 에이전틱 개발은 4가지 AI 코딩 도구에서 작동합니다. 각 도구는
자체 설정을 통해 지식 베이스를 자동으로 로드합니다.

| 도구 | 설정 파일 | 사용 가능한 에이전트 |
|---|---|---|
| **Claude Code** | `CLAUDE.md` | 컨텍스트 라우팅을 통해 4개 에이전트 전체 |
| **GitHub Copilot** | `.github/copilot-instructions.md`, `.github/agents/`의 4개 에이전트, `.github/skills/`의 16개 스킬, `.github/instructions/`의 2개 instruction | `@dx-stream-builder`, `@dx-pipeline-builder`, `@dx-model-manager`, `@dx-validator` |
| **Cursor** | `.cursor/rules/dx-stream.mdc` (항상), `dx-model-manager.mdc`, `dx-pipeline-builder.mdc`, `dx-stream-builder.mdc`, `dx-validator.mdc`, `stream-pipelines.mdc`, `tests.mdc`, 16개 `skill-*.mdc` 파일 (총 23개) | 자동 적용 규칙과 함께 자유 형식 대화 |
| **OpenCode** | `AGENTS.md`, `opencode.json`, `.opencode/agents/`의 4개 에이전트, `.deepx/skills/`의 16개 스킬 | `@dx-stream-builder` 또는 `/dx-agent-stream-build-pipeline` |

### Copilot 파일별 자동 Instruction

| 글로브 패턴 | 주입되는 Instruction | 내용 |
|---|---|---|
| `**/pipeline/**`, `**/pipelines/**`, `**/*pipeline*.py` | `stream-pipelines.instructions.md` | preprocess-id 매칭, queue 배치, RTSP용 DxRate, 엘리먼트 순서 |
| `test/**` | `tests.instructions.md` | pytest 패턴, 파이프라인 fixture, mock 엘리먼트 |

### OpenCode 스킬 (슬래시 명령)

**범용 SWE 프로세스**

| 슬래시 명령 | 설명 |
|---|---|
| `/dx-swe-brainstorm` | 파이프라인 생성 전 브레인스토밍 및 계획 수립 |
| `/dx-swe-tdd` | 점진적 검증을 포함한 테스트 주도 개발 |
| `/dx-swe-verify` | 증거 기반 완료 검증 |
| `/dx-swe-writing-plans` | 스펙에서 구현 계획 작성 |
| `/dx-swe-executing-plans` | 구현 계획 실행 |
| `/dx-swe-debugging` | 수정 제안 전 체계적 디버깅 |
| `/dx-swe-parallel-agents` | 2개 이상의 독립 작업을 병렬 에이전트에 디스패치 |
| `/dx-swe-subagent-dev` | 독립적 하위 에이전트 작업으로 계획 실행 |
| `/dx-swe-receiving-review` | 코드 리뷰 피드백 처리 |
| `/dx-swe-requesting-review` | 코드 리뷰 요청 |
| `/dx-skill-router` | 적절한 스킬로 라우팅 |

**DEEPX 빌드**

| 슬래시 명령 | 설명 |
|---|---|
| `/dx-agent-stream-build-pipeline` | 6개 카테고리의 GStreamer 파이프라인 빌드 |
| `/dx-agent-stream-build-mqtt-kafka` | MQTT/Kafka 브로커 파이프라인 빌드 |
| `/dx-agent-stream-model-management` | .dxnn 모델 다운로드 및 설정 |
| `/dx-agent-stream-validate` | 파이프라인 검증 검사 실행 |

### 플랫폼별 파일 참조

각 AI 코딩 에이전트는 dx_stream 레벨에서 서로 다른 설정 파일을 자동 로딩합니다.

#### 자동 로딩 파일

| 파일 | 자동 로드 주체 | 로딩 |
|------|----------------|------|
| `.github/copilot-instructions.md` | Copilot Chat/CLI | 자동 |
| `CLAUDE.md` | Claude Code | 자동 |
| `AGENTS.md` + `opencode.json` | OpenCode | 자동 |
| `AGENTS.md` + `.codex/skills/dx-codex-identity/SKILL.md` | Codex CLI | 자동 |
| `.cursor/rules/dx-stream.mdc` | Cursor | 자동 |

#### 에이전트 파일 (수동 @mention)

| 에이전트 | Copilot (`@mention`) | OpenCode (`@mention`) | Claude Code (`@mention`) |
|---------|------|---------|---------|
| `dx-stream-builder` | `.github/agents/dx-stream-builder.agent.md` | `.opencode/agents/dx-stream-builder.md` | `.claude/agents/dx-stream-builder.md` |
| `dx-pipeline-builder` | `.github/agents/dx-pipeline-builder.agent.md` | `.opencode/agents/dx-pipeline-builder.md` | `.claude/agents/dx-pipeline-builder.md` |
| `dx-model-manager` | `.github/agents/dx-model-manager.agent.md` | `.opencode/agents/dx-model-manager.md` | `.claude/agents/dx-model-manager.md` |
| `dx-validator` | `.github/agents/dx-validator.agent.md` | `.opencode/agents/dx-validator.md` | `.claude/agents/dx-validator.md` |

#### 스킬 파일 (모든 플랫폼 — `/slash-command`)

| 스킬 | 파일 |
|-------|------|
| `/dx-swe-brainstorm` | `.deepx/skills/dx-swe-brainstorm/SKILL.md` |
| `/dx-agent-stream-build-mqtt-kafka` | `.deepx/skills/dx-agent-stream-build-mqtt-kafka/SKILL.md` |
| `/dx-agent-stream-build-pipeline` | `.deepx/skills/dx-agent-stream-build-pipeline/SKILL.md` |
| `/dx-swe-parallel-agents` | `.deepx/skills/dx-swe-parallel-agents/SKILL.md` |
| `/dx-swe-executing-plans` | `.deepx/skills/dx-swe-executing-plans/SKILL.md` |
| `/dx-agent-stream-model-management` | `.deepx/skills/dx-agent-stream-model-management/SKILL.md` |
| `/dx-swe-receiving-review` | `.deepx/skills/dx-swe-receiving-review/SKILL.md` |
| `/dx-swe-requesting-review` | `.deepx/skills/dx-swe-requesting-review/SKILL.md` |
| `/dx-skill-router` | `.deepx/skills/dx-skill-router/SKILL.md` |
| `/dx-swe-subagent-dev` | `.deepx/skills/dx-swe-subagent-dev/SKILL.md` |
| `/dx-swe-debugging` | `.deepx/skills/dx-swe-debugging/SKILL.md` |
| `/dx-swe-tdd` | `.deepx/skills/dx-swe-tdd/SKILL.md` |
| `/dx-agent-stream-validate` | `.deepx/skills/dx-validate/SKILL.md` |
| `/dx-swe-verify` | `.deepx/skills/dx-swe-verify/SKILL.md` |
| `/dx-swe-writing-plans` | `.deepx/skills/dx-swe-writing-plans/SKILL.md` |

#### 공유 지식 베이스 (`.deepx/`)

`.deepx/` 디렉토리는 모든 에이전틱 개발 콘텐츠의 정식 소스입니다.
플랫폼별 파일(`.claude/`, `.github/`, `.cursor/`, `.opencode/`)은
`dx-agent-gen generate --repo dx-runtime/dx_stream`으로 `.deepx/`에서 생성됩니다.
생성된 파일을 직접 편집하지 마세요 — `.deepx/`를 수정하고 재생성하세요.

| 디렉토리 | 파일 | 설명 |
|-----------|-------|-------------|
| `.deepx/agents/` | 4개 파일 (`dx-stream-builder.md`, `dx-pipeline-builder.md`, `dx-model-manager.md`, `dx-validator.md`) | 권위 있는 에이전트 정의 |
| `.deepx/skills/` | 16개 디렉토리 (스킬당 1개, 각각 `SKILL.md` 포함) | 상세 스킬 워크플로우 |
| `.deepx/toolsets/` | 4개 파일 | GStreamer 요소 및 API 참조 |
| `.deepx/instructions/` | 6개 파일 | 코딩 표준 및 워크플로우 규칙 |
| `.deepx/memory/` | 4개 파일 | 지속적 지식 — 함정 및 세션 메모리 |
| `.deepx/templates/` | 템플릿 파일 | 코드 생성 템플릿 |
| `.deepx/knowledge/` | 지식 파일 | 도메인별 지식 베이스 |
| `.deepx/contextual-rules/` | 규칙 파일 | 에이전트용 컨텍스트 의존 규칙 |
| `.deepx/prompts/` | 프롬프트 파일 | 재사용 가능한 프롬프트 조각 |
| `.deepx/scripts/` | 스크립트 파일 | 검증 및 유틸리티 스크립트 |

#### Claude Code 에이전트 (`.claude/agents/`)

| 에이전트 | 파일 |
|---------|------|
| `dx-model-manager` | `.claude/agents/dx-model-manager.md` |
| `dx-pipeline-builder` | `.claude/agents/dx-pipeline-builder.md` |
| `dx-stream-builder` | `.claude/agents/dx-stream-builder.md` |
| `dx-validator` | `.claude/agents/dx-validator.md` |

#### GitHub Copilot 스킬 (`.github/skills/`)

16개 스킬 디렉토리 (`dx-agent-gen`으로 생성된 인라인 복사본), `.deepx/skills/`와 1:1 대응.

---

## 생성 파이프라인

모든 플랫폼별 설정 파일은 정식 소스인 `.deepx/`에서 `dx-agent-gen` 도구로 생성됩니다:

```bash
dx-agent-gen generate --repo dx-runtime/dx_stream
```

생성 대상:
- `.claude/agents/` — Claude Code 에이전트 파일
- `.github/agents/`, `.github/skills/`, `.github/instructions/` — GitHub Copilot 파일
- `.cursor/rules/` — Cursor 규칙 파일 (총 23개)
- `.opencode/agents/` — OpenCode 에이전트 파일
- `CLAUDE.md`, `AGENTS.md`, `opencode.json` — 플랫폼 진입점

**생성된 파일을 직접 편집하지 마세요.** `.deepx/`를 수정하고 재생성하세요.

---

## 사용자 시나리오

### 시나리오 1: 트래킹 포함 감지 파이프라인 빌드

**프롬프트:**

```
"RTSP 카메라에서 yolo26n으로 객체 감지하고 트래킹하는 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. `CLAUDE.md`가 `dx-agent-stream-build-pipeline` 스킬로 라우팅. RTSP URL, 디스플레이 설정, 트래커 유형 질문 후 DxRate → DxPreprocess → DxInfer → DxTracker → DxOsd 체인 생성. |
| **GitHub Copilot** | `@dx-stream-builder` 뒤에 프롬프트 입력. "단일 모델 + 트래킹"으로 분류, `dx-pipeline-builder`에 핸드오프, `dx-validator` 검사 실행. |
| **Cursor** | 프롬프트를 직접 입력. `dx-stream.mdc`(항상 로드)가 13개 엘리먼트 카탈로그 제공. 파이프라인 파일 편집 시 `stream-pipelines.mdc` 활성화. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

### 시나리오 2: MQTT 브로커 파이프라인 빌드

**프롬프트:**

```
"사람 감지해서 MQTT로 이벤트 보내는 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. `dx-agent-stream-build-mqtt-kafka` 스킬로 라우팅. `DxPostprocess ! DxMsgConv ! DxMsgBroker`로 끝나는 파이프라인 생성. |
| **GitHub Copilot** | `@dx-stream-builder` 뒤에 프롬프트 입력. |
| **Cursor** | 프롬프트를 직접 입력. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-mqtt-kafka` 스킬 직접 사용. |

### 시나리오 3: 다중 모델 캐스케이드 파이프라인

**프롬프트:**

```
"캐스케이드 파이프라인 만들어줘 — 먼저 사람 감지하고, 그 다음 행동 분류"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. 캐스케이드 패턴 생성: `DxInfer (1차) → DxPostprocess → DxTracker → tee → DxPreprocess(secondary-mode=true) → DxInfer(secondary-mode=true) → DxGather`. |
| **GitHub Copilot** | `@dx-pipeline-builder` 뒤에 프롬프트 입력. |
| **Cursor** | 프롬프트를 직접 입력. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

### 시나리오 4: 파이프라인 검증

**프롬프트:**

```
"방금 만든 파이프라인 검증해줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | `@dx-validator` 뒤에 프롬프트 입력. |
| **GitHub Copilot** | `@dx-validator` 뒤에 프롬프트 입력. |
| **Cursor** | 프롬프트를 직접 입력. |
| **OpenCode** | `@dx-validator` 뒤에 프롬프트 입력, 또는 수동 실행: `python .deepx/scripts/validate_app.py <script.sh>` |

### 시나리오 5: 포즈 추정 파이프라인 빌드

**프롬프트:**

```
"USB 카메라에서 yolo26n-pose로 포즈 추정 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. 포즈 추정 모델로 `dx-agent-stream-build-pipeline`에 라우팅. `DxOsd`를 통한 키포인트 오버레이 파이프라인 생성. |
| **GitHub Copilot** | `@dx-stream-builder` 뒤에 프롬프트 입력. "단일 모델 + 포즈"로 분류, `dx-pipeline-builder`에 핸드오프. |
| **Cursor** | 프롬프트를 직접 입력. 파이프라인 파일 생성 시 `stream-pipelines.mdc` 활성화. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

### 시나리오 6: 타일 고해상도 파이프라인 빌드

**프롬프트:**

```
"yolo26n으로 4K 입력 타일 감지 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **GitHub Copilot** | `@dx-pipeline-builder` 뒤에 프롬프트 입력. |
| **Cursor** | 프롬프트를 직접 입력. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

### 시나리오 7: 멀티스트림 병렬 파이프라인 빌드

**프롬프트:**

```
"yolo26n으로 RTSP 카메라 4대 병렬 처리하는 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. 병렬 카테고리로 `dx-agent-stream-build-pipeline`에 라우팅. `DxInputSelector+DxOutputSelector`를 이용한 공유 추론 또는 독립 서브파이프라인으로 4개 소스 처리. |
| **GitHub Copilot** | `@dx-pipeline-builder` 뒤에 프롬프트 입력. |
| **Cursor** | 프롬프트를 직접 입력. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

### 시나리오 8: 세그멘테이션 파이프라인 빌드

**프롬프트:**

```
"yolo26n-seg로 도로 장면 분석하는 세그멘테이션 파이프라인 만들어줘"
```

| 도구 | 사용 방법 |
|---|---|
| **Claude Code** | 프롬프트를 직접 입력. 세그멘테이션 모델로 `dx-agent-stream-build-pipeline`에 라우팅. `DxOsd`를 통한 픽셀별 마스크 오버레이 파이프라인 생성. |
| **GitHub Copilot** | `@dx-stream-builder` 뒤에 프롬프트 입력. "단일 모델 + 세그멘테이션"으로 분류. |
| **Cursor** | 프롬프트를 직접 입력. 파이프라인 파일 생성 시 `stream-pipelines.mdc` 활성화. |
| **OpenCode** | `@dx-stream-builder` 뒤에 프롬프트 입력, 또는 `/dx-agent-stream-build-pipeline` 스킬 직접 사용. |

---

## 빠른 시작

자연어 설명으로 마스터 라우터를 호출하세요:

```
@dx-stream-builder "RTSP 카메라에서 yolo26n으로 객체 감지하고 트래킹하는 파이프라인 만들어줘"
```

에이전트가 수행하는 작업:

1. **명확화 질문** — 파이프라인 카테고리, 소스 유형, 모델 변형, 출력 싱크
2. **파이프라인 계획 제시** — 엘리먼트 체인, 속성, 모델 경로
3. **`dx-pipeline-builder`에 라우팅** — 파이프라인 스크립트 생성
4. **모델 해석** — `dx-model-manager`를 통해 `.dxnn` 파일 다운로드 또는 위치 확인
5. **검증 및 보고** — `dx-validator` 실행 후 결과 반환

---

## 생성 결과물

기본적으로 AI가 생성한 파이프라인 코드는 기존 스크립트와의 충돌을 방지하기 위해
`dx-agent-dev/` 격리 디렉토리에 배치됩니다.

### 기본 출력 (dx-agent-dev/)

```
dx-agent-dev/<session_id>/
├── README.md              # 세션 메타데이터 및 실행 지침
├── session.json           # 기계 판독 가능한 세션 설정
└── {pipeline_name}.py     # 생성된 파이프라인 스크립트
```

세션 ID 형식: `YYYYMMDD-HHMMSS_model_task` (예: `20260403-150045_yolo26n_tracking`).

### 프로덕션 출력

프로덕션 배치를 명시적으로 요청하면 파일이 표준 파이프라인 디렉토리에
직접 작성됩니다.

---

## 파이프라인 카테고리

dx_stream은 6개 파이프라인 카테고리를 지원합니다. 각각 고유한 GStreamer 엘리먼트 패턴을 따릅니다.

| 카테고리 | 패턴 | 핵심 엘리먼트 |
|---|---|---|
| **단일 모델** | `src ! DxPreprocess ! DxInfer ! DxPostprocess ! DxOsd ! sink` | 핵심 추론 트리오 + 화면 표시 |
| **다중 모델** | 각각 고유한 `preprocess-id`를 가진 여러 `DxInfer` 단계 체인 | 순차적 다중 추론 패스 |
| **캐스케이드** | `DxInfer` → `DxPostprocess` → `DxTracker` → `tee` → `DxPreprocess(secondary-mode=true)` → `DxInfer(secondary-mode=true)` → `DxGather` | secondary-mode=true로 2차 추론에 ROI 자동 공급 |
| **병렬** | `DxInputSelector ! DxInfer ! DxOutputSelector` 또는 독립 서브파이프라인 | 다중 스트림 수집 및 처리 |
| **브로커** | `DxPostprocess ! DxMsgConv ! DxMsgBroker` | 감지 결과 직렬화하여 MQTT/Kafka에 퍼블리시 |

---

## GStreamer 엘리먼트 참조

dx_stream은 NPU 가속 파이프라인을 위한 13개 커스텀 GStreamer 엘리먼트를 제공합니다.

| 엘리먼트 | 용도 |
|---|---|
| `DxPreprocess` | 모델 입력을 위한 프레임 리사이즈, 정규화, 색상 변환 |
| `DxInfer` | DEEPX NPU에서 `.dxnn` 모델 실행 |
| `DxPostprocess` | 원시 텐서를 구조화된 감지/분류 결과로 디코딩 |
| `DxTracker` | 다중 객체 추적 (프레임 간 영속적 ID 할당) |
| `DxOsd` | 프레임에 바운딩 박스, 레이블, 오버레이 그리기 |
| `DxGather` | N-to-1 병합 — 여러 브랜치에서 버퍼 수집 |
| `DxInputSelector` | N-to-1 라운드 로빈 입력 선택 |
| `DxOutputSelector` | 1-to-N 디먹스 — 버퍼를 N개 출력 패드 중 하나로 라우팅 |
| `DxRate` | 프레임 속도 제한 (RTSP 소스에 필수) |
| `DxMsgConv` | 감지 메타데이터를 JSON으로 직렬화 |
| `DxMsgBroker` | 직렬화된 메시지를 MQTT 또는 Kafka에 퍼블리시 |
| `DxScale` | 프레임을 대상 해상도로 리사이즈 |
| `DxConvert` | 형식 간 색상 공간 변환 |

---

## 파이프라인 전용 검증 규칙

에이전트는 파이프라인을 생성하고 검증할 때 다음 규칙을 적용합니다:

1. **`preprocess-id` 매칭** — `DxPreprocess`와 `DxInfer`는 동일한 `preprocess-id` 값을 공유해야 합니다. 불일치 시 무음 추론 실패가 발생합니다.
2. **Queue 배치** — 파이프라인 데드락을 방지하기 위해 모든 처리 단계 사이에 `queue` 엘리먼트를 삽입합니다.
3. **RTSP용 `DxRate`** — RTSP 소스 직후에 항상 `DxRate`를 삽입하여 프레임 폭주와 버퍼 오버런을 방지합니다.
4. **`DxMsgConv` → `DxMsgBroker` 순서** — `DxMsgBroker`로 퍼블리시하기 전에 항상 `DxMsgConv`로 메타데이터를 직렬화합니다.
5. **절대 모델 경로** — `DxInfer`의 `model-path` 속성은 절대 파일 시스템 경로여야 합니다. 상대 경로는 런타임 에러를 유발합니다.

---

## 검증 명령

```bash
# 파이프라인 스크립트 검증 (정적 검사)
python .deepx/scripts/validate_app.py <script.sh>

# 스모크 테스트 포함 검증 (NPU 하드웨어 필요)
python .deepx/scripts/validate_app.py <script.sh> --smoke-test

# .deepx/ 프레임워크 무결성 검사
python .deepx/scripts/validate_framework.py
```

검증 검사 항목:
- 엘리먼트 속성 완전성
- `DxPreprocess` / `DxInfer` 쌍 간 `preprocess-id` 일관성
- 처리 단계 간 queue 배치
- 지정된 `model-path`에 모델 파일 존재 여부
- 브로커 엘리먼트 순서 (`DxMsgConv` → `DxMsgBroker`)

---

## 세션 센티넬

에이전트는 자동화된 테스트를 위해 각 작업의 시작과 끝에 고정된 마커를 출력합니다:

| 마커 | 출력 시점 |
|---|---|
| `[DX-AGENT-DEV: START]` | 에이전트 응답의 첫 번째 줄 |
| `[DX-AGENT-DEV: DONE (output-dir: <relative_path>)]` | 모든 작업 완료 후 마지막 줄. `<relative_path>`는 프로젝트 루트 기준 세션 출력 디렉토리의 상대 경로입니다. 파일이 생성되지 않은 경우 `(output-dir: ...)` 부분을 생략합니다. |

핸드오프를 통해 호출된 하위 에이전트는 센티넬을 출력하지 않으며, 최상위 에이전트만 출력합니다.

규칙:
1. **필수** — 첫 번째 응답의 절대적 첫 줄에 `[DX-AGENT-DEV: START]`를 출력합니다. 다른 텍스트, tool call, reasoning보다 반드시 먼저 출력해야 합니다. 사용자가 "알아서 진행해"라고 해도 생략 불가 — 자동 테스트가 실패합니다.
2. 모든 작업 완료 후 마지막 줄로 `[DX-AGENT-DEV: DONE (output-dir: <path>)]`를 출력합니다.
3. 핸드오프를 통해 호출된 하위 에이전트는 센티넬을 출력하지 않습니다.
4. 사용자가 세션에서 여러 프롬프트를 보내는 경우 각 프롬프트마다 START/DONE을 출력합니다.
5. DONE의 `output-dir`는 프로젝트 루트에서 세션 출력 디렉토리까지의 상대 경로여야 합니다.
6. **기획 산출물(spec, plan, 설계 문서)만 작성한 상태에서는 절대 DONE을 출력하지 마세요.** DONE은 모든 산출물(구현 코드, 스크립트, 설정 파일, 검증 결과)이 생성된 후에만 출력합니다.

---

## 문제 해결

| 증상 | 원인 | 해결 방법 |
|---|---|---|
| 추론이 감지 결과를 생성하지 않음 | `DxPreprocess`와 `DxInfer` 간 `preprocess-id` 불일치 | 두 엘리먼트가 동일한 `preprocess-id` 값을 공유하는지 확인 |
| 파이프라인 데드락 또는 멈춤 | 처리 단계 사이에 `queue` 엘리먼트 누락 | 모든 처리 엘리먼트 쌍 사이에 `queue` 엘리먼트 추가 |
| RTSP 스트림 프레임 드롭 | 소스 후 프레임 속도 제한 없음 | RTSP 소스 엘리먼트 직후에 `DxRate` 삽입 |
| 런타임에 `model-path` 미발견 | `model-path` 속성에 상대 경로 사용 | 절대 경로 사용 (예: `/opt/deepx/models/yolo26-n_640x640.dxnn`) |
| `DxInfer` 플러그인 미등록 | 플러그인 미설치 또는 `GST_PLUGIN_PATH` 미설정 | `gst-inspect-1.0 dxinfer`로 확인; `GST_PLUGIN_PATH` 검사 |
| 브로커 파이프라인이 빈 메시지 전송 | `DxMsgBroker` 전에 `DxMsgConv` 누락 | 브로커 엘리먼트 전에 `DxMsgConv`로 메타데이터 직렬화 추가 |
