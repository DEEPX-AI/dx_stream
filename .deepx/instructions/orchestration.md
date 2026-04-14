# dx_stream Orchestration

## 5-Phase Pipeline Build Lifecycle

### Phase 1: Discovery

Determine what the user needs:
- Pipeline category
- Vision task and model
- Input source
- Output mode (display, broker, file)
- Additional features

**Gate:** User confirms build plan before proceeding.

### Phase 2: Context Loading

Load required knowledge:
- Pipeline skill document
- Element reference
- Common pitfalls
- Model registry (if model selection needed)

**Gate:** All referenced models exist in model_list.json.

### Phase 3: Composition

Create pipeline artifacts:
1. Pipeline string (gst-launch-1.0 compatible)
2. Shell script wrapper with model download logic
3. Custom postprocess library (if needed)
4. Config JSON files (if using config-file-path mode)

**Gate:** Pipeline string parses without syntax errors.

### Phase 4: Validation

Run validation checks:
- Element property validation
- ID matching (preprocess-id, inference-id)
- Queue placement verification
- Path existence checks
- Pipeline smoke test (if NPU available)

**Gate:** All validation checks pass.

### Phase 5: Delivery

Present to user:
- Pipeline topology summary
- Created files list with paths
- Run instructions
- Tuning parameters and known limitations
- Next steps (e.g., adding tracking, broker, multi-stream)

## Sub-Agent Types

| Agent | Role | When Used |
|---|---|---|
| **Pipeline Composer** | Builds gst-launch-1.0 strings and shell scripts | Every pipeline build |
| **Postprocess Builder** | Creates C++ postprocess .so with meson build | When custom model needs new postprocess |
| **Config Writer** | Generates JSON config files | When using config-file-path mode |
| **Test Writer** | Creates validation scripts | When user requests test coverage |
| **Broker Integrator** | Adds DxMsgConv + DxMsgBroker chain | When broker output is needed |

## Orchestration Patterns

### Simple Build (Single Network)

```
User Request
  → dx-stream-builder (classify: single_network)
  → dx-pipeline-builder (compose + build)
  → Deliver
```

### Complex Build (Secondary Mode + Broker)

```
User Request
  → dx-stream-builder (classify: secondary_mode + broker)
  → dx-pipeline-builder (compose primary + secondary chains)
  → dx-pipeline-builder (add broker chain)
  → Validate
  → Deliver
```

### Model Query + Build

```
User Request
  → dx-stream-builder (need model info)
  → dx-model-manager (query + download)
  → dx-pipeline-builder (compose with downloaded model)
  → Deliver
```

## Error Handling

| Error | Recovery Action |
|---|---|
| Model not in model_list.json | Suggest closest match or ask user to provide .dxnn |
| Postprocess .so not found | Build from source or check install |
| Element not registered | Run install.sh, check DX-RT version |
| Pipeline link failure | Check caps negotiation, add videoconvert/dxconvert |
| Broker connection refused | Verify broker is running, check conn-info |

## Output Isolation

All AI-generated pipeline applications MUST be created under `dx-agentic-dev/`,
not in production directories. This is a HARD GATE.

### Rules

1. **Default output**: `dx-agentic-dev/<YYYYMMDD-HHMMSS>_<model>_<category>/`
2. **Session metadata**: Include `session.json` and `README.md` in every session
3. **Production write**: Only when user EXPLICITLY requests it
4. **Validation**: `validate_app.py` accepts any directory path

### Integration with 5-Phase Lifecycle

- **Phase 1 (Discovery)**: Determine output path — default `dx-agentic-dev/`
- **Phase 3 (Composition)**: Create session directory, write `session.json`
- **Phase 5 (Delivery)**: Report full path to created session directory
