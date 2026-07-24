---
glob: "dx_stream/pipelines/**"
description: Rules for dx_stream pipeline scripts
---

# Contextual Rules: Stream Pipelines

These rules apply to all files under `dx_stream/pipelines/`.

## Mandatory Rules

1. **preprocess-id Matching**: Every DxPreprocess `preprocess-id` must match the
   corresponding DxInfer `preprocess-id`. Every DxInfer `inference-id` must match
   the downstream DxPostprocess `inference-id`.

2. **Queue Required**: A `queue` element must be placed between every pair of
   dx_stream elements. No exceptions.

3. **DxRate for RTSP**: Any pipeline using RTSP sources (`rtsp://`) must include
   a `dxrate` element between decodebin and DxPreprocess.

4. **model-path Absolute**: DxInfer `model-path` must be an absolute path or
   derived from `$SRC_DIR` variable. Never hardcode relative paths.

5. **library-file-path Absolute**: DxPostprocess `library-file-path` must be
   the absolute path to the installed .so file.

6. **Model Auto-Download**: Shell scripts must include the model auto-download
   block that checks for the model file and downloads if missing.

7. **SCRIPT_DIR Resolution**: All scripts must resolve paths relative to
   `SCRIPT_DIR` and `SRC_DIR`, not assume a working directory.

8. **DxTracker After DxPostprocess**: DxTracker must follow DxPostprocess,
   never DxInfer.

9. **DxMsgConv Before DxMsgBroker**: Broker pipelines must chain
   DxMsgConv → DxMsgBroker, never DxPostprocess → DxMsgBroker directly.

## Style Rules

- Use `gst-launch-1.0` for shell script pipelines
- Use `\` line continuation for readability
- Indent element properties under the element name
- Comment non-obvious pipeline sections
