# dx_stream Agent Protocols

## Protocol 1: Research Before Building

**Never compose a pipeline without first reading:**
- `.deepx/skills/dx-agentic-stream-build-pipeline.md` for templates
- `.deepx/toolsets/dx-stream-elements.md` for element properties
- `.deepx/memory/common_pitfalls.md` for known issues

Verify the model exists in `model_list.json` before using it.

## Protocol 2: Ask Before Assuming

Always confirm with the user:
1. Pipeline category (single/multi/tracking/secondary/rtsp/broker)
2. Vision task (detection/face/pose/segmentation/classification)
3. Input source (file/USB/RTSP)
4. Additional features (tracking, broker, FPS display)

## Protocol 3: Incremental Delivery

Deliver pipeline artifacts incrementally:
1. Pipeline string first (validate with user)
2. Shell script wrapper
3. Custom postprocess (if needed)
4. Run and verify

## Protocol 4: Validate Every Artifact

Before declaring completion:
- Parse the pipeline string for syntax errors
- Verify element property names against `.deepx/toolsets/dx-stream-elements.md`
- Check preprocess-id / inference-id matching
- Verify queue placement between elements
- Confirm model and postprocess library paths

## Protocol 5: Skill Document Sufficiency

The skill document (`.deepx/skills/dx-agentic-stream-build-pipeline.md`) contains complete
templates and patterns. Read it FIRST before exploring source code. Only read
source files if the skill document lacks the specific information needed.

## Protocol 6: Error Recovery

When a pipeline fails:
1. Check `GST_DEBUG=3` output for the first ERROR message
2. Common failures: element not found, property not valid, link negotiation failed
3. Verify GStreamer plugin registration: `gst-inspect-1.0 dxinfer`
4. Check model file exists and is readable
5. Check postprocess .so exists and exports PostProcess

## Protocol 7: Memory Updates

After discovering new patterns or pitfalls during pipeline development:
1. Check if the issue is already in `.deepx/memory/common_pitfalls.md`
2. If new, add it with `[DX_STREAM]` tag
3. If it is a general issue, use `[UNIVERSAL]` tag

## Protocol 8: Context Handoff

When routing from dx-stream-builder to dx-pipeline-builder, pass:
- Pipeline category
- Model name
- Input source type
- Feature list
- Any user-specified constraints

## Protocol 9: File Placement

| Artifact | Directory |
|---|---|
| Pipeline shell scripts | `dx_stream/pipelines/<category>/` |
| Python pipeline scripts | `dx_stream/pipelines/<category>/` |
| Postprocess C++ libraries | `dx_stream/custom_library/postprocess_library/<Model>/` |
| Config JSON files | `dx_stream/configs/<Model>/` |
| Test scripts | `test/` |

## Protocol 10: No Hardcoded Paths

Never hardcode absolute paths in committed scripts. Always derive paths from
`SCRIPT_DIR` using the standard pattern:

```bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
```

Exception: postprocess library paths use the system install location
`/usr/local/share/gstdxstream/lib/` which is consistent across installs.

## Protocol 11: Platform Verification

Before running any pipeline, verify the platform is ready:

```bash
# 1. Verify NPU is accessible
dxrt-cli -s 2>/dev/null || echo "WARNING: NPU not detected"

# 2. Verify GStreamer plugin is registered
gst-inspect-1.0 dxinfer > /dev/null 2>&1 || {
    echo "ERROR: dxinfer element not registered"
    echo "Run: ./install.sh && ./build.sh"
    exit 1
}

# 3. Check DX-RT version
dpkg -l | grep dx-runtime || echo "WARNING: DX-RT package not found"
```
