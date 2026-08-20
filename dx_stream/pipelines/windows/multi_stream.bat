@echo off
setlocal
title DX-Stream Multi-Stream (4ch Compositor Grid)

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

set "MODEL=%MODEL_DIR%/yolov5-s_640x640_ppu.dxnn"
set "LIB=%LIB_DIR%/postprocess_ppu.dll"

set "VIDEO1=%VIDEO_DIR%/blackbox-city-road.mp4"
set "VIDEO2=%VIDEO_DIR%/blackbox-city-road2.mov"
set "VIDEO3=%VIDEO_DIR%/dance-group.mov"
set "VIDEO4=%VIDEO_DIR%/dance-solo.mov"

echo ============================================================
echo  DX-Stream Multi-Stream (4ch Compositor Grid)
echo ============================================================
echo  Output : 1280x720 (2x2 grid, 640x360 each)
echo ============================================================

gst-launch-1.0 -e ^
    urisourcebin uri="file:///%VIDEO1%" ^
    ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=10 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=10 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! queue max-size-buffers=10 ^
    ! dxosd ^
    ! queue max-size-buffers=10 ! dxscale width=640 height=360 ! queue max-size-buffers=10 ! comp.sink_0 ^
    urisourcebin uri="file:///%VIDEO2%" ^
    ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=10 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=10 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! queue max-size-buffers=10 ^
    ! dxosd ^
    ! queue max-size-buffers=10 ! dxscale width=640 height=360 ! queue max-size-buffers=10 ! comp.sink_1 ^
    urisourcebin uri="file:///%VIDEO3%" ^
    ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=10 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=10 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! queue max-size-buffers=10 ^
    ! dxosd ^
    ! queue max-size-buffers=10 ! dxscale width=640 height=360 ! queue max-size-buffers=10 ! comp.sink_2 ^
    urisourcebin uri="file:///%VIDEO4%" ^
    ! queue max-size-buffers=10 ! decodebin ! queue max-size-buffers=10 ^
    ! dxpreprocess preprocess-id=1 resize-width=640 resize-height=640 keep-ratio=true pad-value=114 ^
    ! queue max-size-buffers=10 ^
    ! dxinfer preprocess-id=1 inference-id=1 model-path="%MODEL%" ^
    ! queue max-size-buffers=10 ^
    ! dxpostprocess inference-id=1 library-file-path="%LIB%" function-name=YOLOV5S_PPU ^
    ! queue max-size-buffers=10 ^
    ! dxosd ^
    ! queue max-size-buffers=10 ! dxscale width=640 height=360 ! queue max-size-buffers=10 ! comp.sink_3 ^
    compositor name=comp sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=0 sink_2::ypos=360 sink_3::xpos=640 sink_3::ypos=360 ^
    ! videoconvert ^
    ! fpsdisplaysink sync=false

endlocal
