# SDKREQ-450 — Windows pydxs import 오류 수정 보고서

관련 요구사항: [SDKREQ-450](https://deepx.atlassian.net/browse/SDKREQ-450), AC3: venv 활성화 후 `python -c "import pydxs"` 정상 동작.

## 원인

- Windows Python 3.8+에서는 native extension의 의존 DLL을 일반적인 PATH 추가만으로 해결할 수 없다.
- 기존 pydxs에는 DLL 검색 경로를 등록하는 loader가 없어 일반 import가 실패했다.
- 빌드와 테스트는 import 전에 경로를 수동 등록했으므로, 해당 PASS 결과만으로 AC3를 확인할 수 없었다.

## 대책

- Windows pydxs package에 [DLL loader](../../bindings/python/pydxs/src/pydxs/_windows.py)를 추가하고, native extension을 내부 module `pydxs._pydxs`로 전환했다. 공개 API는 유지했다.
- 선택된 checkout과 GStreamer·DEEPX SDK 경로만 등록하며, DLL directory handle을 유지한다. 누락·등록·native loading 오류에는 경로와 조치 안내를 제공한다.
- 빌드 검증과 API 테스트의 수동 DLL 등록을 제거했다. Linux의 native module 이름과 기존 rpath는 유지했다.
- 기존 bare extension이 새 wheel에 포함되지 않도록 incremental build 잔여물도 처리했다.
- Python source는 `src` layout에 배치하여, Linux 빌드 디렉터리에서 source package가 설치된 native module을 가리지 않도록 했다.

## 재발방지 및 검증

수정 기준: `e1fcb75` 이후 작업 변경분.

검증 환경: Windows 11 Pro / Python 3.12.8 / GStreamer 1.28.4 MSVC x64 / DX-RT 2.4.2.

| 검증 항목 | 결과 |
|-----------|------|
| 수정 전 새 process 일반 import | DLL load 오류 재현 |
| 수정 후 전체 빌드 및 일반 import 검증 | exit 0 |
| 새 CMD에서 venv 활성화 후 일반 import | exit 0 |
| SDK 환경변수와 SDK PATH를 제거한 새 process import 및 API 호출 | PASS |
| checkout 외부의 작업 디렉터리에서 import | PASS |
| binding 빌드 디렉터리에서 import | PASS |
| 실제 wheel을 별도 외부 venv에 설치 후 일반 import 및 API 호출 | PASS |
| 잘못된 checkout 및 실제 의존 DLL 누락 시 진단 안내 | PASS |
| loader regression checks | 24/24 PASS |
| Windows regression suites | 52/52 PASS |
| 기존 pydxs API checks | 17 PASS, 0 FAIL, 4 SKIP |

[test/run_pydxs.bat](../../test/run_pydxs.bat)에 loader regression checks를 연결했다. SDK 선택 우선순위, 다른 checkout의 PATH 제외, handle 유지·실패 시 정리, 일반 import와 공개 API를 지속 검증한다.

## 확인 범위 및 결론

- 현재 검증 환경에서 AC3 실패를 수정하고 실제 일반 import 성공을 확인했다.
- `gi` 미설치로 pipeline 기반 4개 검증은 SKIP했다. NPU inference 전체 검증을 의미하지 않는다.
- 원래 QA 환경의 Python 3.12.10 / DX-RT 3.5.0 조합과 Linux 실제 build/run은 별도 확인이 필요하다. Linux packaging 분기와 Python 3.6 문법은 static check로 확인했다.
- 문서의 generator drift check는 `dx-agent-gen`이 없는 standalone 환경에서 실행하지 못했다. Generator-managed 파일은 수정하지 않았다.
