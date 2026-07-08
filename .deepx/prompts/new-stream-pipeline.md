# Prompt: New Stream Pipeline

Create a dx_stream GStreamer pipeline for **{vision_task}** using **{model_name}**.

## Parameters

- **Vision Task**: `{vision_task}` (detection | face_detection | pose_estimation | segmentation | classification)
- **Model**: `{model_name}` (from model_list.json)
- **Pipeline Type**: `{pipeline_type}` (single_network | multi_stream | tracking)
- **Input**: `{input_source}` (file path | usb | rtsp://...)
- **Display**: `{display}` (fpsdisplaysink | fakesink)

## Steps

### 1. Compose Pipeline

Read `.deepx/skills/dx-agent-stream-build-pipeline/SKILL.md` for the pipeline template.
Select the pattern matching `{pipeline_type}`.

Compose the gst-launch-1.0 pipeline string with:
- Source element matching `{input_source}`
- DxPreprocess with correct resize dimensions for `{model_name}`
- DxInfer with model-path
- DxPostprocess with matching postprocess library
- DxTracker if `{pipeline_type}` is tracking
- Appropriate sink

### 2. Create Pipeline Script

Create `dx_stream/pipelines/{pipeline_type}/run_{app_name}.sh`:
- Model auto-download logic
- DISPLAY detection for headless mode
- Pipeline execution

### 3. Create Python Pipeline (Optional)

If the user needs programmatic control, create `pipeline.py` with:
- argparse CLI
- GStreamer pipeline construction
- Bus message handling
- Clean shutdown

### 4. Validate

```bash
# Verify element registration
gst-inspect-1.0 dxinfer

# Verify model exists
ls dx_stream/samples/models/{model_name}.dxnn

# Run pipeline
bash dx_stream/pipelines/{pipeline_type}/run_{app_name}.sh
```

## Deliverables

- [ ] Pipeline shell script
- [ ] Pipeline Python script (if requested)
- [ ] Validation output
- [ ] Run instructions
