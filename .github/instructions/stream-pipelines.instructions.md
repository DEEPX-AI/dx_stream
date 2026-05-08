---
applyTo: "**/pipeline/**,**/pipelines/**,**/*pipeline*.py"
---

# Pipeline — Contextual Instructions

Working on dx_stream GStreamer pipeline code.

## Required Context
- `.deepx/skills/dx-agentic-stream-build-pipeline.md`
- `.deepx/toolsets/dx-stream-elements.md`
- `.deepx/memory/common_pitfalls.md`

## Rules
- preprocess-id matching: DxPreprocess and DxInfer must share the same value
- Queue between every stage
- DxRate for RTSP sources
- Absolute model-path for DxInfer
- DxMsgConv before DxMsgBroker
