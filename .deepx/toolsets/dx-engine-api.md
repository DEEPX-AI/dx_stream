# DX Engine API Reference (dx_stream subset)

> **SDK Source of Truth**: `dx_rt/python_package/src/dx_engine/`, shared with dx_app

## Overview

This is a minimal reference for the DEEPX inference engine as it relates to
dx_stream pipeline elements. The DxInfer element internally uses the inference
engine to execute .dxnn models on the NPU.

Understanding these parameters helps configure DxInfer element properties.

## Model File Format (.dxnn)

The `.dxnn` format is the compiled model binary for DEEPX NPU:
- Compiled from standard frameworks (ONNX, TFLite, PyTorch) via the DEEPX compiler
- Contains optimized NPU instructions, weight data, and I/O shape metadata
- Hardware-specific: a .dxnn compiled for DX-M1 may not work on DX-M1A (discontinued)

## Key Parameters Mapped to DxInfer

| Engine Parameter | DxInfer Property | Description |
|---|---|---|
| `model_path` | `model-path` | Absolute path to .dxnn file |
| `batch_size` | `batch-size` | Number of frames per inference batch |
| `input_width` | (via DxPreprocess `resize-width`) | Model input width |
| `input_height` | (via DxPreprocess `resize-height`) | Model input height |
| `num_outputs` | (auto-detected from .dxnn) | Number of output tensors |

## Input/Output Shape Information

The inference engine extracts shape information from the .dxnn file:

| Shape | Typical Values | Notes |
|---|---|---|
| Input | `[1, 3, 640, 640]` or `[1, 640, 640, 3]` | Batch x Channels x Height x Width |
| Output (detection) | `[1, N, 85]` | N proposals, 85 = 4 bbox + 1 obj + 80 classes |
| Output (pose) | `[1, N, 57]` | N proposals, 57 = 4 bbox + 1 obj + 17*3 keypoints |
| Output (segmentation) | `[1, N, 117]` + `[1, 32, H, W]` | Proposals + prototype masks |
| Output (classification) | `[1, 1000]` | 1000-class probability vector |

## Batch Inference

DxInfer supports batch inference for throughput optimization:

```bash
# Batch size of 4 (processes 4 frames per NPU call)
dxinfer preprocess-id=1 inference-id=1 model-path=/path/model.dxnn batch-size=4
```

Constraints:
- batch-size must match the .dxnn model's compiled batch dimension
- Higher batch sizes increase latency but improve throughput
- Default batch-size=1 is suitable for most real-time pipelines

## NPU Scheduling

When multiple DxInfer elements exist in a pipeline (e.g., secondary mode),
the NPU schedules inference requests internally:
- Requests are queued and processed in order
- Primary inference has no priority over secondary
- Use `interval` on secondary DxPreprocess to reduce secondary inference load

## Error Conditions

| Error | Cause | Resolution |
|---|---|---|
| "Failed to load model" | .dxnn file not found or corrupted | Check model-path, re-download |
| "NPU device not available" | Driver not loaded or device busy | Check `dxrt-cli -s`, restart driver |
| "Input shape mismatch" | DxPreprocess dimensions don't match model | Adjust resize-width/height |
| "Inference timeout" | NPU hung or overloaded | Reduce batch-size or number of models |
