# Appendix: Performance Evaluation with GstShark

## Overview

GstShark is a powerful performance analysis tool for GStreamer pipelines that provides comprehensive profiling capabilities including CPU usage, processing time, frame rate, and bitrate analysis. This appendix describes how to install and use GstShark for evaluating DX-STREAM pipeline performance.

## Installation

DX-STREAM provides an automated installation script for GstShark. Simply run the following command from the project root directory:

```bash
./install_gstshark.sh
```

This script will:
- Install required dependencies (graphviz, libgraphviz-dev, etc.)
- Clone the GstShark repository from GitHub
- Build and install GstShark with proper configuration
- Add GstShark graphics tools to your PATH

> **Note**: The installation requires sudo privileges for system-wide installation of GstShark libraries.

## Basic Usage

### Environment Variables

GstShark uses several environment variables to control its behavior:

- `GST_TRACER`: Specifies which GstShark tracers to use (semicolon-separated)
- `GST_SHARK_LOCATION`: Directory where GstShark results will be saved
- `GST_DEBUG`: Controls debug output level for tracer information

### Available Tracers

GstShark provides the following tracers for performance analysis:

| Tracer | Description |
|--------|-------------|
| `cpuusage` | CPU usage per element |
| `proctime` | Processing time per element |
| `framerate` | Frame rate analysis |
| `bitrate` | Bitrate monitoring |
| `interlatency` | Inter-element latency |
| `queuelevel` | Queue buffer levels |
| `buffer` | Buffer flow analysis |

## Sample Pipeline Analysis

### Basic Performance Evaluation

Here's a sample command to analyze a standard H.264 video processing pipeline:

```bash
GST_DEBUG=GST_TRACER:7 GST_TRACERS="cpuusage;proctime;framerate;bitrate" \
    gst-launch-1.0 filesrc location=./dx_stream/samples/videos/codec_test_clip_h264_16Mbps.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! videoconvert ! fakesink
```

### Advanced Analysis with Result Storage

For more detailed analysis with result storage:

```bash
# Create result directory
mkdir -p /tmp/gst-shark-results

# Run analysis with result storage
GST_DEBUG="GST_TRACER:7" \
GST_TRACERS="cpuusage;proctime;framerate;bitrate" \
GST_SHARK_LOCATION="/tmp/gst-shark-results" \
    gst-launch-1.0 filesrc location=./dx_stream/samples/videos/codec_test_clip_h264_16Mbps.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! videoconvert ! fakesink

# View results
ls -la /tmp/gst-shark-results/
```

## DX-STREAM Pipeline Analysis

Analyze a basic DX-STREAM pipeline:

```bash
GST_DEBUG="GST_TRACER:7" GST_TRACERS="cpuusage;proctime;framerate;queuelevel" \
    gst-launch-1.0 filesrc location=./dx_stream/samples/videos/codec_test_clip_h264_16Mbps.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! \
    dxpreprocess config-file-path=./dx_stream/configs/Object_Detection/YoloV7/preprocess_config.json ! \
    dxinfer config-file-path=./dx_stream/configs/Object_Detection/YoloV7/inference_config.json ! \
    dxpostprocess config-file-path=./dx_stream/configs/Object_Detection/YoloV7/postprocess_config.json ! \
    dxosd ! fakesink
```


## Result Analysis

### Console Output Analysis

GstShark provides real-time performance information through console output. Key metrics to monitor:

- **Processing Time**: Time spent in each element
- **CPU Usage**: CPU utilization per element
- **Frame Rate**: Actual vs expected frame rates
- **Queue Levels**: Buffer queue status for bottleneck detection

### Graphical Analysis

If GstShark graphics tools are available, generate visual reports:

```bash
# Generate performance graphs (if graphics tools are installed)
gstshark-plot /tmp/gst-shark-results/ -s pdf
```

## Performance Optimization Tips

### Identifying Bottlenecks

1. **High Processing Time**: Look for elements with consistently high `proctime` values
2. **CPU Hotspots**: Monitor `cpuusage` to identify CPU-intensive elements
3. **Queue Overflow**: Check `queuelevel` for buffer overflow issues
4. **Frame Drops**: Compare expected vs actual `framerate`

### Common Optimization Strategies

- **Element Configuration**: Adjust element-specific parameters
- **Buffer Management**: Optimize queue sizes and buffer pools
- **Threading**: Utilize multi-threaded elements where appropriate
- **Hardware Acceleration**: Enable GPU acceleration for supported elements

## Troubleshooting

### Common Issues

1. **No Tracer Output**: Ensure `GST_SHARK_LOCATION` is set and directory exists
2. **Missing Graphics Tools**: Run `./install_gstshark.sh` to install all components
3. **Permission Errors**: Check write permissions for result directory

### GStreamer Registry Issues

If GstShark tracers are not recognized:

```bash
# Clear GStreamer registry cache
rm -rf ~/.cache/gstreamer-1.0/registry.*.bin

# Regenerate registry
gst-inspect-1.0 > /dev/null 2>&1

# Verify GstShark installation
gst-inspect-1.0 | grep shark
```

## Best Practices

1. **Baseline Measurement**: Always establish baseline performance before optimization
2. **Controlled Environment**: Run tests in consistent system conditions
3. **Multiple Iterations**: Average results across multiple test runs
4. **Resource Monitoring**: Monitor system resources (CPU, memory, GPU) during testing
5. **Documentation**: Document test configurations and results for reproducibility

## References

- [GstShark GitHub Repository](https://github.com/RidgeRun/gst-shark)
- [GStreamer Tracer Documentation](https://gstreamer.freedesktop.org/documentation/gstreamer/gsttracer.html)
- DX-STREAM Element Reference (Section 3)
- DX-STREAM Pipeline Examples (Section 5)

---

*This appendix provides comprehensive guidance for performance evaluation using GstShark. For additional support or advanced configuration, please refer to the main DX-STREAM documentation or contact technical support.*