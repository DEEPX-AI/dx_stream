# Common Pitfalls — dx_stream

Domain-tagged known issues. Read at the start of every pipeline task.

---

## 1. [DX_STREAM] DxPreprocess preprocess-id / DxInfer preprocess-id Mismatch

**Symptom:** Pipeline runs but produces no detections or wrong results.
**Cause:** DxPreprocess `preprocess-id` does not match DxInfer `preprocess-id`, so preprocessed data is not routed to the correct inference engine.
**Fix:** Ensure `preprocess-id` on DxPreprocess matches `preprocess-id` on the corresponding DxInfer. In secondary mode, use distinct IDs per branch (e.g., primary=1, secondary=2).

---

## 2. [DX_STREAM] RTSP Buffer Overload Without DxRate

**Symptom:** Memory usage grows continuously, pipeline becomes increasingly laggy, eventual OOM or crash.
**Cause:** RTSP sources produce frames at variable or high rates. Without DxRate, frames accumulate in queue buffers faster than the NPU can process them.
**Fix:** Add `dxrate max-rate=30` (or appropriate value) between decodebin and DxPreprocess for all RTSP sources.

---

## 3. [DX_STREAM] DxMsgBroker Must Follow DxMsgConv

**Symptom:** DxMsgBroker receives raw buffer data instead of JSON, broker messages are garbled or empty.
**Cause:** DxMsgBroker is connected directly to DxPostprocess without DxMsgConv to serialize metadata to JSON.
**Fix:** Always chain: `... ! dxpostprocess ! queue ! dxmsgconv config-file-path=... ! queue ! dxmsgbroker ...`

---

## 4. [DX_STREAM] Secondary Mode ROI Requires DxScale Before 2nd DxInfer

**Symptom:** Secondary inference produces incorrect results or crashes with shape mismatch.
**Cause:** Object ROIs from the primary detector are not resized to the secondary model's expected input dimensions.
**Fix:** Set `secondary-mode=true` and `resize-width`/`resize-height` on the secondary DxPreprocess. It automatically crops and resizes detected object ROIs.

---

## 5. [DX_STREAM] DxPostprocess library-file-path Must Be Absolute

**Symptom:** "Failed to load library" error at pipeline startup.
**Cause:** DxPostprocess `library-file-path` is set to a relative path that cannot be resolved.
**Fix:** Use absolute path: `library-file-path=/usr/local/share/gstdxstream/lib/libpostprocess_xxx.so`

---

## 6. [DX_STREAM] Missing Queue Elements Cause Pipeline Deadlock

**Symptom:** Pipeline hangs immediately after PLAYING state, no frames processed.
**Cause:** GStreamer elements without queue elements between them run in the same thread, causing circular blocking when one element waits for the other.
**Fix:** Place `queue` (or `queue max-size-buffers=N`) between every pair of dx_stream elements.

---

## 7. [UNIVERSAL] Headless Mode: Check DISPLAY Before Video Sink

**Symptom:** Pipeline crashes with "Could not open display" or "No display available" error.
**Cause:** Pipeline uses fpsdisplaysink or ximagesink on a headless server (SSH, Docker) without X11 forwarding.
**Fix:** Check `DISPLAY` environment variable. Use `fakesink sync=false` when no display is available.

```bash
if [ -z "$DISPLAY" ]; then
    SINK="fakesink sync=false"
else
    SINK="videoconvert ! fpsdisplaysink sync=false"
fi
```

---

## 8. [UNIVERSAL] DX-RT Version < 3.0.0 Causes Element Registration Failure

**Symptom:** `gst-inspect-1.0 dxinfer` reports "No such element or plugin".
**Cause:** GStreamer plugin binary is incompatible with the installed DX-RT version.
**Fix:** Upgrade DX-RT to >= 3.0.0, then reinstall dx_stream: `./install.sh && ./build.sh`

---

## 9. [DX_STREAM] DxTracker Must Follow DxPostprocess

**Symptom:** DxTracker produces no track IDs, or pipeline crashes.
**Cause:** DxTracker operates on DXObjectMeta produced by DxPostprocess. Placing it before DxPostprocess (e.g., after DxInfer) means there are no objects to track.
**Fix:** Pipeline order: `... ! dxpostprocess ! queue ! dxtracker ! queue ! dxosd ! ...`

---

## 10. [DX_STREAM] Multi-Stream DxGather num-sources Mismatch

**Symptom:** Pipeline hangs or crashes during secondary mode operation.
**Cause:** DxGather `num-sources` does not match the actual number of connected sink pads from tee branches.
**Fix:** Set `num-sources` to match the exact number of secondary branches connected to the DxGather element. For 2 branches: `dxgather name=gather` (default num-sources=2).

---

## 11. [UNIVERSAL] dx-runtime Not Installed — GStreamer Plugin Registration Fails

**Symptom:** `gst-inspect-1.0 dxinfer` returns "No such element or plugin".
Pipeline fails to start with "Element dxinfer not found" error.

**Cause:** dx-runtime (dx_rt) is not installed or is at an incompatible version.
dx_stream GStreamer plugins depend on dx_rt shared libraries. Without dx_rt,
the plugin `.so` files fail to load even if they exist on disk.

**Fix:**
1. Run the sanity check: `bash ../../scripts/sanity_check.sh --dx_rt`
2. If FAIL, install dx-runtime:
   ```bash
   bash ../../install.sh --target=dx_rt,dx_rt_npu_linux_driver,dx_fw --skip-uninstall --venv-reuse
   ```
3. Then reinstall dx_stream plugins: `cd dx_stream && ./install.sh && ./build.sh`
4. Verify: `gst-inspect-1.0 dxinfer`

---

## 12. [UNIVERSAL] Wrong Sample Video for Task Type

**Symptom:** Pipeline runs but produces zero detections or irrelevant
results during demo or validation — even though the model is correct.

**Cause:** Using a mismatched sample video for the model's vision task.
For example, running a face detection pipeline on `boat.mp4` (no faces),
or using a static landscape video for pose estimation (no people).

**Fix:** Select sample videos that match the model's task:

| Task | Recommended Video |
|---|---|
| Object Detection | `dogs.mp4`, `blackbox-city-road.mp4` |
| Face Detection | Video with people/faces |
| Pose Estimation | Video with people |
| General / Fallback | `boat.mp4` |

Update `run_*.sh` scripts to use task-appropriate default videos.

---

## 13. [DX_STREAM] dx-agentic-dev Path Resolution Trap

**Symptom:** `run_<app>.sh` fails with "No such file or directory" for
`./setup.sh`, model file, or sample video. The error points to a completely
wrong parent directory (e.g., `dx-runtime/` instead of `dx_stream/dx_stream/`).

**Cause:** The `SRC_DIR` calculation was copied from production scripts
(`dx_stream/dx_stream/pipelines/category/task/run_*.sh`) which live 4 levels
below `dx_stream/dx_stream/`. Agent-generated scripts in `dx-agentic-dev/<session>/`
are only 2 levels below `dx_stream/` root — the same dirname-of-dirname-of-dirname
formula lands at the wrong directory.

**Fix:** In `dx-agentic-dev/` scripts, use explicit path resolution:

```bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DX_STREAM_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SRC_DIR="$DX_STREAM_ROOT/dx_stream"
MODEL_PATH="$SRC_DIR/samples/models/$MODEL_NAME"
# setup.sh is at DX_STREAM_ROOT, not SRC_DIR:
(cd "$DX_STREAM_ROOT" && ./setup.sh --model="$MODEL_NAME")
```

**Never** copy the production `SRC_DIR=$(dirname "$(dirname "$(dirname "$SCRIPT_DIR")")")` pattern into `dx-agentic-dev/` scripts.

---

## 14. [DX_STREAM] x264enc Without `tune=zerolatency` Causes Pipeline Deadlock

**Symptom:** Pipeline with `x264enc` (for file recording or tee-based dual
output) freezes immediately after entering PLAYING state. No frames are
processed, no output file is written. The pipeline appears to hang indefinitely.

**Cause:** By default, `x264enc` uses B-frames for better compression. B-frames
require buffering future frames before encoding the current frame. In a live
or real-time pipeline, this creates a deadlock: the encoder waits for frames
that haven't arrived yet, while upstream elements block waiting for the encoder
to consume buffers.

**Fix:** Always set `tune=zerolatency` on `x264enc`. This disables B-frames
and minimizes encoding latency, allowing the pipeline to process frames
without deadlock:

```bash
# Correct — pipeline flows smoothly
tee name=t \
  t. ! queue ! videoconvert ! fpsdisplaysink sync=false \
  t. ! queue ! videoconvert ! x264enc bitrate=4000 speed-preset=ultrafast tune=zerolatency ! h264parse ! mp4mux ! filesink location=output.mp4

# WRONG — pipeline deadlocks (no tune=zerolatency)
... ! x264enc bitrate=4000 speed-preset=ultrafast ! ...
```

**Note:** `tee` itself is safe for dual output (display + file). The deadlock
is caused by `x264enc` B-frame buffering, not by `tee` error propagation.
However, be aware that closing the display window will still send an error
to the shared bus — handle this with `bus.connect('message', ...)` if needed.

---

## 15. [DX_STREAM] pydxs Import Failure — venv Not Activated

**Symptom:** `pipeline.py` crashes immediately with `ModuleNotFoundError: No
module named 'pydxs'`. The script never reaches GStreamer initialization.

**Cause:** The `pydxs` Python module (dx_stream's Python bindings for
GStreamer elements and metadata) is installed inside the `venv-dx_stream/`
virtual environment, which is created by `./install.sh && ./build.sh`.
Running `pipeline.py` with the system Python or a different venv will fail
because `pydxs` is not on `sys.path`.

**Fix:**
1. Activate the dx_stream virtual environment before running:
   ```bash
   source venv-dx_stream/bin/activate
   python3 pipeline.py --input video.mp4 --model model.dxnn --postprocess-lib lib.so
   ```
2. If `venv-dx_stream/` does not exist, create it:
   ```bash
   cd <dx_stream_root>
   ./install.sh && ./build.sh
   ```
3. In generated `pipeline.py`, add a guard at the top of the file:
   ```python
   try:
       import pydxs
   except ImportError:
       print("ERROR: pydxs not found. Activate venv: source ../../venv-dx_stream/bin/activate")
       sys.exit(1)
   ```
4. In generated `README.md`, **always** document the venv activation step
   as a prerequisite — this is the #1 reason users cannot run agent-generated
   pipelines.

**Agent responsibility:** The README.md template in `dx-build-pipeline-app.md`
includes the venv activation instructions. Ensure every generated README.md
follows this template.

---

## 16. [UNIVERSAL] setup.sh Missing Venv Detection / run.sh Using Placeholder Paths

**Symptom**: Running `./setup.sh` directly fails with import errors or package conflicts.
Running `./run.sh` shows `Model not found` or requires users to guess paths.

**Cause**: Agent generates setup.sh without venv detection/activation logic, and run.sh
with `/path/to/<model>.dxnn` placeholders instead of real relative paths.

**Fix**:
1. **setup.sh** MUST detect `venv-dx_stream/` or `venv-dx-runtime/` by searching upward
   from the session directory, activate it, and fall back to creating a local `.venv/`:
   ```bash
   _search="$SCRIPT_DIR"
   for _i in 1 2 3 4 5; do
       _search="$(dirname "$_search")"
       [ -d "$_search/venv-dx_stream" ] && source "$_search/venv-dx_stream/bin/activate" && break
       [ -d "$_search/venv-dx-runtime" ] && source "$_search/venv-dx-runtime/bin/activate" && break
   done
   ```
2. **run.sh** and **README.md** MUST use real paths:
   - Model: `$(realpath ../../samples/models/<model>.dxnn)` (GStreamer needs absolute)
   - Video: `../../samples/videos/<video>.mp4` or `../../assets/videos/dogs.mp4`
   - Never use `/path/to/` placeholders

**Prevention**: Use the setup.sh/run.sh templates in `dx-build-pipeline-app.md`.
