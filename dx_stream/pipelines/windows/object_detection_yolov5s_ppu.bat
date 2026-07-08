@echo off
setlocal
title DX-Stream Object Detection (YoloV5S PPU)

if not defined DXSTREAM_ROOT (
    echo [ERROR] DXSTREAM_ROOT not set. Run run_demo.bat or set DXSTREAM_ROOT manually.
    exit /b 1
)

set "BASE_FWD=%DXSTREAM_ROOT:\=/%"
set "MODEL_DIR=%BASE_FWD%/dx_stream/samples/models"
set "VIDEO_DIR=%BASE_FWD%/dx_stream/samples/videos"
set "LIB_DIR=%BASE_FWD%/install/share/gstdxstream/lib"

set "MODEL=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "LIB=%LIB_DIR%/postprocess_ppu.dll"
set "VIDEO=%VIDEO_DIR%/blackbox-city-road.mp4"

if not "%~1"=="" set "VIDEO=%~1"

echo ============================================================
echo  DX-Stream Object Detection (YoloV5S PPU)
echo ============================================================
echo  Model : %MODEL%
echo  Video : %VIDEO%
echo ============================================================

gst-launch-1.0 ^
    filesrc location="%VIDEO%" ^
    ! decodebin ^
    ! videoconvert ! "video/x-raw,format=NV12" ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=1 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=1 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! dxosd ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false

endlocal
