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