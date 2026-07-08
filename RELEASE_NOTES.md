# RELEASE_NOTES

## v3.1.0 / 2026-06-25

### 1. Changed
- **Multi-Stream Domain**: Introduced `application/x-dxvideoraw` domain caps for unified multi-stream processing — a single chain can now process streams with different resolutions and formats
    - Per-stream sticky events (STREAM_START / CAPS / SEGMENT / EOS) are preserved across the domain so each stream keeps an independent timeline
    - `dxinputselector` and `dxoutputselector` act as explicit domain boundaries; single-stream processing elements can be placed directly in multi-stream pipelines without modification
- **Base class migration**: `dxinputselector` and `dxgather` migrated to `GstAggregator` for standard N:1 multiplexing; `dxvnpuoverlay` migrated to `GstBaseSink` for proper render/preroll lifecycle
- Introduce TransformKernelPool for automatic src-format-based kernel selection with libyuv fallback, and migrate dxscale, dxconvert, dxpreprocess, dxmsgconv to use it
- Refactor dxinfer to use InferBackend abstraction (Put/Get async pattern) with backend property (auto/dxrt/dxvnpu) replacing direct dxrt dependency

### 2. Fixed
- **LATENCY reporting**: All processing elements now correctly account for their own processing time in LATENCY queries, stabilizing synchronization and QoS behavior in pipelines that include live sources
- **FLUSH recovery**: Internal queues, threads, and per-stream state are now properly reset on FLUSH, eliminating hangs and errors on seek / replay
- **Push thread lifecycle**: Separated `drain_push_thread` for safe thread cleanup during EOS / FLUSH / state transitions, resolving single-frame input hangs ()
- **Caps negotiation**: Pad template caps tightened so incompatible element connections fail immediately at negotiation time instead of producing runtime anomalies
- **Error handling**: `abort()` / uncaught exceptions replaced with `GST_ELEMENT_ERROR` across all elements — missing model files, NPU init failures, and errors in user-provided libraries (`dxpreprocess` / `dxpostprocess` / `dxmsgconv`) now produce proper GStreamer error messages instead of crashing the pipeline ()
- Fix pipeline abort on error by replacing g_error with GST logging macros for graceful error handling

### 3. Added
- DEEPX Agent-Driven Development (dx-agent-dev) — Beta.
Describe a pipeline in natural language and an AI agent assembles the GStreamer graph from DEEPX elements, wiring NPU inference into real-time video/stream pipelines end to end.
- **Test Suite**: Reorganized test infrastructure under `test/base/{element,metadata,pipeline}/` with 73 new test binaries covering element contracts, domain boundaries, end-to-end pipelines, metadata, and pydxs scenarios
     - Per-suite runners: `run_element.sh`, `run_pipeline.sh`, `run_metadata.sh`, `run_pydxs.sh`, `run_all.sh`
- Added a comprehensive YOLO26 benchmark toolkit, which provides:
    - Model-level, end-to-end pipeline, and multi-stream performance measurements
    - Automated result aggregation, static dashboard and report generation, and version-trend comparison across runs
    - Detailed documentation and analysis guides, including benchmark summaries for supported environments
- **DxMsgConv**: Add `include-frame` property for base64 JPEG frame encoding with Kafka/MQTT consumer display support

## DX-Stream v3.0.1 / 2026-04-21

### 1. Changed
- None
### 2. Fixed
- Fix Typos in User Manual
- Fix uninstall not removing apps build directories and pydxs build cache
### 3. Added
- Third-party sample model license notice documentation

## DX-Stream v3.0.0 / 2026-04-01

### 1. Changed
- **Model Download**: Switched to `dx-modelzoo`-managed models with updated demo lineup **(Breaking)**
    - Per-model download via `model_list.json` + `setup_sample_models.sh` (`jq` required)
    - Demo model upgrades: YOLOv5s → YOLOv26n, YOLOV5Pose → YOLOv26n_Pose, DeepLabV3+MobileNetV2 → YOLOv26n_Seg (not available in modelzoo; replaced)
    - Models not in dx-modelzoo (manually uploaded to S3, accuracy not measured): SCRFD500M PPU, YOLOV5Pose PPU
- **Installation**: Removed `/etc/profile.d/gstdxstream.sh` (no longer requires sudo); env vars now written directly to `~/.bashrc` **(Breaking)** (old profile.d-based setup no longer used)
- **C++ Standard**: Updated to C++14 (using declarations, enum classes) **(Breaking)**
- **Preprocessing Architecture**: Introduced `IVideoTransformKernel` abstraction (RGA / libyuv / V3 DSP) with `VideoTransformFactory` for auto backend selection
- **DXOSD**: Removed unnecessary color conversion, resize, and intermediate memory allocation; added YUV in-place overlay rendering
- **Dependency**: Added `gstreamer1.0-dev` build dependency; install prefix default `/usr/local`

### 2. Fixed
- NV12 CPU input stride/offset now uses GstVideoInfo instead of RGA heuristic (dxconvert/dxscale)
- DMA-Buffer zero-copy support in RGA preprocessor and DXOSD with proper stride calculation
- Orange Pi 5 Plus (RK3588 / Debian 12) display corruption via automatic I420 conversion
- RGA preprocessing build errors on Rockchip SoC
- SW rendering buffer management: stream-specific caching for multi-stream scenarios
- Memory management with smart pointers and improved tensor handling
- Meson install failure in Python venv environment (PYTHONPATH handling and build directory standardization)

### 3. Added
- **DxScale**: New GstBaseTransform element for HW-accelerated scaling (NV12/I420/RGB/BGR)
- **DxConvert**: New GstBaseTransform element for color format conversion (full 4×4 format matrix)
- RGA HW-accelerated format conversion/scaling (NV12↔RGB↔BGR) with automatic SW fallback
- V3 DSP preprocessor with OSD v3 RGB buffer drawing for DEEPX V3 SoC
- YOLOv26 examples
- Auto-detection and cleanup of old installation files from previous major versions
- Debugging guide documentation

## DX-Stream v2.2.1 / 2026-02-28

### 1. Changed
- Update sample models version from dx_com v2.2.0 to v2.2.1

## DX-Stream v2.2.0 / 2025-12-30

### 1. Changed
- Modified gst-dxpreprocess to automatically deep copy buffers when ref_count > 1 (e.g., after tee element)
- **Metadata Architecture**: Separated existing gst-dxmeta.cpp/hpp into individual metadata files (frame, object, user metadata) and enhanced structures with user metadata lists and count fields
- **Test Framework**: Refactored metadata handling in test files to use dx_get_frame_meta for improved performance and added test plugin installation step in unit test workflow
- docs: update OS requirements in installation guide for debian

### 2. Fixed
- Fixed race condition and segfaults in secondary inference mode when multiple streams access shared buffer metadata
- **API Migration**: Updated object metadata removal to use new API in track function
- **Error Handling**: Enhanced error handling for compilation failures in test scripts

### 3. Added
- Added prepare_output_buffer() override to detect and copy shared buffers
- Added `pydxs` Python binding module for DX Stream metadata,.
- Python bindings for `DXFrameMeta`, `DXObjectMeta`, `DXUserMeta`, and related value types.
- Helper APIs to acquire and attach `DXObjectMeta` to frames and to manage user metadata from Python.
- A `writable_buffer` context manager to safely ensure buffer writability and create/retrieve `DXFrameMeta` inside GStreamer pad probes.
- A new Python sample app `usermeta_app.py` demonstrating creation, attachment, and reading of frame/object/user metadata using `pydxs`.
- **User Metadata Interface**: New API allowing developers to attach custom metadata to frames and objects
- **Management Functions**: Comprehensive metadata structures and management functions (create, copy, release)
- **Test Suite**: Dedicated user metadata test suite (test_usermeta.cpp) with validation framework
- **Documentation**: User Metadata Guide documentation and README updates

---

## DX-Stream v2.1.0 / 2025-11-28

### 1. Changed
- **Model Configuration**: Updated default YOLOv5 from YOLOV5S_3 to YOLOV5S_4 with models-2_1_0.tar.gz
- **Video Sink Settings**: Disabled synchronization in secondary mode for improved performance
- **Demo Scripts**: Simplified video input paths in message broker demos
- **Message Conversion**: Simplified message conversion configuration and improved JSON payload structure
- **Buffer Processing**: Enhanced preprocessing and postprocessing to use direct buffer manipulation for better performance
- **Configuration**: Streamlined msgconv configuration by removing unnecessary sections
- Modified event handling logic in 'dxpreprocess', 'dxinfer', 'dxoutputselector' and 'dxosd' to align with updates to 'dxinputselector'.
- feat: enhance dependency installation for Debian 12
    - Smart version checking for meson (apt/backports/pip fallback)
    - Handle libdrm version conflicts without breaking HW acceleration
- update secondary mode pipeline for SCRFD500M_PPU_SECOND

### 2. Fixed
- **Setup Scripts**: Improved error handling and prevented excessive download retry attempts
- **Shutdown Flow**: Improved shutdown signal processing in dx-infer element
- **Memory Management**: Better buffer handling in preprocessing and postprocessing pipelines
- **Registry Handling**: Fixed GStreamer registry cache issues with GstShark integration
- Fixed an event processing timing issue in 'dxinputselector' that caused compositor pipeline freezes.
- fix: add detection and installation of Rockchip-specific dependencies (librga-dev)
- update MQTT configuration to include connection timeout and state management

### 3. Added
- **PPU Support**: Integrated Post-Processing Unit functionality for YOLOv5s, SCRFD500M, and YOLOv5Pose models
    - NPU-based bounding box decoding and NMS processing to reduce CPU overhead
    - Three new demo options showcasing PPU capabilities
- **Download Reliability**: Enhanced setup scripts with timeout limits and file integrity verification
    - Automatic verification and cleanup of corrupted archives
    - Prevents infinite retry loops that could cause hour-long hangs
- Installation Guide for Orange Pi 5 Plus
- Performance Analysis Tools: Added GstShark integration for comprehensive pipeline performance evaluation
    - Automated installation script 'install_gstshark.sh' for easy setup
    - Complete performance evaluation documentation with sample commands
    - Support for CPU usage, processing time, frame rate, and bitrate analysis
- **Preprocessing Features**: Added preprocess skip functionality for conditional processing
- **Build Support**: Added build configuration for v3 architecture

---

## DX-Stream v2.0.0 / 2025-08-28

### 1. Changed
- Code Examples: The PostProcess examples have been separated and implemented on a per-model basis for clarity.
- DX-RT v3.0.0 Compatibility: This version has been updated to ensure full compatibility with DX-RT v3.0.0.
- Model Support: Inference is now restricted to models (DXNN v7) produced by DX-COM v2.0.0 and later versions.
- Modified dx-gather event handling logic.
- Removed unnecessary print statements.
- feat: enhance build script and update installation documentation
  - Added OS and architecture checks in the build script
  - Updated CPU and OS specifications in the installation documentation for clarity

### 2. Fixed
- Bug Fix: Addressed and alleviated a processing delay issue within the dx-inputselector.
- Corrected a post-processing logic error in the SCRFD model when in secondary inference mode.
- Fixed a bug in dx_rt that occurred when processing multi-tail models.
- feat: improve error handling for setup scripts
- feat: add support for X11 video sink on Ubuntu 18.04 across multiple scripts
  - Force X11 video sink on Ubuntu 18.04
  - Improved compatibility across OS versions
  - Updated multiple pipeline scripts
  - Added OS version check for Ubuntu 18.04

### 3. Added
- feat: add uninstall script and enhance color utility functions
  - Introduced a new uninstall.sh script for cleaning up project files and directories

---

## DX-Stream  v1.7.0 / 2025-07-16

### 1. Changed
- dxinfer : Improved the buffer queue management mechanism. Instead of locking inputs based on queue size within the push thread, the system now adds a req_id to the buffer and utilizes a wait function for more efficient processing.
- feat: auto run setup script when a file not found error occurs during example execution
- chore: apply colors and handle errors in scripts
### 2. Fixed
- dxpreprocess, dxosd: Resolved a video corruption issue that occurred in some streams. The problem was traced to incorrect stride and offset calculations from GstVideoMeta. The calculation now correctly uses GstVideoInfo included in the caps, ensuring stable video rendering.
### 3. Added
