@echo off
REM ============================================================
REM  DX-Stream Demo Menu (Windows - Source Build)
REM  Usage:  run_demo.bat [--internal-rtsp]
REM  Prerequisites:  Run 'build.bat' first.
REM ============================================================

setlocal

set "PROJECT_ROOT=%~dp0"
if "%PROJECT_ROOT:~-1%"=="\" set "PROJECT_ROOT=%PROJECT_ROOT:~0,-1%"

set "INSTALL_DIR=%PROJECT_ROOT%\install"
set "PIPELINES_DIR=%PROJECT_ROOT%\dx_stream\pipelines\windows"

REM ---- Parse arguments ----
set "INTERNAL_RTSP="
if "%~1"=="--internal-rtsp" set "INTERNAL_RTSP=--internal-rtsp"

REM ---- Verify build ----
if not exist "%INSTALL_DIR%\bin\gstdxstream.dll" (
    echo [ERROR] DX-Stream plugin not found.
    echo         Run 'build.bat' first, then re-run run_demo.bat.
    exit /b 1
)

REM ---- Setup environment (inherited by pipeline scripts via DXSTREAM_ROOT) ----
set "DXSTREAM_ROOT=%PROJECT_ROOT%"
set "GST_PLUGIN_PATH=%INSTALL_DIR%\lib\gstreamer-1.0"
set "GST_REGISTRY=%INSTALL_DIR%\gst-registry.bin"
set "PATH=%INSTALL_DIR%\bin;%INSTALL_DIR%\share\gstdxstream\lib;%INSTALL_DIR%\share\gstdxstream\bin;%INSTALL_DIR%\lib\gstreamer-1.0;%PATH%"

if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%"
) else if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64\bin" (
    set "PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\bin;%PATH%"
)

REM ---- Verify GStreamer ----
where gst-launch-1.0 >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] gst-launch-1.0 not found in PATH.
    echo         Install GStreamer MSVC Runtime first.
    exit /b 1
)

REM ---- Verify plugin loads ----
gst-inspect-1.0 dxstream >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] gstdxstream plugin failed to load.
    exit /b 1
)

:menu
cls
echo ============================================================
echo  DX-Stream Demo Menu (Windows)
echo ============================================================
echo.
echo  [0] Object Detection        (YOLOv26n)
echo  [1] Object Detection        (YoloV5S PPU)
echo  [2] Face Detection           (YOLOv5s_Face)
echo  [3] Face Detection           (SCRFD500M PPU)
echo  [4] Pose Estimation          (YOLOv26n_Pose)
echo  [5] Pose Estimation          (YOLOV5Pose PPU)
echo  [6] Instance Segmentation    (YOLOv26n-Seg)
echo  [7] Multi-Object Tracking    (YoloV5S + OC_SORT)
echo  [8] Multi-Stream (4ch)       (Compositor Grid)
echo  [9] Multi-Channel (RTSP)     (dxinputselector)
echo  [-] Secondary Mode           (Multi-Model Cascade)
echo  [Q] Exit
echo.
echo ============================================================

set "SELECT="
set /p SELECT="Select demo [0-9, -, Q=Exit]: "

if /I "%SELECT%"=="Q" goto :exit
if "%SELECT%"=="0" call "%PIPELINES_DIR%\object_detection_yolo26n.bat"
if "%SELECT%"=="1" call "%PIPELINES_DIR%\object_detection_yolov5s_ppu.bat"
if "%SELECT%"=="2" call "%PIPELINES_DIR%\face_detection_yolov5s_face.bat"
if "%SELECT%"=="3" call "%PIPELINES_DIR%\face_detection_scrfd500m_ppu.bat"
if "%SELECT%"=="4" call "%PIPELINES_DIR%\pose_estimation_yolo26n_pose.bat"
if "%SELECT%"=="5" call "%PIPELINES_DIR%\pose_estimation_yolov5pose_ppu.bat"
if "%SELECT%"=="6" call "%PIPELINES_DIR%\segmentation_yolo26n_seg.bat"
if "%SELECT%"=="7" call "%PIPELINES_DIR%\multi_object_tracker.bat"
if "%SELECT%"=="8" call "%PIPELINES_DIR%\multi_stream.bat"
if "%SELECT%"=="9" call "%PIPELINES_DIR%\rtsp.bat" %INTERNAL_RTSP%
if "%SELECT%"=="-" call "%PIPELINES_DIR%\secondary_mode.bat"

echo.
pause
goto :menu

:exit
endlocal
exit /b 0
