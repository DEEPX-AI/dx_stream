# Debugging Guide

This chapter provides comprehensive guidance on debugging DX-STREAM applications using GStreamer's built-in debug logging system. Understanding how to enable and interpret debug logs is essential for diagnosing issues, monitoring performance, and optimizing your AI inference pipelines.

---

## GST_DEBUG Environment Variable

GStreamer provides a powerful debug logging system controlled by the `GST_DEBUG` environment variable. DX-STREAM elements fully integrate with this system, offering detailed insights into pipeline execution, buffer flow, and element-specific operations.

### Debug Level Overview

The `GST_DEBUG` variable accepts numeric levels from 0 to 7:

| **Level** | **Name**    | **Description**                                                                 |
|-----------|-------------|---------------------------------------------------------------------------------|
| 0         | NONE        | No debug output                                                                 |
| 1         | ERROR       | Fatal errors that prevent normal operation                                       |
| 2         | WARNING     | Non-fatal issues that may affect behavior                                        |
| 3         | FIXME       | Fixme messages (rarely used in DX-STREAM)                                        |
| 4         | INFO        | Informational messages about state changes and major operations                  |
| 5         | DEBUG       | Detailed operational information useful for troubleshooting                      |
| 6         | LOG         | Per-buffer trace — every frame/buffer processed logs at this level               |
| 7         | TRACE       | Internal algorithm details (e.g., inference queue state, tensor contents)         |

### Basic Usage

**Enable debug logging for all DX-STREAM elements:**

```bash
$ GST_DEBUG=dx*:4 ./your_application
```

**Enable debug logging for specific elements:**

```bash
$ GST_DEBUG=dxinfer:4,dxtracker:4 ./your_application
```

**Combine global level with element-specific levels:**

```bash
$ GST_DEBUG=2,dxinfer:4,dxpreprocess:4 ./your_application
```

This sets the global level to WARNING (2) while enabling DEBUG (4) for `dxinfer` and `dxpreprocess`.

### Advanced Usage

**Redirect debug output to a file:**

```bash
$ GST_DEBUG=dx*:4 GST_DEBUG_FILE=/tmp/debug.log ./your_application
```

**Disable colored output (useful for log files):**

```bash
$ GST_DEBUG_NO_COLOR=1 GST_DEBUG=dx*:4 ./your_application
```

**Filter output with grep:**

```bash
$ GST_DEBUG=dxinfer:4 ./your_application 2>&1 | grep "Inference completed"
```

---

## Element-Specific Debugging

Each DX-STREAM element has its own debug category that logs element-specific operations and state information.

### DxPreprocess

**Debug Category:** `dxpreprocess`

**Key Log Messages:**

- `Transitioning to READY state` - Element state changes
- `Loaded configuration from: <path>` - Config file loading
- `Processing buffer: pts=<timestamp>` - Buffer flow tracking
- `Operating in secondary mode` - Mode confirmation
- `Dropping buffer due to QoS` - QoS-triggered frame drops

**Common Use Cases:**

```bash
# Debug preprocessing issues
$ GST_DEBUG=dxpreprocess:4 ./your_app

# Monitor QoS behavior
$ GST_DEBUG=dxpreprocess:4 ./your_app 2>&1 | grep "QoS"

# Trace buffer flow
$ GST_DEBUG=dxpreprocess:4 ./your_app 2>&1 | grep "Processing buffer"
```

### DxInfer

**Debug Category:** `dxinfer`

**Key Log Messages:**

- `Model loaded: <path>` - Model initialization
- `Inference completed in X.XX ms` - Inference latency measurement
- `Queue status: X/Y buffers` - Input queue monitoring
- `Dropping buffer due to QoS` - Performance optimization
- `Received throttle QoS event` - Frame rate control

**Common Use Cases:**

```bash
# Monitor inference performance
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "completed in"

# Debug model loading
$ GST_DEBUG=dxinfer:3 ./your_app

# Analyze queue behavior
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "Queue status"
```

### DxPostprocess

**Debug Category:** `dxpostprocess`

**Key Log Messages:**

- `Loaded postprocess library: <path>` - Custom library loading
- `Processing buffer: pts=<timestamp>` - Buffer processing
- `Operating in secondary mode` - Mode confirmation
- `Added X objects to frame metadata` - Detection results

**Common Use Cases:**

```bash
# Debug custom postprocess library
$ GST_DEBUG=dxpostprocess:4 ./your_app

# Monitor detection results
$ GST_DEBUG=dxpostprocess:4 ./your_app 2>&1 | grep "objects to frame"
```

### DxTracker

**Debug Category:** `dxtracker`

**Key Log Messages:**

- `Tracker initialized: algorithm=<name>` - Tracker setup
- `Processing buffer: pts=<timestamp>` - Frame tracking
- `Tracking X objects` - Active track count
- `Updated track IDs` - ID assignment

**Common Use Cases:**

```bash
# Debug tracking behavior
$ GST_DEBUG=dxtracker:4 ./your_app

# Monitor track updates
$ GST_DEBUG=dxtracker:4 ./your_app 2>&1 | grep "Tracking"
```

### DxOsd

**Debug Category:** `dxosd`

**Key Log Messages:**

- `Initializing OSD element (passthrough transform)` - Element initialization
- `Processing buffer: pts=<timestamp>` - Frame rendering
- `Drawing on NV12: N objects` - NV12 format visualization status
- `Drawing on I420: N objects` - I420 format visualization status

**Common Use Cases:**

```bash
# Debug rendering issues
$ GST_DEBUG=dxosd:4 ./your_app

# Monitor draw operations
$ GST_DEBUG=dxosd:4 ./your_app 2>&1 | grep "Drawing"
```

### DxMsgBroker

**Debug Category:** `dxmsgbroker`

**Key Log Messages:**

- `Connected to broker: <host>:<port>` - Connection status
- `Publishing message to topic: <topic>` - Message transmission
- `Connection lost, reconnecting` - Connection recovery
- `Message sent successfully` - Delivery confirmation

**Common Use Cases:**

```bash
# Debug broker connection
$ GST_DEBUG=dxmsgbroker:4 ./your_app

# Monitor message publishing
$ GST_DEBUG=dxmsgbroker:4 ./your_app 2>&1 | grep "Publishing"

# Track connection issues
$ GST_DEBUG=dxmsgbroker:3 ./your_app 2>&1 | grep -E "Connected|lost"
```

### DxMsgConv

**Debug Category:** `dxmsgconv`

**Key Log Messages:**

- `Processing buffer: pts=<timestamp> seq=<id>` - Message conversion
- `Loaded message conversion library: <path>` - Custom library loading
- `Generated message payload: size=<bytes>` - Payload creation

**Common Use Cases:**

```bash
# Debug message conversion
$ GST_DEBUG=dxmsgconv:4 ./your_app

# Monitor sequence numbers
$ GST_DEBUG=dxmsgconv:4 ./your_app 2>&1 | grep "seq="
```

### DxRate

**Debug Category:** `dxrate`

**Key Log Messages:**

- `Processing buffer: pts=<timestamp>` - Rate control operation
- `Sending throttle QoS event: delay=<ms>` - Throttling notification
- `Frame rate control active` - Rate limiting status

**Common Use Cases:**

```bash
# Debug frame rate control
$ GST_DEBUG=dxrate:4 ./your_app

# Monitor throttling
$ GST_DEBUG=dxrate:4 ./your_app 2>&1 | grep "throttle"
```

### DxGather, DxInputSelector, DxOutputSelector

**Debug Categories:** `dxgather`, `dxinputselector`, `dxoutputselector`

**Key Log Messages:**

- `Processing buffer: pts=<timestamp>` - Stream routing
- `Selecting input stream: <id>` - Input selection (DxInputSelector)
- `Routing to output pad: <id>` - Output routing (DxOutputSelector)
- `Merging X streams` - Stream merging (DxGather)

**Common Use Cases:**

```bash
# Debug multi-stream routing
$ GST_DEBUG=dxgather:4,dxinputselector:4,dxoutputselector:4 ./your_app
```

---

### DxScale, DxConvert

**Debug Categories:** `dxscale`, `dxconvert`

**Key Log Messages:**

- `Setting target resolution: <width>x<height>` - Scale target configuration (DxScale)
- `Backend selected: <name>` - Transform backend selection
- `Passthrough mode enabled` - No-op when no transform is needed
- `Falling back to libyuv backend` - RGA does not support the source format

**Common Use Cases:**

```bash
# Debug video scaling and format conversion
$ GST_DEBUG=dxscale:4,dxconvert:4 ./your_app
```

---

## Metadata Debugging

DX-STREAM uses custom GStreamer metadata structures to pass information between elements. All metadata logging is unified under a single debug category.

### Metadata Category

**Debug Category:** `dxmeta`

Covers: DXFrameMeta, DXObjectMeta, DXMsgMeta, DXUserMeta

**Key Log Messages:**

- `Creating DXFrameMeta` - Frame metadata creation
- `Adding DXObjectMeta to DXFrameMeta` - Object attachment
- `Acquiring DXUserMeta from pool` - Custom metadata allocation
- `Releasing DXUserMeta` - Memory cleanup

**Usage:**

```bash
# Debug all metadata operations
$ GST_DEBUG=dxmeta:5 ./your_app

# Monitor object metadata
$ GST_DEBUG=dxmeta:5 ./your_app 2>&1 | grep "DXObjectMeta"
```

---

## Subsystem Debugging

DX-STREAM internal subsystems (shared across multiple elements) use dedicated debug categories for focused diagnostics.

### Transform Kernel

**Debug Category:** `transform_kernel`

Covers: libyuv, RGA, V3 DSP kernels, TransformKernelPool, PreprocessorFactory

**Key Log Messages:**

- `TransformKernelPool: using backend '<name>' (single/pool mode)` - Backend selection
- `RgaTransformKernel: imcheck rejected` - RGA validation failure with fallback
- `TransformKernelPool: '<name>' failed, falling back to libyuv` - Automatic fallback
- `PreprocessorFactory: kernel pool created` - Pool initialization

**Usage:**

```bash
# Debug video transform operations (resize, color convert, crop)
$ GST_DEBUG=transform_kernel:5 ./your_app

# Trace per-frame transform execution
$ GST_DEBUG=transform_kernel:6 ./your_app
```

### Inference Backend

**Debug Category:** `inference_backend`

Covers: DXRT runtime, InferBackendFactory

**Key Log Messages:**

- `DxrtBackend: initialized (DXRT <version>, <N> devices)` - DXRT init
- `Put req_id=<id> (pending=<N>)` - Inference request submitted (TRACE level)
- `Got req_id=<id> (pending=<N>)` - Inference result received (TRACE level)
- `Get SKIP: seq=<N>, device[<idx>]` - Inference timeout

**Usage:**

```bash
# Debug inference backend initialization and errors
$ GST_DEBUG=inference_backend:5 ./your_app

# Trace per-request inference flow (very verbose)
$ GST_DEBUG=inference_backend:7 ./your_app
```

---

## Common Debugging Scenarios

### Performance Analysis

**Monitor inference latency:**

```bash
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "completed in" | awk '{print $NF}' | sed 's/ms//'
```

**Identify bottlenecks:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep -E "completed in|took|duration"
```

### Pipeline Flow Debugging

**Trace buffer flow through entire pipeline:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep "Processing buffer"
```

**Monitor state transitions:**

```bash
$ GST_DEBUG=dx*:3 ./your_app 2>&1 | grep -E "Transitioning|state"
```

### QoS and Frame Drop Analysis

**Track dropped frames:**

```bash
$ GST_DEBUG=dxpreprocess:4,dxinfer:4 ./your_app 2>&1 | grep "Dropping"
```

**Monitor QoS events:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep "QoS"
```

### Configuration Issues

**Verify element initialization:**

```bash
$ GST_DEBUG=dx*:3 ./your_app 2>&1 | grep -E "Loaded|initialized|configuration"
```

**Check property values:**

```bash
$ GST_DEBUG=dx*:3 ./your_app 2>&1 | grep -E "property|config"
```

### Memory and Resource Tracking

**Monitor metadata lifecycle:**

```bash
$ GST_DEBUG=dxmeta:5 ./your_app 2>&1 | grep -E "Creating|Freeing|Releasing"
```

**Track buffer allocation:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep -E "Acquiring|Releasing|pool"
```

---

## Best Practices

### Development Phase

1. **Start with INFO level** (`:3`) to monitor state changes and initialization
2. **Enable DEBUG level** (`:4`) for specific elements showing issues
3. **Use targeted filtering** with grep to focus on relevant messages
4. **Capture logs to file** for detailed analysis

```bash
$ GST_DEBUG=2,dxinfer:4,dxtracker:4 GST_DEBUG_FILE=/tmp/debug.log ./your_app
```

### Production Deployment

(1) **Set global level to WARNING** (`:2`) or ERROR (`:1`)
(2) **Disable unnecessary element logging** to reduce overhead
(3) **Use log rotation** if file logging is enabled
(4) **Monitor ERROR and WARNING** messages for operational issues

```bash
$ GST_DEBUG=1 ./your_app 2>&1 | tee -a /var/log/dxstream.log
```

### Performance Testing

(1) **Minimize debug output** during benchmarking
(2) **Use selective logging** for targeted profiling
(3) **Capture timing information** for latency analysis

```bash
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "completed in" > inference_times.txt
```

---

## Tips and Tricks

### Combining Multiple Tools

**Use timestamps for timing analysis:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | ts '[%Y-%m-%d %H:%M:%.S]'
```

**Pipe to text editor for analysis:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | tee debug.log | less
```

### Quick Diagnostics

**Check if elements are loading:**

```bash
$ gst-inspect-1.0 dxinfer
```

**Verify debug categories:**

```bash
$ GST_DEBUG=dx*:3,dxmeta:3,transform_kernel:3,inference_backend:3 gst-launch-1.0 --gst-debug-help | grep -E "dx|transform_kernel|inference_backend|dxmeta"
```

### Common Patterns

**Find all ERROR messages:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep "ERROR"
```

**Count specific events:**

```bash
$ GST_DEBUG=dxinfer:4 ./your_app 2>&1 | grep "Inference completed" | wc -l
```

**Extract PTS timestamps:**

```bash
$ GST_DEBUG=dx*:4 ./your_app 2>&1 | grep -oP 'pts=\K[0-9:]+'
```

---

## Additional Resources

- **GStreamer Debug Documentation:** https://gstreamer.freedesktop.org/documentation/gstreamer/running.html
- **DX-STREAM Troubleshooting:** See Chapter 06 - Troubleshooting and FAQ
- **Element-Specific Documentation:** See Chapter 03 - DX-STREAM Elements

!!! note "Pro Tip"

    When reporting issues, always include relevant debug logs with `GST_DEBUG=dx*:4` to help diagnose problems quickly.

---
