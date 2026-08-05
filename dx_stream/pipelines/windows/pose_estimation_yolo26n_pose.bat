@echo off
setlocal
title DX-Stream Pose Estimation (YOLOv26n_Pose)

if not defined DXSTREAM_ROOT (
    pushd "%~dp0..\..\..\"
    set "DXSTREAM_ROOT=%CD%"
    popd
)

set "GST_PLUGIN_PATH=%DXSTREAM_ROOT%\install\lib\gstreamer-1.0"
set "GST_REGISTRY=%DXSTREAM_ROOT%\install\gst-registry.bin"
set "PATH=%DXSTREAM_ROOT%\install\bin;%DXSTREAM_ROOT%\install\share\gstdxstream\lib;%DXSTREAM_ROOT%\install\share\gstdxstream\bin;%DXSTREAM_ROOT%\install\lib\gstreamer-1.0;%PATH%"

if defined GSTREAMER_1_0_ROOT_MSVC_X86_64 (
    set "PATH=%GSTREAMER_1_0_ROOT_MSVC_X86_64%\bin;%PATH%"
) else if exist "C:\Program Files\gstreamer\1.0\msvc_x86_64\bin" (
    set "PATH=C:\Program Files\gstreamer\1.0\msvc_x86_64\bin;%PATH%"
)

if not exist "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll" (
    echo [ERROR] DX-Stream plugin not found at "%DXSTREAM_ROOT%\install\bin\gstdxstream.dll".
    echo         Run 'build.bat' first.
    exit /b 1
)

set "BASE_FWD=%DXSTREAM_ROOT:\=/%"
set "MODEL_DIR=%BASE_FWD%/dx_stream/samples/models"
set "VIDEO_DIR=%BASE_FWD%/dx_stream/samples/videos"
set "LIB_DIR=%BASE_FWD%/install/share/gstdxstream/lib"

set "MODEL=%MODEL_DIR%/yolo26-n-pose_640x640.dxnn"
set "LIB=%LIB_DIR%/postprocess_yolo26pose.dll"
set "VIDEO=%VIDEO_DIR%/dance-group.mov"

if not "%~1"=="" set "VIDEO=%~1"

echo ============================================================
echo  DX-Stream Pose Estimation (YOLOv26n_Pose)
echo ============================================================
echo  Model : %MODEL%
echo  Video : %VIDEO%
echo ============================================================

gst-launch-1.0 ^
    filesrc location="%VIDEO%" ^
    ! decodebin ^
    ! videoconvert ! "video/x-raw,format=NV12" ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 ^
    ! queue max-size-buffers=1 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=1 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=PostProcess ^
    ! dxosd ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false

endlocal
