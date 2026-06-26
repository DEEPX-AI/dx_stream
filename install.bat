@echo off
REM ============================================================
REM  DX-Stream Windows Dependency Installer
REM  Installs all build and runtime dependencies for development.
REM  Usage:
REM    install.bat        -> check prerequisites and install dependencies
REM ============================================================

setlocal EnableDelayedExpansion

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

echo ============================================================
echo  DX-Stream Windows Dependency Installer
echo ============================================================
echo.

set "FAIL=0"

REM ============================================================
REM  Phase 1: Manual prerequisites check
REM ============================================================
echo [Phase 1] Checking prerequisites...
echo.

REM ---- Visual Studio 2022 ----
set "VS_FOUND=0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_FOUND=1"
    )
)
if "!VS_FOUND!"=="0" (
    for %%E in (Community Professional Enterprise BuildTools) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\2022\%%E\VC\Auxiliary\Build\vcvarsall.bat" set "VS_FOUND=1"
    )
)
if "!VS_FOUND!"=="1" (
    echo   [OK] Visual Studio 2022 with C++ workload
) else (
    echo   [MISSING] Visual Studio 2022 with C++ workload
    echo            Download: https://visualstudio.microsoft.com/
    echo            Install "Desktop development with C++" workload
    set "FAIL=1"
)

REM ---- Git ----
where git >nul 2>nul
if %ERRORLEVEL%==0 (
    echo   [OK] Git
) else (
    echo   [MISSING] Git
    echo            Download: https://git-scm.com/download/win
    set "FAIL=1"
)

REM ---- Python ----
set "PYTHON_VERSION="
python --version >nul 2>nul
if not errorlevel 1 (
    for /f "delims=" %%V in ('python --version 2^>^&1') do set "PYTHON_VERSION=%%V"
    echo   [OK] !PYTHON_VERSION!
    echo   [INFO] pydxs ^(Python binding^): pipeline tests require PyGObject ^(gi^).
    echo          GStreamer MSI bundles gi compiled for a specific Python version.
    echo          ^(e.g., GStreamer 1.26.1 = cp313 only^)
    echo          Use Python version matching your GStreamer installer for full test coverage.
) else (
    echo   [MISSING] Python 3.9+
    echo            Download: https://www.python.org/downloads/
    echo            Check "Add Python to PATH" during installation
    set "FAIL=1"
)

REM ---- GStreamer MSVC ----
set "GST_FOUND=0"
if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    if exist "%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin\gst-inspect-1.0.exe" set "GST_FOUND=1"
)
if "!GST_FOUND!"=="0" (
    if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64\bin\gst-inspect-1.0.exe" set "GST_FOUND=1"
)
if "!GST_FOUND!"=="1" (
    echo   [OK] GStreamer MSVC x86_64
) else (
    echo   [MISSING] GStreamer MSVC x86_64 [Runtime + Development]
    echo            Download: https://gstreamer.freedesktop.org/download/
    echo            Install BOTH "Runtime" and "Development" MSVC 64-bit packages
    set "FAIL=1"
)

echo.
if "!FAIL!"=="1" (
    echo [ERROR] Missing prerequisites. Install the items above and re-run.
    exit /b 1
)
echo [Phase 1] All prerequisites OK.
echo.

REM ============================================================
REM  Phase 2: Auto-install build tools and libraries
REM ============================================================
echo [Phase 2] Installing build tools and libraries...
echo.

REM ---- meson / ninja ----
where meson >nul 2>nul
if %ERRORLEVEL%==0 (
    echo   [OK] meson already installed
) else (
    echo   [INSTALL] Installing meson and ninja...
    pip install meson ninja
    if errorlevel 1 (
        echo   [ERROR] Failed to install meson/ninja via pip
        exit /b 1
    )
    echo   [OK] meson and ninja installed
)

REM ---- vcpkg ----
set "VCPKG_EXE="
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\vcpkg.exe" set "VCPKG_EXE=%VCPKG_ROOT%\vcpkg.exe"
)
if not defined VCPKG_EXE (
    if exist "C:\vcpkg\vcpkg.exe" set "VCPKG_EXE=C:\vcpkg\vcpkg.exe"
)
if not defined VCPKG_EXE (
    echo   [INSTALL] Cloning vcpkg to C:\vcpkg...
    git clone https://github.com/microsoft/vcpkg C:\vcpkg
    if errorlevel 1 (
        echo   [ERROR] Failed to clone vcpkg
        exit /b 1
    )
    call C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics
    if errorlevel 1 (
        echo   [ERROR] Failed to bootstrap vcpkg
        exit /b 1
    )
    set "VCPKG_EXE=C:\vcpkg\vcpkg.exe"
    echo   [OK] vcpkg installed at C:\vcpkg
) else (
    echo   [OK] vcpkg found at !VCPKG_EXE!
)

REM ---- vcpkg packages ----
set "VCPKG_INSTALLED=%PROJECT_ROOT%\vcpkg_installed\x64-windows"
call :CheckVcpkgDeps
if not errorlevel 1 (
    echo   [OK] vcpkg packages already installed
) else (
    echo   [INSTALL] Installing vcpkg packages [this may take 30-60 minutes]...
    REM Manifest mode: run from the dir holding vcpkg.json so the pinned
    REM baseline in vcpkg-configuration.json is honored. Listing package
    REM names here would switch vcpkg to classic mode and ignore the manifest.
    pushd "%PROJECT_ROOT%"
    "!VCPKG_EXE!" install --triplet x64-windows --x-install-root="%PROJECT_ROOT%\vcpkg_installed"
    set "VCPKG_RC=!errorlevel!"
    popd
    if !VCPKG_RC! neq 0 (
        echo   [ERROR] vcpkg install failed
        echo          If on air-gapped network, copy vcpkg_installed/ from an internet-connected machine.
        exit /b 1
    )
    call :CheckVcpkgDeps
    if errorlevel 1 (
        echo   [ERROR] vcpkg install completed but required dependency files are missing
        exit /b 1
    )
    echo   [OK] vcpkg packages installed
)

REM ---- opencv4.pc generation ----
set "OPENCV_PC=!VCPKG_INSTALLED!\lib\pkgconfig\opencv4.pc"
if not exist "!OPENCV_PC!" (
    echo   [INSTALL] Generating opencv4.pc...
    if not exist "!VCPKG_INSTALLED!\lib\pkgconfig" mkdir "!VCPKG_INSTALLED!\lib\pkgconfig"
    set "OPENCV_VERSION=0"
    set "OPENCV_VERSION_HEADER="
    if exist "!VCPKG_INSTALLED!\include\opencv4\opencv2\core\version.hpp" set "OPENCV_VERSION_HEADER=!VCPKG_INSTALLED!\include\opencv4\opencv2\core\version.hpp"
    if not defined OPENCV_VERSION_HEADER if exist "!VCPKG_INSTALLED!\include\opencv2\core\version.hpp" set "OPENCV_VERSION_HEADER=!VCPKG_INSTALLED!\include\opencv2\core\version.hpp"
    if defined OPENCV_VERSION_HEADER (
        for /f "tokens=3" %%V in ('findstr /R /C:"^#define CV_VERSION " "!OPENCV_VERSION_HEADER!"') do (
            set "OPENCV_VERSION=%%~V"
            set "OPENCV_VERSION=!OPENCV_VERSION:"=!"
        )
    )
    if "!OPENCV_VERSION!"=="0" (
        echo   [WARN] OpenCV version header not found; writing opencv4.pc Version: 0
    )
    (
        echo prefix=${pcfiledir}/../..
        echo exec_prefix=${prefix}
        echo libdir=${prefix}/lib
        echo includedir=${prefix}/include
        echo.
        echo Name: OpenCV
        echo Description: Open Source Computer Vision Library
        echo Version: !OPENCV_VERSION!
        echo Cflags: -I${includedir} -I${includedir}/opencv4
        echo Libs: -L${libdir} -lopencv_core4 -lopencv_imgproc4 -lopencv_imgcodecs4 -lopencv_highgui4 -lopencv_video4 -lopencv_videoio4 -lopencv_dnn4 -lopencv_objdetect4 -lopencv_calib3d4 -lopencv_features2d4 -lopencv_flann4 -lopencv_ml4 -lopencv_photo4 -lopencv_stitching4
    ) > "!OPENCV_PC!"
    echo   [OK] opencv4.pc generated
) else (
    echo   [OK] opencv4.pc exists
)

echo.
echo [Phase 2] Build tools and libraries OK.
echo.

REM ============================================================
REM  Phase 3: DEEPX internal libraries check
REM ============================================================
echo [Phase 3] Checking DEEPX internal libraries...
echo.

REM ---- dxrt ----
if not defined DEEPX_SDK_DIR (
    echo   [ERROR] DEEPX_SDK_DIR environment variable is not set.
    echo           Set DEEPX_SDK_DIR to the DEEPX SDK install directory.
    echo           Expected: %%DEEPX_SDK_DIR%%\include\ and %%DEEPX_SDK_DIR%%\lib\dxrt.lib
    exit /b 1
)
set "DXRT_SDK_DIR=%DEEPX_SDK_DIR%"
if not exist "%DXRT_SDK_DIR%\lib\x64\dxrt.lib" (
    echo   [ERROR] dxrt.lib not found at %DXRT_SDK_DIR%\lib\x64
    echo           Check that DEEPX_SDK_DIR points to the correct SDK directory.
    exit /b 1
)
echo   [OK] dxrt sdk: %DXRT_SDK_DIR%

REM ---- dxvnpu (optional) ----
if defined DXVNPU_DIR (
    if exist "%DXVNPU_DIR%\lib\dxvnpu.lib" (
        echo   [OK] dxvnpu: %DXVNPU_DIR%
    ) else (
        echo   [WARN] DXVNPU_DIR is set but dxvnpu.lib not found at %DXVNPU_DIR%\lib
    )
) else (
    echo   [INFO] DXVNPU_DIR not set [optional]. Set it for VNPU element builds.
)

echo.
echo ============================================================
echo  Installation complete.
echo  Next step: run build.bat to compile DX-Stream.
echo ============================================================
endlocal
exit /b 0

:CheckVcpkgDeps
set "VCPKG_DEPS_OK=1"
if not exist "!VCPKG_INSTALLED!\include\eigen3\Eigen\Core" (
    echo   [MISSING] vcpkg eigen3: !VCPKG_INSTALLED!\include\eigen3\Eigen\Core
    set "VCPKG_DEPS_OK=0"
)
if not exist "!VCPKG_INSTALLED!\lib\opencv_core4.lib" (
    echo   [MISSING] vcpkg opencv4: !VCPKG_INSTALLED!\lib\opencv_core4.lib
    set "VCPKG_DEPS_OK=0"
)
if not exist "!VCPKG_INSTALLED!\include\opencv4\opencv2\core\version.hpp" if not exist "!VCPKG_INSTALLED!\include\opencv2\core\version.hpp" (
    echo   [MISSING] vcpkg opencv4 header: !VCPKG_INSTALLED!\include\opencv4\opencv2\core\version.hpp
    set "VCPKG_DEPS_OK=0"
)
if not exist "!VCPKG_INSTALLED!\lib\libyuv.lib" (
    echo   [MISSING] vcpkg libyuv: !VCPKG_INSTALLED!\lib\libyuv.lib
    set "VCPKG_DEPS_OK=0"
)
if not exist "!VCPKG_INSTALLED!\lib\pkgconfig\libmosquitto.pc" (
    echo   [MISSING] vcpkg mosquitto: !VCPKG_INSTALLED!\lib\pkgconfig\libmosquitto.pc
    set "VCPKG_DEPS_OK=0"
)
if not exist "!VCPKG_INSTALLED!\lib\pkgconfig\rdkafka.pc" (
    echo   [MISSING] vcpkg librdkafka: !VCPKG_INSTALLED!\lib\pkgconfig\rdkafka.pc
    set "VCPKG_DEPS_OK=0"
)
if "!VCPKG_DEPS_OK!"=="1" exit /b 0
exit /b 1
