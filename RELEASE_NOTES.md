# RELEASE_NOTES
## v2.1.0 / 2025-11-28

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

## v2.0.0 / 2025-08-28

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

## v1.7.0 / 2025-07-16
### 1. Changed
- dxinfer : Improved the buffer queue management mechanism. Instead of locking inputs based on queue size within the push thread, the system now adds a req_id to the buffer and utilizes a wait function for more efficient processing.
- feat: auto run setup script when a file not found error occurs during example execution
- chore: apply colors and handle errors in scripts
### 2. Fixed
- dxpreprocess, dxosd: Resolved a video corruption issue that occurred in some streams. The problem was traced to incorrect stride and offset calculations from GstVideoMeta. The calculation now correctly uses GstVideoInfo included in the caps, ensuring stable video rendering.
### 3. Added

## v1.6.4 / 2025-07-03
### 1. Changed
- Auto dependency installation for Ubuntu 18.04
### 2. Fixed
- memory issue in segmentation postprocess fixed
### 3. Added

## v1.6.3 / 2025-06-25
### 1. Changed
### 2. Fixed
- Display Crash in Orange Pi 5+ fixed
### 3. Added


## v1.6.1 / 2025-05-21
### 1. Changed
- None
### 2. Fixed
- Fix a potential out-of-bounds memory access bug.
### 3. Added
- None

## v1.6.0 / 2025-05-09
### 1. Changed
- None
### 2. Fixed
- None
### 3. Added
- Supporting the Ubuntu 18.04 OS environment
- onnxruntime segmentation post process added

## v1.5.0 / 2025-05-07
### 1. Changed
- OSD push resized BGR format Buffer (property fixed)
- update multi-stream pipeline demo with compositor
### 2. Fixed
- Fixed buffer copy in dx-gather
### 3. Added
- Add RGA based preprocess & osd process

## v1.4.0 / 2025-04-24
### 1. Changed
- None
### 2. Fixed
- None
### 3. Added
- Add example for YOLOV5S Face Model

## v1.3.1 / 2025-04-23
### 1. Changed
- None
### 2. Fixed
- Modify to stop the build process upon encountering an error and require explicit error handling in Jenkins, etc.
### 3. Added
- None

## v1.3.0 / 2025-04-10
### 1. Changed
- None
### 2. Fixed
- add option '--force' in the setup scritps
- update get_resource.sh to check internel structure of the tar file
### 3. Added
- None

## v1.2.2 / 2025-03-25
### 1. Changed
- None
### 2. Fixed
- Fix buffer shuffle problem in dx-infer element
### 3. Added
- None

## v1.2.1 / 2025-03-20
### 1. Changed
- Download assets(model, video) from AWS S3
### 2. Fixed
- update dependency installation (librdkafka)
### 3. Added
- None

## v1.2.0 / 2025-03-10
### 1. Changed
- None
### 2. Fixed
- None
### 3. Added
- Update pipeline scripts for supporting intel GPU HW Acceleration (VAAPI)

## v1.1.1 / 2025-03-05
### 1. Changed
- DX-RT API interface changed (inference engine function)
- setup_dxnn_assets.sh 실행 시, regression ID 번호를 인자로 받아서 해당하는 모델들을 복사해서 가져오도록 변경. (default regID : 3148)
### 2. Fixed
- Memory free issue (NV12 format Resize)
- display crash issue (NV12 format color converting)
- DX-RT에서 DataType의 순서가 변경됨에 따라, dxcommon.hpp에서도 동일하게 DataType을 수정.
- PPU 모델의 경우 Output 배열에 Batch Size 차원이 추가됨에 따라, 관련된 PostProcess 코드들도 이에 맞게 수정. (YOLO, SCRFD)
### 3. Added
- None
