# Prompt: Orchestrated dx_stream Build

Meta-template for orchestrated pipeline builds that span multiple categories
or require coordinated sub-agent work.

## Parameters

- **Primary Task**: `{primary_task}` (e.g., "Multi-stream detection with Kafka publishing")
- **Models**: `{model_list}` (e.g., ["yolo26n", "EfficientNet_Lite0"])
- **Pipeline Categories**: `{categories}` (e.g., [multi_stream, broker])
- **Input Sources**: `{inputs}` (e.g., ["rtsp://cam1", "rtsp://cam2"])

## Orchestration Flow

### Phase 1: Discovery

1. Parse user requirements into pipeline categories
2. Identify all required models
3. Determine element composition for each category
4. Check model availability in model_list.json

### Phase 2: Model Preparation

For each model in `{model_list}`:
```bash
./setup.sh --model="{model}.dxnn"
```

### Phase 3: Pipeline Composition

For each category in `{categories}`:
1. Load the corresponding pipeline pattern from skill documents
2. Compose element chain with correct ID matching
3. Generate pipeline script

### Phase 4: Integration

If multiple categories are combined (e.g., tracking + broker):
1. Merge pipeline chains at integration points
2. Verify element ordering rules
3. Add tee elements for parallel output paths

### Phase 5: Validation

```bash
# Per-pipeline validation
for script in generated_scripts; do
    bash -n "$script"  # Syntax check
done

# Integration test
bash run_orchestrated.sh
```

### Phase 6: Delivery

- Pipeline topology diagram showing all streams and branches
- Generated files list
- Run instructions for each component
- Known limitations and tuning parameters

## Sub-Agent Coordination

| Sub-Agent | Task | Dependencies |
|---|---|---|
| dx-model-manager | Download required models | None |
| dx-pipeline-builder | Build primary pipeline | Models downloaded |
| dx-pipeline-builder | Build secondary/broker chain | Primary pipeline complete |

## Example: Multi-Stream Detection + Kafka

```
Phase 1: 2 categories (multi_stream + broker)
Phase 2: Download yolov5-s_640x640_ppu.dxnn
Phase 3: Compose multi-stream pipeline with compositor
Phase 4: Add DxMsgConv + DxMsgBroker branch via tee
Phase 5: Validate combined pipeline
Phase 6: Deliver scripts + Kafka consumer
```
