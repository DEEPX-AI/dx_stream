@echo off
REM Usage: call setup_gstreamer.bat runtime^|development
REM Exports GST_ROOT and configures the calling CMD environment.
setlocal EnableDelayedExpansion

set "GST_MODE=%~1"
if /I not "!GST_MODE!"=="runtime" if /I not "!GST_MODE!"=="development" (
    echo [ERROR] GStreamer setup requires runtime or development mode.
    exit /b 2
)

set "GST_ROOT="
for %%G in (
    "%GSTREAMER_1_0_ROOT_MSVC_X86_64%"
    "%GSTREAMER_1_0_ROOT_X86_64%"
    "C:\Program Files\gstreamer\1.0\msvc_x86_64"
) do (
    if not defined GST_ROOT if not "%%~G"=="" (
        if /I "!GST_MODE!"=="development" (
            if exist "%%~G\lib\pkgconfig\gstreamer-1.0.pc" set "GST_ROOT=%%~fG"
        ) else (
            if exist "%%~G\bin\gst-launch-1.0.exe" if exist "%%~G\bin\gst-inspect-1.0.exe" set "GST_ROOT=%%~fG"
        )
    )
)

if not defined GST_ROOT (
    echo [ERROR] GStreamer !GST_MODE! files not found.
    echo         Install GStreamer MSVC x64 or set GSTREAMER_1_0_ROOT_MSVC_X86_64
    echo         or GSTREAMER_1_0_ROOT_X86_64 to a valid installation root.
    exit /b 1
)

if "!GST_ROOT:~-1!"=="\" if not "!GST_ROOT:~3!"=="" set "GST_ROOT=!GST_ROOT:~0,-1!"
set "PATH=!GST_ROOT!\bin;!PATH!"
if /I "!GST_MODE!"=="development" set "PKG_CONFIG_PATH=!GST_ROOT!\lib\pkgconfig;!PKG_CONFIG_PATH!"
endlocal & set "GST_ROOT=%GST_ROOT%" & set "GSTREAMER_1_0_ROOT_MSVC_X86_64=%GST_ROOT%" & set "PATH=%PATH%" & set "PKG_CONFIG_PATH=%PKG_CONFIG_PATH%"
exit /b 0
