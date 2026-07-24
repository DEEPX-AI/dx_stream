# Pipeline Optimization — dx_stream

Performance tuning patterns for dx_stream GStreamer pipelines.

---

## Queue Sizing

Queue elements control buffering between pipeline stages. Sizing affects
latency vs. throughput trade-off.

| Use Case | Recommended `max-size-buffers` | Rationale |
|---|---|---|
| Single network (low latency) | 1 | Minimize buffering, process latest frame |
| Multi-stream (throughput) | 10 | Allow buffering to smooth frame scheduling |
| RTSP (variable rate) | 2 | Small buffer to absorb rate variation |
| Secondary mode | 1 | Prevent stale ROI accumulation |

```bash
# Low-latency single stream
queue max-size-buffers=1

# High-throughput multi-stream
queue max-size-buffers=10 max-size-bytes=0 max-size-time=0
```

---

## DxRate for Frame Rate Control

DxRate drops excess frames to match a target rate. Critical for RTSP and
high-FPS camera sources.

```bash
# Limit to 15 FPS for power efficiency
dxrate max-rate=15

# Match NPU processing speed (~25 FPS typical)
dxrate max-rate=25
```

**Tuning:** Set `max-rate` to match or slightly exceed the NPU's processing
throughput. Too low wastes NPU capacity; too high overwhelms the pipeline.

---

## Multi-Stream Balancing

For multi-stream pipelines, balance the load across streams:

1. **Compositor grid**: Use DxScale to resize each stream to grid cell dimensions
   before compositing. Avoid scaling in the compositor itself.

2. **Input/Output Selector**: DxInputSelector round-robins through N streams.
   Effective throughput per stream = total_fps / N.
   For 4 streams at 30 FPS total: each stream gets ~7.5 FPS.

3. **Independent pipelines**: Each stream has its own inference chain.
   Better per-stream FPS but higher NPU contention.

```bash
# Selector approach (shared inference, N streams)
# Total FPS ÷ N = per-stream FPS
dxinputselector name=in ! dxpreprocess ! dxinfer ! dxpostprocess ! dxosd ! dxoutputselector name=out

# Independent approach (per-stream inference)
# Each stream gets full inference FPS, but NPU contention
stream1: source ! dxpreprocess ! dxinfer ! dxpostprocess ! dxosd ! sink
stream2: source ! dxpreprocess ! dxinfer ! dxpostprocess ! dxosd ! sink
```

---

## Tracker Overhead

DxTracker adds per-frame computation for multi-object tracking:

- **OC-SORT** overhead: ~1-3ms per frame depending on object count
- Tracker runs on CPU, not NPU
- High object counts (>50) may cause visible FPS impact

**Optimization:**
- Set tracker `max_age` in config to limit track retention
- Use `min-object-width`/`min-object-height` on DxPreprocess to reduce small-object detections
- Increase tracker `min_hits` to filter unstable tracks

---

## DxConvert Placement

DxConvert handles color space conversion. Placement matters for performance:

```bash
# GOOD: Convert once before display
... ! dxosd ! dxconvert ! video/x-raw,format=I420 ! sink

# BAD: Convert in middle of inference chain (unnecessary overhead)
... ! dxpreprocess ! dxconvert ! dxinfer ! ...
```

DxPreprocess handles color conversion internally for NPU input.
Only use DxConvert for display format requirements.

---

## GST_TRACERS Profiling

Use GStreamer tracers to identify bottlenecks:

```bash
# Frame rate per element
GST_TRACERS="framerate" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Per-element processing time (requires GstShark)
GST_TRACERS="proctime" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Inter-element latency
GST_TRACERS="interlatency" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...

# Combined analysis
GST_TRACERS="framerate;interlatency;proctime" GST_DEBUG=GST_TRACER:7 gst-launch-1.0 ...
```

Interpret results:
- **framerate** shows FPS at each element's src pad
- **proctime** shows time spent in each element's chain/event function
- **interlatency** shows time between elements

Bottleneck is the element with lowest FPS or highest proctime.

---

## Batch Inference

DxInfer supports batch inference for throughput optimization:

```bash
dxinfer ... batch-size=4
```

Trade-offs:
- **Batch=1**: Lowest latency, one frame per NPU call
- **Batch=4**: Higher throughput, but 4-frame latency
- **Batch=8**: Maximum throughput, 8-frame latency

Only use batch > 1 when throughput matters more than latency
(e.g., offline video processing, multi-stream with selector).

---

## Zero-Copy Buffer Optimization

dx_stream elements support zero-copy buffer passing where possible:
- DxPreprocess → DxInfer: Zero-copy when formats match
- Between queue elements: Always zero-copy (GstBuffer refcounting)
- DxOsd rendering: In-place modification when possible

To maximize zero-copy:
- Avoid unnecessary videoconvert elements in the inference chain
- Use DxConvert only at the final display stage
- Keep consistent color format through the pipeline

---

## Pipeline Latency Budget

Typical latency breakdown for a single-network pipeline:

| Stage | Typical Latency | Notes |
|---|---|---|
| Source decode | 5-15ms | Depends on codec (H.264 < H.265) |
| DxPreprocess | 1-3ms | Resize + normalize |
| Queue | <1ms | Buffer passing |
| DxInfer | 8-25ms | Model-dependent, NPU execution |
| DxPostprocess | 1-5ms | NMS + decode in CPU .so |
| DxTracker | 1-3ms | OC-SORT on CPU |
| DxOsd | 1-2ms | Drawing overlays |
| Display | 2-5ms | Video sink rendering |
| **Total** | **~20-60ms** | **16-50 FPS** |

Primary bottleneck is usually DxInfer (NPU inference time).
