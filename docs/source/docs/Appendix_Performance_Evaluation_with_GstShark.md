# Appendix: Performance Evaluation with GstShark

GstShark is a powerful performance analysis tool for GStreamer pipelines that provides comprehensive profiling capabilities including CPU usage, processing time, frame rate, and bitrate analysis. This appendix describes how to install and use GstShark for evaluating DX-STREAM pipeline performance.

## Installation

DX-Stream provides an automated installation script to simplify the setup of GstShark and its dependencies.
Execute the following command from the DX-Stream project root directory:

```bash
./install_gstshark.sh
```

This automated script performs the following actions required for a system-wide GstShark installation:

- **Dependency Installation**: Installs necessary system packages (e.g., graphviz, libgraphviz-dev).
- **Source Acquisition**: Clones the GstShark repository from GitHub.
- **Build and Configuration**: Compiles and installs GstShark with the appropriate GStreamer configurations.
- **PATH Update**: Adds GstShark visualization and analysis tools to your system's PATH environment variable.


!!! note "NOTE" 

    The installation requires sudo privileges for system-wide installation of GstShark libraries and tools.

## Usage and Analysis Methodology

This section details how to use GstShark to analyze the performance of GStreamer pipelines, ranging from standard video processing to complex DX-Stream AI pipelines.

### Basic Usage

GstShark uses environment variables to control which performance data is collected and where the results are stored.

#### Environment Variables

- `GST_TRACER`: Specifies which GstShark tracers to use (semicolon-separated)
- `GST_SHARK_LOCATION`: Directory where GstShark results will be saved
- `GST_DEBUG`: Controls debug output level for tracer information

#### Available Tracers

GstShark provides the following tracers for performance analysis:

| Tracer | Description | Key Metric Monitored |
|--------|-------------|---------------------|
| `cpuusage` | Measures CPU usage consumed per element. | Resource Utilization |
| `proctime` | Measures the processing time (latency) spent inside each element. | Element Latency |
| `framerate` | Analyzes the actual frame rate achieved by the pipeline. | Throughput |
| `bitrate` | Monitors the data throughput (bitrate) of the stream. | Data Flow Rate |
| `interlatency` | Measures latency between connected elements. | Inter-Element Delays |
| `queuelevel` | Monitors Queue buffer levels to identify bottlenecks and backpressure. | Bottleneck Detection |
| `buffer` | Provides detailed analysis of buffer flow and timestamping. | Data Consistency |

### Sample Pipeline Analysis

#### Basic Performance Evaluation

This command demonstrates how to use GstShark to analyze a standard H.264 video processing pipeline, displaying the output directly to the console:

```bash
GST_DEBUG=GST_TRACER:7 GST_TRACERS="cpuusage;proctime;framerate;bitrate" \
    gst-launch-1.0 filesrc location=./dx_stream/samples/videos/codec_test_clip_h264_16Mbps.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! videoconvert ! fakesink
```

#### Advanced Analysis with Result Storage

For persistent storage and graphical generation, specify the **GST_SHARK_LOCATION** environment variable.

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

### DX-STREAM Pipeline Analysis

To analyze the performance contribution of the NPU-accelerated elements (dxpreprocess, dxinfer, dxpostprocess), include them in the pipeline and use relevant tracers like queuelevel to detect bottlenecks around the NPU element.

```bash
GST_DEBUG="GST_TRACER:7" GST_TRACERS="cpuusage;proctime;framerate;queuelevel" \
    gst-launch-1.0 filesrc location=./dx_stream/samples/videos/codec_test_clip_h264_16Mbps.mp4 ! \
    qtdemux ! h264parse ! avdec_h264 ! \
    dxpreprocess config-file-path=./dx_stream/configs/Object_Detection/YoloV7/preprocess_config.json ! \
    dxinfer config-file-path=./dx_stream/configs/Object_Detection/YoloV7/inference_config.json ! \
    dxpostprocess config-file-path=./dx_stream/configs/Object_Detection/YoloV7/postprocess_config.json ! \
    dxosd ! fakesink
```


### Result Analysis

#### Console Output Analysis

GstShark provides real-time performance information through console output. Key metrics to monitor:

- **Processing Time**: Time spent in each element
- **CPU Usage**: CPU utilization per element
- **Frame Rate**: Actual vs expected frame rates
- **Queue Levels**: Buffer queue status for bottleneck detection

#### Graphical Analysis

If GstShark graphics tools (such as gstshark-plot) were installed with the necessary dependencies, you can generate visual reports for clearer performance trending and bottleneck visualization.

```bash
# Generate performance graphs (if graphics tools are installed)
gstshark-plot /tmp/gst-shark-results/ -s pdf
```

## Performance Optimization Tips

This section provides strategies for interpreting GstShark results and applying common optimization techniques to enhance DX-Stream pipeline performance.

### Identifying Bottlenecks

GstShark tracers help pinpoint the exact location and nature of performance bottlenecks within the GStreamer pipeline.

| Tracer/Metric | Bottleneck Indication | Description |
|---------------|----------------------|-------------|
| High Processing Time | Slow element processing (e.g., complex pre-processing or slow NPU execution). | Look for elements with consistently high `proctime` values. |
| CPU Hotspots | Host CPU is overloaded by a specific element. | Monitor `cpuusage` to identify CPU-intensive elements (e.g., software video decoding). |
| Queue Overflow | Backpressure issue where a producer is faster than the consumer. | Check `queuelevel` for buffer overflow issues, indicating a slow consumer. |
| Frame Drops | Pipeline cannot maintain the required throughput. | Compare expected vs. actual `framerate` to quantify the performance deficit. |

### Common Optimization Strategies

Applying these strategies can alleviate identified bottlenecks:

- Element Configuration: Adjust element-specific parameters, such as quantization settings in the NPU elements or interpolation methods in video conversion elements.
- Buffer Management: Optimize queue sizes and buffer pools to balance latency and throughput. Larger queues reduce frame drops but increase latency.
- Threading: Utilize multi-threaded elements, where available, to leverage multiple CPU cores for parallel processing.
- Hardware Acceleration: Crucially, ensure GPU acceleration (Mali-G610 MP4) is enabled for supported non-NPU elements (e.g., video conversion) to offload the host CPU.


## Troubleshooting

This section addresses common issues encountered when using GstShark for performance analysis.

### Common Issues

| Issue | Cause | Resolution |
|-------|-------|------------|
| No Tracer Output | GstShark is running but not logging data correctly. | Ensure the `GST_SHARK_LOCATION` environment variable is set and the specified directory exists. |
| Missing Graphics Tools | Unable to run analysis commands like `gstshark-plot`. | Re-run the installation script (`./install_gstshark.sh`) to ensure all graphical components and dependencies are installed. |
| Permission Errors | Cannot save results to the specified output directory. | Check write permissions for the result directory (e.g., use `sudo` or change permissions with `chmod`). |

### GStreamer Registry Issues

If the GstShark tracers (e.g., sharktime, sharklog) are not recognized by GStreamer, the plugin registry cache is likely outdated. Follow these steps to force GStreamer to rescan and register the tracers.

- Clear GStreamer Registry Cache

    This step removes the old, potentially corrupt, or incomplete cache files.

    ```
    rm -rf ~/.cache/gstreamer-1.0/registry.*.bin
    ```

- Regenerate Registry

    Running the inspection tool forces GStreamer to scan all plugin paths and create a new, fresh registry file that includes GstShark.

    ```
    gst-inspect-1.0 > /dev/null 2>&1
    ```

- Verify GstShark Installation

    Check if the GstShark elements are now successfully recognized by the system. If successful, you will see a list of tracers printed in your terminal.

    ```
    gst-inspect-1.0 | grep shark
    ```



!!! note "NOTE" 

    **Best Practices for GstShark Analysis**

    Baseline Measurement: Always establish baseline performance before optimization
    Controlled Environment: Run tests in consistent system conditions
    Multiple Iterations: Average results across multiple test runs
    Resource Monitoring: Monitor system resources (CPU, memory, GPU) during testing
    Documentation: Document test configurations and results for reproducibility
