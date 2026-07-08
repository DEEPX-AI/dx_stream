# Building DX-Stream on Windows

This guide covers building the DX-Stream GStreamer plugin on Windows (MSVC x64).

The Windows build produces `gstdxstream.dll`, the same GStreamer plugin that Linux builds as `libgstdxstream.so`.


## Quick Start

```cmd
install.bat          :: Install all dependencies (one-time)
build.bat            :: Build the plugin + custom libraries + apps
setup.bat            :: Download sample models and videos (one-time)
run_demo.bat         :: Run demo menu
```

### Custom Libraries & Apps

```cmd
dx_stream\custom_library\build.bat       :: Build postprocess/msgconv custom libraries
dx_stream\apps\build.bat                 :: Build example apps (mqtt, kafka, usermeta)
```

These require the main plugin to be built first (`build.bat`).


## Prerequisites

### Hardware Requirements
- x86_64 Windows 10/11 PC
- DEEPX DX-M1 or compatible NPU (for runtime inference)

### Software Requirements

The following must be installed manually before running `install.bat`:

| # | Software | Purpose | Download |
|---|----------|---------|----------|
| 1 | Visual Studio 2022 | C++ compiler (MSVC) | [visualstudio.microsoft.com](https://visualstudio.microsoft.com/) |
| 2 | Git for Windows | Source control, vcpkg | [git-scm.com](https://git-scm.com/download/win) |
| 3 | Python 3.9+ | Meson build system host | [python.org](https://www.python.org/downloads/) |
| 4 | GStreamer MSVC x86_64 | Multimedia framework | [gstreamer.freedesktop.org](https://gstreamer.freedesktop.org/download/) |
| 5 | DEEPX SDK | NPU runtime (dxrt) | DEEPX SDK installer |

#### Visual Studio 2022

Run the installer and select the **"Desktop development with C++"** workload.

#### Git for Windows

Install with default options. Ensure `git` is in your PATH.

#### Python

Download the official installer. Check **"Add Python to PATH"** during installation.

#### GStreamer

Download and install **both** packages from the official site:
- **Runtime** (MSVC 64-bit)
- **Development** (MSVC 64-bit)

Default install path: `C:\Program Files\gstreamer\1.0\msvc_x86_64`

> **Important**: The Development package is required for headers and `.pc` files.

#### DEEPX SDK

Install the DEEPX SDK using the official installer (`DX_SDK_*.exe`).
The installer registers `DEEPX_SDK_DIR` as a system environment variable automatically —
no manual path configuration is needed.

Default install path: `C:\Program Files\DEEPX\DX_SDK_<version>`

Expected directory structure (set up by the installer):
```
%DEEPX_SDK_DIR%\
    include\
    lib\x64\dxrt.lib
    bin\dxrt.dll
```


## Environment Variables

Most environment variables are configured automatically — no manual `set` commands are needed.

### DEEPX_SDK_DIR (auto-configured by DEEPX SDK installer)

The DEEPX SDK installer sets `DEEPX_SDK_DIR` as a system environment variable.
All build scripts read this variable automatically to locate the NPU runtime headers and libraries.

If for any reason the variable is missing, set it manually:
```cmd
set DEEPX_SDK_DIR=C:\Program Files\DEEPX\DX_SDK_<version>
```

### DXVNPU_DIR (Optional)

Required only when building with `--dxvnpu` flag:
```
%DXVNPU_DIR%\
    include\
    lib\dxvnpu.lib
    bin\dxvnpu.dll
```


## Installing Dependencies

After the prerequisites and environment variables are set:

```cmd
install.bat
```

This automatically:
1. Verifies all prerequisites are present
2. Installs `meson` and `ninja` via pip
3. Clones and bootstraps vcpkg (if not present)
4. Installs C++ libraries via vcpkg: eigen3, opencv4, libyuv, mosquitto, librdkafka
5. Generates `opencv4.pc` (required for meson to find OpenCV)
6. Validates DEEPX_SDK_DIR path

### Air-Gapped Network (No Internet)

On an internet-connected machine:
```cmd
C:\vcpkg\vcpkg install eigen3:x64-windows opencv4:x64-windows libyuv:x64-windows ^
                       mosquitto:x64-windows librdkafka:x64-windows
C:\vcpkg\vcpkg export eigen3:x64-windows opencv4:x64-windows libyuv:x64-windows ^
                       mosquitto:x64-windows librdkafka:x64-windows --zip
```

Copy the resulting zip to the air-gapped machine, extract `installed/x64-windows/` to:
```
dx_stream\vcpkg_installed\x64-windows\
```

Then run `build.bat` directly (vcpkg_installed already present).


## Building

### Basic Build

```cmd
build.bat
```

Output: `gst-dxstream-plugin\builddir\src\gstdxstream.dll`

### Build Options

```cmd
build.bat --clean              :: Remove builddir and rebuild from scratch
build.bat --dxvnpu             :: Enable DXVNPU elements (requires DXVNPU_DIR)
build.bat --type=debug         :: Debug build (symbols, no optimization)
build.bat --clean --dxvnpu     :: Combine options
```


## Building Custom Libraries & Apps

Custom libraries (postprocess, message_convert) and example apps have their own build scripts, separate from the main plugin build.

### Custom Libraries

```cmd
dx_stream\custom_library\build.bat              :: Build all
dx_stream\custom_library\build.bat --clean      :: Clean rebuild
dx_stream\custom_library\build.bat --type=debug :: Debug build
```

Builds every subdirectory under `postprocess_library/` and `message_convert_library/` that contains a `meson.build`.

Output DLLs are in each subdirectory's `builddir/`.

### Example Apps

```cmd
dx_stream\apps\build.bat              :: Build all
dx_stream\apps\build.bat --clean      :: Clean rebuild
```

Builds `mqtt_sub_example`, `kafka_consume_example`, `usermeta_app`.

Output executables are in each subdirectory's `builddir/`.

> Both scripts require the main plugin to be built first — they resolve `dependency('gstdxstream')` via the auto-generated `gstdxstream-uninstalled.pc` in the plugin builddir.


## Demo Assets (Models & Videos)

```cmd
setup.bat
```

Downloads sample AI models and test videos required by the demo pipelines.
Assets are saved to `dx_stream\samples\`.

For air-gapped environments, manually place model files (`.dxnn`) in `dx_stream\samples\models\` and video files in `dx_stream\samples\videos\`.


## Running Demos

```cmd
run_demo.bat                    :: Interactive demo menu
run_demo.bat --internal-rtsp    :: Use internal RTSP server for demo 9
```

The demo menu matches the Linux `run_demo.sh` layout:

| # | Demo | Model |
|---|------|-------|
| 0 | Object Detection | YOLOv26n |
| 1 | Object Detection | YoloV5S PPU |
| 2 | Face Detection | YOLOv5s_Face |
| 3 | Face Detection | SCRFD500M PPU |
| 4 | Pose Estimation | YOLOv26n_Pose |
| 5 | Pose Estimation | YOLOV5Pose PPU |
| 6 | Semantic Segmentation | YOLOv26n-Seg |
| 7 | Multi-Object Tracking | YoloV5S + OC_SORT |
| 8 | Multi-Stream (4ch) | Compositor Grid |
| 9 | Multi-Channel (RTSP) | dxinputselector |
| - | Secondary Mode | Multi-Model Cascade |

Pipeline scripts are located in `dx_stream\pipelines\windows\`. Each script is self-contained and can be run independently if `DXSTREAM_ROOT` is set:

```cmd
set DXSTREAM_ROOT=C:\path\to\dx_stream
dx_stream\pipelines\windows\object_detection_yolo26n.bat
dx_stream\pipelines\windows\rtsp.bat --internal-rtsp
```

### Full Build + Demo Workflow

```cmd
build.bat --clean
setup.bat                              :: Download models & videos (first time only)
run_demo.bat
```


## Deployment (Target Machine)

### Prerequisites

The target machine must have:
1. **GStreamer Runtime** (MSVC 64-bit) installed — provides core GStreamer DLLs
2. **VC++ Redistributable 2015-2022** (x64) — provides MSVCP140.dll, VCRUNTIME140.dll

### Source Build Deployment

DX-Stream on Windows uses source-build distribution. Clone the repository, build, and run on the target machine:

```cmd
git clone <repo-url>
cd dx_stream
install.bat
build.bat
setup.bat
run_demo.bat
```

> **Note**: `DEEPX_SDK_DIR` is set automatically by the DEEPX SDK installer.
> Install the DEEPX SDK before running `build.bat`.

`build.bat` automatically:
- Builds the plugin, custom libraries, and apps
- Collects all DLLs into `install\bin\`
- Registers `GST_PLUGIN_PATH` and user `Path` entries

### Required Environment Variables

The following are set automatically by `build.bat`:

```cmd
set GST_PLUGIN_PATH=%PROJECT_ROOT%\install\lib\gstreamer-1.0
set PATH=%PROJECT_ROOT%\install\bin;%PROJECT_ROOT%\install\share\gstdxstream\lib;%PROJECT_ROOT%\install\share\gstdxstream\bin;%PROJECT_ROOT%\install\lib\gstreamer-1.0;%PATH%
```

| Variable | Purpose |
|----------|---------|
| `GST_PLUGIN_PATH` | Tells GStreamer where to find `gstdxstream.dll` |
| `PATH` | Makes dependency DLLs (opencv, dxrt, etc.) discoverable at runtime |

### Verification

After building, open a new `cmd.exe` and run:
```cmd
gst-inspect-1.0 dxpreprocess
```

If this prints element details, DX-Stream is correctly installed.

### DLL Naming (Important)

The plugin DLL **must** be named `gstdxstream.dll` (no version suffix). GStreamer derives
the plugin entry point symbol from the filename:
- `gstdxstream.dll` → looks for `gst_plugin_dxstream_get_desc` ✓
- `gstdxstream-0.dll` → looks for `gst_plugin_dxstream_0_get_desc` ✗ (symbol not found)

The build system already handles this — `soversion` is only applied on Linux where symlinks
resolve the naming. Do not rename the DLL after building.


## Dependency Summary

| Dependency | Source | Method |
|---|---|---|
| gstreamer-1.0, gstreamer-video-1.0, json-glib-1.0, zlib | GStreamer installer | `dependency()` (pkg-config) |
| eigen3, opencv4 | vcpkg | `dependency()` (pkg-config) |
| libyuv | vcpkg | `cc.find_library()` |
| libmosquitto, rdkafka | vcpkg | `dependency()` (pkg-config) |
| dxrt, dxvnpu, dxdsp | DEEPX internal | `cc.find_library()` |
| dl (POSIX dynamic linking) | Linux only | Skipped on Windows (`dx_dlfcn.h` shim) |


## Differences from Linux Build

| Aspect | Linux | Windows |
|--------|-------|---------|
| Dependency install | `install.sh` (apt/source) | `install.bat` (vcpkg) |
| Compiler | GCC / Clang | MSVC (cl.exe) |
| Build | `build.sh` | `build.bat` |
| Plugin DLL name | `libgstdxstream.so` (with .so.0 symlink) | `gstdxstream.dll` (no version suffix) |
| Dynamic loading | `dlopen` / `dlsym` | `LoadLibrary` / `GetProcAddress` (via `dx_dlfcn.h`) |
| Plugin scanner | Default | Default |
| Distribution | Source build | Source build |
| Demo launcher | `run_demo.sh` | `run_demo.bat` |
| Pipeline scripts | `dx_stream/pipelines/` (shell scripts) | `dx_stream/pipelines/windows/` (batch files) |
| Apps / custom_library | `build.sh` (meson install) | `build.bat` (meson compile only) |
| Python bindings (pydxs) | Built | Not included |


## Troubleshooting

### GStreamer Registry Cache

GStreamer caches plugin scan results in:
```
%LOCALAPPDATA%\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64-msvc.bin
```

If the plugin was previously blacklisted (e.g., environment not set up correctly at first launch), delete this cache and retry:
```cmd
del "%LOCALAPPDATA%\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64-msvc.bin"
```

### DEEPX_SDK_DIR not set

```
[ERROR] DEEPX_SDK_DIR environment variable is not set.
```

Install the DEEPX SDK using the official installer — it sets `DEEPX_SDK_DIR` automatically.
If already installed, open a new terminal so the updated system environment is loaded.
As a last resort, set it manually:
```cmd
set DEEPX_SDK_DIR=C:\Program Files\DEEPX\DX_SDK_<version>
```

### install.bat fails: prerequisites missing

Install the listed software and re-run. `install.bat` will not proceed until all prerequisites are satisfied.

### meson setup fails: dependency not found

Run `install.bat` first. If already run, check that `vcpkg_installed\x64-windows\lib\pkgconfig\` contains the expected `.pc` files.

### Link error: unresolved external symbol

Verify `vcpkg_installed\x64-windows\lib` contains the required `.lib` files.

### MSVC not found

Run from a regular `cmd.exe` prompt (not PowerShell). `build.bat` calls `vcvarsall.bat` internally.

### gst-inspect-1.0 dxpreprocess: No such element

Check in order:
1. `GST_PLUGIN_PATH` points to the folder containing `gstdxstream.dll`
2. `PATH` includes `install\bin` for dependency DLLs
3. Delete registry cache: `del "%LOCALAPPDATA%\Microsoft\Windows\INetCache\gstreamer-1.0\registry.x86_64-msvc.bin"`
4. Verify DLL is named `gstdxstream.dll` (not `gstdxstream-0.dll`)
