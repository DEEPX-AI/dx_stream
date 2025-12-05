## Version 2.1.0 (Nov 2025)

#### Changed
- **Model Configuration**: Updated default YOLOv5 from YOLOV5S_3 to YOLOV5S_4 with models-2_1_0.tar.gz
- **Video Sink Settings**: Disabled synchronization in secondary mode for improved performance
- **Demo Scripts**: Simplified video input paths in message broker demos
- **Message Conversion**: Simplified message conversion configuration and improved JSON payload structure
- **Buffer Processing**: Enhanced preprocessing and postprocessing to use direct buffer manipulation for better performance
- **Configuration**: Streamlined msgconv configuration by removing unnecessary sections
- Modified event handling logic in 'dxpreprocess', 'dxinfer', 'dxoutputselector' and 'dxosd' to align with updates to 'dxinputselector'.

#### Fixed
- **Setup Scripts**: Improved error handling and prevented excessive download retry attempts
- **Shutdown Flow**: Improved shutdown signal processing in dx-infer element
- **Memory Management**: Better buffer handling in preprocessing and postprocessing pipelines
- **Registry Handling**: Fixed GStreamer registry cache issues with GstShark integration
- Fixed an event processing timing issue in 'dxinputselector' that caused compositor pipeline freezes.

#### Added
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

**Known Issues.** 
- DeepLabV3 Semantic Segmentation model accuracy may be slightly degraded in dx-compiler(dx_com) v2.1.0. This will be fixed in the next release. The DeepLabV3 model used in the demo was converted using dx-compiler v2.0.0.
- When using the PPU model for face detection & pose estimation, dx-compiler v2.1.0 does not currently support converting face and pose models to PPU format. This feature will be added in a future release. The PPU models used in the demo were converted using dx-compiler v1.0.0(dx_com v1.60.1).

## Version 2.0.0 (Aug 2025)

#### Changed

- Code Examples: The PostProcess examples have been separated and implemented on a per-model basis for clarity.
- DX-RT v3.0.0 Compatibility: This version has been updated to ensure full compatibility with DX-RT v3.0.0.
- Model Support: Inference is now restricted to models (DXNN v7) produced by DX-COM v2.0.0 and later versions.
Modified dx-gather event handling logic.
- Removed unnecessary print statements.
- feat: enhance build script and update installation documentation
    - Added OS and architecture checks in the build script
    - Updated CPU and OS specifications in the installation documentation for clarity

#### Fixed

- Bug Fix: Addressed and alleviated a processing delay issue within the dx-inputselector.
- Corrected a post-processing logic error in the SCRFD model when in secondary inference mode.
- Fixed a bug in dx_rt that occurred when processing multi-tail models.
- feat: improve error handling for setup scripts
- feat: add support for X11 video sink on Ubuntu 18.04 across multiple scripts
    - Force X11 video sink on Ubuntu 18.04
    - Improved compatibility across OS versions
    - Updated multiple pipeline scripts
    - Added OS version check for Ubuntu 18.04

#### Added

- feat: add uninstall script and enhance color utility functions
    - Introduced a new uninstall.sh script for cleaning up project files and directories

## Version 1.7.0 (Jul 2025)

#### Changed
- Improved the buffer queue management mechanism. Instead of locking inputs based on queue size within the push thread, the system now adds a req_id to the buffer and utilizes a wait function for more efficient processing.
- auto run setup script when a file not found error occurs during example execution
- apply colors and handle errors in scripts

#### Fixed
- dxpreprocess, dxosd: Resolved a video corruption issue that occurred in some streams. The problem was traced to incorrect stride and offset calculations from GstVideoMeta. The calculation now correctly uses GstVideoInfo included in the caps, ensuring stable video rendering.
