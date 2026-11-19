There are two possible approaches to building a pipeline structure for multi-stream inference.  

One approach is to configure an independent inference sub-pipeline for each stream, as shown in the figure below.  

In this case, each sub-pipeline performs inference asynchronously, maximizing the utilization of hardware resources.  

![](./../../resources/05_02_multi_stream_subpipeline.png)

The pipeline in the figure is defined in 
`dx_stream/pipelines/multi_stream/run_multi_stream.sh` and can be used as a reference for execution.  


The other approach uses **DxInputSelector** and **DxOutputSelector**.  
**DxInputSelector** is an `N:1 ` element that receives buffers from multiple input streams and forwards the one with the smallest PTS downstream first.  
The selected buffer passes through a single inference pipeline for processing, and **DxOutputSelector** then redistributes the results back to their corresponding streams.  

![](./../../resources/05_02_multi_stream_selector.png)

The pipeline in the figure is defined in 
`dx_stream/pipelines/multi_stream/run_multi_stream_selector.sh` and can be used as a reference for execution.  

### **RTSP Input**  

`dx_stream/pipelines/rtsp/run_RTSP.sh` (Windows: `dx_stream\pipelines\windows\rtsp.bat`) runs the selector-based pipeline on RTSP streams. One channel is created per `--rtsp-url`, in the given order, and the output grid is sized to the number of channels.  

```
$ ./dx_stream/pipelines/rtsp/run_RTSP.sh --rtsp-url rtsp://127.0.0.1:8554/stream1 --rtsp-url rtsp://127.0.0.1:8554/stream2
```

- At least one `--rtsp-url` is required. Only `rtsp://` and `rtsps://` URLs without whitespace are accepted. `-h`/`--help` prints the usage.
- Missing or invalid arguments print the usage and exit with code `1`, before any model download.
- Credentials can be embedded in the URL (`rtsp://user:password@host:port/path`). The script does not print the URLs. On Windows, wrap each URL in double quotes.
- If any stream cannot be opened or fails to authenticate, GStreamer reports the error and the whole pipeline stops with a non-zero exit code. The script does not reconnect; restart it after fixing the stream.
- RTSP sources are live and do not wait for the pipeline. If the pipeline (mainly NPU inference, shared by all channels) cannot keep up with the source frame rate, frames may be dropped and latency may grow. Adding channels lowers the throughput available to each channel.
- On Windows, `run_demo.bat` (demo 8) rejects arguments containing `%` (for example a percent-encoded password such as `p%40ss`), because `call` would expand them again. Run `rtsp.bat` directly in that case.

### **Explanation**  

**Element Descriptions**  

- **`compositor`**: Draws multiple stream buffers received through sink pads at specified positions. In the example above, it is used for tiled display of inference results from each stream.  
- **`dxinputselector`**: Selects the buffer with the smallest PTS among multiple input streams received through sink pads and pushes it downstream. 
- **`dxoutputselector`**: Routes buffers received from upstream back into multiple output streams.  
- **`dxscale`**: Scales video frames to a specified resolution. Used to resize streams before display.  

### **Usage Notes**

- Depending on the model size, using a multi sub-pipeline structure may put a burden on memory resources
- While sub-pipeline structures can offer advantages in processing speed, the performance gain may be negligible compared to selector-based pipelines depending on the environment.
- **Multi-stream domain placement constraints.** The chain between `DxInputSelector` and `DxOutputSelector` uses `application/x-dxvideoraw` caps and carries per-stream identity in `DXFrameMeta`. Several elements cannot operate inside this domain and must be placed *per stream upstream of `DxInputSelector`* or *per output downstream of `DxOutputSelector`*:
    - **DxScale** and **DxConvert** — they require a single stable caps for buffer pool allocation and kernel setup. Inside the domain they would see interleaved buffers with different resolutions/formats and cannot reinitialize per buffer.
    - **DxGather** — designed to merge branches that share a single source (split by `tee`); it has no concept of `stream_id` and cannot demultiplex the merged domain stream.

    For the full element placement matrix, see [Multi-Stream Domain](./../Elements/03_00_Multi_Stream_Domain.md).

---
